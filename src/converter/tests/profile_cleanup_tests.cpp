// Tests create and retire only uniquely named synthetic profiles owned here.
#include "processcontainment.h"
#include <QCoreApplication>
#include <cstdio>
#include <stdexcept>

using au::converter::detail::ProcessContainment;
using State = ProcessContainment::ProfileCleanupState;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
LPCWSTR wide(const QString& value) { return reinterpret_cast<LPCWSTR>(value.utf16()); }
QString uniqueName() { return QStringLiteral("Audacity.Test.") + QUuid::createUuid().toString(QUuid::WithoutBraces); }
QString sidString(PSID sid) {
    LPWSTR text = nullptr;
    require(sid && ConvertSidToStringSidW(sid, &text), "retain exact owned profile SID");
    const QString result = QString::fromWCharArray(text); LocalFree(text); return result;
}
HRESULT folderLookup(const QString& sid) {
    LPWSTR path = nullptr;
    const HRESULT result = GetAppContainerFolderPath(wide(sid), &path);
    if (path) CoTaskMemFree(path);
    return result;
}
void requireRegistered(const QString& sid) {
    require(folderLookup(sid) == S_OK, "supported folder lookup resolves existing registration even without physical storage");
}
void requireAbsent(const QString& sid) {
    require(folderLookup(sid) == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "supported folder lookup no longer resolves deleted registration");
}

class OwnedProfile final {
public:
    explicit OwnedProfile(QString value = uniqueName()) : name(std::move(value)) {
        const HRESULT result = CreateAppContainerProfile(wide(name), L"Cleanup test fixture", L"Synthetic owned profile", nullptr, 0, &sid);
        owned = SUCCEEDED(result);
        require(owned, "create exact new owned profile registration");
    }
    ~OwnedProfile() {
        if (owned && FAILED(DeleteAppContainerProfile(wide(name))))
            std::fprintf(stderr, "FAIL fixture profile teardown\n");
        if (sid) FreeSid(sid);
    }
    void remove() {
        require(owned && SUCCEEDED(DeleteAppContainerProfile(wide(name))), "delete exact owned fixture profile");
        owned = false;
        requireAbsent(identity());
    }
    QString identity() const { return sidString(sid); }
    QString name;
private:
    PSID sid = nullptr;
    bool owned = false;
};

QStringList* messages = nullptr;
void capture(QtMsgType type, const QMessageLogContext&, const QString& message) {
    if (type == QtWarningMsg && messages) messages->push_back(message);
}
class Diagnostics final {
public:
    Diagnostics() { messages = &values; previous = qInstallMessageHandler(capture); }
    ~Diagnostics() { qInstallMessageHandler(previous); messages = nullptr; }
    QStringList values;
private:
    QtMessageHandler previous = nullptr;
};

void proveRecreationAndRemove(const QString& name, const QString& sid) {
    requireAbsent(sid);
    OwnedProfile proof(name);
    require(proof.identity() == sid, "same-name recreation is the exact retired identity");
    requireRegistered(sid);
    proof.remove();
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try {
        int cases = 0;
        QString normalName, normalSid;
        {
            ProcessContainment container;
            require(container.valid(), "normal profile staging succeeds");
            normalName = container.ownedProfileNameForTest(); normalSid = sidString(container.identity());
            require(container.profileCleanupReport().state == State::Pending, "created profile reports pending cleanup");
            requireRegistered(normalSid);
        }
        proveRecreationAndRemove(normalName, normalSid);
        ++cases; std::fprintf(stderr, "PASS destructor removes exact registration: folder lookup changes S_OK to 0x80070002, then recreation succeeds\n");

        std::unique_ptr<OwnedProfile> replacement;
        {
            ProcessContainment container;
            require(container.valid(), "explicit cleanup profile is valid");
            const QString name = container.ownedProfileNameForTest(), sid = sidString(container.identity());
            const auto result = container.cleanupProfile();
            require(result.state == State::Removed && result.result == S_OK && !container.valid(), "checked success invalidates completed container");
            require(container.identity() && IsValidSid(container.identity()), "SID remains retained through checked deletion");
            requireAbsent(sid);
            replacement = std::make_unique<OwnedProfile>(name);
            const auto repeated = container.cleanupProfile();
            require(repeated.state == State::Removed && repeated.result == S_OK, "successful cleanup report is idempotent");
            requireRegistered(replacement->identity());
        }
        requireRegistered(replacement->identity()); replacement->remove(); replacement.reset();
        ++cases; std::fprintf(stderr, "PASS checked cleanup retains SID and cannot delete a same-name replacement on repeated call or destruction\n");

        OwnedProfile other;
        QString retainedName, retainedSid;
        Diagnostics diagnostics;
        {
            ProcessContainment container;
            require(container.valid(), "failure-injection profile is valid");
            retainedName = container.ownedProfileNameForTest(); retainedSid = sidString(container.identity());
            ProcessContainment::testFailProfileDeletion = true;
            const auto failed = container.cleanupProfile();
            ProcessContainment::testFailProfileDeletion = false;
            require(failed.state == State::Failed && failed.result == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), "actual injected HRESULT is observable in typed cleanup report");
            require(container.identity() && IsValidSid(container.identity()), "failed deletion retains owned SID until destruction");
            require(container.cleanupProfile().state == State::Failed, "failure remains visible without an implicit retry");
            requireRegistered(retainedSid); requireRegistered(other.identity());
        }
        requireRegistered(retainedSid); requireRegistered(other.identity());
        require(diagnostics.values.size() == 1 && diagnostics.values.first() == QStringLiteral(
            "PDF worker profile cleanup failed (HRESULT 0x80070005); an owned registration may remain."), "one bounded diagnostic reports deletion failure without paths or contents");
        // Recovery is restricted to the exact registration created by this case.
        require(SUCCEEDED(DeleteAppContainerProfile(wide(retainedName))), "recover exact deliberately retained fixture profile");
        proveRecreationAndRemove(retainedName, retainedSid);
        ++cases; std::fprintf(stderr, "PASS injected deletion failure is observable, retains its registration and preserves another profile\n");

        ProcessContainment::testProfileName = other.name;
        {
            ProcessContainment collision;
            require(!collision.valid(), "existing profile collision is refused");
            require(collision.cleanupProfile().state == State::NoOwnedProfile, "collision gives no deletion ownership");
        }
        ProcessContainment::testProfileName.clear();
        requireRegistered(other.identity()); other.remove();
        ++cases; std::fprintf(stderr, "PASS a rejected profile-name collision never deletes the existing registration\n");

        {
            ProcessContainment existing;
            require(existing.valid(), "existing registration without physical storage is valid");
            const QString sid = sidString(existing.identity());
            require(!QFileInfo::exists(existing.ownedProfileDirectory()), "existing registration deliberately has no physical package directory");
            ProcessContainment::testProfileName = existing.ownedProfileNameForTest();
            {
                ProcessContainment collision;
                require(!collision.valid() && collision.cleanupProfile().state == State::NoOwnedProfile,
                    "registration lookup prevents adoption when physical storage is absent");
            }
            ProcessContainment::testProfileName.clear();
            requireRegistered(sid);
            require(existing.cleanupProfile().state == State::Removed, "original owner alone removes its registration");
            requireAbsent(sid);
        }
        ++cases; std::fprintf(stderr, "PASS a registration with absent physical storage cannot be adopted or deleted by a colliding constructor\n");
        require(diagnostics.values.size() == 1, "non-owning collisions emit no false cleanup diagnostic");
        std::fprintf(stderr, "Profile cleanup integration: %d cases, %d passed, 0 failed\n", cases, cases);
        return 0;
    } catch (const std::exception& error) {
        ProcessContainment::testFailProfileDeletion = false;
        ProcessContainment::testProfileName.clear();
        std::fprintf(stderr, "FAIL profile cleanup: %s\n", error.what());
        return 1;
    }
}

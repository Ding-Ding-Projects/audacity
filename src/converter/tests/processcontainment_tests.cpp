#include <winsock2.h>
#include "processcontainment.h"
#include "qpdfbundle.h"
#include <QCoreApplication>
#include <cstdio>
#include <stdexcept>
using namespace au::converter::detail;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void put(const QString& path) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write("canary", 6) == 6, "owned canary creation"); }
void grantAllPackagesRead(const QString& path) {
    PACL original = nullptr, updated = nullptr; PSECURITY_DESCRIPTOR descriptor = nullptr;
    const QString native = QDir::toNativeSeparators(path);
    require(GetNamedSecurityInfoW(const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(native.utf16())), SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION, nullptr, nullptr, &original, nullptr, &descriptor) == ERROR_SUCCESS, "owned canary ACL");
    BYTE sid[SECURITY_MAX_SID_SIZE]; DWORD bytes = sizeof(sid);
    require(CreateWellKnownSid(WinBuiltinAnyPackageSid, nullptr, sid, &bytes), "ALL APPLICATION PACKAGES test SID");
    EXPLICIT_ACCESSW entry = {}; entry.grfAccessPermissions = FILE_GENERIC_READ; entry.grfAccessMode = GRANT_ACCESS;
    entry.Trustee.TrusteeForm = TRUSTEE_IS_SID; entry.Trustee.ptstrName = reinterpret_cast<LPWSTR>(sid);
    const bool changed = SetEntriesInAclW(1, &entry, original, &updated) == ERROR_SUCCESS
        && SetNamedSecurityInfoW(const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(native.utf16())), SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION, nullptr, nullptr, updated, nullptr) == ERROR_SUCCESS;
    if (updated) LocalFree(updated); LocalFree(descriptor);
    require(changed, "grant only synthetic canary to ALL APPLICATION PACKAGES");
}
DWORD run(const QString& worker, const QStringList& args, const ProcessContainment& container) {
    ContainedProcess process;
    if (!process.start(worker, args, container, 128LL * 1024 * 1024, 5000)) throw std::runtime_error(process.diagnostic().toStdString());
    require(process.identityVerified(), "kernel LPAC identity, exact SID, exact registryRead capability and Job verified before resume");
    require(process.wait(5000), "bounded console worker exit");
    QByteArray out, err; process.read(out, err); if (!err.isEmpty()) std::fwrite(err.constData(), 1, size_t(err.size()), stderr);
    return process.exitCode();
}
}
int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir owned;
        require(owned.isValid(), "owned fixtures directory");
        ProcessContainment container;
        require(container.valid(), "read-only isolated staging available");
        QFile executable(QCoreApplication::applicationDirPath() + QStringLiteral("/containment_worker.exe"));
        require(executable.open(QIODevice::ReadOnly), "native synthetic worker exists");
        const QString worker = container.copy(executable, QStringLiteral("containment_worker.exe"), 4 * 1024 * 1024);
        require(!worker.isEmpty(), "bounded worker copied and held");
        const QString outside = owned.filePath(QStringLiteral("outside.txt")); put(outside);
        QFile source(outside); require(source.open(QIODevice::ReadOnly), "read owned fixture");
        const QString inside = container.copy(source, QStringLiteral("input.txt"), 64);
        require(!inside.isEmpty(), "bounded input copied and held");
        require(container.copy(source, QStringLiteral("too-large.bin"), 5).isEmpty(), "staging byte cap refuses oversized owned fixture");
        require(container.copy(source, QStringLiteral("bad-hash.bin"), 64, QString(64, QLatin1Char('0'))).isEmpty(), "independent expected hash refuses changed content");
        int cases = 0;
        auto check = [&](const char* name, const QStringList& args, DWORD expected) {
            const DWORD actual = run(worker, args, container);
            if (actual != expected) throw std::runtime_error(std::string(name) + ": exit " + std::to_string(actual));
            ++cases; std::fprintf(stderr, "PASS %s (exit %lu)\n", name, actual);
        };
        for (auto it = QpdfFiles.cbegin(); it != QpdfFiles.cend(); ++it) {
            QFile component(QCoreApplication::applicationDirPath() + QStringLiteral("/converter-tools/qpdf/") + it.key());
            require(component.open(QIODevice::ReadOnly), "pinned runtime component");
            require(!container.copy(component, it.key(), 32 * 1024 * 1024, it.value()).isEmpty(), "stage runtime component");
        }
        for (bool* boundary : {&ContainedProcess::testOmitContainer, &ContainedProcess::testOmitLpac, &ContainedProcess::testUnexpectedNetworkCapability}) {
            *boundary = true;
            bool launched = false;
            { ContainedProcess altered; launched = altered.start(worker, {QStringLiteral("create"), container.directory() + QStringLiteral("/must-not-run.txt")}, container, 128LL * 1024 * 1024, 5000); }
            *boundary = false;
            require(!launched && !QFileInfo::exists(container.directory() + QStringLiteral("/must-not-run.txt")), "missing isolation boundary rejected before worker resume");
            ++cases; std::fprintf(stderr, "PASS removed isolation boundary rejected before resume\n");
        }
        check("LPAC reads granted bounded input", {QStringLiteral("read"), inside}, 0);
        const QString shared = owned.filePath(QStringLiteral("shared.txt")); put(shared); grantAllPackagesRead(shared);
        check("LPAC refuses ALL APPLICATION PACKAGES canary", {QStringLiteral("read"), shared}, 10);
        check("LPAC cannot read owned outside canary", {QStringLiteral("read"), outside}, 10);
        check("LPAC cannot write owned outside canary", {QStringLiteral("write"), outside}, 10);
        check("LPAC cannot write staged input", {QStringLiteral("write"), inside}, 10);
        check("LPAC cannot create temporary files", {QStringLiteral("create"), container.directory() + QStringLiteral("/escape.txt")}, 10);
        check("LPAC cannot create outside files", {QStringLiteral("create"), owned.filePath(QStringLiteral("escape.txt"))}, 10);
        check("LPAC cannot recreate removed package storage", {QStringLiteral("mkdir"), container.ownedProfileDirectory()}, 10);
        require(!QFileInfo::exists(container.ownedProfileDirectory()), "no writable package storage exists before launch");
        check("Job refuses a second process", {QStringLiteral("spawn"), QDir::toNativeSeparators(container.directory() + QStringLiteral("/containment_worker.exe"))}, 10);
        SECURITY_ATTRIBUTES inheritable = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
        Handle extra(CreateEventW(&inheritable, FALSE, FALSE, nullptr));
        require(extra.valid(), "unrelated inheritable synthetic event exists");
        check("explicit handle list excludes unrelated inherited event", {QStringLiteral("inherit"), QString::number(reinterpret_cast<quintptr>(extra.value))}, 10);
        WSADATA data = {}; require(WSAStartup(MAKEWORD(2, 2), &data) == 0, "loopback fixture Winsock");
        SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        require(listener != INVALID_SOCKET, "loopback fixture socket");
        sockaddr_in address = {}; address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        require(bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0 && listen(listener, 1) == 0, "owned loopback listener");
        int size = sizeof(address); require(getsockname(listener, reinterpret_cast<sockaddr*>(&address), &size) == 0, "ephemeral port");
        SOCKET control = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        require(connect(control, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0, "parent reaches same owned loopback listener");
        SOCKET accepted = accept(listener, nullptr, nullptr); require(accepted != INVALID_SOCKET, "parent control accepted");
        closesocket(accepted); closesocket(control);
        check("LPAC without network capabilities refuses reachable owned loopback", {QStringLiteral("network"), QString::number(ntohs(address.sin_port))}, 10);
        closesocket(listener); WSACleanup();
        source.seek(0); require(source.readAll() == "canary", "original owned fixture unchanged");
        std::fprintf(stderr, "Containment integration: %d cases, %d passed, 0 failed\n", cases, cases);
        return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "FAIL containment: %s\n", error.what()); return 1; }
}

/* Windows PDF worker containment. No original-file or machine-wide policy mutation. */
#pragma once
#include "nativefiletransaction.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>
#include <QDirIterator>
#include <functional>
#ifdef Q_OS_WIN
#include <aclapi.h>
#include <userenv.h>
#include <sddl.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "onecore.lib")

namespace au::converter::detail {
// Each worker family uses a fresh profile identity with its physical storage
// removed before launch. Only registryRead is granted, for SxS activation.
class ProcessContainment final {
public:
    enum class ProfileCleanupState { NoOwnedProfile, Pending, Removed, Failed };
    struct ProfileCleanupReport {
        ProfileCleanupState state = ProfileCleanupState::NoOwnedProfile;
        HRESULT result = S_OK;
    };
#ifdef AU_CONVERTER_TEST_HOOKS
    static inline thread_local bool testFailProfileDeletion = false;
    static inline thread_local QString testProfileName;
    QString ownedProfileNameForTest() const { return profileName; }
#endif
    ProcessContainment() : scratch(QDir::tempPath() + QStringLiteral("/audacity-pdf-XXXXXX")) {
        profileName = QStringLiteral("Audacity.Pdf.") + QUuid::createUuid().toString(QUuid::WithoutBraces);
#ifdef AU_CONVERTER_TEST_HOOKS
        if (!testProfileName.isEmpty()) profileName = testProfileName;
#endif
        // CreateAppContainerProfile can recreate physical storage for an
        // existing registration. Derive the candidate identity, then use the
        // supported registration-backed lookup before acquiring ownership.
        PSID candidateSid = nullptr;
        if (FAILED(DeriveAppContainerSidFromAppContainerName(reinterpret_cast<LPCWSTR>(profileName.utf16()), &candidateSid))) return;
        LPWSTR candidateText = nullptr, registeredFolder = nullptr;
        if (!ConvertSidToStringSidW(candidateSid, &candidateText)) { FreeSid(candidateSid); return; }
        const HRESULT registration = GetAppContainerFolderPath(candidateText, &registeredFolder);
        if (registeredFolder) CoTaskMemFree(registeredFolder);
        LocalFree(candidateText); FreeSid(candidateSid);
        if (registration != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) return;
        if (FAILED(CreateAppContainerProfile(reinterpret_cast<LPCWSTR>(profileName.utf16()), L"Audacity PDF worker",
            L"Ephemeral converter worker", nullptr, 0, &sid))) return;
        ownsProfile = true;
        cleanupReport.state = ProfileCleanupState::Pending;
        LPWSTR sidText = nullptr, folder = nullptr;
        if (!ConvertSidToStringSidW(sid, &sidText)) return;
        const HRESULT folderResult = GetAppContainerFolderPath(sidText, &folder);
        LocalFree(sidText);
        if (FAILED(folderResult)) return;
        profilePath = QDir::fromNativeSeparators(QString::fromWCharArray(folder)); CoTaskMemFree(folder);
        if (QFileInfo(profilePath).fileName() != QStringLiteral("AC")) return;
        profilePath = QFileInfo(profilePath).dir().path();
        // CreateAppContainerProfile establishes a fresh unique registration.
        // Remove its newly created storage before starting any worker; otherwise
        // package storage is writable even with a read-only working directory.
        if (QFileInfo(profilePath).fileName().compare(profileName, Qt::CaseInsensitive) != 0
            || QFileInfo(profilePath).isSymLink() || !profilePin.open(profilePath)) return;
        int entriesCount = 0; qint64 profileBytes = 0;
        QDirIterator entries(profilePath, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
        while (entries.hasNext()) {
            entries.next();
            if (entries.fileInfo().isSymLink() || ++entriesCount > 128) return;
            if (entries.fileInfo().isFile()) profileBytes += entries.fileInfo().size();
            if (profileBytes > 1024 * 1024) return;
        }
        if (!QDir(profilePath).removeRecursively() || QFileInfo::exists(profilePath)) return;
        PSID *groupSids = nullptr, *capabilitySids = nullptr;
        DWORD groupCount = 0, capabilityCount = 0;
        const BOOL derived = DeriveCapabilitySidsFromName(L"registryRead", &groupSids, &groupCount, &capabilitySids, &capabilityCount);
        if (derived && capabilityCount == 1) {
            capabilityStorage.resize(GetLengthSid(capabilitySids[0]));
            CopySid(DWORD(capabilityStorage.size()), capabilityStorage.data(), capabilitySids[0]);
            capability.Sid = capabilityStorage.data(); capability.Attributes = SE_GROUP_ENABLED;
        }
        for (DWORD i = 0; i < groupCount; ++i) LocalFree(groupSids[i]);
        for (DWORD i = 0; i < capabilityCount; ++i) LocalFree(capabilitySids[i]);
        if (groupSids) LocalFree(groupSids); if (capabilitySids) LocalFree(capabilitySids);
        if (!capability.Sid) return;
        if (!scratch.isValid() || !rootPin.open(scratch.filePath(QStringLiteral("anchor")))) return;
        // Only this new, pinned directory is changed. Existing parent ACLs and
        // original source files are never modified.
        Handle root(CreateFileW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(scratch.path()).utf16()),
            READ_CONTROL | WRITE_DAC, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        ready = root.valid() && grantRead(root.value);
    }
    ~ProcessContainment() {
        cleanupProfile();
        // Keep the SID and ownership record alive through the deletion attempt.
        if (sid) FreeSid(sid);
    }
    // Call only after every worker using this container has been closed. The
    // destructor uses the same one-attempt path. A failed result is observable
    // and retained; repeated cleanup calls never retry or broaden its target.
    ProfileCleanupReport cleanupProfile() {
        ready = false;
        files.clear();
        pins.clear();
        if (!ownsProfile || cleanupReport.state != ProfileCleanupState::Pending) return cleanupReport;
        HRESULT result;
#ifdef AU_CONVERTER_TEST_HOOKS
        if (testFailProfileDeletion) result = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        else
#endif
            result = DeleteAppContainerProfile(reinterpret_cast<LPCWSTR>(profileName.utf16()));
        cleanupReport.result = result;
        if (SUCCEEDED(result)) {
            cleanupReport.state = ProfileCleanupState::Removed;
            ownsProfile = false;
        } else {
            cleanupReport.state = ProfileCleanupState::Failed;
            // Fixed bounded diagnostic, with no profile path, source name or
            // parser content. Host logging receives the actual API HRESULT.
            qWarning().noquote() << QStringLiteral("PDF worker profile cleanup failed (HRESULT 0x%1); an owned registration may remain.")
                .arg(quint32(result), 8, 16, QLatin1Char('0'));
        }
        return cleanupReport;
    }
    ProfileCleanupReport profileCleanupReport() const { return cleanupReport; }
    bool valid() const { return ready; }
    QString directory() const { return scratch.path(); }
    PSID identity() const { return sid; }
    const SID_AND_ATTRIBUTES* allowedCapability() const { return &capability; }
    QString ownedProfileDirectory() const { return profilePath; }
    size_t mark() const { return files.size(); }
    void release(size_t marker) { files.resize(marker); pins.resize(marker); }
    // Read through a caller-owned pinned handle. The copy is bounded, hashed
    // against the actual bytes read and held against replacement until teardown.
    QString copy(QIODevice& source, const QString& name, qint64 limit, const QString& expectedHash = {}, const std::function<bool()>& alive = {}) {
        if (!ready || name.isEmpty() || name.contains('/') || name.contains('\\') || name.contains(':')
            || source.size() < 1 || source.size() > limit || !source.seek(0)) return {};
        qint64 total = source.size();
        for (const auto& file : files) total += file->size();
        if (total > 1024LL * 1024 * 1024) return {};
        auto pin = std::make_unique<PinnedPath>();
        if (!pin->open(scratch.filePath(name))) return {};
        auto out = std::make_unique<HandleDevice>(pin->path(), true, limit, nullptr, true);
        if (!out->isOpen()) return {};
        QCryptographicHash hash(QCryptographicHash::Sha256);
        while (!source.atEnd()) {
            if (alive && !alive()) return {};
            const QByteArray bytes = source.read(1024 * 1024);
            if (bytes.isEmpty() || out->write(bytes) != bytes.size()) return {};
            hash.addData(bytes);
        }
        const QByteArray digest = hash.result();
        if (!expectedHash.isEmpty() && QString::fromLatin1(digest.toHex()) != expectedHash) return {};
        if (!out->seek(0)) return {};
        QCryptographicHash copied(QCryptographicHash::Sha256);
        while (!out->atEnd()) {
            if (alive && !alive()) return {};
            const QByteArray bytes = out->read(1024 * 1024);
            if (bytes.isEmpty()) return {};
            copied.addData(bytes);
        }
        if (copied.result() != digest) return {};
        // Open only the object pinned above; WRITE_DAC is not a data write and
        // does not require relaxing the data/rename sharing restrictions.
        Handle acl(CreateFileW(reinterpret_cast<LPCWSTR>(pin->path().utf16()), READ_CONTROL | WRITE_DAC,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        if (!acl.valid() || !resolvesTo(acl.value, pin->path()) || !grantRead(acl.value) || !out->sealForSubprocess()) return {};
        QString path = pin->path();
        path[2] = QLatin1Char('.'); // qpdf wildcard-free volume namespace.
        pins.push_back(std::move(pin)); files.push_back(std::move(out));
        return path;
    }
private:
    bool grantRead(HANDLE object) {
        PACL oldAcl = nullptr, newAcl = nullptr;
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (GetSecurityInfo(object, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr,
                            &oldAcl, nullptr, &descriptor) != ERROR_SUCCESS) return false;
        EXPLICIT_ACCESSW entry = {};
        entry.grfAccessPermissions = FILE_GENERIC_READ | FILE_GENERIC_EXECUTE;
        entry.grfAccessMode = GRANT_ACCESS;
        entry.grfInheritance = NO_INHERITANCE;
        entry.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        entry.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
        entry.Trustee.ptstrName = static_cast<LPWSTR>(sid);
        const bool ok = SetEntriesInAclW(1, &entry, oldAcl, &newAcl) == ERROR_SUCCESS
            && SetSecurityInfo(object, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr,
                               newAcl, nullptr) == ERROR_SUCCESS;
        if (newAcl) LocalFree(newAcl);
        LocalFree(descriptor);
        return ok;
    }
    QTemporaryDir scratch;
    PinnedPath rootPin, profilePin;
    std::vector<std::unique_ptr<PinnedPath>> pins;
    std::vector<std::unique_ptr<HandleDevice>> files;
    PSID sid = nullptr;
    bool ready = false;
    bool ownsProfile = false;
    ProfileCleanupReport cleanupReport;
    QString profileName, profilePath;
    std::vector<unsigned char> capabilityStorage;
    SID_AND_ATTRIBUTES capability = {};
};

class ContainedProcess final {
public:
#ifdef AU_CONVERTER_TEST_HOOKS
    static inline thread_local bool testOmitLpac = false;
    static inline thread_local bool testOmitContainer = false;
    static inline thread_local bool testUnexpectedNetworkCapability = false;
#endif
    ~ContainedProcess() {
        if (job) { TerminateJobObject(job, 1); CloseHandle(job); }
        if (process.hProcess) { WaitForSingleObject(process.hProcess, 5000); CloseHandle(process.hProcess); }
        if (process.hThread) CloseHandle(process.hThread);
        if (attributes) DeleteProcThreadAttributeList(attributes);
        for (HANDLE h : {outRead, outWrite, errRead, errWrite}) if (h) CloseHandle(h);
    }
    bool start(const QString& tool, const QStringList& args, const ProcessContainment& container,
               qint64 memoryBytes, int deadlineMilliseconds, const std::function<void()>& limitsInstalled = {}) {
        if (!container.valid()) return false;
        stage = QStringLiteral("Job");
        job = CreateJobObjectW(nullptr, nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
            | JOB_OBJECT_LIMIT_ACTIVE_PROCESS | JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_PROCESS_TIME;
        limits.BasicLimitInformation.ActiveProcessLimit = 1;
        limits.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = LONGLONG(std::max(1, deadlineMilliseconds)) * 10000;
        limits.ProcessMemoryLimit = SIZE_T(memoryBytes);
        if (!job || !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) return false;
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION installed = {};
        if (!QueryInformationJobObject(job, JobObjectExtendedLimitInformation, &installed, sizeof(installed), nullptr)
            || installed.ProcessMemoryLimit != limits.ProcessMemoryLimit
            || installed.BasicLimitInformation.ActiveProcessLimit != 1
            || installed.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart != limits.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart
            || (installed.BasicLimitInformation.LimitFlags & limits.BasicLimitInformation.LimitFlags) != limits.BasicLimitInformation.LimitFlags) return false;
        if (limitsInstalled) limitsInstalled();
        stage = QStringLiteral("Pipes");
        SECURITY_ATTRIBUTES pipeSecurity = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
        if (!CreatePipe(&outRead, &outWrite, &pipeSecurity, 0) || !CreatePipe(&errRead, &errWrite, &pipeSecurity, 0)
            || !SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0) || !SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0)) return false;
        stage = QStringLiteral("Attribute list");
        SIZE_T bytes = 0;
        InitializeProcThreadAttributeList(nullptr, 4, 0, &bytes);
        if (!bytes) return false;
        storage.resize(bytes);
        auto* candidate = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
        if (!InitializeProcThreadAttributeList(candidate, 4, 0, &bytes)) return false;
        attributes = candidate;
        stage = QStringLiteral("Security attributes");
        SECURITY_CAPABILITIES capabilities = {};
        capabilities.AppContainerSid = container.identity();
        capabilities.Capabilities = const_cast<PSID_AND_ATTRIBUTES>(container.allowedCapability());
        capabilities.CapabilityCount = 1;
#ifdef AU_CONVERTER_TEST_HOOKS
        BYTE networkSid[SECURITY_MAX_SID_SIZE]; DWORD networkSidBytes = sizeof(networkSid);
        SID_AND_ATTRIBUTES networkCapability = {};
        if (testUnexpectedNetworkCapability) {
            if (!CreateWellKnownSid(WinCapabilityInternetClientSid, nullptr, networkSid, &networkSidBytes)) return false;
            networkCapability = {networkSid, SE_GROUP_ENABLED}; capabilities.Capabilities = &networkCapability;
        }
#endif
        DWORD optOut = PROCESS_CREATION_ALL_APPLICATION_PACKAGES_OPT_OUT;
        HANDLE inherited[] = {outWrite, errWrite};
        bool useContainer = true, useLpac = true;
#ifdef AU_CONVERTER_TEST_HOOKS
        useContainer = !testOmitContainer; useLpac = !testOmitLpac;
#endif
        if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, &job, sizeof(job), nullptr, nullptr)
            || (useContainer && !UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES, &capabilities, sizeof(capabilities), nullptr, nullptr))
            || (useContainer && useLpac && !UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY, &optOut, sizeof(optOut), nullptr, nullptr))
            || !UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, sizeof(inherited), nullptr, nullptr)) return false;
        STARTUPINFOEXW startup = {};
        startup.StartupInfo.cb = sizeof(startup);
        startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        startup.StartupInfo.hStdOutput = outWrite;
        startup.StartupInfo.hStdError = errWrite;
        startup.lpAttributeList = attributes;
        QString command = quote(tool);
        for (const auto& arg : args) command += QLatin1Char(' ') + quote(arg);
        wchar_t root[32768] = {};
        const UINT length = GetWindowsDirectoryW(root, 32768);
        if (!length || length >= 32768) return false;
        QString environment = QStringLiteral("LOCALAPPDATA=") + container.directory() + QChar::Null
            + QStringLiteral("PATH=") + container.directory() + QChar::Null
            + QStringLiteral("SystemRoot=") + QString::fromWCharArray(root, int(length)) + QChar::Null
            + QStringLiteral("TEMP=") + container.directory() + QChar::Null
            + QStringLiteral("TMP=") + container.directory() + QChar::Null + QChar::Null;
        const QString directory = QDir::toNativeSeparators(container.directory());
        stage = QStringLiteral("CreateProcess");
        QString launchPath = tool;
        if (!launchPath.startsWith(QStringLiteral("\\\\.\\Volume{"))) return false;
        // Use the Win32 device spelling accepted by the contained loader.
        const bool created = CreateProcessW(reinterpret_cast<LPCWSTR>(launchPath.utf16()), reinterpret_cast<LPWSTR>(command.data()),
            nullptr, nullptr, TRUE, EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
            environment.data(), reinterpret_cast<LPCWSTR>(directory.utf16()), &startup.StartupInfo, &process);
        if (!created) { failureCode = GetLastError(); return false; }
        CloseHandle(outWrite); outWrite = nullptr; CloseHandle(errWrite); errWrite = nullptr;
        // Fail closed before any instruction of the worker executes.
        if (QFileInfo::exists(container.ownedProfileDirectory())) { stage = QStringLiteral("Profile storage recreated"); return false; }
        stage = QStringLiteral("OpenProcessToken");
        HANDLE token = nullptr;
        if (!OpenProcessToken(process.hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &token)) return false;
        Handle tokenOwner(token);
        stage = QStringLiteral("Identity flags");
        DWORD count = 0, isContainer = 0;
        if (!GetTokenInformation(token, TokenIsAppContainer, &isContainer, sizeof(isContainer), &count) || isContainer != 1) return false;
        stage = QStringLiteral("LPAC groups");
        GetTokenInformation(token, TokenGroups, nullptr, 0, &count);
        std::vector<unsigned char> groupBytes(count);
        if (!count || !GetTokenInformation(token, TokenGroups, groupBytes.data(), count, &count)) return false;
        auto* groups = reinterpret_cast<TOKEN_GROUPS*>(groupBytes.data());
        BYTE allPackages[SECURITY_MAX_SID_SIZE]; DWORD sidBytes = sizeof(allPackages);
        if (!CreateWellKnownSid(WinBuiltinAnyPackageSid, nullptr, allPackages, &sidBytes)) return false;
        for (DWORD i = 0; i < groups->GroupCount; ++i)
            if (EqualSid(groups->Groups[i].Sid, allPackages) && (groups->Groups[i].Attributes & SE_GROUP_ENABLED)) return false;
        HANDLE membershipToken = nullptr;
        if (!DuplicateToken(token, SecurityIdentification, &membershipToken)) return false;
        Handle membershipOwner(membershipToken);
        // Group enumeration/membership does not distinguish LPAC on all
        // supported systems. Ask the kernel whether this exact token can read
        // a resource granted only to Everyone plus ALL APPLICATION PACKAGES.
        // Ordinary AppContainer passes; LPAC must fail. No thread impersonation.
        stage = QStringLiteral("LPAC access check");
        PSECURITY_DESCRIPTOR probe = nullptr;
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"O:SYG:SYD:(A;;0x1;;;WD)(A;;0x1;;;AC)", SDDL_REVISION_1, &probe, nullptr)) return false;
        GENERIC_MAPPING mapping = {1, 2, 4, 7};
        BYTE privilegeBytes[1024] = {}; DWORD privilegeSize = sizeof(privilegeBytes), granted = 0; BOOL access = FALSE;
        const BOOL checked = AccessCheck(probe, membershipToken, 1, &mapping, reinterpret_cast<PRIVILEGE_SET*>(privilegeBytes), &privilegeSize, &granted, &access);
        LocalFree(probe);
        if (!checked || access) return false;
        stage = QStringLiteral("Identity SID");
        GetTokenInformation(token, TokenAppContainerSid, nullptr, 0, &count);
        std::vector<unsigned char> identity(count);
        if (!count || !GetTokenInformation(token, TokenAppContainerSid, identity.data(), count, &count)
            || !EqualSid(reinterpret_cast<TOKEN_APPCONTAINER_INFORMATION*>(identity.data())->TokenAppContainer, container.identity())) return false;
        stage = QStringLiteral("Capabilities");
        GetTokenInformation(token, TokenCapabilities, nullptr, 0, &count);
        std::vector<unsigned char> capabilityBytes(count);
        if (!count || !GetTokenInformation(token, TokenCapabilities, capabilityBytes.data(), count, &count)
            || reinterpret_cast<TOKEN_GROUPS*>(capabilityBytes.data())->GroupCount != 1
            || !EqualSid(reinterpret_cast<TOKEN_GROUPS*>(capabilityBytes.data())->Groups[0].Sid, container.allowedCapability()->Sid)) return false;
        stage = QStringLiteral("Job and resume");
        BOOL inJob = FALSE;
        if (!IsProcessInJob(process.hProcess, job, &inJob) || !inJob || ResumeThread(process.hThread) != 1) return false;
        verified = true;
        return true;
    }
    QString diagnostic() const { return stage + (failureCode
        ? QStringLiteral(" (Windows code %1)").arg(failureCode)
        : QStringLiteral(" (required boundary could not be verified)")); }
    bool identityVerified() const { return verified; }
    bool running() const { return process.hProcess && WaitForSingleObject(process.hProcess, 0) == WAIT_TIMEOUT; }
    bool wait(DWORD milliseconds) const { return WaitForSingleObject(process.hProcess, milliseconds) == WAIT_OBJECT_0; }
    DWORD exitCode() const { DWORD code = DWORD(-1); GetExitCodeProcess(process.hProcess, &code); return code; }
    bool read(QByteArray& output, QByteArray& errors) { return drain(outRead, output) && drain(errRead, errors); }
private:
    static QString quote(const QString& argument) {
        QString result = QStringLiteral("\""); int slashes = 0;
        for (QChar ch : argument) {
            if (ch == QLatin1Char('\\')) { ++slashes; continue; }
            result += QString(slashes * (ch == QLatin1Char('"') ? 2 : 1), QLatin1Char('\\')); slashes = 0;
            if (ch == QLatin1Char('"')) result += QLatin1Char('\\');
            result += ch;
        }
        return result + QString(slashes * 2, QLatin1Char('\\')) + QLatin1Char('"');
    }
    static bool drain(HANDLE pipe, QByteArray& bytes) {
        DWORD available = 0;
        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr)) return GetLastError() == ERROR_BROKEN_PIPE;
        if (!available) return true;
        // Drain one bounded chunk per iteration so neither channel can starve
        // cancellation, the deadline, or the other channel.
        bytes.resize(std::min<DWORD>(available, 64 * 1024));
        DWORD received = 0;
        if (!ReadFile(pipe, bytes.data(), DWORD(bytes.size()), &received, nullptr)) return false;
        bytes.resize(received); return true;
    }
    HANDLE job = nullptr, outRead = nullptr, outWrite = nullptr, errRead = nullptr, errWrite = nullptr;
    PROCESS_INFORMATION process = {};
    LPPROC_THREAD_ATTRIBUTE_LIST attributes = nullptr;
    std::vector<unsigned char> storage;
    bool verified = false;
    QString stage;
    DWORD failureCode = 0;
};
}
#endif

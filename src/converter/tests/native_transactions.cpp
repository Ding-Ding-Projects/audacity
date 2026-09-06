/*
* Audacity: A Digital Audio Editor
*/
#include "conversionengine.h"
#include "nativefiletransaction.h"
#include <QCoreApplication>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QtEndian>
#include <winioctl.h>
#include <functional>
#include <cstdio>
#include <stdexcept>

using namespace au::converter;
namespace {
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
LPCWSTR wide(const QString& value) { return reinterpret_cast<LPCWSTR>(value.utf16()); }
void put(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    require(file.open(QIODevice::WriteOnly), "create fixture");
    require(file.write(bytes) == bytes.size(), "write fixture");
}
QByteArray bytes(const QString& path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "read fixture");
    return file.readAll();
}
struct Fixture {
    QTemporaryDir directory;
    QString source = directory.filePath(QStringLiteral("source.png"));
    QString output = directory.filePath(QStringLiteral("output.bmp"));
    Fixture()
    {
        require(directory.isValid(), "temporary directory");
        QImage image(40, 30, QImage::Format_ARGB32);
        image.fill(Qt::blue);
        require(image.save(source, "png"), "bundled PNG encoder");
    }
    ConversionResult convert(const std::atomic_bool* cancel = nullptr, bool overwrite = false)
    {
        return ConversionEngine().convert({source, output, QStringLiteral("bmp"), overwrite}, cancel);
    }
    void noTemporary() const
    {
        require(QDir(directory.path()).entryList({QStringLiteral(".audacity-convert-*.tmp")}, QDir::Files | QDir::Hidden).isEmpty(), "temporary cleanup");
    }
    void converted(const ConversionResult& result)
    {
        if (result.status != ConversionStatus::Converted) throw std::runtime_error(result.message.toStdString());
        QImage image(output);
        require(!image.isNull() && image.size() == QSize(40, 30), "fully readable published result");
        noTemporary();
    }
};
// A directory mount-point reparse fixture does not require symbolic-link
// privilege. Its payload is the documented REPARSE_DATA_BUFFER layout.
bool junction(const QString& link, const QString& target, DWORD access = GENERIC_WRITE)
{
    if (!QDir().mkpath(link)) return false;
    detail::Handle handle(CreateFileW(wide(QDir::toNativeSeparators(link)), access,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                     FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr));
    if (!handle.valid()) return false;
    const QString substitute = QStringLiteral("\\??\\") + QDir::toNativeSeparators(target);
    const QString print = QDir::toNativeSeparators(target);
    struct MountPoint {
        DWORD tag;
        WORD length;
        WORD reserved;
        WORD substituteOffset;
        WORD substituteLength;
        WORD printOffset;
        WORD printLength;
        wchar_t path[32768];
    } data = {};
    data.tag = IO_REPARSE_TAG_MOUNT_POINT;
    data.substituteLength = WORD(substitute.size() * 2);
    data.printOffset = WORD(data.substituteLength + 2);
    data.printLength = WORD(print.size() * 2);
    data.length = WORD(8 + data.printOffset + data.printLength + 2);
    memcpy(data.path, substitute.utf16(), data.substituteLength);
    memcpy(reinterpret_cast<char*>(data.path) + data.printOffset, print.utf16(), data.printLength);
    DWORD returned = 0;
    return DeviceIoControl(handle.value, FSCTL_SET_REPARSE_POINT, &data, data.length + 8, nullptr, 0, &returned, nullptr);
}
}
int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"verified conversion and cleanup", [] { Fixture f; f.converted(f.convert()); }},
        {"byte signature despite wrong extension", [] {
            Fixture f; const auto renamed = f.directory.filePath(QStringLiteral("source.txt"));
            require(QFile::rename(f.source, renamed), "rename fixture"); f.source = renamed;
            require(ConversionEngine::detectFormat(f.source) == QStringLiteral("PNG"), "bounded detection");
            f.converted(f.convert());
        }},
        {"existing destination preserved", [] {
            Fixture f; put(f.output, "existing bytes");
            require(f.convert().status == ConversionStatus::Rejected, "refuse existing destination");
            require(bytes(f.output) == "existing bytes", "preserve existing bytes"); f.noTemporary();
        }},
        {"explicit overwrite remains refused", [] {
            Fixture f; put(f.output, "existing bytes");
            require(f.convert(nullptr, true).status == ConversionStatus::Rejected, "refuse unsupported overwrite");
            require(bytes(f.output) == "existing bytes", "preserve approved overwrite original");
        }},
        {"source and case alias preserved", [] {
            Fixture f; const auto original = bytes(f.source); f.output = f.source.toUpper();
            require(f.convert().status == ConversionStatus::Rejected, "reject case alias");
            require(bytes(f.source) == original, "preserve source");
        }},
        {"source hard-link identity preserved", [] {
            Fixture f; require(CreateHardLinkW(wide(f.output), wide(f.source), nullptr), "hard link fixture");
            const auto original = bytes(f.source);
            require(f.convert().status == ConversionStatus::Rejected, "reject same file ID");
            require(bytes(f.source) == original && bytes(f.output) == original, "preserve hard-linked bytes");
        }},
        {"source symlink rejected", [] {
            Fixture f; const QString link = f.directory.filePath(QStringLiteral("link.png"));
            require(CreateSymbolicLinkW(wide(link), wide(f.source), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE), "file symlink fixture requires Developer Mode or privilege");
            f.source = link;
            require(f.convert().status == ConversionStatus::Rejected, "reject source symlink");
            require(ConversionEngine::detectFormat(link).isEmpty(), "detection also rejects symlink");
            require(!QFile::exists(f.output), "no publication");
        }},
        {"dangling destination symlink preserved", [] {
            Fixture f; const QString missing = f.directory.filePath(QStringLiteral("missing.bmp"));
            require(CreateSymbolicLinkW(wide(f.output), wide(missing), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE), "destination symlink fixture");
            require(f.convert().status == ConversionStatus::Rejected, "reject dangling destination");
            require(GetFileAttributesW(wide(f.output)) & FILE_ATTRIBUTE_REPARSE_POINT, "link preserved");
            require(!QFile::exists(missing), "link target remains absent");
        }},
        {"source ancestor junction rejected", [] {
            Fixture f; const QString link = f.directory.filePath(QStringLiteral("linked"));
            const QString target = f.directory.filePath(QStringLiteral("real"));
            require(QDir().mkpath(target), "target directory");
            require(QFile::copy(f.source, target + QStringLiteral("/source.png")), "nested source");
            require(junction(link, target), "junction fixture");
            f.source = link + QStringLiteral("/source.png");
            require(f.convert().status == ConversionStatus::Rejected, "reject source ancestor reparse");
            require(RemoveDirectoryW(wide(link)), "remove junction fixture");
        }},
        {"destination ancestor junction rejected", [] {
            Fixture f; const QString link = f.directory.filePath(QStringLiteral("linked"));
            const QString target = f.directory.filePath(QStringLiteral("real"));
            require(QDir().mkpath(target + QStringLiteral("/nested")), "nested directory");
            require(junction(link, target), "junction fixture");
            f.output = link + QStringLiteral("/nested/out.bmp");
            require(f.convert().status == ConversionStatus::Rejected, "reject intermediate reparse");
            require(!QFile::exists(target + QStringLiteral("/nested/out.bmp")), "no redirected output");
            require(RemoveDirectoryW(wide(link)), "remove junction fixture");
        }},
        {"introduced destination wins atomic publication", [] {
            Fixture f; bool reached = false;
            ConversionEngine::testHook = [&](auto phase) {
                if (phase == ConversionEngine::TestPhase::BeforePublish) { reached = true; put(f.output, "race winner"); }
            };
            require(f.convert().status == ConversionStatus::Failed && reached, "publication must lose race");
            require(bytes(f.output) == "race winner", "preserve destination created during transaction"); f.noTemporary();
        }},
        {"introduced destination hard link cannot replace source", [] {
            Fixture f; const auto original = bytes(f.source); bool reached = false;
            ConversionEngine::testHook = [&](auto phase) {
                if (phase == ConversionEngine::TestPhase::BeforePublish) {
                    reached = true; require(CreateHardLinkW(wide(f.output), wide(f.source), nullptr), "race hard link");
                }
            };
            require(f.convert().status == ConversionStatus::Failed && reached, "hard-link destination race rejected");
            require(bytes(f.source) == original && bytes(f.output) == original, "source identity preserved"); f.noTemporary();
        }},
        {"source swap and writer blocked after open", [] {
            Fixture f; bool reached = false; const auto original = bytes(f.source);
            ConversionEngine::testHook = [&](auto phase) {
                if (phase == ConversionEngine::TestPhase::SourceOpened) {
                    reached = true;
                    require(!MoveFileExW(wide(f.source), wide(f.source + QStringLiteral(".old")), 0), "source rename blocked");
                    detail::Handle writer(CreateFileW(wide(f.source), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                     nullptr, OPEN_EXISTING, 0, nullptr));
                    require(!writer.valid(), "source write blocked");
                }
            };
            f.converted(f.convert()); require(reached && bytes(f.source) == original, "unchanged opened source");
        }},
        {"native read error remains a negative QIODevice result", [] {
            Fixture f; std::unique_ptr<detail::Handle> locker; OVERLAPPED range = {}; bool reached = false;
            ConversionEngine::testHook = [&](auto phase) {
                if (phase == ConversionEngine::TestPhase::SourceOpened) {
                    reached = true;
                    locker = std::make_unique<detail::Handle>(CreateFileW(wide(f.source), GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
                    require(locker->valid(), "read-lock fixture handle");
                    require(LockFileEx(locker->value, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 32, 0, &range), "exclusive byte range fixture");
                }
            };
            require(f.convert().status == ConversionStatus::Rejected && reached, "native locked read refused safely");
            require(!QFile::exists(f.output), "no output after read error");
        }},
        {"destination ancestor swap blocked after pinning", [] {
            Fixture f; const auto parent = f.directory.filePath(QStringLiteral("parent"));
            require(QDir().mkpath(parent), "destination parent"); f.output = parent + QStringLiteral("/out.bmp"); bool reached = false;
            ConversionEngine::testHook = [&](auto phase) {
                if (phase == ConversionEngine::TestPhase::TemporaryOpened) {
                    reached = true;
                    require(!MoveFileExW(wide(parent), wide(parent + QStringLiteral(".old")), 0), "directory rename blocked");
                    require(!junction(parent, f.directory.path(), FILE_WRITE_ATTRIBUTES), "nonempty pinned directory cannot become a junction");
                }
            };
            f.converted(f.convert()); require(reached, "destination pin barrier reached");
        }},
        {"temporary substitution blocked", [] {
            Fixture f; bool reached = false;
            ConversionEngine::testHook = [&](auto phase) {
                if (phase == ConversionEngine::TestPhase::TemporaryOpened) {
                    reached = true;
                    const auto names = QDir(f.directory.path()).entryList({QStringLiteral(".audacity-convert-*.tmp")}, QDir::Files | QDir::Hidden);
                    require(names.size() == 1, "one private temporary");
                    const QString temp = f.directory.filePath(names.first());
                    require(!DeleteFileW(wide(temp)), "temporary deletion blocked");
                    detail::Handle writer(CreateFileW(wide(temp), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                     nullptr, OPEN_EXISTING, 0, nullptr));
                    require(!writer.valid(), "temporary writes blocked");
                }
            };
            f.converted(f.convert()); require(reached, "temporary barrier reached");
        }},
        {"empty parent redirected before temporary creation", [] {
            Fixture f; const auto parent = f.directory.filePath(QStringLiteral("parent"));
            const auto other = f.directory.filePath(QStringLiteral("other"));
            require(QDir().mkpath(parent) && QDir().mkpath(other), "race directories");
            f.output = parent + QStringLiteral("/out.bmp"); bool reached = false;
            ConversionEngine::testHook = [&](auto phase) {
                if (phase == ConversionEngine::TestPhase::DestinationPinned) {
                    reached = true;
                    require(junction(parent, other, FILE_WRITE_ATTRIBUTES), "attribute-only empty-parent redirection fixture");
                }
            };
            require(f.convert().status == ConversionStatus::Failed && reached, "reject redirected temporary handle");
            require(QDir(other).entryList(QDir::Files | QDir::Hidden).isEmpty(), "redirected temporary removed by handle");
            require(RemoveDirectoryW(wide(parent)), "remove race junction");
        }},
        {"nonempty parent resists attribute-only reparse mutation", [] {
            Fixture f; const auto parent = f.directory.filePath(QStringLiteral("parent"));
            const auto other = f.directory.filePath(QStringLiteral("other"));
            require(QDir().mkpath(parent) && QDir().mkpath(other), "race directories");
            f.output = parent + QStringLiteral("/out.bmp"); bool reached = false;
            ConversionEngine::testHook = [&](auto phase) {
                if (phase == ConversionEngine::TestPhase::BeforePublish) {
                    reached = true;
                    require(!junction(parent, other, FILE_WRITE_ATTRIBUTES), "NTFS refuses reparse on parent containing exclusive temporary");
                }
            };
            f.converted(f.convert()); require(reached, "reparse barrier reached");
            require(QDir(other).entryList(QDir::Files | QDir::Hidden).isEmpty(), "no redirected output");
        }},
        {"oversized valid BMP dimensions rejected", [] {
            Fixture f; QByteArray header(54, 0); header[0] = 'B'; header[1] = 'M';
            qToLittleEndian<quint32>(54, header.data() + 2); qToLittleEndian<quint32>(54, header.data() + 10);
            qToLittleEndian<quint32>(40, header.data() + 14); qToLittleEndian<quint32>(50000, header.data() + 18);
            qToLittleEndian<quint32>(50000, header.data() + 22); qToLittleEndian<quint16>(1, header.data() + 26);
            qToLittleEndian<quint16>(24, header.data() + 28); put(f.source, header);
            const auto result = ConversionEngine().convert({f.source, f.output, QStringLiteral("png")});
            require(result.status == ConversionStatus::Rejected && result.message.contains(QStringLiteral("dimensions")), "reject header bomb at dimension validation");
            require(!QFile::exists(f.output), "no oversized publication");
        }},
        {"oversized opened input rejected", [] {
            Fixture f; QFile input(f.source); require(input.open(QIODevice::ReadWrite), "input fixture");
            require(input.resize(ConversionEngine::MaxInputBytes + 1), "oversized fixture"); input.close();
            require(f.convert().status == ConversionStatus::Rejected, "bound input handle size");
            require(ConversionEngine::detectFormat(f.source).isEmpty(), "bounded detection size");
        }},
        {"truncated image rejected", [] {
            Fixture f; const auto original = bytes(f.source); put(f.source, original.left(32));
            require(f.convert().status == ConversionStatus::Rejected, "reject truncated image data");
            require(!QFile::exists(f.output), "no truncated publication");
        }},
        {"unsupported path forms rejected", [] {
            Fixture f;
            for (const QString& path : {QStringLiteral("relative.bmp"), f.output + QStringLiteral(":stream"),
                 f.directory.filePath(QStringLiteral("../escape.bmp")), f.directory.filePath(QStringLiteral("NUL.bmp")), f.output + QLatin1Char('.')}) {
                require(ConversionEngine().convert({f.source, path, QStringLiteral("bmp")}).status == ConversionStatus::Rejected, "reject ambiguous destination");
            }
        }},
        {"bounded handle output refuses overflow", [] {
            Fixture f; detail::PinnedPath path; require(path.open(f.output), "pin output");
            { detail::HandleDevice output(path.temporaryPath(), true, 8, nullptr);
              require(output.isOpen(), "create bounded temporary"); require(output.write("123456789", 9) == -1, "output limit enforced"); }
            f.noTemporary();
        }},
        {"cancellation before source open", [] {
            Fixture f; std::atomic_bool cancel = true;
            require(f.convert(&cancel).status == ConversionStatus::Cancelled, "pre-open cancellation"); require(!QFile::exists(f.output), "no cancelled output");
        }},
        {"cancellation at source open", [] {
            Fixture f; std::atomic_bool cancel = false;
            ConversionEngine::testHook = [&](auto phase) { if (phase == ConversionEngine::TestPhase::SourceOpened) cancel = true; };
            require(f.convert(&cancel).status == ConversionStatus::Cancelled, "source barrier cancellation"); require(!QFile::exists(f.output), "no cancelled output");
        }},
        {"cancellation during temporary output", [] {
            Fixture f; std::atomic_bool cancel = false;
            ConversionEngine::testHook = [&](auto phase) { if (phase == ConversionEngine::TestPhase::TemporaryOpened) cancel = true; };
            require(f.convert(&cancel).status == ConversionStatus::Cancelled, "write barrier cancellation"); require(!QFile::exists(f.output), "no cancelled output"); f.noTemporary();
        }},
        {"cancellation immediately before publication", [] {
            Fixture f; std::atomic_bool cancel = false;
            ConversionEngine::testHook = [&](auto phase) { if (phase == ConversionEngine::TestPhase::BeforePublish) cancel = true; };
            require(f.convert(&cancel).status == ConversionStatus::Cancelled, "commit barrier cancellation"); require(!QFile::exists(f.output), "no cancelled output"); f.noTemporary();
        }}
    };
    int failed = 0;
    for (const auto& test : tests) {
        ConversionEngine::testHook = {};
        try { test.second(); std::fprintf(stderr, "PASS %s\n", test.first); }
        catch (const std::exception& error) { ++failed; std::fprintf(stderr, "FAIL %s: %s (Win32 %lu)\n", test.first, error.what(), GetLastError()); }
        ConversionEngine::testHook = {};
    }
    std::fprintf(stderr, "Native transaction cases: %zu passed, %d failed, 0 skipped\n", tests.size() - failed, failed);
    return failed ? 1 : 0;
}

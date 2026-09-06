/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QUuid>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace au::converter::detail {
class Handle
{
public:
    explicit Handle(HANDLE value = INVALID_HANDLE_VALUE) : value(value) {}
    ~Handle() { if (valid()) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    bool valid() const { return value != INVALID_HANDLE_VALUE; }
    HANDLE value;
};

inline bool information(HANDLE handle, BY_HANDLE_FILE_INFORMATION& info, bool directory)
{
    return GetFileType(handle) == FILE_TYPE_DISK && GetFileInformationByHandle(handle, &info)
           && !(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
           && bool(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == directory;
}

inline bool sameIdentity(const BY_HANDLE_FILE_INFORMATION& a, const BY_HANDLE_FILE_INFORMATION& b)
{
    return a.dwVolumeSerialNumber == b.dwVolumeSerialNumber
           && a.nFileIndexHigh == b.nFileIndexHigh && a.nFileIndexLow == b.nFileIndexLow;
}

inline QString resolvedPath(HANDLE handle)
{
    wchar_t path[32768] = {};
    const DWORD length = GetFinalPathNameByHandleW(handle, path, 32768, FILE_NAME_NORMALIZED | VOLUME_NAME_GUID);
    return length && length < 32768 ? QString::fromWCharArray(path, int(length)) : QString();
}

inline bool resolvesTo(HANDLE handle, const QString& expected)
{
    const QString actual = resolvedPath(handle);
    return !actual.isEmpty() && actual.compare(expected, Qt::CaseInsensitive) == 0;
}

// Every ancestor remains open without delete sharing until publication. Write
// sharing is necessary for the native rename's destination-directory open.
// This prevents renaming an ancestor. Resolved-path validation catches an empty
// directory changed into a reparse point through attribute-only access before
// its child opens. Once the child is open, NTFS refuses a reparse point on its
// nonempty parent. Only local, fixed NTFS volumes are
// supported; network, device, stream and ambiguous DOS path forms fail closed.
class PinnedPath
{
public:
    bool open(const QString& input)
    {
        const QString path = QDir::fromNativeSeparators(input);
        if (path.size() < 4 || !path[0].isLetter() || path.mid(1, 2) != QStringLiteral(":/")
            || path.contains(QChar::Null) || path.mid(2).contains(QLatin1Char(':'))) return false;
        const QStringList parts = path.mid(3).split(QLatin1Char('/'));
        for (const QString& part : parts) {
            if (part.isEmpty() || part.endsWith(QLatin1Char('.')) || part.endsWith(QLatin1Char(' '))) return false;
            for (const QChar ch : part) {
                if (ch.unicode() < 32 || QStringLiteral("<>\"|?*").contains(ch)) return false;
            }
            const QString stem = part.section(QLatin1Char('.'), 0, 0).toUpper();
            if (stem == QStringLiteral("CON") || stem == QStringLiteral("PRN") || stem == QStringLiteral("AUX")
                || stem == QStringLiteral("NUL") || stem == QStringLiteral("CONIN$") || stem == QStringLiteral("CONOUT$")
                || (stem.size() == 4 && (stem.startsWith(QStringLiteral("COM")) || stem.startsWith(QStringLiteral("LPT")))
                    && (stem[3].isDigit() || QStringLiteral("¹²³").contains(stem[3])))) return false;
        }
        const QString root = QDir::toNativeSeparators(path.left(3));
        if (GetDriveTypeW(reinterpret_cast<LPCWSTR>(root.utf16())) != DRIVE_FIXED || !pin(root, false)) return false;
        wchar_t filesystem[32] = {};
        if (!GetVolumeInformationByHandleW(m_directories.back()->value, nullptr, 0, nullptr, nullptr, nullptr,
                                          filesystem, 32) || QString::fromWCharArray(filesystem) != QStringLiteral("NTFS")) return false;
        // Bind subsequent path resolution to the opened volume, not a mutable
        // drive-letter mapping. Keep the root handle alive with the other pins.
        m_parent = resolvedPath(m_directories.back()->value);
        if (m_parent.isEmpty()) return false;
        if (!m_parent.endsWith(QLatin1Char('\\'))) m_parent += QLatin1Char('\\');
        for (int i = 0; i + 1 < parts.size(); ++i) {
            m_parent += parts[i];
            if (!pin(m_parent)) return false;
            m_parent += QLatin1Char('\\');
        }
        m_path = m_parent + parts.last();
        return true;
    }
    QString path() const { return m_path; }
    QString temporaryPath() const { return m_parent + QStringLiteral(".audacity-convert-") + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".tmp"); }
private:
    bool pin(const QString& path, bool validatePath = true)
    {
        auto handle = std::make_unique<Handle>(CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),
              FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        BY_HANDLE_FILE_INFORMATION info = {};
        if (!handle->valid() || !information(handle->value, info, true)
            || (validatePath && !resolvesTo(handle->value, path))) return false;
        m_directories.push_back(std::move(handle));
        return true;
    }
    QString m_parent;
    QString m_path;
    std::vector<std::unique_ptr<Handle>> m_directories;
};

// No filename reopen occurs between signature inspection, decoding, encoding,
// output validation and rename. Unbuffered QIODevice IO is bounded and seekable.
class HandleDevice final : public QIODevice
{
public:
    HandleDevice(const QString& path, bool temporary, qint64 limit, const std::atomic_bool* cancellation)
        : m_handle(CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),
                   temporary ? GENERIC_READ | GENERIC_WRITE | DELETE : GENERIC_READ,
                   temporary ? 0 : FILE_SHARE_READ, nullptr, temporary ? CREATE_NEW : OPEN_EXISTING,
                   FILE_FLAG_OPEN_REPARSE_POINT | (temporary ? FILE_ATTRIBUTE_TEMPORARY : 0), nullptr)),
          m_temporary(temporary), m_limit(limit), m_cancellation(cancellation)
    {
        if (!m_handle.valid() || !information(m_handle.value, m_initial, false) || !resolvesTo(m_handle.value, path)) return;
        const qint64 length = size();
        if (length < 0 || length > limit || (!temporary && !length)) return;
        QIODevice::open((temporary ? ReadWrite : ReadOnly) | Unbuffered);
    }
    ~HandleDevice() override
    {
        if (m_temporary && m_handle.valid()) {
            FILE_DISPOSITION_INFO remove = { TRUE };
            SetFileInformationByHandle(m_handle.value, FileDispositionInfo, &remove, sizeof(remove));
        }
    }
    qint64 size() const override
    {
        LARGE_INTEGER length = {};
        return GetFileSizeEx(m_handle.value, &length) ? length.QuadPart : -1;
    }
    bool seek(qint64 offset) override
    {
        if (offset < 0 || offset > m_limit || interrupted()) return false;
        LARGE_INTEGER position = {};
        position.QuadPart = offset;
        return SetFilePointerEx(m_handle.value, position, nullptr, FILE_BEGIN) && QIODevice::seek(offset);
    }
    bool unchanged() const
    {
        BY_HANDLE_FILE_INFORMATION current = {};
        return information(m_handle.value, current, false) && sameIdentity(m_initial, current)
               && current.nFileSizeHigh == m_initial.nFileSizeHigh && current.nFileSizeLow == m_initial.nFileSizeLow
               && CompareFileTime(&current.ftLastWriteTime, &m_initial.ftLastWriteTime) == 0;
    }
    const BY_HANDLE_FILE_INFORMATION& identity() const { return m_initial; }
    bool publish(const QString& target)
    {
        BY_HANDLE_FILE_INFORMATION current = {};
        if (interrupted() || !information(m_handle.value, current, false) || !sameIdentity(m_initial, current)
            || !FlushFileBuffers(m_handle.value)) return false;
        const DWORD nameBytes = DWORD(target.size() * sizeof(wchar_t));
        std::vector<unsigned char> storage(sizeof(FILE_RENAME_INFO) + nameBytes, 0);
        auto* rename = reinterpret_cast<FILE_RENAME_INFO*>(storage.data());
        rename->ReplaceIfExists = FALSE;
        rename->RootDirectory = nullptr;
        rename->FileNameLength = nameBytes;
        memcpy(rename->FileName, target.utf16(), nameBytes);
        if (interrupted() || !SetFileInformationByHandle(m_handle.value, FileRenameInfo, rename, DWORD(storage.size()))) return false;
        m_temporary = false;
        return true;
    }
protected:
    qint64 readData(char* data, qint64 amount) override
    {
        if (interrupted() || pos() < 0 || pos() > m_limit) return -1;
        const qint64 remaining = std::min(m_limit - pos(), size() - pos());
        if (remaining <= 0) return 0;
        DWORD read = 0;
        const DWORD bounded = DWORD(std::min({ amount, remaining, qint64(1024 * 1024) }));
        return ReadFile(m_handle.value, data, bounded, &read, nullptr) ? qint64(read) : qint64(-1);
    }
    qint64 writeData(const char* data, qint64 amount) override
    {
        if (interrupted() || amount < 0 || pos() < 0 || amount > m_limit - pos()) return -1;
        DWORD written = 0;
        const DWORD bounded = DWORD(std::min(amount, qint64(1024 * 1024)));
        return WriteFile(m_handle.value, data, bounded, &written, nullptr) ? qint64(written) : qint64(-1);
    }
private:
    bool interrupted() const { return m_cancellation && m_cancellation->load(std::memory_order_acquire); }
    Handle m_handle;
    bool m_temporary;
    qint64 m_limit;
    const std::atomic_bool* m_cancellation;
    BY_HANDLE_FILE_INFORMATION m_initial = {};
};
}
#endif

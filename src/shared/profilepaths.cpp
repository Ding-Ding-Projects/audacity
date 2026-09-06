/* Audacity: isolated verification profiles. */
#include "profilepaths.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSaveFile>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace au::profile {
namespace {
QString profileRoot;
bool settingsStarted = false;
bool initialized = false;
const char* markerName = ".audacity-isolated-profile.json";
QString normalized(const QString& p) { return QDir::cleanPath(QDir::fromNativeSeparators(p)); }
QString canonicalPath(QString path)
{
    path = normalized(path);
    QStringList suffix;
    while (!QFileInfo::exists(path)) {
        const QFileInfo info(path);
        const QString parent = info.dir().absolutePath();
        if (parent == path) return {};
        suffix.prepend(info.fileName());
        path = parent;
    }
#ifdef Q_OS_WIN
    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), 0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return {};
    wchar_t buffer[32768];
    const DWORD length = GetFinalPathNameByHandleW(handle, buffer, 32768, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    CloseHandle(handle);
    if (length == 0 || length >= 32768) return {};
    path = QString::fromWCharArray(buffer, int(length));
    if (path.startsWith("\\\\?\\")) path.remove(0, 4);
#else
    path = QFileInfo(path).canonicalFilePath();
#endif
    if (!suffix.isEmpty()) path += '/' + suffix.join('/');
    return normalized(path);
}
bool reparse(const QString& path)
{
    if (QFileInfo(path).isSymLink()) return true;
#ifdef Q_OS_WIN
    const DWORD attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()));
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    return false;
#endif
}
bool safeAncestors(QString path)
{
    while (!path.isEmpty()) {
        if (reparse(path)) return false;
        const QString parent = QFileInfo(path).dir().absolutePath();
        if (parent == path) break;
        path = parent;
    }
    return true;
}
QString subfolder(QStandardPaths::StandardLocation location)
{
    switch (location) {
    case QStandardPaths::AppDataLocation: return "data/roaming";
    case QStandardPaths::AppLocalDataLocation: return "data/local";
    case QStandardPaths::GenericDataLocation: return "data/shared";
    case QStandardPaths::ConfigLocation: case QStandardPaths::AppConfigLocation:
    case QStandardPaths::GenericConfigLocation: return "config";
    case QStandardPaths::CacheLocation: case QStandardPaths::GenericCacheLocation: return "cache";
    case QStandardPaths::TempLocation: return "temp";
    case QStandardPaths::DocumentsLocation: return "documents";
    case QStandardPaths::DownloadLocation: return "downloads";
    case QStandardPaths::HomeLocation: return "home";
    case QStandardPaths::DesktopLocation: return "desktop";
    case QStandardPaths::MusicLocation: return "music";
    case QStandardPaths::MoviesLocation: return "movies";
    case QStandardPaths::PicturesLocation: return "pictures";
    case QStandardPaths::RuntimeLocation: return "runtime";
    case QStandardPaths::GenericStateLocation: case QStandardPaths::StateLocation: return "state";
    default: return "other-" + QString::number(int(location));
    }
}
}
bool Paths::contains(const QString& parent, const QString& child)
{
#ifdef Q_OS_WIN
    constexpr auto cs = Qt::CaseInsensitive;
#else
    constexpr auto cs = Qt::CaseSensitive;
#endif
    const QString p = normalized(parent), c = normalized(child);
    return c.compare(p, cs) == 0 || c.startsWith(p.endsWith('/') ? p : p + '/', cs);
}
bool Paths::initialize(const QString& requested, QString* error)
{
    auto fail = [error](const QString& message) { if (error) *error = message; return false; };
    if (initialized || settingsStarted || QCoreApplication::instance())
        return fail("The verification profile must be selected once, before application or settings initialization.");
    if (requested.isEmpty()) { initialized = true; return true; }
    if (!QDir::isAbsolutePath(requested)) return fail("The verification profile must be an absolute directory.");
    const QString input = normalized(requested);
#ifdef Q_OS_WIN
    // Win32 strips trailing dots/spaces and interprets colons as data streams.
    // Refuse ambiguous spellings before any directory is created.
    const auto components = input.mid(3).split('/');
    for (const QString& component : components)
        if (component.endsWith('.') || component.endsWith(' ') || component.contains(':'))
            return fail("The profile contains an ambiguous Windows path component.");
#endif
    if (input.startsWith("//") || !safeAncestors(input))
        return fail("Network paths, symbolic links, and reparse points cannot own a verification profile.");
    const QString root = canonicalPath(input);
    if (root.isEmpty() || root.startsWith("//") || !safeAncestors(root))
        return fail("Network paths, symbolic links, and reparse points cannot own a verification profile.");
    // Container roots are protected themselves and as ancestors. A new scratch
    // child of Home or Temp is allowed; real application profile subtrees are not.
    for (auto location : { QStandardPaths::HomeLocation, QStandardPaths::DocumentsLocation,
                          QStandardPaths::GenericDataLocation, QStandardPaths::GenericConfigLocation,
                          QStandardPaths::TempLocation, QStandardPaths::DownloadLocation }) {
        const QString p = canonicalPath(QStandardPaths::writableLocation(location));
        if (!p.isEmpty() && contains(root, p)) return fail("The selected directory contains a protected user location.");
    }
    QStringList profiles;
    for (auto location : { QStandardPaths::AppDataLocation, QStandardPaths::AppLocalDataLocation,
                          QStandardPaths::AppConfigLocation, QStandardPaths::CacheLocation }) {
        profiles << canonicalPath(QStandardPaths::writableLocation(location));
    }
    for (auto location : { QStandardPaths::GenericDataLocation, QStandardPaths::GenericConfigLocation }) {
        const QString base = QStandardPaths::writableLocation(location);
        for (const QString& name : { QString("Audacity"), QString("audacity"), QString("Audacity4"), QString("Audacity4Development") })
            profiles << canonicalPath(base + '/' + name);
    }
    for (const QString& p : profiles)
        if (!p.isEmpty() && (contains(root, p) || contains(p, root)))
            return fail("The selected directory overlaps an actual application profile.");
    const QFileInfo info(root);
    if (info.exists() && !info.isDir()) return fail("The profile is not a directory.");
    const bool empty = !info.exists() || QDir(root).entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot).isEmpty();
    if (!empty) {
        QFile marker(root + '/' + markerName);
        if (!marker.open(QIODevice::ReadOnly) || marker.size() > 4096)
            return fail("The non-empty directory has no bounded ownership marker.");
        const auto object = QJsonDocument::fromJson(marker.readAll()).object();
        if (object.size() != 3 || object.value("owner").toString() != "audacity-verification"
            || object.value("version").toInt() != 1 || object.value("root").toString() != root)
            return fail("The profile ownership marker is invalid or belongs to another path.");
        QDirIterator it(root, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) if (reparse(it.next())) return fail("The profile contains a symbolic link or reparse point.");
    }
    if (!QDir().mkpath(root)) return fail("The profile directory could not be created.");
    if (!safeAncestors(root)) return fail("The profile path changed during creation.");
    if (empty) {
        QSaveFile marker(root + '/' + markerName);
        const QByteArray bytes = QJsonDocument(QJsonObject{{"owner", "audacity-verification"}, {"version", 1}, {"root", root}}).toJson();
        if (!marker.open(QIODevice::WriteOnly) || marker.write(bytes) != bytes.size() || !marker.commit())
            return fail("The profile ownership marker could not be committed.");
    }
    for (const QString& child : { "config", "config-system", "data/roaming", "data/local", "data/shared", "cache", "temp", "documents", "downloads", "home", "desktop", "music", "movies", "pictures", "runtime", "state" }) {
        if (!QDir().mkpath(root + '/' + child)) return fail("A profile storage directory could not be created.");
    }
    profileRoot = root;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, root + "/config");
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, root + "/config-system");
    initialized = true;
    return true;
}
bool Paths::initializeArguments(const QStringList& arguments, QString* error)
{
    QString requested;
    bool found = false;
    for (int i = 1; i < arguments.size(); ++i) {
        const QString arg = arguments[i];
        if (arg == "--") break;
        if (arg == "--profile-dir" || arg.startsWith("--profile-dir=")) {
            if (found) { if (error) *error = "Only one --profile-dir is allowed."; return false; }
            found = true;
            if (arg == "--profile-dir") {
                if (++i >= arguments.size() || arguments[i].startsWith('-')) { if (error) *error = "--profile-dir needs an absolute path."; return false; }
                requested = arguments[i];
            } else requested = arg.mid(14);
            if (requested.isEmpty()) { if (error) *error = "--profile-dir cannot be empty."; return false; }
        }
    }
    return initialize(requested, error);
}
void Paths::settingsAccessed() { settingsStarted = true; }
bool Paths::active() { return !profileRoot.isEmpty(); }
QString Paths::root() { return profileRoot; }
QString Paths::writableLocation(QStandardPaths::StandardLocation location)
{
    if (!active()) return QStandardPaths::writableLocation(location);
    const QString path = profileRoot + '/' + subfolder(location);
    // Detect replacement after initialization rather than falling back to real data.
    if (!safeAncestors(path)) qFatal("The isolated profile storage path was replaced.");
    return path;
}
QString Paths::temporaryPath() { return active() ? writableLocation(QStandardPaths::TempLocation) : QDir::tempPath(); }
QString Paths::ipcName(const QString& name)
{
    if (!active()) return name;
    QString canonical = profileRoot;
#ifdef Q_OS_WIN
    canonical = canonical.toCaseFolded();
#endif
    return name + "-profile-" + QString::fromLatin1(QCryptographicHash::hash(canonical.toUtf8(), QCryptographicHash::Sha256).toHex());
}
QStringList Paths::childArguments(const QStringList& arguments)
{
    if (!active()) return arguments;
    QStringList result { "--profile-dir", profileRoot };
    result.append(arguments);
    return result;
}
}

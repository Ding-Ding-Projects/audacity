#include "profilepaths.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
using au::profile::Paths;
Q_DECL_IMPORT bool consumerProfileActive();
Q_DECL_IMPORT QString consumerProfileRoot();
static int checks = 0;
static void check(bool result, const char* name) {
    ++checks;
    if (!result) { fprintf(stderr, "FAIL %s\n", name); std::exit(1); }
}
static bool child(const QString& exe, const QStringList& args, QByteArray* out = nullptr) {
    QProcess p; p.start(exe, args);
    if (!p.waitForStarted(5000) || !p.waitForFinished(20000)) return false;
    if (out) *out = p.readAllStandardOutput();
    if (p.exitCode() != 0) fprintf(stderr, "%s", p.readAllStandardError().constData());
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}
int main(int argc, char** argv) {
    QCoreApplication::setOrganizationName("Audacity");
    QCoreApplication::setApplicationName("Audacity4");
    QStringList args;
    for (int i=0; i<argc; ++i) args << QString::fromLocal8Bit(argv[i]);
    if (args.size() > 1) {
        const QString mode = args[1];
        QString error;
        if (mode == "reject") return Paths::initialize(args[2], &error) ? 1 : 0;
        if (mode == "late-settings") { Paths::settingsAccessed(); return Paths::initialize(args[2], &error) ? 1 : 0; }
        if (mode == "late-app") { QCoreApplication app(argc, argv); return Paths::initialize(args[2], &error) ? 1 : 0; }
        if (mode == "arguments") {
            check(!Paths::initializeArguments({"app", "--profile-dir="}, &error), "empty flag");
            check(!Paths::initializeArguments({"app", "--profile-dir"}, &error), "missing flag value");
            check(!Paths::initializeArguments({"app", "--profile-dir", args[2], "--profile-dir", args[2]}, &error), "duplicate flag");
            check(Paths::initializeArguments({"app", "--profile-dir=" + args[2]}, &error), "equals flag");
            return 0;
        }
        check(Paths::initialize(args[2], &error), qPrintable(error));
        check(Paths::active(), "profile active");
        check(consumerProfileActive() && consumerProfileRoot() == Paths::root(), "singleton across DLLs");
        check(!Paths::initialize(args[2], &error), "reinitialization rejected");
        QCoreApplication app(argc, argv);
        check(QCoreApplication::applicationName() == "Audacity4", "identity preserved");
        for (auto location : {QStandardPaths::ConfigLocation, QStandardPaths::AppConfigLocation,
                QStandardPaths::AppDataLocation, QStandardPaths::AppLocalDataLocation,
                QStandardPaths::GenericDataLocation, QStandardPaths::CacheLocation,
                QStandardPaths::TempLocation, QStandardPaths::DocumentsLocation,
                QStandardPaths::DownloadLocation, QStandardPaths::HomeLocation}) {
            const QString path = Paths::writableLocation(location);
            check(Paths::contains(args[2], path) && QDir(path).exists(), "redirected location");
            QFile f(path + "/fixture.txt"); check(f.open(QIODevice::WriteOnly), "fixture write"); f.write("fixture");
        }
        QSettings settings;
        check(settings.format() == QSettings::IniFormat, "INI format");
        check(Paths::contains(args[2], settings.fileName()), "user settings contained");
        QSettings system(QSettings::IniFormat, QSettings::SystemScope, "Audacity", "Audacity4");
        check(Paths::contains(args[2], system.fileName()), "system settings contained");
        settings.setValue("fixture", args[2]); settings.sync();
        check(settings.status() == QSettings::NoError, "settings sync");
        const QString id = Paths::ipcName("Audacity4");
        check(id != "Audacity4", "IPC isolation");
        check(Paths::childArguments({"--factory-settings"}) == QStringList({"--profile-dir", Paths::root(), "--factory-settings"}), "child inheritance");
        printf("%s\n", id.toUtf8().constData());
        return 0;
    }
    QCoreApplication app(argc, argv);
    QTemporaryDir fixture;
    check(fixture.isValid(), "fixture root");
    const QString exe = QCoreApplication::applicationFilePath();
    check(Paths::contains("C:/root", "C:/root/child"), "contain child");
    check(!Paths::contains("C:/root", "C:/root-other"), "sibling boundary");
    check(!Paths::contains("C:/root", "C:/root/../escape"), "traversal boundary");
    check(child(exe, {"reject", "relative/path"}), "relative rejected");
    for (auto location : {QStandardPaths::HomeLocation, QStandardPaths::DocumentsLocation,
            QStandardPaths::GenericDataLocation, QStandardPaths::AppDataLocation,
            QStandardPaths::AppLocalDataLocation, QStandardPaths::AppConfigLocation})
        check(child(exe, {"reject", QStandardPaths::writableLocation(location)}), "real location rejected");
    check(child(exe, {"reject", QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/fixture"}), "real profile descendant rejected");
#ifdef Q_OS_WIN
    check(child(exe, {"reject", fixture.path() + "/ambiguous."}), "trailing dot rejected");
    check(child(exe, {"reject", fixture.path() + "/ambiguous "}), "trailing space rejected");
    check(child(exe, {"reject", fixture.path() + "/name:stream"}), "alternate data stream rejected");
    const QString realHome = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    wchar_t shortName[32768];
    const DWORD shortLength = GetShortPathNameW(reinterpret_cast<LPCWSTR>(realHome.utf16()), shortName, 32768);
    check(shortLength > 0 && shortLength < 32768, "home short spelling available");
    check(child(exe, {"reject", QString::fromWCharArray(shortName, shortLength)}), "home alternate spelling rejected");
#endif
    QString occupied = fixture.path() + "/occupied"; QDir().mkpath(occupied);
    QFile sentinel(occupied + "/sentinel"); check(sentinel.open(QIODevice::WriteOnly), "sentinel"); sentinel.write("untouched"); sentinel.close();
    check(child(exe, {"reject", occupied}), "unowned directory rejected");
    check(sentinel.open(QIODevice::ReadOnly) && sentinel.readAll() == "untouched", "sentinel unchanged"); sentinel.close();
    check(child(exe, {"late-settings", fixture.path() + "/late-settings"}), "settings install order");
    check(child(exe, {"late-app", fixture.path() + "/late-app"}), "application install order");
    check(child(exe, {"arguments", fixture.path() + "/arguments"}), "argument parsing");
    QString a = fixture.path() + "/profile-a", b = fixture.path() + "/profile-b";
    QProcess pa, pb; pa.start(exe, {"write", a}); pb.start(exe, {"write", b});
    check(pa.waitForStarted(5000) && pb.waitForStarted(5000), "parallel profiles start");
    check(pa.waitForFinished(20000) && pb.waitForFinished(20000), "parallel profiles finish");
    if(pa.exitCode()) fprintf(stderr,"%s",pa.readAllStandardError().constData());
    if(pb.exitCode()) fprintf(stderr,"%s",pb.readAllStandardError().constData());
    check(pa.exitCode() == 0 && pb.exitCode() == 0, "parallel profiles succeed");
    const QByteArray aid = pa.readAllStandardOutput(), bid = pb.readAllStandardOutput();
    check(!aid.isEmpty() && !bid.isEmpty() && aid != bid, "parallel IPC names differ");
    QByteArray reopened; check(child(exe, {"write", a}, &reopened) && reopened == aid, "owned profile reopens with stable IPC");
    QFile marker(a + "/.audacity-isolated-profile.json"); check(marker.open(QIODevice::ReadWrite), "marker read");
    const auto old = marker.readAll(); auto object = QJsonDocument::fromJson(old).object(); object["root"] = b;
    marker.resize(0); marker.write(QJsonDocument(object).toJson()); marker.close();
    check(child(exe, {"reject", a}), "marker path transplant rejected");
    check(marker.open(QIODevice::WriteOnly|QIODevice::Truncate), "marker restore"); marker.write(old); marker.close();
#ifdef Q_OS_WIN
    // Junctions are reparse points and do not require Developer Mode privileges.
    QString junction = fixture.path() + "/junction";
    QProcess mklink; mklink.start("cmd.exe", {"/d", "/c", "mklink", "/J", QDir::toNativeSeparators(junction), QDir::toNativeSeparators(a)});
    check(mklink.waitForFinished(5000) && mklink.exitCode() == 0, "junction fixture created");
    check(child(exe, {"reject", junction}), "junction root rejected");
    RemoveDirectoryW(reinterpret_cast<LPCWSTR>(junction.utf16()));
    QString inside = a + "/escaped";
    mklink.start("cmd.exe", {"/d", "/c", "mklink", "/J", QDir::toNativeSeparators(inside), QDir::toNativeSeparators(b)});
    check(mklink.waitForFinished(5000) && mklink.exitCode() == 0, "nested junction fixture");
    check(child(exe, {"reject", a}), "nested junction rejected");
    RemoveDirectoryW(reinterpret_cast<LPCWSTR>(inside.utf16()));
#endif
    printf("PASS %d parent assertions plus child assertions\n", checks);
    return 0;
}

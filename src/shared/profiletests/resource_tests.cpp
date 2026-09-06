#include "profilepaths.h"
#include "ipclock.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <cstdio>
using au::profile::Paths;
bool staticConsumerActive();
QString staticConsumerRoot();
static bool acquired(QProcess& p) {
    if (!p.waitForStarted(5000) || !p.waitForReadyRead(5000)) return false;
    return p.readAllStandardOutput().trimmed() == "acquired";
}
int main(int argc, char** argv) {
    QCoreApplication::setOrganizationName("Audacity");
    QCoreApplication::setApplicationName("Audacity4");
    if (argc >= 3) {
        QString error;
        if (!Paths::initialize(QString::fromLocal8Bit(argv[1]), &error)) return 1;
        QCoreApplication app(argc, argv);
        if (!staticConsumerActive() || staticConsumerRoot() != Paths::root()) return 2;
        // Exercise the exact production IpcLock with the name supplied by the
        // patched MultiProcessProvider::lock. The source contract binds that call.
        const QString logicalName = QString::fromLocal8Bit(argv[2]);
        muse::ipc::IpcLock lock(argc == 4 ? logicalName : Paths::ipcName(logicalName));
        if (!lock.lock()) return 3;
        QTemporaryFile temporary(Paths::temporaryPath() + "/chronicle-bundle-XXXXXX");
        if (!temporary.open() || !Paths::contains(Paths::root(), temporary.fileName())) return 4;
        temporary.write("fixture");
        printf("acquired\n"); fflush(stdout);
        if (getchar() == EOF) return 5;
        return lock.unlock() ? 0 : 6;
    }
    QCoreApplication app(argc, argv);
    QTemporaryDir fixture;
    if (!fixture.isValid()) return 10;
    QProcess first, second;
    const QString executable = QCoreApplication::applicationFilePath();
    const QString resource = "profile-regression-" + fixture.path();
    QStringList firstArgs {fixture.path() + "/first", resource};
    QStringList secondArgs {fixture.path() + "/second", resource};
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == "--legacy-name") {
        firstArgs.append("legacy"); secondArgs.append("legacy");
    }
    first.start(executable, firstArgs);
    if (!acquired(first)) { first.kill(); first.waitForFinished(5000); return 11; }
    second.start(executable, secondArgs);
    const bool parallel = acquired(second) && first.state() == QProcess::Running;
    // Both processes must acquire before either receives permission to release.
    if (parallel) { first.write("\n"); second.write("\n"); }
    else { first.kill(); second.kill(); }
    const bool endedFirst = first.state() == QProcess::NotRunning || first.waitForFinished(5000);
    const bool endedSecond = second.state() == QProcess::NotRunning || second.waitForFinished(5000);
    if (!parallel || !endedFirst || !endedSecond || first.exitCode() != 0 || second.exitCode() != 0) {
        fprintf(stderr, "FAIL independent profile resource acquisition\n"); return 12;
    }
    printf("PASS static provider/consumer linkage, contained temporary files, and concurrent profile resource locks\n");
    return 0;
}

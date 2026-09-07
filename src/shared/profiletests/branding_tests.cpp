#include "profilepaths.h"
#include "brandingstore.h"
#include "brandingmodel.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QTemporaryDir>
#include <QUrl>
#include <cstdio>
using au::profile::Paths;
using au::branding::BrandingStore;
using au::personalize::BrandingModel;
static int checks = 0;
static void check(bool value, const char* name) {
    ++checks;
    if (!value) { fprintf(stderr, "FAIL %s\n", name); std::exit(1); }
}
static void child(const QString& executable, const QStringList& arguments) {
    QProcess process; process.start(executable, arguments);
    check(process.waitForStarted(5000), "branding child started");
    check(process.waitForFinished(20000), "branding child completed");
    if (process.exitCode() != 0) fprintf(stderr, "%s", process.readAllStandardError().constData());
    check(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0, "branding child passed");
    printf("%s", process.readAllStandardOutput().constData());
}
int main(int argc, char** argv) {
    QCoreApplication::setOrganizationName("Audacity");
    QCoreApplication::setApplicationName("Audacity4");
    if (argc == 4) {
        QString error;
        check(Paths::initialize(QString::fromLocal8Bit(argv[1]), &error), qPrintable(error));
        QCoreApplication app(argc, argv);
        const QString operation = QString::fromLocal8Bit(argv[2]);
        const QColor color(QString::fromLocal8Bit(argv[3]));
        // This is the isolated GlobalConfiguration route. The companion source
        // inventory independently binds it to the actual singleton registration.
        const QString brandingRoot = Paths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/personalize";
        const QString cache = brandingRoot + "/branding-v1";
        check(Paths::contains(Paths::root(), brandingRoot), "branding root contained");
        const QByteArray shipped("fixture-shipped-mark");
        BrandingModel model(brandingRoot, shipped);
        if (operation == "save") {
            check(!model.hasCustomLogo(), "new profile has no imported logo");
            QImage image(4, 4, QImage::Format_ARGB32); image.fill(color);
            const QString source = Paths::temporaryPath() + "/fixture.png";
            check(image.save(source, "PNG"), "fixture image saved");
            check(model.loadFile(source), "real model saves logo");
            check(model.hasCustomLogo() && model.statusCode() == "custom-saved", "custom model state");
        }
        if (operation == "reset") {
            check(model.hasCustomLogo(), "reset loaded existing isolated logo");
            model.reset();
            check(!model.hasCustomLogo() && model.statusCode() == "shipped", "model reset restores shipped state");
            check(!QDir(cache).exists(), "reset removes only isolated cache");
            check(model.previewPath().startsWith("data:image/png;base64,"), "shipped mark restored");
        } else {
            check(model.hasCustomLogo(), "existing model reloads isolated logo");
            BrandingStore store(brandingRoot);
            check(store.hasCustomLogo(), "real store reloads logo");
            check(store.derivativePaths().size() == 6, "all six derivatives saved");
            for (const QString& path : store.derivativePaths()) {
                check(Paths::contains(cache, path), "derivative contained");
                check(QImage(path).pixelColor(0, 0) == color, "profile-specific derivative pixels preserved");
            }
            const QUrl preview(model.previewPath());
            check(preview.isLocalFile() && Paths::contains(cache, preview.toLocalFile()), "model preview stays in isolated cache");
            QFile metadata(cache + "/metadata.json");
            check(metadata.open(QIODevice::ReadOnly), "metadata exists");
            const QByteArray bytes = metadata.readAll();
            check(!bytes.contains(Paths::root().toUtf8()), "metadata omits profile path");
        }
        printf("PASS %d combined profile-branding child assertions (%s)\n", checks, operation.toUtf8().constData());
        return 0;
    }
    QCoreApplication app(argc, argv);
    QTemporaryDir fixture;
    check(fixture.isValid(), "fixture root");
    const QString a = fixture.path() + "/profile-a", b = fixture.path() + "/profile-b";
    QFile sentinel(fixture.path() + "/unrelated.txt");
    check(sentinel.open(QIODevice::WriteOnly), "unrelated sentinel prepared"); sentinel.write("preserved"); sentinel.close();
    const QString executable = QCoreApplication::applicationFilePath();
    child(executable, {a, "save", "#ff0000"});
    child(executable, {b, "save", "#0000ff"});
    child(executable, {a, "verify", "#ff0000"});
    child(executable, {a, "reset", "#ff0000"});
    child(executable, {b, "verify", "#0000ff"});
    check(sentinel.open(QIODevice::ReadOnly) && sentinel.readAll() == "preserved", "unrelated sentinel preserved");
    check(!QDir(a + "/data/local/personalize/branding-v1").exists(), "first profile reset persisted");
    check(QDir(b + "/data/local/personalize/branding-v1").exists(), "second profile retained");
    printf("PASS %d combined profile-branding parent assertions; no GUI application created\n", checks);
    return 0;
}

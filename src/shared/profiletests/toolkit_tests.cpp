#include "profilepaths.h"
#include "bookmarkmodel.h"
#include "hardwarefitservice.h"
#include "hardwareprobe.h"
#include "externaleditorservice.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>
using au::profile::Paths;
using namespace au::toolkit;
static int checks = 0;
static void check(bool value, const char* name) {
    ++checks;
    if (!value) { fprintf(stderr, "FAIL %s\n", name); std::exit(1); }
}
int main(int argc, char** argv) {
    QCoreApplication::setOrganizationName("Audacity");
    QCoreApplication::setApplicationName("Audacity4");
    QTemporaryDir fixture;
    QString error;
    check(fixture.isValid(), "fixture root");
    check(Paths::initialize(fixture.path() + "/profile", &error), qPrintable(error));
    QCoreApplication app(argc, argv);
    BookmarkModel bookmarks;
    check(bookmarks.rowCount() == 0, "empty isolated bookmarks");
    bookmarks.add("isolation-evidence", "Profile fixture");
    const QString store = Paths::writableLocation(QStandardPaths::AppDataLocation) + "/toolkit/docs-bookmarks.json";
    check(Paths::contains(Paths::root(), store) && QFile::exists(store), "bookmark file is contained");
    BookmarkModel reopened;
    check(reopened.isBookmarked("isolation-evidence"), "bookmark reopened from isolated store");
    const auto hardware = HardwareProbe::measure("C:/");
    check(hardware.totalRamBytes == 0 && hardware.freeDiskBytes == 0 && hardware.vramBytes == -1, "hardware measurements unavailable");
    HardwareFitService fit;
    check(fit.evidenceSummary() == "Hardware probing is unavailable in an isolated verification profile.", "truthful hardware status");
    check(fit.verdictFor(1024) == "Unknown", "hardware fit not guessed");
    ExternalEditorService editors;
    editors.addCustomEditor("fixture", "Existing fixture executable", QCoreApplication::applicationFilePath());
    bool foundFixture = false;
    for (const QVariant& item : editors.detectEditors()) {
        const QVariantMap entry = item.toMap();
        check(!entry.value("available").toBool(), "editor unavailable");
        check(entry.value("unavailableReason").toString() == "External editors are unavailable in an isolated verification profile.", "editor unavailable reason");
        if (entry.value("id").toString() == "fixture") {
            check(entry.value("found").toBool(), "installed editor fact preserved");
            foundFixture = true;
        }
    }
    check(foundFixture, "custom editor retained");
    check(!editors.openFile("fixture", fixture.path() + "/file"), "file handoff refused");
    check(!editors.openFolder("fixture", fixture.path()), "folder handoff refused");
    printf("PASS %d toolkit profile assertions\n", checks);
    return 0;
}

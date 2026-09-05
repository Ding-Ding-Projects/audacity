/*
* Audacity: A Digital Audio Editor
*
* The guard for docs/inventory/search-inventory.md.
*
* The inventory is hand written, so nothing keeps it true except a test that
* reads the source tree. These tests scan every QML file under src for
* M3SearchBar instances and fail when the inventory and the tree disagree in
* either direction.
*/

#include <gtest/gtest.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>

namespace {
const QString SOURCE_ROOT = QStringLiteral(AU_COMPANION_SOURCE_ROOT);
const QString REPO_ROOT = QStringLiteral(AU_COMPANION_REPO_ROOT);
const QString INVENTORY = REPO_ROOT + QStringLiteral("/docs/inventory/search-inventory.md");

//! The component definition itself is not a field.
const QString SEARCH_BAR_COMPONENT = QStringLiteral("qml/Audacity/M3/M3SearchBar.qml");

QString readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    const QString text = QString::fromUtf8(file.readAll());
    file.close();
    return text;
}

struct FieldSite
{
    QString file;      // repository relative
    QString objectName;
};

//! Every M3SearchBar instance in the tree, with the objectName that follows it.
QVector<FieldSite> scanSearchBars()
{
    QVector<FieldSite> sites;

    QDirIterator it(SOURCE_ROOT, QStringList { QStringLiteral("*.qml") }, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QString relative = QStringLiteral("src/") + QDir(SOURCE_ROOT).relativeFilePath(path);
        if (relative.endsWith(SEARCH_BAR_COMPONENT)) {
            continue;
        }

        const QString text = readFile(path);
        if (!text.contains(QStringLiteral("M3SearchBar {"))) {
            continue;
        }

        // Take the objectName that appears within the few lines that open each
        // M3SearchBar block. Anything further away is a different item.
        static const QRegularExpression instance(
            QStringLiteral("M3SearchBar\\s*\\{((?:[^{}]|\\n){0,400})"));
        static const QRegularExpression objectName(
            QStringLiteral("objectName\\s*:\\s*\"([^\"]+)\""));

        QRegularExpressionMatchIterator matches = instance.globalMatch(text);
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            const QRegularExpressionMatch named = objectName.match(match.captured(1));

            FieldSite site;
            site.file = relative;
            site.objectName = named.hasMatch() ? named.captured(1) : QString();
            sites.append(site);
        }
    }

    return sites;
}

struct InventoryRow
{
    QString surface;
    QString file;
    QString objectName;
    QString anchored;
    QString store;
};

QVector<InventoryRow> readInventory()
{
    QVector<InventoryRow> rows;
    const QString text = readFile(INVENTORY);

    const QStringList lines = text.split(u'\n');
    for (const QString& line : lines) {
        if (!line.startsWith(u'|')) {
            continue;
        }
        QStringList cells = line.split(u'|');
        // A markdown row has empty first and last cells.
        if (cells.size() < 7) {
            continue;
        }
        cells.removeFirst();
        cells.removeLast();
        for (QString& cell : cells) {
            cell = cell.trimmed();
            cell.remove(u'`');
        }
        if (cells.at(0) == QStringLiteral("Surface") || cells.at(0).startsWith(QStringLiteral("---"))) {
            continue;
        }

        InventoryRow row;
        row.surface = cells.at(0);
        row.file = cells.at(1);
        row.objectName = cells.at(2);
        row.anchored = cells.at(3);
        row.store = cells.at(4);
        rows.append(row);
    }
    return rows;
}
}

TEST(SearchInventoryTests, TheInventoryFileExistsAndHasRows)
{
    ASSERT_TRUE(QFile::exists(INVENTORY)) << INVENTORY.toStdString();
    EXPECT_FALSE(readInventory().isEmpty());
}

TEST(SearchInventoryTests, TheSourceTreeContainsSearchFields)
{
    // A failure here means the scan itself is broken rather than the inventory.
    EXPECT_FALSE(scanSearchBars().isEmpty());
}

TEST(SearchInventoryTests, EverySearchFieldCarriesAnObjectName)
{
    for (const FieldSite& site : scanSearchBars()) {
        EXPECT_FALSE(site.objectName.isEmpty())
            << "an M3SearchBar in " << site.file.toStdString()
            << " has no objectName, so it cannot be listed in the search inventory";
    }
}

TEST(SearchInventoryTests, EverySearchFieldHasAnInventoryRow)
{
    const QVector<InventoryRow> rows = readInventory();

    QSet<QString> listed;
    for (const InventoryRow& row : rows) {
        listed.insert(row.objectName);
    }

    for (const FieldSite& site : scanSearchBars()) {
        if (site.objectName.isEmpty()) {
            continue; // reported by the test above
        }
        EXPECT_TRUE(listed.contains(site.objectName))
            << "the search field " << site.objectName.toStdString()
            << " in " << site.file.toStdString()
            << " has no row in docs/inventory/search-inventory.md";
    }
}

TEST(SearchInventoryTests, EveryInventoryRowHasALiveField)
{
    for (const InventoryRow& row : readInventory()) {
        const QString path = REPO_ROOT + u'/' + row.file;
        ASSERT_TRUE(QFile::exists(path))
            << "the inventory names " << row.file.toStdString() << ", which does not exist";

        const QString text = readFile(path);
        EXPECT_TRUE(text.contains(QStringLiteral("objectName: \"") + row.objectName + u'"'))
            << "the inventory names the field " << row.objectName.toStdString()
            << " in " << row.file.toStdString() << ", which no longer carries it";
    }
}

TEST(SearchInventoryTests, EveryAnchoredRowCarriesABuilderInItsOwnFile)
{
    for (const InventoryRow& row : readInventory()) {
        if (row.anchored != QStringLiteral("yes")) {
            continue;
        }
        const QString text = readFile(REPO_ROOT + u'/' + row.file);
        EXPECT_TRUE(text.contains(QStringLiteral("RegexBuilderSheet {")))
            << row.file.toStdString()
            << " claims an anchored regular expression builder but carries no RegexBuilderSheet";
        EXPECT_TRUE(text.contains(QStringLiteral("storeName: \"") + row.store + u'"'))
            << row.file.toStdString() << " does not use the store name the inventory gives it";
    }
}

TEST(SearchInventoryTests, EveryForwardedRowRaisesTheHook)
{
    for (const InventoryRow& row : readInventory()) {
        if (row.anchored != QStringLiteral("forwarded")) {
            continue;
        }
        const QString text = readFile(REPO_ROOT + u'/' + row.file);
        EXPECT_TRUE(text.contains(QStringLiteral("regexBuilderRequested")))
            << row.file.toStdString()
            << " is listed as forwarding to a host but never mentions regexBuilderRequested";
        EXPECT_FALSE(row.store.isEmpty())
            << "the forwarded field " << row.objectName.toStdString()
            << " does not name the host that answers it";
    }
}

TEST(SearchInventoryTests, TheAnchoredColumnOnlyHoldsKnownValues)
{
    for (const InventoryRow& row : readInventory()) {
        EXPECT_TRUE(row.anchored == QStringLiteral("yes")
                    || row.anchored == QStringLiteral("forwarded")
                    || row.anchored == QStringLiteral("demonstration"))
            << "the inventory row for " << row.objectName.toStdString()
            << " has the unknown anchor value " << row.anchored.toStdString();
    }
}

TEST(SearchInventoryTests, EveryAnchoredStoreNameIsUnique)
{
    QSet<QString> seen;
    for (const InventoryRow& row : readInventory()) {
        if (row.anchored != QStringLiteral("yes")) {
            continue;
        }
        EXPECT_FALSE(seen.contains(row.store))
            << "two anchored fields share the store name " << row.store.toStdString()
            << ", so their saved test cases would collide";
        seen.insert(row.store);
    }
}

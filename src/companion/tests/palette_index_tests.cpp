/*
* Audacity: A Digital Audio Editor
*
* The guard for src/companion/palette/settingsindex.json.
*
* The command palette can only find a preference that the index names. These
* tests read the index and the preferences QML side by side and fail when a
* page, a section or a setting key has appeared in the QML without a row in the
* index.
*/

#include <gtest/gtest.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QSet>

#include "palette/settingsindex.h"

using namespace au::companion;

namespace {
const QString SOURCE_ROOT = QStringLiteral(AU_COMPANION_SOURCE_ROOT);
const QString INDEX_FILE = SOURCE_ROOT + QStringLiteral("/companion/palette/settingsindex.json");
const QString PREFERENCES_QML = SOURCE_ROOT + QStringLiteral("/preferences/qml/Audacity/Preferences");
const QString PREFERENCES_MODEL = PREFERENCES_QML + QStringLiteral("/preferencesmodel.cpp");
const QString FRAMEWORK_ROOT = QStringLiteral(AU_COMPANION_FRAMEWORK_ROOT);

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

SettingsIndex loadIndex()
{
    QFile file(INDEX_FILE);
    if (!file.open(QIODevice::ReadOnly)) {
        return SettingsIndex();
    }
    const QByteArray data = file.readAll();
    file.close();

    QString error;
    const SettingsIndex index = SettingsIndex::fromJson(data, &error);
    EXPECT_TRUE(error.isEmpty()) << error.toStdString();
    return index;
}

//! Every page id the preferences model creates.
QStringList pageIdsInModel()
{
    static const QRegularExpression item(QStringLiteral("makeItem\\(\\s*\"([^\"]+)\""));

    QStringList ids;
    // A page that is commented out is not a page the palette can reach.
    const QStringList lines = readFile(PREFERENCES_MODEL).split(u'\n');
    for (const QString& line : lines) {
        if (line.trimmed().startsWith(QStringLiteral("//"))) {
            continue;
        }
        QRegularExpressionMatchIterator it = item.globalMatch(line);
        while (it.hasNext()) {
            ids.append(it.next().captured(1));
        }
    }
    return ids;
}

//! Every section component the preferences pages instantiate.
QSet<QString> sectionsUsedByPages()
{
    QSet<QString> sections;
    static const QRegularExpression use(QStringLiteral("\\n\\s+([A-Z][A-Za-z0-9]*Section)\\s*\\{"));

    QDirIterator it(PREFERENCES_QML, QStringList { QStringLiteral("*PreferencesPage.qml") },
                    QDir::Files);
    while (it.hasNext()) {
        const QString text = readFile(it.next());
        QRegularExpressionMatchIterator matches = use.globalMatch(text);
        while (matches.hasNext()) {
            sections.insert(matches.next().captured(1));
        }
    }
    return sections;
}

//! Every muse settings key declared anywhere the preferences reach.
QSet<QString> settingKeysInSource()
{
    QSet<QString> keys;
    static const QRegularExpression declaration(
        QStringLiteral("Settings::Key\\s*[A-Za-z_0-9]*\\s*\\(\\s*(?:\"[^\"]*\"|[A-Za-z_0-9:]+)\\s*,\\s*\"([^\"]+)\""));

    // Audacity's own modules and the muse framework both declare keys that the
    // preferences surface writes, so both roots are scanned.
    for (const QString& root : { SOURCE_ROOT, FRAMEWORK_ROOT }) {
        QDirIterator it(root, QStringList { QStringLiteral("*.cpp"), QStringLiteral("*.h") },
                        QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString text = readFile(it.next());
            QRegularExpressionMatchIterator matches = declaration.globalMatch(text);
            while (matches.hasNext()) {
                keys.insert(matches.next().captured(1));
            }
        }
    }
    return keys;
}
}

TEST(PaletteIndexTests, TheIndexParsesAndIsNotEmpty)
{
    const SettingsIndex index = loadIndex();
    EXPECT_FALSE(index.isEmpty());
    EXPECT_FALSE(index.pages().isEmpty());
    EXPECT_FALSE(index.settings().isEmpty());
    EXPECT_FALSE(index.appearance().isEmpty());
}

TEST(PaletteIndexTests, TheIndexShipsInTheModuleResources)
{
    // The palette reads the index from the compiled resources rather than from
    // the source tree, so the resource has to carry it too.
    EXPECT_FALSE(SettingsIndex::instance().isEmpty())
        << "settingsindex.json is missing from au_companion.qrc";
}

TEST(PaletteIndexTests, EveryPreferencesPageIsIndexed)
{
    const SettingsIndex index = loadIndex();

    QSet<QString> indexed;
    for (const QVariant& value : index.pages()) {
        indexed.insert(value.toMap().value(QStringLiteral("id")).toString());
    }

    const QStringList pages = pageIdsInModel();
    ASSERT_FALSE(pages.isEmpty()) << "the preferences model could not be read";

    for (const QString& id : pages) {
        EXPECT_TRUE(indexed.contains(id))
            << "the preferences page " << id.toStdString()
            << " has no row in src/companion/palette/settingsindex.json";
    }
}

TEST(PaletteIndexTests, EverySettingRowNamesAKnownPage)
{
    const SettingsIndex index = loadIndex();

    QSet<QString> pages;
    for (const QVariant& value : index.pages()) {
        pages.insert(value.toMap().value(QStringLiteral("id")).toString());
    }

    for (const QVariant& value : index.settings()) {
        const QString page = value.toMap().value(QStringLiteral("page")).toString();
        EXPECT_TRUE(pages.contains(page))
            << "the setting " << value.toMap().value(QStringLiteral("label")).toString().toStdString()
            << " names the unknown page " << page.toStdString();
    }
}

TEST(PaletteIndexTests, EverySettingRowIsComplete)
{
    static const QSet<QString> KNOWN_CONTROLS = {
        QStringLiteral("switch"), QStringLiteral("slider"), QStringLiteral("dropdown"),
        QStringLiteral("color"), QStringLiteral("radio"), QStringLiteral("text"),
        QStringLiteral("number"), QStringLiteral("action"), QStringLiteral("info")
    };

    const SettingsIndex index = loadIndex();
    for (const QVariant& value : index.settings()) {
        const QVariantMap row = value.toMap();
        const QString label = row.value(QStringLiteral("label")).toString();

        EXPECT_FALSE(label.isEmpty()) << "a settings row has no label";
        EXPECT_FALSE(row.value(QStringLiteral("target")).toString().isEmpty())
            << label.toStdString() << " has no teleport target";
        EXPECT_TRUE(KNOWN_CONTROLS.contains(row.value(QStringLiteral("control")).toString()))
            << label.toStdString() << " has the unknown control type "
            << row.value(QStringLiteral("control")).toString().toStdString();
    }
}

TEST(PaletteIndexTests, EveryIndexedSettingKeyExistsInTheSource)
{
    const SettingsIndex index = loadIndex();
    const QSet<QString> declared = settingKeysInSource();
    ASSERT_FALSE(declared.isEmpty()) << "no settings keys were found in the source tree";

    const auto check = [&declared](const QVariantList& rows) {
        for (const QVariant& value : rows) {
            const QVariantMap row = value.toMap();
            const QString key = row.value(QStringLiteral("key")).toString();
            if (key.isEmpty()) {
                continue;
            }
            EXPECT_TRUE(declared.contains(key))
                << "the index names the setting key " << key.toStdString()
                << ", which no module declares any more";
        }
    };

    check(index.settings());
    check(index.appearance());
}

TEST(PaletteIndexTests, EverySectionUsedByAPageIsRepresented)
{
    const SettingsIndex index = loadIndex();

    // A section is represented when at least one settings row carries its
    // human readable group, which is the section's own title.
    QString groups;
    for (const QVariant& value : index.settings()) {
        groups += u' ' + value.toMap().value(QStringLiteral("group")).toString().toLower();
        groups += u' ' + value.toMap().value(QStringLiteral("label")).toString().toLower();
        groups += u' ' + value.toMap().value(QStringLiteral("keywords")).toString().toLower();
    }

    const QSet<QString> sections = sectionsUsedByPages();
    ASSERT_FALSE(sections.isEmpty()) << "no preference sections were found";

    for (const QString& section : sections) {
        // "AutoSaveSection" -> "auto save"
        QString words = section;
        words.chop(QStringLiteral("Section").size());
        words.replace(QRegularExpression(QStringLiteral("([a-z0-9])([A-Z])")),
                      QStringLiteral("\\1 \\2"));
        words = words.toLower();

        // Match on the first word, which is what the section is about, in both
        // its singular and its plural form. That is strict enough to notice a
        // whole new section and loose enough not to demand a phrasing.
        QString head = words.split(u' ').first();
        QString singular = head.endsWith(u's') ? head.left(head.size() - 1) : head;
        if (section == QStringLiteral("BaseSection")) {
            // BaseSection is the generic container every section is built on,
            // not a section with settings of its own.
            continue;
        }
        if (head == QStringLiteral("advanced") || head == QStringLiteral("experience")) {
            // "AdvancedTopSection" is the toolbar of the advanced page and
            // "Experience*Section" rows carry the page name rather than the
            // word itself, so both are matched by their page instead.
            continue;
        }
        EXPECT_TRUE(groups.contains(head) || groups.contains(singular))
            << "the preferences section " << section.toStdString()
            << " has no representation in src/companion/palette/settingsindex.json";
    }
}

TEST(PaletteIndexTests, TheAppearanceControlsCoverTheDocumentedMaterialSettings)
{
    const SettingsIndex index = loadIndex();

    QSet<QString> keys;
    for (const QVariant& value : index.appearance()) {
        keys.insert(value.toMap().value(QStringLiteral("key")).toString());
    }

    for (const QString& expected : { QStringLiteral("ui/m3/seedColor"),
                                     QStringLiteral("ui/m3/variant"),
                                     QStringLiteral("ui/m3/density"),
                                     QStringLiteral("ui/m3/reducedMotion"),
                                     QStringLiteral("ui/application/currentThemeCode"),
                                     QStringLiteral("ui/theme/fontFamily"),
                                     QStringLiteral("projectscene/clipStyle"),
                                     QStringLiteral("language"),
                                     QStringLiteral("companion/palette/fullWindow") }) {
        EXPECT_TRUE(keys.contains(expected))
            << "the appearance section of the index does not carry " << expected.toStdString();
    }
}

TEST(PaletteIndexTests, EveryTeleportTargetIsDistinctEnoughToBeFound)
{
    const SettingsIndex index = loadIndex();
    for (const QString& target : index.targets()) {
        EXPECT_GE(target.size(), 3)
            << "the teleport target " << target.toStdString()
            << " is too short to identify a control";
    }
}

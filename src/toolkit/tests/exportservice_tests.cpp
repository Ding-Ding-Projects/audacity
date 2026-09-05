/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>

#include "internal/exportservice.h"

using namespace au::toolkit;

namespace {
QVariantList sampleRows()
{
    QVariantMap flat;
    flat[QStringLiteral("id")] = 1;
    flat[QStringLiteral("name")] = QStringLiteral("Alpha");

    QVariantMap nested;
    QVariantMap inner;
    inner[QStringLiteral("x")] = 1;
    nested[QStringLiteral("id")] = 2;
    nested[QStringLiteral("meta")] = inner;

    return { flat, nested };
}
}

TEST(ExportServiceTests, CsvDropsNestedFields)
{
    const QStringList dropped = fieldsDroppedByFormat(ExportFormat::Csv, sampleRows());
    EXPECT_TRUE(dropped.contains(QStringLiteral("meta")));
}

TEST(ExportServiceTests, JsonDropsNothing)
{
    const QStringList dropped = fieldsDroppedByFormat(ExportFormat::Json, sampleRows());
    EXPECT_TRUE(dropped.isEmpty());
}

TEST(ExportServiceTests, JsonRoundTripsThroughQtJson)
{
    const QByteArray rendered = renderExport(ExportFormat::Json, sampleRows());
    const QJsonDocument doc = QJsonDocument::fromJson(rendered);
    ASSERT_TRUE(doc.isArray());
    EXPECT_EQ(doc.array().size(), 2);
    EXPECT_EQ(doc.array().at(0).toObject().value(QStringLiteral("name")).toString(), QStringLiteral("Alpha"));
}

TEST(ExportServiceTests, CsvHasAHeaderRowAndOneRowPerRecord)
{
    QVariantMap a, b;
    a[QStringLiteral("id")] = 1;
    a[QStringLiteral("name")] = QStringLiteral("Alpha");
    b[QStringLiteral("id")] = 2;
    b[QStringLiteral("name")] = QStringLiteral("Beta");

    const QByteArray rendered = renderExport(ExportFormat::Csv, { a, b });
    const QString text = QString::fromUtf8(rendered);
    const QStringList lines = text.split(QStringLiteral("\n"), Qt::SkipEmptyParts);

    ASSERT_EQ(lines.size(), 3);
    EXPECT_TRUE(lines.first().contains(QStringLiteral("id")));
    EXPECT_TRUE(lines.first().contains(QStringLiteral("name")));
}

TEST(ExportServiceTests, CsvEscapesACommaInAValue)
{
    QVariantMap row;
    row[QStringLiteral("id")] = 1;
    row[QStringLiteral("name")] = QStringLiteral("Alpha, comma");

    const QByteArray rendered = renderExport(ExportFormat::Csv, { row }, { QStringLiteral("id"), QStringLiteral("name") });
    const QString text = QString::fromUtf8(rendered);

    EXPECT_TRUE(text.contains(QStringLiteral("\"Alpha, comma\"")));
}

TEST(ExportServiceTests, StoreZipProducesAStructurallyValidArchive)
{
    QList<QPair<QString, QByteArray> > entries;
    entries << qMakePair(QStringLiteral("one.txt"), QByteArray("hello"));
    entries << qMakePair(QStringLiteral("two.txt"), QByteArray("world, again"));

    const QByteArray zipBytes = buildStoreZip(entries);

    // Local file header signature at the very start.
    ASSERT_GE(zipBytes.size(), 4);
    EXPECT_EQ(static_cast<unsigned char>(zipBytes.at(0)), 0x50);
    EXPECT_EQ(static_cast<unsigned char>(zipBytes.at(1)), 0x4b);

    // End of central directory signature must be present somewhere near
    // the tail of the archive.
    const QByteArray eocdSignature = QByteArray::fromHex("504b0506");
    EXPECT_TRUE(zipBytes.contains(eocdSignature));

    // Both entry names must appear verbatim (store, no compression).
    EXPECT_TRUE(zipBytes.contains("one.txt"));
    EXPECT_TRUE(zipBytes.contains("two.txt"));
    EXPECT_TRUE(zipBytes.contains("hello"));
    EXPECT_TRUE(zipBytes.contains("world, again"));
}

TEST(ExportServiceTests, EveryDeclaredFormatHasALabelAndAnExtension)
{
    const QList<ExportFormat> formats = {
        ExportFormat::Json, ExportFormat::JsonLines, ExportFormat::Yaml, ExportFormat::Toml,
        ExportFormat::Xml, ExportFormat::Csv, ExportFormat::Tsv, ExportFormat::Markdown,
        ExportFormat::Html, ExportFormat::Sql, ExportFormat::Zip
    };

    for (ExportFormat format : formats) {
        EXPECT_FALSE(exportFormatLabel(format).isEmpty());
        EXPECT_FALSE(exportFormatFileExtensions(format).isEmpty());
    }
}

/*
* Audacity: A Digital Audio Editor
*/

#include "exportservice.h"

#include <QFile>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QSet>

using namespace au::toolkit;

namespace {
//! A small self-contained CRC-32 (ISO 3309 / ZIP) implementation, so the
//! store-only ZIP writer below needs no external compression library that
//! this build does not guarantee is linked.
quint32 crc32Of(const QByteArray& data)
{
    static quint32 table[256];
    static bool tableReady = false;
    if (!tableReady) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        tableReady = true;
    }

    quint32 crc = 0xFFFFFFFFu;
    for (unsigned char byte : data) {
        crc = table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}
}

namespace {
bool isTabularFormat(ExportFormat format)
{
    return format == ExportFormat::Csv || format == ExportFormat::Tsv;
}

QStringList flattenedColumnNames(const QVariantList& rows)
{
    QStringList columns;
    QSet<QString> seen;
    for (const QVariant& rowVariant : rows) {
        const QVariantMap row = rowVariant.toMap();
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            if (!seen.contains(it.key())) {
                seen.insert(it.key());
                columns << it.key();
            }
        }
    }
    return columns;
}

QString csvEscape(const QString& value, QChar separator)
{
    QString v = value;
    v.replace(QStringLiteral("\""), QStringLiteral("\"\""));
    if (v.contains(separator) || v.contains(QStringLiteral("\"")) || v.contains(QStringLiteral("\n"))) {
        return QStringLiteral("\"") + v + QStringLiteral("\"");
    }
    return v;
}

QJsonValue toJsonValue(const QVariant& v)
{
    return QJsonValue::fromVariant(v);
}
}

QString au::toolkit::exportFormatId(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Json: return QStringLiteral("json");
    case ExportFormat::JsonLines: return QStringLiteral("jsonl");
    case ExportFormat::Yaml: return QStringLiteral("yaml");
    case ExportFormat::Toml: return QStringLiteral("toml");
    case ExportFormat::Xml: return QStringLiteral("xml");
    case ExportFormat::Csv: return QStringLiteral("csv");
    case ExportFormat::Tsv: return QStringLiteral("tsv");
    case ExportFormat::Markdown: return QStringLiteral("markdown");
    case ExportFormat::Html: return QStringLiteral("html");
    case ExportFormat::Sql: return QStringLiteral("sql");
    case ExportFormat::Zip: return QStringLiteral("zip");
    }
    return QString();
}

QString au::toolkit::exportFormatLabel(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Json: return QStringLiteral("JSON");
    case ExportFormat::JsonLines: return QStringLiteral("JSON Lines (NDJSON)");
    case ExportFormat::Yaml: return QStringLiteral("YAML");
    case ExportFormat::Toml: return QStringLiteral("TOML");
    case ExportFormat::Xml: return QStringLiteral("XML");
    case ExportFormat::Csv: return QStringLiteral("CSV");
    case ExportFormat::Tsv: return QStringLiteral("TSV");
    case ExportFormat::Markdown: return QStringLiteral("Markdown");
    case ExportFormat::Html: return QStringLiteral("HTML");
    case ExportFormat::Sql: return QStringLiteral("SQL");
    case ExportFormat::Zip: return QStringLiteral("ZIP archive (store only; 7z is not available in this build)");
    }
    return QString();
}

QStringList au::toolkit::exportFormatFileExtensions(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Json: return { QStringLiteral("json") };
    case ExportFormat::JsonLines: return { QStringLiteral("jsonl"), QStringLiteral("ndjson") };
    case ExportFormat::Yaml: return { QStringLiteral("yaml"), QStringLiteral("yml") };
    case ExportFormat::Toml: return { QStringLiteral("toml") };
    case ExportFormat::Xml: return { QStringLiteral("xml") };
    case ExportFormat::Csv: return { QStringLiteral("csv") };
    case ExportFormat::Tsv: return { QStringLiteral("tsv") };
    case ExportFormat::Markdown: return { QStringLiteral("md") };
    case ExportFormat::Html: return { QStringLiteral("html") };
    case ExportFormat::Sql: return { QStringLiteral("sql") };
    case ExportFormat::Zip: return { QStringLiteral("zip") };
    }
    return {};
}

QStringList au::toolkit::fieldsDroppedByFormat(ExportFormat format, const QVariantList& rows)
{
    QStringList dropped;
    if (!isTabularFormat(format)) {
        return dropped;
    }

    QSet<QString> seen;
    for (const QVariant& rowVariant : rows) {
        const QVariantMap row = rowVariant.toMap();
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            const QVariant value = it.value();
            const bool isNested = (value.typeId() == QMetaType::QVariantMap) || (value.typeId() == QMetaType::QVariantList);
            if (isNested && !seen.contains(it.key())) {
                seen.insert(it.key());
                dropped << it.key();
            }
        }
    }
    return dropped;
}

QByteArray au::toolkit::renderExport(ExportFormat format, const QVariantList& rows, const QStringList& columnOrderIn)
{
    switch (format) {
    case ExportFormat::Json: {
        QJsonArray arr;
        for (const QVariant& r : rows) {
            arr.append(QJsonObject::fromVariantMap(r.toMap()));
        }
        return QJsonDocument(arr).toJson(QJsonDocument::Indented);
    }
    case ExportFormat::JsonLines: {
        QByteArray out;
        for (const QVariant& r : rows) {
            out += QJsonDocument(QJsonObject::fromVariantMap(r.toMap())).toJson(QJsonDocument::Compact);
            out += '\n';
        }
        return out;
    }
    case ExportFormat::Yaml: {
        QString out;
        for (const QVariant& r : rows) {
            const QVariantMap row = r.toMap();
            out += QStringLiteral("-\n");
            for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
                out += QStringLiteral("  ") + it.key() + QStringLiteral(": ")
                       + it.value().toString().replace(QStringLiteral("\n"), QStringLiteral(" ")) + QStringLiteral("\n");
            }
        }
        return out.toUtf8();
    }
    case ExportFormat::Toml: {
        QString out;
        int index = 0;
        for (const QVariant& r : rows) {
            const QVariantMap row = r.toMap();
            out += QStringLiteral("[[row]]\n");
            for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
                out += it.key() + QStringLiteral(" = \"")
                       + it.value().toString().replace(QStringLiteral("\""), QStringLiteral("\\\"")) + QStringLiteral("\"\n");
            }
            out += QStringLiteral("\n");
            index++;
        }
        Q_UNUSED(index);
        return out.toUtf8();
    }
    case ExportFormat::Xml: {
        QString out = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<rows>\n");
        for (const QVariant& r : rows) {
            const QVariantMap row = r.toMap();
            out += QStringLiteral("  <row>\n");
            for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
                QString value = it.value().toString();
                value.replace(QStringLiteral("&"), QStringLiteral("&amp;"));
                value.replace(QStringLiteral("<"), QStringLiteral("&lt;"));
                value.replace(QStringLiteral(">"), QStringLiteral("&gt;"));
                out += QStringLiteral("    <") + it.key() + QStringLiteral(">") + value
                       + QStringLiteral("</") + it.key() + QStringLiteral(">\n");
            }
            out += QStringLiteral("  </row>\n");
        }
        out += QStringLiteral("</rows>\n");
        return out.toUtf8();
    }
    case ExportFormat::Csv:
    case ExportFormat::Tsv: {
        const QChar sep = (format == ExportFormat::Csv) ? QChar(',') : QChar('\t');
        QStringList columns = columnOrderIn.isEmpty() ? flattenedColumnNames(rows) : columnOrderIn;
        QString out = columns.join(sep) + QStringLiteral("\n");
        for (const QVariant& r : rows) {
            const QVariantMap row = r.toMap();
            QStringList cells;
            for (const QString& col : columns) {
                const QVariant value = row.value(col);
                if (value.typeId() == QMetaType::QVariantMap || value.typeId() == QMetaType::QVariantList) {
                    cells << QString();
                } else {
                    cells << csvEscape(value.toString(), sep);
                }
            }
            out += cells.join(sep) + QStringLiteral("\n");
        }
        return out.toUtf8();
    }
    case ExportFormat::Markdown: {
        QStringList columns = columnOrderIn.isEmpty() ? flattenedColumnNames(rows) : columnOrderIn;
        QString out = QStringLiteral("| ") + columns.join(QStringLiteral(" | ")) + QStringLiteral(" |\n");
        out += QStringLiteral("|");
        for (int i = 0; i < columns.size(); ++i) {
            out += QStringLiteral(" --- |");
        }
        out += QStringLiteral("\n");
        for (const QVariant& r : rows) {
            const QVariantMap row = r.toMap();
            QStringList cells;
            for (const QString& col : columns) {
                cells << row.value(col).toString();
            }
            out += QStringLiteral("| ") + cells.join(QStringLiteral(" | ")) + QStringLiteral(" |\n");
        }
        return out.toUtf8();
    }
    case ExportFormat::Html: {
        QStringList columns = columnOrderIn.isEmpty() ? flattenedColumnNames(rows) : columnOrderIn;
        QString out = QStringLiteral("<!doctype html>\n<html><head><meta charset=\"utf-8\"><title>Export</title></head><body>\n<table>\n<thead><tr>");
        for (const QString& col : columns) {
            out += QStringLiteral("<th>") + col + QStringLiteral("</th>");
        }
        out += QStringLiteral("</tr></thead>\n<tbody>\n");
        for (const QVariant& r : rows) {
            const QVariantMap row = r.toMap();
            out += QStringLiteral("<tr>");
            for (const QString& col : columns) {
                out += QStringLiteral("<td>") + row.value(col).toString().toHtmlEscaped() + QStringLiteral("</td>");
            }
            out += QStringLiteral("</tr>\n");
        }
        out += QStringLiteral("</tbody>\n</table>\n</body></html>\n");
        return out.toUtf8();
    }
    case ExportFormat::Sql: {
        QStringList columns = columnOrderIn.isEmpty() ? flattenedColumnNames(rows) : columnOrderIn;
        QString out = QStringLiteral("-- Generated export\n");
        for (const QVariant& r : rows) {
            const QVariantMap row = r.toMap();
            QStringList values;
            for (const QString& col : columns) {
                QString value = row.value(col).toString();
                value.replace(QStringLiteral("'"), QStringLiteral("''"));
                values << QStringLiteral("'") + value + QStringLiteral("'");
            }
            out += QStringLiteral("INSERT INTO export_rows (") + columns.join(QStringLiteral(", "))
                   + QStringLiteral(") VALUES (") + values.join(QStringLiteral(", ")) + QStringLiteral(");\n");
        }
        return out.toUtf8();
    }
    case ExportFormat::Zip:
        return QByteArray();
    }
    return QByteArray();
}

namespace {
struct ZipCentralRecord {
    QString name;
    quint32 crc;
    quint32 size;
    quint32 offset;
};

void appendLE16(QByteArray& out, quint16 v)
{
    out.append(char(v & 0xff));
    out.append(char((v >> 8) & 0xff));
}

void appendLE32(QByteArray& out, quint32 v)
{
    out.append(char(v & 0xff));
    out.append(char((v >> 8) & 0xff));
    out.append(char((v >> 16) & 0xff));
    out.append(char((v >> 24) & 0xff));
}
}

QByteArray au::toolkit::buildStoreZip(const QList<QPair<QString, QByteArray> >& entries)
{
    // A minimal "store" (no compression) ZIP writer. This keeps the export
    // service free of an external ZIP library dependency that this build
    // does not guarantee, while still producing a real, standard,
    // extractable ZIP file. 7z packaging is intentionally not offered
    // here; the export sheet says so plainly next to the ZIP option.
    QByteArray out;
    QList<ZipCentralRecord> centralRecords;

    for (const auto& entry : entries) {
        const QByteArray nameUtf8 = entry.first.toUtf8();
        const QByteArray& data = entry.second;
        const quint32 crc = crc32Of(data);

        ZipCentralRecord record;
        record.name = entry.first;
        record.crc = crc;
        record.size = static_cast<quint32>(data.size());
        record.offset = static_cast<quint32>(out.size());
        centralRecords << record;

        appendLE32(out, 0x04034b50); // local file header signature
        appendLE16(out, 20); // version needed
        appendLE16(out, 0); // flags
        appendLE16(out, 0); // compression method: store
        appendLE16(out, 0); // mod time
        appendLE16(out, 0); // mod date
        appendLE32(out, crc);
        appendLE32(out, record.size);
        appendLE32(out, record.size);
        appendLE16(out, static_cast<quint16>(nameUtf8.size()));
        appendLE16(out, 0); // extra length
        out.append(nameUtf8);
        out.append(data);
    }

    const quint32 centralStart = static_cast<quint32>(out.size());
    for (const ZipCentralRecord& record : centralRecords) {
        const QByteArray nameUtf8 = record.name.toUtf8();
        appendLE32(out, 0x02014b50); // central directory header signature
        appendLE16(out, 20);
        appendLE16(out, 20);
        appendLE16(out, 0);
        appendLE16(out, 0);
        appendLE16(out, 0);
        appendLE16(out, 0);
        appendLE32(out, record.crc);
        appendLE32(out, record.size);
        appendLE32(out, record.size);
        appendLE16(out, static_cast<quint16>(nameUtf8.size()));
        appendLE16(out, 0);
        appendLE16(out, 0);
        appendLE16(out, 0);
        appendLE16(out, 0);
        appendLE32(out, 0);
        appendLE32(out, record.offset);
        out.append(nameUtf8);
    }
    const quint32 centralSize = static_cast<quint32>(out.size()) - centralStart;

    appendLE32(out, 0x06054b50); // end of central directory
    appendLE16(out, 0);
    appendLE16(out, 0);
    appendLE16(out, static_cast<quint16>(centralRecords.size()));
    appendLE16(out, static_cast<quint16>(centralRecords.size()));
    appendLE32(out, centralSize);
    appendLE32(out, centralStart);
    appendLE16(out, 0);

    return out;
}

bool ExportService::exportRowsToFile(ExportFormat format, const QVariantList& rows, const QString& filePath,
                                      const QStringList& columnOrder)
{
    const QByteArray content = renderExport(format, rows, columnOrder);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(content);
    file.close();
    return true;
}

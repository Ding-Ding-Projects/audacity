/*
* Audacity: A Digital Audio Editor
*/

#include "exportservicewrapper.h"

#include <QFile>

#include "exportservice.h"

using namespace au::toolkit;

namespace {
ExportFormat formatFromId(const QString& id)
{
    if (id == QStringLiteral("json")) {
        return ExportFormat::Json;
    }
    if (id == QStringLiteral("jsonl")) {
        return ExportFormat::JsonLines;
    }
    if (id == QStringLiteral("yaml")) {
        return ExportFormat::Yaml;
    }
    if (id == QStringLiteral("toml")) {
        return ExportFormat::Toml;
    }
    if (id == QStringLiteral("xml")) {
        return ExportFormat::Xml;
    }
    if (id == QStringLiteral("csv")) {
        return ExportFormat::Csv;
    }
    if (id == QStringLiteral("tsv")) {
        return ExportFormat::Tsv;
    }
    if (id == QStringLiteral("markdown")) {
        return ExportFormat::Markdown;
    }
    if (id == QStringLiteral("html")) {
        return ExportFormat::Html;
    }
    if (id == QStringLiteral("sql")) {
        return ExportFormat::Sql;
    }
    return ExportFormat::Zip;
}
}

ExportServiceWrapper::ExportServiceWrapper(QObject* parent)
    : QObject(parent)
{
}

QStringList ExportServiceWrapper::formatIds() const
{
    return {
        QStringLiteral("json"), QStringLiteral("jsonl"), QStringLiteral("yaml"), QStringLiteral("toml"),
        QStringLiteral("xml"), QStringLiteral("csv"), QStringLiteral("tsv"), QStringLiteral("markdown"),
        QStringLiteral("html"), QStringLiteral("sql"), QStringLiteral("zip")
    };
}

QString ExportServiceWrapper::formatLabel(const QString& formatId) const
{
    return exportFormatLabel(formatFromId(formatId));
}

QStringList ExportServiceWrapper::droppedFields(const QString& formatId, const QVariantList& rows) const
{
    return fieldsDroppedByFormat(formatFromId(formatId), rows);
}

bool ExportServiceWrapper::exportRows(const QString& formatId, const QVariantList& rows, const QString& filePath,
                                       const QString& archiveEntryName) const
{
    const ExportFormat format = formatFromId(formatId);

    if (format != ExportFormat::Zip) {
        ExportService service;
        return service.exportRowsToFile(format, rows, filePath);
    }

    // The ZIP format wraps a JSON rendering of the rows as one entry in a
    // store-only archive; there is no external compression library
    // dependency here, and the export sheet says so plainly.
    const QByteArray jsonContent = renderExport(ExportFormat::Json, rows);
    QList<QPair<QString, QByteArray> > entries;
    entries << qMakePair(archiveEntryName, jsonContent);
    const QByteArray zipBytes = buildStoreZip(entries);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(zipBytes);
    file.close();
    return true;
}

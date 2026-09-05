/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>
#include <QString>
#include <QStringList>

namespace au::toolkit {
enum class ExportFormat {
    Json,
    JsonLines,
    Yaml,
    Toml,
    Xml,
    Csv,
    Tsv,
    Markdown,
    Html,
    Sql,
    Zip
};

QString exportFormatId(ExportFormat format);
QString exportFormatLabel(ExportFormat format);
QStringList exportFormatFileExtensions(ExportFormat format);

//! Fields that a given format cannot faithfully carry for a row shaped like
//! typical toolkit rows (nested objects, for instance, cannot survive a CSV
//! or TSV cell). The caller must disclose this list to the user before the
//! export runs.
QStringList fieldsDroppedByFormat(ExportFormat format, const QVariantList& rows);

//! Renders rows (a list of QVariantMap-like objects) to UTF-8 text in the
//! given format. Tabular formats flatten one level of nesting into a
//! dotted-key column; anything left over after that flattening is the set
//! reported by fieldsDroppedByFormat.
QByteArray renderExport(ExportFormat format, const QVariantList& rows, const QStringList& columnOrder = {});

//! A minimal store-only ZIP archive (no compression), written without any
//! external ZIP library since one is not guaranteed available in this
//! build. Each entry is one exported file. This honestly documents that
//! 7z packaging is not available in this build; only ZIP is offered.
QByteArray buildStoreZip(const QList<QPair<QString, QByteArray> >& entries);

class IExportService
{
public:
    virtual ~IExportService() = default;

    virtual bool exportRowsToFile(ExportFormat format, const QVariantList& rows, const QString& filePath,
                                   const QStringList& columnOrder = {}) = 0;
};

class ExportService : public IExportService
{
public:
    bool exportRowsToFile(ExportFormat format, const QVariantList& rows, const QString& filePath,
                           const QStringList& columnOrder = {}) override;
};
}

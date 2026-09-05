/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>
#include <QStringList>

namespace au::toolkit {
//! The QML-facing entry point for the export service. ExportSheet.qml
//! calls this to find out which fields a tabular format would drop before
//! the user commits to exporting, and to actually write the file once a
//! destination is chosen. formatId is one of the ids returned by
//! exportFormatId() in exportservice.h ("json", "csv", "zip", and so on).
class ExportServiceWrapper : public QObject
{
    Q_OBJECT

public:
    explicit ExportServiceWrapper(QObject* parent = nullptr);

    //! Every supported format id, in the order the export sheet lists them.
    Q_INVOKABLE QStringList formatIds() const;
    Q_INVOKABLE QString formatLabel(const QString& formatId) const;

    //! Fields a tabular format (csv, tsv) would silently drop because they
    //! hold a nested object or list. Empty for every other format.
    Q_INVOKABLE QStringList droppedFields(const QString& formatId, const QVariantList& rows) const;

    //! Writes rows to filePath in the given format. archiveEntryName only
    //! matters for the "zip" format id, where the rendered content (as
    //! JSON) becomes the single named entry inside a store-only ZIP
    //! archive; every other format writes filePath directly.
    Q_INVOKABLE bool exportRows(const QString& formatId, const QVariantList& rows, const QString& filePath,
                                const QString& archiveEntryName = QStringLiteral("export.json")) const;
};
}

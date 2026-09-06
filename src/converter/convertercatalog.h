/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QString>
#include <QVector>

namespace au::converter {

//! An adapter is visible even when it cannot run.  The UI must present the
//! reason rather than silently dropping a format from the catalog.
struct AdapterDescriptor {
    QString id;
    QString category;
    QString sourceFormat;
    QString targetFormat;
    QString displayName;
    bool bundled = false;
    bool enabled = false;
    bool lossy = false;
    QString unavailableReason;
};

class ConverterCatalog
{
public:
    //! Returns the intentionally small, offline-only first catalog.  Image
    //! adapters are enabled only when the installed Qt image plugin reports
    //! that it can encode the target.  No executable discovery is performed.
    static QVector<AdapterDescriptor> adapters();
    static AdapterDescriptor find(const QString& sourceFormat, const QString& targetFormat);
};
}

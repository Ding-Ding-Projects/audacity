/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>

#include <atomic>

namespace au::converter {

enum class PdfOperation { Inspect, Split, Merge, Extract, Reorder, Rotate, SetMetadata };

struct PdfRequest {
    PdfOperation operation = PdfOperation::Inspect;
    QStringList sourcePaths;
    QString outputPath;
    QString pageSpec;
    int rotation = 0;
    QString title;
    QMap<QString, QString> metadata; //!< Title, Author, Subject, Keywords
    bool allowOverwrite = false;
};

struct PdfResult {
    bool ok = false;
    bool cancelled = false;
    int pageCount = 0;
    QString message;
    struct Output { QString path; int pageCount = 0; bool committed = false; };
    QVector<Output> outputs;
};

//! qpdf is intentionally never located through PATH.  It is usable only when
//! the product-installed binary is present and its SHA-256 matches the pinned
//! distribution lock.  The adapter uses static argv, a bounded child process,
//! private output names, and reopens each result with qpdf before publishing.
class PdfProcessor final
{
public:
    static constexpr qint64 MaxInputBytes = 256LL * 1024 * 1024;
    static constexpr qint64 MaxOutputBytes = 512LL * 1024 * 1024;
    static constexpr int TimeoutMilliseconds = 60000;

    static QString bundledToolPath();
    static QString availabilityReason();
    static bool available();
    PdfResult process(const PdfRequest& request, const std::atomic_bool* cancellationRequested = nullptr) const;
};
}

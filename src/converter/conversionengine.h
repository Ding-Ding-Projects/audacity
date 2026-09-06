/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QString>

namespace au::converter {

enum class ConversionStatus {
    Converted,
    Cancelled,
    Rejected,
    Failed
};

struct ConversionRequest {
    QString sourcePath;
    QString outputPath;
    QString targetFormat;
    bool allowOverwrite = false;
};

struct ConversionResult {
    ConversionStatus status = ConversionStatus::Rejected;
    QString sourceFormat;
    QString message;
};

//! First conversion worker.  It is intentionally local-only: input bytes are
//! inspected before image decoding, adapter availability comes only from Qt's
//! bundled plugin capability, and it never launches a shell command or asks a
//! service for a converter.
class ConversionEngine
{
public:
    static constexpr qint64 MaxInputBytes = 256LL * 1024 * 1024;
    static constexpr qint64 MaxDecodedPixels = 100LL * 1000 * 1000;

    ConversionResult convert(const ConversionRequest& request, const bool* cancellationRequested = nullptr) const;
    static QString detectFormat(const QString& sourcePath, QString* error = nullptr);
};
}

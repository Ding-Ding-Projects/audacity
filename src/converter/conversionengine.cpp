/*
* Audacity: A Digital Audio Editor
*/

#include "conversionengine.h"

#include "convertercatalog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QTemporaryFile>

using namespace au::converter;

namespace {
bool cancelled(const std::atomic_bool* requested)
{
    return requested != nullptr && requested->load(std::memory_order_acquire);
}

ConversionResult result(ConversionStatus status, const QString& sourceFormat, const QString& message)
{
    ConversionResult value;
    value.status = status;
    value.sourceFormat = sourceFormat;
    value.message = message;
    return value;
}

bool matches(const QByteArray& bytes, const char* signature, int offset = 0)
{
    const QByteArray wanted(signature);
    return bytes.size() >= offset + wanted.size() && bytes.mid(offset, wanted.size()) == wanted;
}

QString lowerFormat(const QString& format)
{
    return format.trimmed().toLower();
}
}

QString ConversionEngine::detectFormat(const QString& sourcePath, QString* error)
{
    QFile file(sourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("The source file cannot be opened.");
        }
        return {};
    }
    const QByteArray bytes = file.read(32);
    if (matches(bytes, "\x89PNG\r\n\x1a\n")) {
        return QStringLiteral("PNG");
    }
    if (matches(bytes, "\xff\xd8\xff")) {
        return QStringLiteral("JPEG");
    }
    if (matches(bytes, "BM")) {
        return QStringLiteral("BMP");
    }
    if (matches(bytes, "%PDF-")) {
        return QStringLiteral("PDF");
    }
    if (matches(bytes, "RIFF") && matches(bytes, "WAVE", 8)) {
        return QStringLiteral("WAV");
    }
    if (matches(bytes, "PK\x03\x04")) {
        return QStringLiteral("ZIP");
    }
    if (error) {
        *error = QStringLiteral("The source bytes do not match a supported file signature.");
    }
    return {};
}

ConversionResult ConversionEngine::convert(const ConversionRequest& request, const std::atomic_bool* cancellationRequested) const
{
    if (cancelled(cancellationRequested)) {
        return result(ConversionStatus::Cancelled, {}, QStringLiteral("Conversion was cancelled before reading the source."));
    }

    const QFileInfo sourceInfo(request.sourcePath);
    if (!sourceInfo.isFile() || sourceInfo.isSymLink()) {
        return result(ConversionStatus::Rejected, {}, QStringLiteral("The selected source is not a regular file."));
    }
    if (sourceInfo.size() <= 0 || sourceInfo.size() > MaxInputBytes) {
        return result(ConversionStatus::Rejected, {}, QStringLiteral("The source exceeds the allowed input size."));
    }
    if (request.outputPath.isEmpty() || request.targetFormat.trimmed().isEmpty()) {
        return result(ConversionStatus::Rejected, {}, QStringLiteral("A target format and destination are required."));
    }

    QFile source(request.sourcePath);
    if (!source.open(QIODevice::ReadOnly) || source.size() != sourceInfo.size()) {
        return result(ConversionStatus::Rejected, {}, QStringLiteral("The source changed while it was being opened."));
    }
    const QByteArray signature = source.read(32);
    QString detectionError;
    QString sourceFormat;
    if (matches(signature, "\x89PNG\r\n\x1a\n")) sourceFormat = QStringLiteral("PNG");
    else if (matches(signature, "\xff\xd8\xff")) sourceFormat = QStringLiteral("JPEG");
    else if (matches(signature, "BM")) sourceFormat = QStringLiteral("BMP");
    else detectionError = QStringLiteral("The source bytes do not match a supported image signature.");
    if (sourceFormat.isEmpty()) {
        return result(ConversionStatus::Rejected, {}, detectionError);
    }

    const AdapterDescriptor adapter = ConverterCatalog::find(sourceFormat, request.targetFormat);
    if (!adapter.enabled || !adapter.bundled) {
        return result(ConversionStatus::Rejected, sourceFormat, adapter.unavailableReason);
    }

    const QFileInfo outputInfo(request.outputPath);
    if (QDir::cleanPath(sourceInfo.absoluteFilePath()).compare(QDir::cleanPath(outputInfo.absoluteFilePath()), Qt::CaseInsensitive) == 0) {
        return result(ConversionStatus::Rejected, sourceFormat, QStringLiteral("The destination must differ from the source."));
    }
    if (QFileInfo(outputInfo.dir().absolutePath()).isSymLink()) {
        return result(ConversionStatus::Rejected, sourceFormat, QStringLiteral("The destination directory must not be a symbolic link."));
    }
    if (outputInfo.exists()) {
        if (!request.allowOverwrite) {
            return result(ConversionStatus::Rejected, sourceFormat,
                          QStringLiteral("The destination already exists. Explicit overwrite approval is required."));
        }
        // Replacing an existing file after reopen verification needs the
        // application-owned atomic-replace service on Windows.  This core
        // deliberately refuses it rather than delete first and risk data loss.
        return result(ConversionStatus::Rejected, sourceFormat,
                      QStringLiteral("Safe overwrite is not available in this standalone core. The existing file was preserved."));
    }

    source.seek(0);
    QImageReader::setAllocationLimit(384);
    QImageReader reader(&source, sourceFormat.toLatin1());
    reader.setAutoTransform(false);
    const QSize size = reader.size();
    if (!size.isValid() || size.width() <= 0 || size.height() <= 0
        || static_cast<qint64>(size.width()) * size.height() > MaxDecodedPixels) {
        return result(ConversionStatus::Rejected, sourceFormat, QStringLiteral("The image dimensions are malformed or exceed the decode limit."));
    }
    if (cancelled(cancellationRequested)) {
        return result(ConversionStatus::Cancelled, sourceFormat, QStringLiteral("Conversion was cancelled before decoding."));
    }

    const QImage image = reader.read();
    if (image.isNull()) {
        return result(ConversionStatus::Rejected, sourceFormat, QStringLiteral("The image could not be decoded safely."));
    }
    if (cancelled(cancellationRequested)) {
        return result(ConversionStatus::Cancelled, sourceFormat, QStringLiteral("Conversion was cancelled before writing output."));
    }

    QDir destinationDir = outputInfo.dir();
    const QFileInfo destinationDirectoryInfo(destinationDir.absolutePath());
    if (!destinationDir.exists() || !destinationDirectoryInfo.isReadable() || !destinationDirectoryInfo.isWritable()) {
        return result(ConversionStatus::Rejected, sourceFormat, QStringLiteral("The destination directory is not writable."));
    }
    QTemporaryFile temporary(destinationDir.filePath(QStringLiteral(".audacity-convert-XXXXXX.tmp")));
    temporary.setAutoRemove(true);
    if (!temporary.open()) {
        return result(ConversionStatus::Failed, sourceFormat, QStringLiteral("A private temporary output file could not be created."));
    }
    const QString temporaryPath = temporary.fileName();
    temporary.close();

    QImageWriter writer(temporaryPath, lowerFormat(request.targetFormat).toLatin1());
    if (!writer.write(image)) {
        return result(ConversionStatus::Failed, sourceFormat, QStringLiteral("The bundled image encoder did not produce output."));
    }
    if (cancelled(cancellationRequested)) {
        return result(ConversionStatus::Cancelled, sourceFormat, QStringLiteral("Conversion was cancelled before publishing output."));
    }

    QImageReader verifier(temporaryPath, lowerFormat(request.targetFormat).toLatin1());
    const QSize reopenedSize = verifier.size();
    if (!reopenedSize.isValid() || reopenedSize != image.size() || !verifier.canRead()) {
        return result(ConversionStatus::Failed, sourceFormat, QStringLiteral("The temporary output failed reopen verification."));
    }
    if (!QFile::rename(temporaryPath, request.outputPath)) {
        return result(ConversionStatus::Failed, sourceFormat, QStringLiteral("The verified temporary output could not be published atomically."));
    }
    temporary.setAutoRemove(false);
    return result(ConversionStatus::Converted, sourceFormat, QStringLiteral("Converted with a verified atomic publish."));
}

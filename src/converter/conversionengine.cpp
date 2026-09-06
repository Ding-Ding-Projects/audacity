/*
* Audacity: A Digital Audio Editor
*/
#include "conversionengine.h"
#include "convertercatalog.h"
#include "nativefiletransaction.h"

#include <QImage>
#include <QImageReader>
#include <QImageWriter>

using namespace au::converter;

#ifdef AU_CONVERTER_TEST_HOOKS
thread_local std::function<void(ConversionEngine::TestPhase)> ConversionEngine::testHook;
#define CONVERSION_BARRIER(phase) if (testHook) testHook(TestPhase::phase)
#else
#define CONVERSION_BARRIER(phase) ((void)0)
#endif

namespace {
bool cancelled(const std::atomic_bool* requested)
{
    return requested && requested->load(std::memory_order_acquire);
}
ConversionResult result(ConversionStatus status, const QString& format, const QString& message)
{
    return { status, format, message };
}
bool matches(const QByteArray& bytes, const char* signature, int offset = 0)
{
    const QByteArray wanted(signature);
    return bytes.size() >= offset + wanted.size() && bytes.mid(offset, wanted.size()) == wanted;
}
QString detect(const QByteArray& bytes)
{
    if (matches(bytes, "\x89PNG\r\n\x1a\n")) return QStringLiteral("PNG");
    if (matches(bytes, "\xff\xd8\xff")) return QStringLiteral("JPEG");
    if (matches(bytes, "BM")) return QStringLiteral("BMP");
    if (matches(bytes, "%PDF-")) return QStringLiteral("PDF");
    if (matches(bytes, "RIFF") && matches(bytes, "WAVE", 8)) return QStringLiteral("WAV");
    if (matches(bytes, "PK\x03\x04")) return QStringLiteral("ZIP");
    return {};
}
}

QString ConversionEngine::detectFormat(const QString& sourcePath, QString* error)
{
    if (error) error->clear();
#ifdef Q_OS_WIN
    detail::PinnedPath path;
    if (path.open(sourcePath)) {
        detail::HandleDevice source(path.path(), false, MaxInputBytes, nullptr);
        if (source.isOpen()) {
            const QString format = detect(source.read(32));
            if (!format.isEmpty() && source.unchanged()) return format;
        }
    }
#else
    Q_UNUSED(sourcePath);
#endif
    if (error) *error = QStringLiteral("A bounded, regular source on a supported local volume is required.");
    return {};
}

ConversionResult ConversionEngine::convert(const ConversionRequest& request, const std::atomic_bool* cancellationRequested) const
{
    if (cancelled(cancellationRequested)) {
        return result(ConversionStatus::Cancelled, {}, QStringLiteral("Conversion was cancelled before reading the source."));
    }
#ifndef Q_OS_WIN
    Q_UNUSED(request);
    return result(ConversionStatus::Rejected, {}, QStringLiteral("Native file transactions are available only on Windows local NTFS volumes."));
#else
    detail::PinnedPath sourcePath;
    if (!sourcePath.open(request.sourcePath)) {
        return result(ConversionStatus::Rejected, {}, QStringLiteral("The source path must use a supported local NTFS volume without reparse components."));
    }
    detail::HandleDevice source(sourcePath.path(), false, MaxInputBytes, cancellationRequested);
    if (!source.isOpen()) {
        return result(ConversionStatus::Rejected, {}, QStringLiteral("The source must be a bounded regular file without a reparse point or an active writer."));
    }
    CONVERSION_BARRIER(SourceOpened);
    const QString sourceFormat = detect(source.read(32));
    if (sourceFormat.isEmpty()) {
        return result(cancelled(cancellationRequested) ? ConversionStatus::Cancelled : ConversionStatus::Rejected, {},
                      QStringLiteral("The source bytes do not match a supported signature."));
    }
    const AdapterDescriptor adapter = ConverterCatalog::find(sourceFormat, request.targetFormat);
    if (!adapter.enabled || !adapter.bundled) {
        return result(ConversionStatus::Rejected, sourceFormat, adapter.unavailableReason);
    }
    detail::PinnedPath outputPath;
    if (!outputPath.open(request.outputPath)) {
        return result(ConversionStatus::Rejected, sourceFormat, QStringLiteral("The destination path must use a supported local NTFS volume without reparse components."));
    }
    // The check explains pre-existing collisions. The commit itself, rather
    // than this check, authoritatively enforces create-if-absent under races.
    detail::Handle existing(CreateFileW(reinterpret_cast<LPCWSTR>(outputPath.path().utf16()),
                            FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr));
    if (existing.valid()) {
        BY_HANDLE_FILE_INFORMATION identity = {};
        if (GetFileInformationByHandle(existing.value, &identity) && detail::sameIdentity(identity, source.identity())) {
            return result(ConversionStatus::Rejected, sourceFormat, QStringLiteral("The destination identifies the source file."));
        }
        return result(ConversionStatus::Rejected, sourceFormat, request.allowOverwrite
                      ? QStringLiteral("Safe overwrite is not available in this standalone core. The existing file was preserved.")
                      : QStringLiteral("The destination already exists and was preserved."));
    }
    if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        return result(ConversionStatus::Rejected, sourceFormat, QStringLiteral("Destination absence could not be verified safely."));
    }
    if (!source.seek(0)) {
        return result(cancelled(cancellationRequested) ? ConversionStatus::Cancelled : ConversionStatus::Rejected, sourceFormat,
                      QStringLiteral("The source could not be read safely."));
    }
    QImageReader reader(&source, sourceFormat.toLatin1());
    reader.setAutoTransform(false);
    const QSize size = reader.size();
    if (!size.isValid() || size.width() <= 0 || size.height() <= 0
        || static_cast<qint64>(size.width()) * size.height() > MaxDecodedPixels) {
        return result(cancelled(cancellationRequested) ? ConversionStatus::Cancelled : ConversionStatus::Rejected, sourceFormat,
                      QStringLiteral("The image dimensions are malformed or exceed the decode limit."));
    }
    const QImage image = reader.read();
    if (cancelled(cancellationRequested)) {
        return result(ConversionStatus::Cancelled, sourceFormat, QStringLiteral("Conversion was cancelled during decoding."));
    }
    if (image.isNull() || image.size() != size || !source.unchanged()) {
        return result(ConversionStatus::Rejected, sourceFormat, QStringLiteral("The image could not be decoded safely from the unchanged source."));
    }
    CONVERSION_BARRIER(DestinationPinned);
    detail::HandleDevice temporary(outputPath.temporaryPath(), true, MaxOutputBytes, cancellationRequested);
    if (!temporary.isOpen()) {
        return result(ConversionStatus::Failed, sourceFormat, QStringLiteral("A private temporary output could not be created."));
    }
    CONVERSION_BARRIER(TemporaryOpened);
    const QByteArray targetFormat = request.targetFormat.trimmed().toLower().toLatin1();
    QImageWriter writer(&temporary, targetFormat);
    if (!writer.write(image)) {
        return result(cancelled(cancellationRequested) ? ConversionStatus::Cancelled : ConversionStatus::Failed, sourceFormat,
                      QStringLiteral("The bounded image encoder did not produce output."));
    }
    if (!temporary.seek(0)) {
        return result(cancelled(cancellationRequested) ? ConversionStatus::Cancelled : ConversionStatus::Failed, sourceFormat,
                      QStringLiteral("The temporary output could not be validated."));
    }
    // Fully decode from the same exclusive handle. A header-only size check
    // cannot prove that the encoder finished the image data.
    QImageReader verifier(&temporary, targetFormat);
    if (verifier.size() != image.size() || verifier.read().isNull()) {
        return result(cancelled(cancellationRequested) ? ConversionStatus::Cancelled : ConversionStatus::Failed, sourceFormat,
                      QStringLiteral("The temporary output failed full decode verification."));
    }
    CONVERSION_BARRIER(BeforePublish);
    if (cancelled(cancellationRequested)) {
        return result(ConversionStatus::Cancelled, sourceFormat, QStringLiteral("Conversion was cancelled before publishing output."));
    }
    if (!source.unchanged() || !temporary.publish(outputPath.path())) {
        return result(cancelled(cancellationRequested) ? ConversionStatus::Cancelled : ConversionStatus::Failed, sourceFormat,
                      QStringLiteral("The verified temporary output could not be published atomically. Any existing destination was preserved."));
    }
    return result(ConversionStatus::Converted, sourceFormat, QStringLiteral("Converted with a verified atomic publish."));
#endif
}

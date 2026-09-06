/*
* Audacity: A Digital Audio Editor
*/

#include "convertercatalog.h"

#include <QImageReader>
#include <QImageWriter>

using namespace au::converter;

namespace {
bool canRead(const QByteArray& format)
{
    return QImageReader::supportedImageFormats().contains(format);
}

bool canWrite(const QByteArray& format)
{
    return QImageWriter::supportedImageFormats().contains(format);
}

AdapterDescriptor imageAdapter(const char* source, const char* target, bool lossy)
{
    const QByteArray sourceFormat(source);
    const QByteArray targetFormat(target);
    const bool enabled = canRead(sourceFormat) && canWrite(targetFormat);
    AdapterDescriptor descriptor;
    descriptor.id = QStringLiteral("qt-image-%1-to-%2").arg(QString::fromLatin1(source), QString::fromLatin1(target));
    descriptor.category = QStringLiteral("Images");
    descriptor.sourceFormat = QString::fromLatin1(source).toUpper();
    descriptor.targetFormat = QString::fromLatin1(target).toUpper();
    descriptor.displayName = QStringLiteral("%1 to %2").arg(descriptor.sourceFormat, descriptor.targetFormat);
    descriptor.bundled = enabled;
    descriptor.enabled = enabled;
    descriptor.lossy = lossy;
    if (!enabled) {
        descriptor.unavailableReason = QStringLiteral("The bundled Qt image plugin cannot decode %1 or encode %2 in this build.")
                                         .arg(descriptor.sourceFormat, descriptor.targetFormat);
    }
    return descriptor;
}

AdapterDescriptor unavailable(const QString& category, const QString& source, const QString& target, const QString& reason)
{
    AdapterDescriptor descriptor;
    descriptor.id = QStringLiteral("unavailable-%1-to-%2").arg(source.toLower(), target.toLower());
    descriptor.category = category;
    descriptor.sourceFormat = source;
    descriptor.targetFormat = target;
    descriptor.displayName = QStringLiteral("%1 to %2").arg(source, target);
    descriptor.unavailableReason = reason;
    return descriptor;
}
}

QVector<AdapterDescriptor> ConverterCatalog::adapters()
{
    return {
        imageAdapter("png", "jpeg", true),
        imageAdapter("jpeg", "png", false),
        imageAdapter("png", "bmp", false),
        imageAdapter("bmp", "png", false),
        unavailable(QStringLiteral("Documents/PDF"), QStringLiteral("PDF"), QStringLiteral("PDF"),
                    QStringLiteral("No bundled offline PDF adapter is registered in this build.")),
        unavailable(QStringLiteral("Audio"), QStringLiteral("WAV"), QStringLiteral("MP3"),
                    QStringLiteral("No bundled offline audio conversion adapter is registered in this build.")),
        unavailable(QStringLiteral("Video"), QStringLiteral("MP4"), QStringLiteral("WEBM"),
                    QStringLiteral("No bundled offline video conversion adapter is registered in this build.")),
        unavailable(QStringLiteral("Archives"), QStringLiteral("ZIP"), QStringLiteral("TAR"),
                    QStringLiteral("No bundled offline archive conversion adapter is registered in this build.")),
        unavailable(QStringLiteral("Structured Data/Spreadsheets"), QStringLiteral("CSV"), QStringLiteral("XLSX"),
                    QStringLiteral("No bundled offline spreadsheet conversion adapter is registered in this build.")),
        unavailable(QStringLiteral("Code/Text"), QStringLiteral("TXT"), QStringLiteral("HTML"),
                    QStringLiteral("No bundled offline text conversion adapter is registered in this build.")),
        unavailable(QStringLiteral("Binary Encodings"), QStringLiteral("Base64"), QStringLiteral("Hex"),
                    QStringLiteral("No bundled offline binary encoding adapter is registered in this build.")),
    };
}

AdapterDescriptor ConverterCatalog::find(const QString& sourceFormat, const QString& targetFormat)
{
    for (const AdapterDescriptor& adapter : adapters()) {
        if (adapter.sourceFormat.compare(sourceFormat, Qt::CaseInsensitive) == 0
            && adapter.targetFormat.compare(targetFormat, Qt::CaseInsensitive) == 0) {
            return adapter;
        }
    }
    return unavailable(QStringLiteral("Unknown"), sourceFormat, targetFormat,
                       QStringLiteral("No registered offline adapter supports this conversion."));
}

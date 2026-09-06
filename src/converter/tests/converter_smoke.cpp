/*
* Audacity: A Digital Audio Editor
*/

#include "conversionengine.h"
#include "conversionqueue.h"
#include "pdfprocessor.h"

#include <QCoreApplication>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>

#include <atomic>
#include <cstdio>

using namespace au::converter;

namespace {
bool expect(bool condition, const char* message)
{
    if (!condition) std::fprintf(stderr, "%s\n", message);
    return condition;
}

QString png(const QTemporaryDir& directory, const QString& name)
{
    const QString path = directory.filePath(name);
    QImage image(4, 3, QImage::Format_ARGB32);
    image.fill(Qt::blue);
    return image.save(path, "png") ? path : QString();
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    if (!expect(directory.isValid(), "temporary directory")) return 1;
    const QString source = png(directory, QStringLiteral("input.png"));
    if (!expect(!source.isEmpty(), "create png")) return 1;
    ConversionEngine engine;
    const QString output = directory.filePath(QStringLiteral("output.jpg"));
    const ConversionResult converted = engine.convert({ source, output, QStringLiteral("jpeg") });
    if (!expect(converted.status == ConversionStatus::Converted, qPrintable(converted.message))) return 1;
    if (!expect(!QImage(output).isNull(), "reopen published image")) return 1;
    const std::atomic_bool cancelled = true;
    if (!expect(engine.convert({ source, directory.filePath(QStringLiteral("cancel.jpg")), QStringLiteral("jpeg") }, &cancelled).status == ConversionStatus::Cancelled, "atomic cancellation")) return 1;
    if (!expect(engine.convert({ source, source, QStringLiteral("jpeg") }).status == ConversionStatus::Rejected, "source destination collision")) return 1;
    ConversionQueue queue(directory.filePath(QStringLiteral("queue")));
    if (!expect(queue.enqueue({ source, output, QStringLiteral("jpeg") }), "enqueue")) return 1;
    const QVector<QueueItem> page = queue.page(0, 1);
    if (!expect(page.size() == 1 && queue.cancel(page.first().id), "paged cancellation")) return 1;
    ConversionQueue restarted(directory.filePath(QStringLiteral("queue")));
    if (!expect(restarted.load() && restarted.count() == 1, "durable restart")) return 1;
    PdfProcessor pdf;
    const PdfResult unavailable = pdf.process({ PdfOperation::Inspect, { directory.filePath(QStringLiteral("missing.pdf")) } });
    if (!expect(!unavailable.ok && !unavailable.message.isEmpty(), "bounded PDF unavailable state")) return 1;
    return 0;
}

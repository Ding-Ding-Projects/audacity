/*
* Audacity: A Digital Audio Editor
*/
#include "pdfprocessor.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QUuid>

using namespace au::converter;

namespace {
constexpr auto QpdfSha256 = "43f79db620ce09529a67572a5de87aec4065b95f11ba6e5918db557f943a7eac";
constexpr qint64 MaxProcessOutputBytes = 1024 * 1024;

bool cancelled(const std::atomic_bool* value) { return value && value->load(std::memory_order_acquire); }
QString hashFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray block = file.read(1024 * 1024);
        if (block.isEmpty() && file.error() != QFile::NoError) return {};
        hash.addData(block);
    }
    return QString::fromLatin1(hash.result().toHex());
}
bool boundedPdf(const QString& path, qint64 limit)
{
    const QFileInfo info(path);
    if (!info.isFile() || info.isSymLink() || info.size() <= 0 || info.size() > limit) return false;
    QFile file(path);
    return file.open(QIODevice::ReadOnly) && file.read(5) == QByteArrayLiteral("%PDF-");
}
struct CommandResult { bool ok = false; bool cancelled = false; QByteArray output; QString message; };
CommandResult execute(const QStringList& arguments, const std::atomic_bool* cancellation)
{
    QProcess process;
    process.setProgram(PdfProcessor::bundledToolPath());
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProcessEnvironment(QProcessEnvironment());
    process.start();
    if (!process.waitForStarted(5000)) return { false, false, {}, QStringLiteral("The bundled qpdf process could not start.") };
    qint64 waited = 0;
    while (!process.waitForFinished(100)) {
        waited += 100;
        if (cancelled(cancellation)) { process.kill(); process.waitForFinished(5000); return { false, true, {}, QStringLiteral("PDF operation was cancelled.") }; }
        if (waited >= PdfProcessor::TimeoutMilliseconds) { process.kill(); process.waitForFinished(5000); return { false, false, {}, QStringLiteral("PDF operation exceeded its 60 second limit.") }; }
        if (process.bytesAvailable() > MaxProcessOutputBytes) { process.kill(); process.waitForFinished(5000); return { false, false, {}, QStringLiteral("PDF tool output exceeded its limit.") }; }
    }
    const QByteArray output = process.readAll();
    if (output.size() > MaxProcessOutputBytes) return { false, false, {}, QStringLiteral("PDF tool output exceeded its limit.") };
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return { false, false, {}, QStringLiteral("qpdf rejected the PDF: %1").arg(QString::fromLocal8Bit(output.left(2048)).trimmed()) };
    return { true, false, output, {} };
}
int pageCount(const QString& path, QString* failure)
{
    const CommandResult checked = execute({ QStringLiteral("--check"), path }, nullptr);
    if (!checked.ok) { if (failure) *failure = checked.message; return 0; }
    const CommandResult pages = execute({ QStringLiteral("--show-npages"), path }, nullptr);
    bool numeric = false;
    const int count = QString::fromLatin1(pages.output).trimmed().toInt(&numeric);
    if (!pages.ok || !numeric || count < 1) { if (failure) *failure = pages.ok ? QStringLiteral("qpdf did not report a valid page count.") : pages.message; return 0; }
    return count;
}
bool validPages(const QString& spec) { return QRegularExpression(QStringLiteral(R"(^[1-9][0-9]*(?:-[1-9][0-9]*)?(?:,[1-9][0-9]*(?:-[1-9][0-9]*)?)*$)")).match(spec).hasMatch(); }
}

QString PdfProcessor::bundledToolPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("converter-tools/qpdf/qpdf.exe"));
}

QString PdfProcessor::availabilityReason()
{
#ifndef Q_OS_WIN
    return QStringLiteral("The bundled qpdf adapter is currently packaged only for Windows.");
#else
    const QString path = bundledToolPath();
    if (!QFileInfo::exists(path)) return QStringLiteral("The required bundled qpdf binary is absent from this installation.");
    if (hashFile(path) != QString::fromLatin1(QpdfSha256)) return QStringLiteral("The bundled qpdf binary failed its pinned SHA-256 verification.");
    return {};
#endif
}

bool PdfProcessor::available() { return availabilityReason().isEmpty(); }

PdfResult PdfProcessor::process(const PdfRequest& request, const std::atomic_bool* cancellationRequested) const
{
    if (cancelled(cancellationRequested)) return { false, true, 0, QStringLiteral("PDF operation was cancelled before reading the source.") };
    const QString reason = availabilityReason();
    if (!reason.isEmpty()) return { false, false, 0, reason };
    if (request.sourcePaths.isEmpty() || request.sourcePaths.size() > 32) return { false, false, 0, QStringLiteral("PDF operations require between one and 32 source files.") };
    for (const QString& input : request.sourcePaths) {
        if (!boundedPdf(input, MaxInputBytes)) return { false, false, 0, QStringLiteral("Each source must be a regular, bounded PDF file.") };
        const CommandResult encryption = execute({ QStringLiteral("--show-encryption"), input }, cancellationRequested);
        if (!encryption.ok) return { false, encryption.cancelled, 0, encryption.message };
        if (QString::fromLatin1(encryption.output).contains(QStringLiteral("encrypted"), Qt::CaseInsensitive)
            && !QString::fromLatin1(encryption.output).contains(QStringLiteral("not encrypted"), Qt::CaseInsensitive))
            return { false, false, 0, QStringLiteral("The encrypted PDF requires credentials that this offline adapter does not accept.") };
    }
    if (request.operation == PdfOperation::Inspect) {
        QString failure; const int pages = pageCount(request.sourcePaths.first(), &failure);
        return { pages > 0, false, pages, pages > 0 ? QStringLiteral("PDF reopened and verified.") : failure };
    }
    if (request.outputPath.isEmpty() || QFileInfo::exists(request.outputPath) || request.allowOverwrite)
        return { false, false, 0, QStringLiteral("PDF output must be a new path. Existing files are always preserved.") };
    const QFileInfo outputInfo(request.outputPath);
    if (!outputInfo.dir().exists()) return { false, false, 0, QStringLiteral("The output directory does not exist.") };
    const QString outputStem = outputInfo.completeBaseName();
    if (request.operation == PdfOperation::Split
        && !outputInfo.dir().entryList({ outputStem + QStringLiteral("-*.pdf") }, QDir::Files).isEmpty())
        return { false, false, 0, QStringLiteral("A split output already exists and was preserved.") };
    const QString temp = outputInfo.dir().filePath(QStringLiteral(".audacity-pdf-") + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".pdf"));
    QStringList args;
    switch (request.operation) {
    case PdfOperation::Merge: args << QStringLiteral("--empty") << QStringLiteral("--pages") << request.sourcePaths << QStringLiteral("--") << temp; break;
    case PdfOperation::Extract:
    case PdfOperation::Reorder:
        if (!validPages(request.pageSpec)) return { false, false, 0, QStringLiteral("Page selections must contain only positive page numbers, commas, and ranges.") };
        args << QStringLiteral("--empty") << QStringLiteral("--pages") << request.sourcePaths.first() << request.pageSpec << QStringLiteral("--") << temp; break;
    case PdfOperation::Rotate:
        if (request.rotation != 90 && request.rotation != 180 && request.rotation != 270) return { false, false, 0, QStringLiteral("Rotation must be 90, 180, or 270 degrees.") };
        args << (QStringLiteral("--rotate=+") + QString::number(request.rotation) + QStringLiteral(":1-z")) << request.sourcePaths.first() << temp; break;
    case PdfOperation::Split:
        if (!validPages(request.pageSpec)) return { false, false, 0, QStringLiteral("Split requires a bounded positive page selection.") };
        args << (QStringLiteral("--split-pages=") + request.pageSpec) << request.sourcePaths.first() << temp; break;
    case PdfOperation::SetMetadata:
        return { false, false, 0, QStringLiteral("Metadata editing needs the bundled qpdf JSON profile and is not enabled until that profile is packaged and verified.") };
    default: return { false, false, 0, QStringLiteral("Unsupported PDF operation.") };
    }
    const CommandResult transformed = execute(args, cancellationRequested);
    if (!transformed.ok) { QFile::remove(temp); return { false, transformed.cancelled, 0, transformed.message }; }
    if (request.operation == PdfOperation::Split) {
        const QString temporaryStem = QFileInfo(temp).completeBaseName();
        const QStringList pieces = outputInfo.dir().entryList({ temporaryStem + QStringLiteral("-*.pdf") }, QDir::Files, QDir::Name);
        if (pieces.isEmpty()) return { false, false, 0, QStringLiteral("qpdf did not create any split output.") };
        int totalPages = 0;
        QStringList published;
        for (const QString& piece : pieces) {
            QString failure;
            const QString sourcePiece = outputInfo.dir().filePath(piece);
            const int pages = pageCount(sourcePiece, &failure);
            if (pages < 1 || !boundedPdf(sourcePiece, MaxOutputBytes)) {
                for (const QString& cleanup : pieces) QFile::remove(outputInfo.dir().filePath(cleanup));
                return { false, false, 0, failure.isEmpty() ? QStringLiteral("A split PDF failed reopen verification.") : failure };
            }
            const QString suffix = piece.mid(temporaryStem.size());
            const QString targetPiece = outputInfo.dir().filePath(outputStem + suffix);
            if (!QFile::rename(sourcePiece, targetPiece)) {
                for (const QString& cleanup : pieces) QFile::remove(outputInfo.dir().filePath(cleanup));
                for (const QString& cleanup : published) QFile::remove(cleanup);
                return { false, false, 0, QStringLiteral("A verified split PDF could not be published without overwriting a destination.") };
            }
            published << targetPiece;
            totalPages += pages;
        }
        return { true, false, totalPages, QStringLiteral("Split PDFs were reopened and published without overwriting existing files.") };
    }
    QString failure; const int pages = pageCount(temp, &failure);
    if (!pages || !boundedPdf(temp, MaxOutputBytes)) { QFile::remove(temp); return { false, false, 0, failure.isEmpty() ? QStringLiteral("Generated PDF failed bounded reopen verification.") : failure }; }
    if (!QFile::rename(temp, request.outputPath)) { QFile::remove(temp); return { false, false, 0, QStringLiteral("Verified temporary PDF could not be published without overwriting a destination.") }; }
    return { true, false, pages, QStringLiteral("PDF output was reopened and published without overwriting an existing file.") };
}

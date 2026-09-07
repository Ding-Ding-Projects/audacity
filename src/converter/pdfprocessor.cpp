/*
* Audacity: A Digital Audio Editor
*/
#include "pdfprocessor.h"
#include "nativefiletransaction.h"
#include "qpdfbundle.h"
#include "processcontainment.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

using namespace au::converter;
#ifdef AU_CONVERTER_TEST_HOOKS
thread_local std::function<void(PdfProcessor::TestPhase, const QString&)> PdfProcessor::testHook;
thread_local int PdfProcessor::testTimeoutMilliseconds = PdfProcessor::TimeoutMilliseconds;
thread_local qint64 PdfProcessor::testProcessOutputBytes = 1024 * 1024;
thread_local qint64 PdfProcessor::testOutputBytes = PdfProcessor::MaxOutputBytes;
thread_local qint64 PdfProcessor::testProcessMemoryBytes = PdfProcessor::MaxProcessMemoryBytes;
#define PDF_BARRIER(phase, path) if (PdfProcessor::testHook) PdfProcessor::testHook(PdfProcessor::TestPhase::phase, path)
#else
#define PDF_BARRIER(phase, path) ((void)0)
#endif
namespace {
bool cancelled(const std::atomic_bool* value) { return value && value->load(std::memory_order_acquire); }
QString hashDevice(QIODevice& file)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray block = file.read(1024 * 1024);
        if (block.isEmpty()) return {};
        hash.addData(block);
    }
    return QString::fromLatin1(hash.result().toHex());
}
struct CommandResult { bool ok = false; bool cancelled = false; QByteArray output; QString message; };
struct Budget {
    QElapsedTimer timer;
    QString toolPath;
#ifdef Q_OS_WIN
    detail::ProcessContainment containment;
#endif
    int milliseconds = PdfProcessor::TimeoutMilliseconds;
    qint64 processBytes = 1024 * 1024;
    qint64 outputBytes = PdfProcessor::MaxOutputBytes;
    Budget() {
#ifdef AU_CONVERTER_TEST_HOOKS
        milliseconds = std::clamp(PdfProcessor::testTimeoutMilliseconds, 0, milliseconds);
        processBytes = std::clamp(PdfProcessor::testProcessOutputBytes, qint64(0), processBytes);
        outputBytes = std::clamp(PdfProcessor::testOutputBytes, qint64(0), outputBytes);
#endif
        timer.start();
    }
};
CommandResult execute(const QStringList& arguments, const std::atomic_bool* cancellation, Budget& budget, QIODevice* sink = nullptr)
{
    if (cancelled(cancellation)) return {false, true, {}, QStringLiteral("PDF operation was cancelled.")};
#ifndef Q_OS_WIN
    return {false, false, {}, QStringLiteral("PDF containment requires Windows.")};
#else
    const auto alive = [&] { return !cancelled(cancellation) && budget.timer.elapsed() < budget.milliseconds; };
    struct Copies {
        detail::ProcessContainment& container;
        size_t marker;
        ~Copies() { container.release(marker); }
    } copies { budget.containment, budget.containment.mark() };
    QStringList staged;
    int sequence = 0;
    for (const QString& argument : arguments) {
        const int prefix = argument.startsWith(QStringLiteral("--update-from-json=")) ? 19 : 0;
        QString path = argument.mid(prefix);
        if (path.startsWith(QStringLiteral("\\\\?\\Volume{")) || path.startsWith(QStringLiteral("\\\\.\\Volume{"))) {
            path[2] = QLatin1Char('?');
            detail::HandleDevice input(path, false, PdfProcessor::MaxOutputBytes, cancellation);
            const QString copied = input.isOpen() ? budget.containment.copy(input,
                QStringLiteral("input-%1.bin").arg(sequence++), PdfProcessor::MaxOutputBytes, {}, alive) : QString();
            if (copied.isEmpty()) return {false, cancelled(cancellation), {}, QStringLiteral("PDF read-only staging exceeded its bound or could not be secured.")};
            staged << argument.left(prefix) + copied;
        } else staged << argument;
    }
    qint64 memory = PdfProcessor::MaxProcessMemoryBytes;
#ifdef AU_CONVERTER_TEST_HOOKS
    memory = std::clamp(PdfProcessor::testProcessMemoryBytes, qint64(1), memory);
#endif
    detail::ContainedProcess process;
    if (!process.start(budget.toolPath, staged, budget.containment, memory,
                       std::max(1, budget.milliseconds - int(budget.timer.elapsed())), [&] {
                           PDF_BARRIER(ProcessLimitsInstalled, QStringLiteral("memory=%1;processes=1;killOnClose=1").arg(memory));
                       }))
        return {false, false, {}, QStringLiteral("The required Windows LPAC worker could not start: ") + process.diagnostic()};
    PDF_BARRIER(ProcessStarted, arguments.join(QLatin1Char(' ')));
    QByteArray output;
    qint64 received = 0;
    for (;;) {
        if (cancelled(cancellation)) return {false, true, {}, QStringLiteral("PDF operation was cancelled.")};
        if (budget.timer.elapsed() >= budget.milliseconds) return {false, false, {}, QStringLiteral("PDF operation exceeded its execution deadline.")};
        QByteArray bytes, diagnostics;
        if (!process.read(bytes, diagnostics)) return {false, false, {}, QStringLiteral("PDF worker output could not be read.")};
        received += diagnostics.size() + (sink ? 0 : bytes.size());
        if (received > budget.processBytes) return {false, false, {}, QStringLiteral("PDF tool output exceeded its limit.")};
        if (sink) {
            if (sink->size() > budget.outputBytes - bytes.size() || sink->write(bytes) != bytes.size())
                return {false, cancelled(cancellation), {}, QStringLiteral("PDF output exceeded its bound or could not be written.")};
        } else output += bytes;
        if (!process.running() && bytes.isEmpty() && diagnostics.isEmpty()) break;
        if (bytes.isEmpty() && diagnostics.isEmpty()) process.wait(20);
    }
    if (process.exitCode() != 0) return {false, false, {}, QStringLiteral("qpdf rejected the PDF (exit %1).").arg(process.exitCode())};
    return {true, false, output, {}};
#endif
}
CommandResult countPages(const QString& path, const std::atomic_bool* cancellation, Budget& budget, int& count)
{
    const auto checked = execute({QStringLiteral("--check"), path}, cancellation, budget);
    if (!checked.ok) return checked;
    auto pages = execute({QStringLiteral("--show-npages"), path}, cancellation, budget);
    bool numeric = false;
    count = QString::fromLatin1(pages.output).trimmed().toInt(&numeric);
    if (pages.ok && (!numeric || count < 1 || count > 10000))
        return {false, false, {}, QStringLiteral("PDF page count is invalid or exceeds 10000 pages.")};
    return pages;
}
bool validPages(const QString& spec) {
    return spec.size() <= 4096 && QRegularExpression(QStringLiteral(R"(^[1-9][0-9]*(?:-[1-9][0-9]*)?(?:,[1-9][0-9]*(?:-[1-9][0-9]*)?)*$)")).match(spec).hasMatch();
}
#ifdef Q_OS_WIN
struct PinnedFile {
    detail::PinnedPath path;
    std::unique_ptr<detail::HandleDevice> file;
    bool open(const QString& name, qint64 limit) {
        if (!path.open(name)) return false;
        file = std::make_unique<detail::HandleDevice>(path.path(), false, limit, nullptr);
        return file->isOpen();
    }
};
QString pinBundle(std::vector<std::unique_ptr<PinnedFile>>& files, QString* toolPath = nullptr)
{
    const QDir folder(QFileInfo(PdfProcessor::bundledToolPath()).absolutePath());
    const auto entries = folder.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    if (entries.size() != detail::QpdfFiles.size()) return QStringLiteral("The qpdf bundle contains missing or unexpected entries.");
    for (const auto& entry : entries) {
        if (!entry.isFile() || entry.isSymLink() || !detail::QpdfFiles.contains(entry.fileName()))
            return QStringLiteral("The qpdf bundle contains an unexpected or redirected entry.");
    }
    for (auto it = detail::QpdfFiles.cbegin(); it != detail::QpdfFiles.cend(); ++it) {
        auto pin = std::make_unique<PinnedFile>();
        if (!pin->open(folder.filePath(it.key()), PdfProcessor::MaxInputBytes) || hashDevice(*pin->file) != it.value())
            return QStringLiteral("Bundled qpdf component %1 is missing or failed pinned SHA-256 verification.").arg(it.key());
        if (toolPath && it.key() == QStringLiteral("qpdf.exe")) {
            *toolPath = pin->path.path();
            (*toolPath)[2] = QLatin1Char('.');
        }
        files.push_back(std::move(pin));
    }
    return {};
}
QString prepareRuntime(Budget& budget, std::vector<std::unique_ptr<PinnedFile>>& bundle, const std::atomic_bool* cancellation)
{
    const QString reason = pinBundle(bundle);
    if (!reason.isEmpty()) return reason;
    if (!budget.containment.valid()) return QStringLiteral("Required Windows LPAC staging is unavailable.");
    size_t component = 0;
    for (auto it = detail::QpdfFiles.cbegin(); it != detail::QpdfFiles.cend(); ++it, ++component) {
        const QString copied = budget.containment.copy(*bundle[component]->file, it.key(), PdfProcessor::MaxInputBytes, it.value(),
            [&] { return !cancelled(cancellation) && budget.timer.elapsed() < budget.milliseconds; });
        if (copied.isEmpty()) return budget.timer.elapsed() >= budget.milliseconds
            ? QStringLiteral("PDF operation exceeded its execution deadline.")
            : QStringLiteral("Verified PDF runtime could not be staged within its bound.");
        if (it.key() == QStringLiteral("qpdf.exe")) budget.toolPath = copied;
    }
    return {};
}
#endif
}
QString PdfProcessor::bundledToolPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("converter-tools/qpdf/qpdf.exe"));
}
QString PdfProcessor::availabilityReason()
{
#ifdef Q_OS_WIN
    Budget budget;
    std::vector<std::unique_ptr<PinnedFile>> files;
    const QString reason = prepareRuntime(budget, files, nullptr);
    if (!reason.isEmpty()) return reason;
    const auto probe = execute({QStringLiteral("--version")}, nullptr, budget);
    return probe.ok ? QString() : probe.message;
#else
    return QStringLiteral("The bundled qpdf adapter is currently packaged only for Windows.");
#endif
}
bool PdfProcessor::available() { return availabilityReason().isEmpty(); }

PdfResult PdfProcessor::process(const PdfRequest& request, const std::atomic_bool* cancellation) const
{
    PdfResult result;
    auto fail = [&](const QString& message, bool wasCancelled = false) {
        result.ok = false; result.cancelled = wasCancelled; result.message = message;
        return result;
    };
    if (cancelled(cancellation)) return fail(QStringLiteral("PDF operation was cancelled before reading the source."), true);
#ifndef Q_OS_WIN
    return fail(availabilityReason());
#else
    Budget budget;
    std::vector<std::unique_ptr<PinnedFile>> bundle;
    const auto reason = prepareRuntime(budget, bundle, cancellation);
    if (!reason.isEmpty()) return fail(reason, cancelled(cancellation));
    if (request.sourcePaths.isEmpty() || request.sourcePaths.size() > 32
        || (request.operation != PdfOperation::Merge && request.sourcePaths.size() != 1))
        return fail(QStringLiteral("PDF operations require one source, or up to 32 sources for merge."));
    std::vector<std::unique_ptr<PinnedFile>> inputs;
    QVector<int> counts;
    QStringList inputPaths;
    for (const QString& source : request.sourcePaths) {
        auto input = std::make_unique<PinnedFile>();
        if (!input->open(source, MaxInputBytes) || input->file->read(5) != QByteArrayLiteral("%PDF-"))
            return fail(QStringLiteral("Each source must be a pinned regular bounded PDF on local NTFS."));
        inputPaths << input->path.path();
        inputs.push_back(std::move(input));
    }
    PDF_BARRIER(SourcesPinned, inputPaths.first());
    for (const auto& input : inputPaths) {
        const auto encryption = execute({QStringLiteral("--show-encryption"), input}, cancellation, budget);
        if (!encryption.ok) return fail(encryption.message, encryption.cancelled);
        if (encryption.output.trimmed() != QByteArrayLiteral("File is not encrypted"))
            return fail(QStringLiteral("Encrypted PDFs are unsupported; this adapter never accepts credentials."));
        int count = 0;
        const auto pages = countPages(input, cancellation, budget, count);
        if (!pages.ok) return fail(pages.message, pages.cancelled);
        counts << count;
    }
    if (request.operation == PdfOperation::Inspect) {
        result.ok = true; result.pageCount = counts.first(); result.message = QStringLiteral("PDF reopened and verified."); return result;
    }
    if (request.outputPath.isEmpty() || request.allowOverwrite || QFileInfo::exists(request.outputPath))
        return fail(QStringLiteral("PDF output must be a new path. Existing files are always preserved."));
    detail::PinnedPath destination;
    if (!destination.open(request.outputPath)) return fail(QStringLiteral("Output requires an existing pinned local NTFS directory."));
    const QFileInfo outputInfo(request.outputPath);
    const QString outputStem = outputInfo.completeBaseName();
    if (request.operation == PdfOperation::Split
        && !outputInfo.dir().entryList({outputStem + QStringLiteral("-*.pdf")}, QDir::Files | QDir::Dirs | QDir::Hidden).isEmpty())
        return fail(QStringLiteral("A split destination already exists and was preserved."));

    int chunk = 0;
    if (request.operation == PdfOperation::Split) {
        bool valid = false; chunk = request.pageSpec.toInt(&valid);
        if (!valid || !QRegularExpression(QStringLiteral("^[1-9][0-9]*$")).match(request.pageSpec).hasMatch() || chunk < 1 || chunk > counts.first())
            return fail(QStringLiteral("Split size must be an integer between one and the source page count."));
        if ((counts.first() + chunk - 1) / chunk > 1000) return fail(QStringLiteral("Split output count exceeds 1000 files."));
    }
    std::unique_ptr<detail::HandleDevice> metadataFile;
    QString metadataPath;
    if (request.operation == PdfOperation::SetMetadata) {
        const QStringList fields {QStringLiteral("Title"), QStringLiteral("Author"), QStringLiteral("Subject"), QStringLiteral("Keywords")};
        if (request.metadata.isEmpty()) return fail(QStringLiteral("Metadata updates require an allowlisted field."));
        for (auto it = request.metadata.cbegin(); it != request.metadata.cend(); ++it)
            if (!fields.contains(it.key()) || it.value().size() > 1024 || it.value().contains(QChar::Null))
                return fail(QStringLiteral("Only bounded Title, Author, Subject, and Keywords metadata is allowed."));
        const auto json = execute({QStringLiteral("--json-output"), inputPaths.first()}, cancellation, budget);
        if (!json.ok) return fail(json.message, json.cancelled);
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(json.output, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) return fail(QStringLiteral("qpdf metadata JSON was malformed."));
        auto root = document.object(); auto objects = root.value(QStringLiteral("qpdf")).toArray();
        if (objects.size() != 2) return fail(QStringLiteral("qpdf metadata JSON lacked its object table."));
        auto table = objects[1].toObject();
        auto trailerObject = table.value(QStringLiteral("trailer")).toObject();
        auto trailer = trailerObject.value(QStringLiteral("value")).toObject();
        QString infoRef = trailer.value(QStringLiteral("/Info")).toString();
        if (infoRef.isEmpty()) {
            auto header = objects[0].toObject();
            const int next = header.value(QStringLiteral("maxobjectid")).toInt() + 1;
            if (next < 1) return fail(QStringLiteral("qpdf object inventory is invalid."));
            infoRef = QStringLiteral("%1 0 R").arg(next);
            header.insert(QStringLiteral("maxobjectid"), next); objects[0] = header;
            trailer.insert(QStringLiteral("/Info"), infoRef);
            trailerObject.insert(QStringLiteral("value"), trailer); table.insert(QStringLiteral("trailer"), trailerObject);
        }
        const QString key = QStringLiteral("obj:") + infoRef;
        auto infoObject = table.value(key).toObject(); auto values = infoObject.value(QStringLiteral("value")).toObject();
        for (auto it = request.metadata.cbegin(); it != request.metadata.cend(); ++it) values.insert(QLatin1Char('/') + it.key(), QStringLiteral("u:") + it.value());
        infoObject.insert(QStringLiteral("value"), values); table.insert(key, infoObject); objects[1] = table; root.insert(QStringLiteral("qpdf"), objects);
        metadataPath = destination.temporaryPath();
        metadataFile = std::make_unique<detail::HandleDevice>(metadataPath, true, MaxInputBytes, cancellation, true);
        const auto bytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
        if (!metadataFile->isOpen() || metadataFile->write(bytes) != bytes.size() || !metadataFile->sealForSubprocess()) return fail(QStringLiteral("Metadata update JSON could not be staged safely."), cancelled(cancellation));
    }
    const int operations = chunk ? (counts.first() + chunk - 1) / chunk : 1;
    for (int index = 0; index < operations; ++index) {
        if (cancelled(cancellation)) return fail(QStringLiteral("PDF operation cancelled; committed outputs are retained."), true);
        QString target = request.outputPath;
        if (chunk) {
            const int start = index * chunk + 1, end = std::min(start + chunk - 1, counts.first());
            const int digits = QString::number(counts.first()).size();
            QString suffix = QStringLiteral("-%1").arg(start, digits, 10, QLatin1Char('0'));
            if (end != start) suffix += QStringLiteral("-%1").arg(end, digits, 10, QLatin1Char('0'));
            target = outputInfo.dir().filePath(outputStem + suffix + QStringLiteral(".pdf"));
        }
        detail::PinnedPath targetPin;
        if (!targetPin.open(target)) return fail(QStringLiteral("Output path could not be pinned; committed outputs are retained."));
        const QString temp = targetPin.temporaryPath();
        detail::HandleDevice output(temp, true, budget.outputBytes, cancellation, true);
        if (!output.isOpen()) return fail(QStringLiteral("A private output could not be created."));
        QStringList args;
        switch (request.operation) {
        case PdfOperation::Merge:
            args << QStringLiteral("--empty") << QStringLiteral("--pages");
            for (const auto& input : inputPaths) args << input << QStringLiteral("1-z");
            args << QStringLiteral("--"); break;
        case PdfOperation::Extract:
        case PdfOperation::Reorder:
            if (!validPages(request.pageSpec)) return fail(QStringLiteral("Page selections must contain bounded positive page numbers and ranges."));
            args << QStringLiteral("--empty") << QStringLiteral("--pages") << inputPaths.first() << request.pageSpec << QStringLiteral("--"); break;
        case PdfOperation::Split:
            args << QStringLiteral("--empty") << QStringLiteral("--pages") << inputPaths.first()
                 << QStringLiteral("%1-%2").arg(index * chunk + 1).arg(std::min((index + 1) * chunk, counts.first())) << QStringLiteral("--"); break;
        case PdfOperation::Rotate:
            if (request.rotation != 90 && request.rotation != 180 && request.rotation != 270) return fail(QStringLiteral("Rotation must be 90, 180, or 270 degrees."));
            args << (QStringLiteral("--rotate=+") + QString::number(request.rotation) + QStringLiteral(":1-z")) << inputPaths.first(); break;
        case PdfOperation::SetMetadata: args << inputPaths.first() << (QStringLiteral("--update-from-json=") + metadataPath); break;
        default: return fail(QStringLiteral("Unsupported PDF operation."));
        }
        args << QStringLiteral("-");
        const auto transformed = execute(args, cancellation, budget, &output);
        if (!transformed.ok) return fail(transformed.message, transformed.cancelled);
        if (!output.sealForSubprocess() || !output.seek(0) || output.read(5) != QByteArrayLiteral("%PDF-")) return fail(QStringLiteral("Generated output lacked a PDF signature."), cancelled(cancellation));
        int count = 0; const auto checked = countPages(temp, cancellation, budget, count);
        if (!checked.ok) return fail(checked.message, checked.cancelled);
        PDF_BARRIER(BeforePublish, target);
        if (!output.publish(targetPin.path())) return fail(QStringLiteral("Publication stopped without overwriting destinations; committed outputs are retained."), cancelled(cancellation));
        result.outputs.push_back({target, count, true}); result.pageCount += count;
        PDF_BARRIER(OutputPublished, target);
    }
    result.ok = true; result.message = QStringLiteral("PDF outputs were reopened and published without overwriting originals.");
    return result;
#endif
}

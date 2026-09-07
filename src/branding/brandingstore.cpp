/*
 * Audacity: A Digital Audio Editor
 */
#include "brandingstore.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSaveFile>
#include <QUuid>

using namespace au::branding;

namespace {
constexpr int DerivativeSizes[] { 16, 32, 48, 64, 128, 256 };
constexpr auto CacheVersion = 1;

bool cancelled(const BrandingStore::Cancellation& check)
{
    return check && check();
}

bool writeAtomically(const QString& path, const QByteArray& bytes)
{
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}

QString activePath(const QString& root)
{
    return QDir(root).filePath("branding-v1");
}
}

BrandingStore::BrandingStore(QString profileRoot)
    : m_profileRoot(std::move(profileRoot))
{
    const QString metadataPath = QDir(activePath(m_profileRoot)).filePath("metadata.json");
    QFile metadata(metadataPath);
    if (!metadata.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(metadata.readAll());
    const QJsonObject object = document.object();
    if (!document.isObject() || object.value("version").toInt() != CacheVersion) {
        return;
    }
    QFile source(QDir(activePath(m_profileRoot)).filePath("source.bin"));
    if (!source.open(QIODevice::ReadOnly)) {
        return;
    }
    const QByteArray sourceBytes = source.readAll();
    QImage image;
    QString format;
    if (!decode(sourceBytes, &image, &format).ok) {
        return;
    }
    const QDir active(activePath(m_profileRoot));
    for (int size : DerivativeSizes) {
        const QImage derivativeImage(active.filePath(QString::number(size) + ".png"));
        if (derivativeImage.isNull() || derivativeImage.size() != QSize(size, size)) {
            return;
        }
    }
    m_source = sourceBytes;
    m_format = format;
    m_options.fitMode = object.value("fitMode").toString() == "crop" ? LogoFitMode::Crop : LogoFitMode::Fit;
    m_options.background = QColor(object.value("background").toString());
    if (!m_options.background.isValid()) {
        m_options.background = Qt::transparent;
    }
    m_hasCustomLogo = true;
}

QByteArray BrandingStore::presetDefaultMark(const QByteArray& shippedMark)
{
    return shippedMark;
}

LogoResult BrandingStore::loadCustom(const QByteArray& bytes, const LogoOptions& options, const Cancellation& check)
{
    QImage image;
    QString format;
    LogoResult decoded = decode(bytes, &image, &format);
    if (!decoded.ok) {
        return decoded;
    }
    return commit(bytes, format, options, check);
}

LogoResult BrandingStore::update(const LogoOptions& options, const Cancellation& check)
{
    if (!m_hasCustomLogo) {
        return { false, "No custom logo is loaded" };
    }
    return commit(m_source, m_format, options, check);
}

LogoResult BrandingStore::reset()
{
    const QString active = activePath(m_profileRoot);
    if (!QDir(active).removeRecursively() && QDir(active).exists()) {
        return { false, "Could not reset the local logo cache" };
    }
    m_source.clear();
    m_format.clear();
    m_options = {};
    m_hasCustomLogo = false;
    return { true, {} };
}

bool BrandingStore::hasCustomLogo() const { return m_hasCustomLogo; }
QString BrandingStore::profileRoot() const { return m_profileRoot; }
LogoOptions BrandingStore::options() const { return m_options; }

QStringList BrandingStore::derivativePaths() const
{
    QStringList paths;
    if (!m_hasCustomLogo) {
        return paths;
    }
    const QDir active(activePath(m_profileRoot));
    for (int size : DerivativeSizes) {
        const QString path = active.filePath(QString::number(size) + ".png");
        if (QFile::exists(path)) {
            paths.append(path);
        }
    }
    return paths;
}

QImage BrandingStore::derivative(int size) const
{
    return QImage(QDir(activePath(m_profileRoot)).filePath(QString::number(size) + ".png"));
}

LogoResult BrandingStore::decode(const QByteArray& bytes, QImage* image, QString* format)
{
    if (bytes.isEmpty() || bytes.size() > MaxSourceBytes) {
        return { false, "Logo data is empty or exceeds the byte limit" };
    }
    const QByteArray lower = bytes.left(4096).toLower();
    if (lower.contains("<svg") && !svgIsSafe(bytes)) {
        return { false, "SVG contains external or executable content" };
    }
    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    const QString decodedFormat = QString::fromLatin1(reader.format()).toLower();
    if (decodedFormat != "png" && decodedFormat != "jpeg" && decodedFormat != "jpg"
        && decodedFormat != "webp" && decodedFormat != "svg" && decodedFormat != "ico") {
        return { false, "Unsupported logo format" };
    }
    const QSize size = reader.size();
    if (!size.isValid() || size.width() > MaxDimension || size.height() > MaxDimension
        || qint64(size.width()) * size.height() > qint64(MaxDimension) * MaxDimension) {
        return { false, "Logo dimensions exceed the limit" };
    }
    QImage decoded = reader.read();
    if (decoded.isNull()) {
        return { false, "Logo data could not be decoded" };
    }
    *image = decoded;
    *format = decodedFormat == "jpg" ? "jpeg" : decodedFormat;
    return { true, {} };
}

bool BrandingStore::svgIsSafe(const QByteArray& bytes)
{
    const QByteArray text = bytes.toLower();
    static const QByteArray forbidden[] { "<script", "javascript:", "<foreignobject", "<iframe", "<object",
                                           "<embed", "<image", "href=", "xlink:href", "url(", "@import" };
    for (const QByteArray& value : forbidden) {
        if (text.contains(value)) {
            return false;
        }
    }
    return text.contains("<svg") && !text.contains("<!entity") && !text.contains("<!doctype");
}

QImage BrandingStore::makeDerivative(const QImage& source, int size, const LogoOptions& options)
{
    QImage destination(size, size, QImage::Format_ARGB32_Premultiplied);
    destination.fill(options.background.rgba());
    QPainter painter(&destination);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QSize scaled = source.size();
    if (options.fitMode == LogoFitMode::Crop) {
        scaled.scale(size, size, Qt::KeepAspectRatioByExpanding);
    } else {
        scaled.scale(size, size, Qt::KeepAspectRatio);
    }
    const QRect target((size - scaled.width()) / 2, (size - scaled.height()) / 2, scaled.width(), scaled.height());
    painter.drawImage(target, source);
    return destination;
}

LogoResult BrandingStore::commit(const QByteArray& source, const QString& format, const LogoOptions& options,
                                 const Cancellation& check)
{
    QImage decoded;
    QString revalidatedFormat;
    LogoResult decodedResult = decode(source, &decoded, &revalidatedFormat);
    if (!decodedResult.ok || revalidatedFormat != format || cancelled(check)) {
        return { false, cancelled(check) ? "Logo update was cancelled" : "Logo data failed revalidation" };
    }
    QDir root(m_profileRoot);
    if (!root.exists() && !QDir().mkpath(m_profileRoot)) {
        return { false, "Could not create the logo cache directory" };
    }
    const QString stagingName = ".branding-staging-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString staging = root.filePath(stagingName);
    if (!QDir().mkpath(staging)) {
        return { false, "Could not prepare the logo cache" };
    }
    auto cleanup = [&] { QDir(staging).removeRecursively(); };
    if (!writeAtomically(QDir(staging).filePath("source.bin"), source)) {
        cleanup(); return { false, "Could not write the logo source" };
    }
    for (int size : DerivativeSizes) {
        if (cancelled(check)) { cleanup(); return { false, "Logo update was cancelled" }; }
        QImage derivativeImage = makeDerivative(decoded, size, options);
        QBuffer output;
        output.open(QIODevice::WriteOnly);
        if (!derivativeImage.save(&output, "PNG")
            || !writeAtomically(QDir(staging).filePath(QString::number(size) + ".png"), output.data())) {
            cleanup(); return { false, "Could not write a logo derivative" };
        }
    }
    QJsonObject metadata { { "version", CacheVersion }, { "format", format },
                           { "fitMode", options.fitMode == LogoFitMode::Crop ? "crop" : "fit" },
                           { "background", options.background.name(QColor::HexArgb) } };
    if (!writeAtomically(QDir(staging).filePath("metadata.json"), QJsonDocument(metadata).toJson(QJsonDocument::Compact))) {
        cleanup(); return { false, "Could not write logo metadata" };
    }
    if (cancelled(check)) { cleanup(); return { false, "Logo update was cancelled" }; }
    const QString active = activePath(m_profileRoot);
    const QString previous = root.filePath(".branding-previous");
    QDir(previous).removeRecursively();
    if (QDir(active).exists() && !root.rename("branding-v1", ".branding-previous")) {
        cleanup(); return { false, "Could not replace the previous logo cache" };
    }
    if (!root.rename(stagingName, "branding-v1")) {
        if (QDir(previous).exists()) { root.rename(".branding-previous", "branding-v1"); }
        cleanup(); return { false, "Could not activate the logo cache" };
    }
    QDir(previous).removeRecursively();
    m_source = source;
    m_format = format;
    m_options = options;
    m_hasCustomLogo = true;
    return { true, {} };
}

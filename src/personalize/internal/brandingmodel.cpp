/* Audacity: A Digital Audio Editor */
#include "brandingmodel.h"

#include <QFile>
#include <QUrl>

using namespace au::personalize;

BrandingModel::BrandingModel(const QString& profileRoot, const QByteArray& shippedMark, QObject* parent)
    : QObject(parent), m_store(profileRoot), m_shippedMark(shippedMark)
{
    refresh();
}
bool BrandingModel::hasCustomLogo() const { return m_store.hasCustomLogo(); }
QString BrandingModel::previewPath() const
{
    return pathForSize(128);
}
QString BrandingModel::logo16Path() const { return pathForSize(16); }
QString BrandingModel::logo32Path() const { return pathForSize(32); }
QString BrandingModel::logo48Path() const { return pathForSize(48); }
QString BrandingModel::logo64Path() const { return pathForSize(64); }
QString BrandingModel::logoPath(int size) const { return pathForSize(size); }
QString BrandingModel::pathForSize(int size) const
{
    if (m_store.hasCustomLogo()) {
        const QString path = m_store.derivative(size).isNull() ? QString() : m_store.derivativePaths().filter(QString::number(size) + ".png").value(0);
        return path.isEmpty() ? QString() : QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded) + "?brandingRevision=" + QString::number(m_revision);
    }
    return m_shippedMark.isEmpty() ? QString() : "data:image/png;base64," + QString::fromLatin1(m_shippedMark.toBase64());
}
bool BrandingModel::crop() const { return m_store.options().fitMode == au::branding::LogoFitMode::Crop; }
QString BrandingModel::background() const { return m_store.options().background.name(QColor::HexArgb); }
QString BrandingModel::statusCode() const { return m_statusCode; }
void BrandingModel::refresh(const au::branding::LogoResult& result)
{
    if (result.ok) {
        ++m_revision;
    }
    m_statusCode = statusCodeFor(result);
    emit changed();
}
QString BrandingModel::statusCodeFor(const au::branding::LogoResult& result) const
{
    if (result.ok) {
        return m_store.hasCustomLogo() ? "custom-saved" : "shipped";
    }
    if (result.error == "unreadable") {
        return "unreadable";
    }
    if (result.error.contains("byte limit")) {
        return "too-large";
    }
    if (result.error.contains("cancelled")) {
        return "cancelled";
    }
    if (result.error.contains("write") || result.error.contains("cache") || result.error.contains("activate") || result.error.contains("prepare")) {
        return "cache-write";
    }
    if (result.error == "invalid background") {
        return "invalid-background";
    }
    return "unsupported-or-invalid";
}
bool BrandingModel::loadFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) { refresh({ false, "unreadable" }); return false; }
    au::branding::LogoOptions options = m_store.options();
    const QByteArray bytes = file.read(au::branding::BrandingStore::MaxSourceBytes + 1);
    const auto result = bytes.size() > au::branding::BrandingStore::MaxSourceBytes
                            ? au::branding::LogoResult { false, "byte limit" }
                            : m_store.loadCustom(bytes, options);
    refresh(result);
    return result.ok;
}
void BrandingModel::reset() { refresh(m_store.reset()); }
void BrandingModel::setCrop(bool value)
{
    if (!m_store.hasCustomLogo()) { refresh({ false, "no custom logo" }); return; }
    auto options = m_store.options(); options.fitMode = value ? au::branding::LogoFitMode::Crop : au::branding::LogoFitMode::Fit;
    refresh(m_store.update(options));
}
void BrandingModel::setBackground(const QString& value)
{
    QColor color(value); if (!color.isValid()) { refresh({ false, "invalid background" }); return; }
    if (!m_store.hasCustomLogo()) { refresh({ false, "no custom logo" }); return; }
    auto options = m_store.options(); options.background = color;
    refresh(m_store.update(options));
}

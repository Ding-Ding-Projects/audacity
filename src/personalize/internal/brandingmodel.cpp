/* Audacity: A Digital Audio Editor */
#include "brandingmodel.h"

#include <QFile>

using namespace au::personalize;

BrandingModel::BrandingModel(const QString& profileRoot, const QByteArray& shippedMark, QObject* parent)
    : QObject(parent), m_store(profileRoot), m_shippedMark(shippedMark)
{
    refresh();
}
bool BrandingModel::hasCustomLogo() const { return m_store.hasCustomLogo(); }
QString BrandingModel::previewPath() const
{
    if (m_store.hasCustomLogo()) {
        return m_store.derivative(128).isNull() ? QString() : m_store.derivativePaths().filter("128.png").value(0);
    }
    return m_shippedMark.isEmpty() ? QString() : "data:image/png;base64," + QString::fromLatin1(m_shippedMark.toBase64());
}
bool BrandingModel::crop() const { return m_store.options().fitMode == au::branding::LogoFitMode::Crop; }
QString BrandingModel::background() const { return m_store.options().background.name(QColor::HexArgb); }
QString BrandingModel::status() const { return m_status; }
void BrandingModel::refresh(const au::branding::LogoResult& result)
{
    m_status = result.ok ? (m_store.hasCustomLogo() ? "Custom logo saved locally" : "Using the shipped logo") : result.error;
    emit changed();
}
bool BrandingModel::loadFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) { refresh({ false, "The selected logo could not be read" }); return false; }
    au::branding::LogoOptions options = m_store.options();
    const auto result = m_store.loadCustom(file.read(au::branding::BrandingStore::MaxSourceBytes + 1), options);
    refresh(result);
    return result.ok;
}
void BrandingModel::reset() { refresh(m_store.reset()); }
void BrandingModel::setCrop(bool value)
{
    auto options = m_store.options(); options.fitMode = value ? au::branding::LogoFitMode::Crop : au::branding::LogoFitMode::Fit;
    refresh(m_store.hasCustomLogo() ? m_store.update(options) : au::branding::LogoResult { true, {} });
}
void BrandingModel::setBackground(const QString& value)
{
    QColor color(value); if (!color.isValid()) { refresh({ false, "Choose a valid background colour" }); return; }
    auto options = m_store.options(); options.background = color;
    refresh(m_store.hasCustomLogo() ? m_store.update(options) : au::branding::LogoResult { true, {} });
}

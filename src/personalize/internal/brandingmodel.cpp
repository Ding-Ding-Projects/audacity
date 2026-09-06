/* Audacity: A Digital Audio Editor */
#include "brandingmodel.h"

#include <QFile>
#include <QCoreApplication>
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
QString BrandingModel::status() const { return m_status; }
void BrandingModel::refresh(const au::branding::LogoResult& result)
{
    if (result.ok) {
        ++m_revision;
    }
    m_status = translatedStatus(result);
    emit changed();
}
QString BrandingModel::translatedStatus(const au::branding::LogoResult& result) const
{
    if (result.ok) {
        return QCoreApplication::translate("personalize/branding", m_store.hasCustomLogo() ? "Custom logo saved locally" : "Using the shipped logo");
    }
    return QCoreApplication::translate("personalize/branding", "The selected logo could not be applied");
}
bool BrandingModel::loadFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) { refresh({ false, "unreadable" }); return false; }
    au::branding::LogoOptions options = m_store.options();
    const auto result = m_store.loadCustom(file.read(au::branding::BrandingStore::MaxSourceBytes + 1), options);
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

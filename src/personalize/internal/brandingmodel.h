/* Audacity: A Digital Audio Editor */
#pragma once

#include <QObject>

#include "brandingstore.h"

namespace au::personalize {
class BrandingModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasCustomLogo READ hasCustomLogo NOTIFY changed)
    Q_PROPERTY(QString previewPath READ previewPath NOTIFY changed)
    Q_PROPERTY(QString logo16Path READ logo16Path NOTIFY changed)
    Q_PROPERTY(QString logo32Path READ logo32Path NOTIFY changed)
    Q_PROPERTY(QString logo48Path READ logo48Path NOTIFY changed)
    Q_PROPERTY(QString logo64Path READ logo64Path NOTIFY changed)
    Q_PROPERTY(bool crop READ crop WRITE setCrop NOTIFY changed)
    Q_PROPERTY(QString background READ background WRITE setBackground NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit BrandingModel(const QString& profileRoot, const QByteArray& shippedMark, QObject* parent = nullptr);
    bool hasCustomLogo() const;
    QString previewPath() const;
    QString logo16Path() const;
    QString logo32Path() const;
    QString logo48Path() const;
    QString logo64Path() const;
    bool crop() const;
    QString background() const;
    QString status() const;
    Q_INVOKABLE bool loadFile(const QString& filePath);
    Q_INVOKABLE void reset();
    Q_INVOKABLE QString logoPath(int size) const;
    void setCrop(bool crop);
    void setBackground(const QString& background);
signals:
    void changed();
private:
    QString pathForSize(int size) const;
    void refresh(const au::branding::LogoResult& result = { true, {} });
    au::branding::BrandingStore m_store;
    QByteArray m_shippedMark;
    QString m_status;
};
}

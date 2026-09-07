/*
 * Audacity: A Digital Audio Editor
 * Presentation-only custom application mark storage.
 */
#pragma once

#include <QColor>
#include <QImage>
#include <QString>
#include <QStringList>

#include <functional>
#include <utility>

namespace au::branding {

enum class LogoFitMode { Fit, Crop };

struct LogoOptions {
    LogoFitMode fitMode { LogoFitMode::Fit };
    QColor background { Qt::transparent };
};

struct LogoResult {
    bool ok { false };
    QString error;
};

/**
 * Stores only presentation assets below an explicitly supplied profile root.
 * It never changes executable, installer, updater, or packaged application identity.
 */
class BrandingStore final
{
public:
    using Cancellation = std::function<bool()>;

    explicit BrandingStore(QString profileRoot);

    static constexpr qint64 MaxSourceBytes = 8 * 1024 * 1024;
    static constexpr int MaxDimension = 4096;

    LogoResult loadCustom(const QByteArray& bytes, const LogoOptions& options = {}, const Cancellation& cancelled = {});
    LogoResult update(const LogoOptions& options, const Cancellation& cancelled = {});
    LogoResult reset();

    bool hasCustomLogo() const;
    QStringList derivativePaths() const;
    QImage derivative(int size) const;
    LogoOptions options() const;
    QString profileRoot() const;

    // The owning UI supplies the shipped mark as bytes. No source path is persisted.
    static QByteArray presetDefaultMark(const QByteArray& shippedMark);

private:
    LogoResult commit(const QByteArray& source, const QString& format, const LogoOptions& options,
                      const Cancellation& cancelled);
    static LogoResult decode(const QByteArray& bytes, QImage* image, QString* format);
    static bool svgIsSafe(const QByteArray& bytes);
    static QImage makeDerivative(const QImage& source, int size, const LogoOptions& options);

    QString m_profileRoot;
    QByteArray m_source;
    QString m_format;
    LogoOptions m_options;
    bool m_hasCustomLogo { false };
};

} // namespace au::branding

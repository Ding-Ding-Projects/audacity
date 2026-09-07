#pragma once
#include <QObject>
#include <QFutureWatcher>
#include <atomic>
#include "conversionengine.h"
namespace au::converter {
class ConverterPresentationModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    bool busy() const { return m_busy; } int progress() const { return m_progress; } QString status() const { return m_status; }
    Q_INVOKABLE void probe();
    Q_INVOKABLE void convert(const QString& input, const QString& output, const QString& format);
    Q_INVOKABLE void cancel();
signals: void changed(); void finished(bool success, const QString& message);
private:
    void start(const ConversionRequest& request, bool probeOnly);
    std::atomic_bool m_cancel = false; bool m_busy = false; int m_progress = 0; QString m_status;
};
}

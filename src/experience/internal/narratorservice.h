/* Audacity: A Digital Audio Editor */
#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include <memory>
#include "narratorqueue.h"

class QTimer;
namespace au::experience {
enum class NarratorEngineKind { None = 0, QtTextToSpeech = 1, ProcessBackend = 2, WindowsSapi = 3 };
struct NarratorVoice {
    QString id;
    QString displayName;
    NarratorLanguage language = NarratorLanguage::English;
};
// Nonblocking backend contract: start and poll never wait for speech completion.
// Each backend owns cancellation and releases its resources before destruction.
class NarratorBackend {
public:
    enum class State { Speaking, Finished, Failed };
    virtual ~NarratorBackend() = default;
    virtual NarratorEngineKind kind() const = 0;
    virtual QVector<NarratorVoice> voices() const = 0;
    virtual bool start(const QString& text, const QString& voiceId, double rate, double pitch) = 0;
    virtual State poll() = 0;
    virtual void cancel() = 0;
};
std::unique_ptr<NarratorBackend> makeNativeNarratorBackend();

class NarratorEngine : public QObject {
    Q_OBJECT
public:
    explicit NarratorEngine(QObject* parent = nullptr);
    // Injection exercises the production lifecycle without producing audible speech.
    NarratorEngine(std::unique_ptr<NarratorBackend> backend, int timeoutMs, QObject* parent = nullptr);
    ~NarratorEngine() override;
    NarratorEngineKind engineKind() const;
    QString engineDescription() const;
    QString lastStatus() const { return m_status; }
    QVector<NarratorVoice> voicesFor(NarratorLanguage language) const;
    void speak(const QString& text, NarratorLanguage language, const QString& voiceId, double rate, double pitch, bool quietMode);
    void stop();
    static bool screenReaderActive();
signals:
    void speechFinished();
private:
    void finishOnce(quint64 generation, const QString& status, bool cancel);
    std::unique_ptr<NarratorBackend> m_backend;
    QTimer* m_poll = nullptr;
    QTimer* m_timeout = nullptr;
    quint64 m_generation = 0;
    bool m_active = false;
    bool m_completing = false;
    QString m_status;
    int m_timeoutMs;
};
}

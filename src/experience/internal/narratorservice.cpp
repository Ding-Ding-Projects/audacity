/* Audacity: A Digital Audio Editor */
#include "narratorservice.h"
#include <algorithm>
#include <cmath>
#include <QAccessible>
#include <QTimer>
using namespace au::experience;
NarratorEngine::NarratorEngine(QObject* parent)
    : NarratorEngine(makeNativeNarratorBackend(), 10000, parent) {}
NarratorEngine::NarratorEngine(std::unique_ptr<NarratorBackend> backend, int timeoutMs, QObject* parent)
    : QObject(parent), m_backend(std::move(backend)), m_timeoutMs(qBound(1, timeoutMs, 60000))
{
    m_poll = new QTimer(this);
    m_poll->setInterval(20);
    m_timeout = new QTimer(this);
    m_timeout->setSingleShot(true);
}
NarratorEngine::~NarratorEngine()
{
    m_poll->stop();
    m_timeout->stop();
    if (m_backend) m_backend->cancel();
}
NarratorEngineKind NarratorEngine::engineKind() const
{ return m_backend ? m_backend->kind() : NarratorEngineKind::None; }
QString NarratorEngine::engineDescription() const
{
    if (!m_status.isEmpty()) return m_status;
    return engineKind() == NarratorEngineKind::WindowsSapi
        ? tr("Using Windows SAPI. Local English voices: %1; local Cantonese voices: %2. A missing language stays silent.")
              .arg(voicesFor(NarratorLanguage::English).size()).arg(voicesFor(NarratorLanguage::Cantonese).size())
        : tr("No supported speech engine is available; narration stays silent.");
}
QVector<NarratorVoice> NarratorEngine::voicesFor(NarratorLanguage language) const
{
    QVector<NarratorVoice> result;
    if (m_backend) for (const auto& voice : m_backend->voices()) {
        if (voice.language == language) result.push_back(voice);
    }
    return result;
}
bool NarratorEngine::screenReaderActive() { return QAccessible::isActive(); }
void NarratorEngine::finishOnce(quint64 generation, const QString& status, bool cancel)
{
    if (!m_active || m_completing || generation != m_generation) return;
    m_completing = true;
    m_poll->stop();
    m_timeout->stop();
    if (cancel && m_backend) m_backend->cancel();
    m_status = status;
    // Deliver asynchronously. Consumers can start the next line without recursion,
    // and an old completion can never complete a newer generation.
    QTimer::singleShot(0, this, [this, generation] {
        if (generation != m_generation) return;
        m_active = false;
        m_completing = false;
        emit speechFinished();
    });
}
void NarratorEngine::speak(const QString& text, NarratorLanguage language, const QString& voiceId,
                           double rate, double pitch, bool quietMode)
{
    if (m_active) return; // The consumer owns the serialized queue.
    const quint64 generation = ++m_generation;
    m_active = true;
    const auto finish = [this, generation](const QString& status) { finishOnce(generation, status, false); };
    if (quietMode || screenReaderActive()) { finish(tr("Narration is muted for quiet mode or assistive technology.")); return; }
    if (!m_backend || text.isEmpty() || language == NarratorLanguage::Both) {
        finish(tr("No supported speech engine or language-specific text is available.")); return;
    }
    const auto voices = voicesFor(language);
    if (voices.isEmpty()) {
        finish(language == NarratorLanguage::Cantonese ? tr("No local Cantonese voice is installed; this line stays silent.")
                                                     : tr("No local English voice is installed; this line stays silent.")); return;
    }
    QString selected = voices.front().id;
    bool found = voiceId.isEmpty();
    for (const auto& voice : voices) if (voice.id == voiceId) { selected = voice.id; found = true; break; }
    m_status = found ? QString() : tr("The selected voice is unavailable; using an installed voice of the same language.");
    rate = std::isfinite(rate) ? std::clamp(rate, -1.0, 1.0) : 0.0;
    pitch = std::isfinite(pitch) ? std::clamp(pitch, -1.0, 1.0) : 0.0;
    QString boundedText = text.left(1000);
    if (!boundedText.isEmpty() && boundedText.back().isHighSurrogate()) boundedText.chop(1);
    if (!m_backend->start(boundedText, selected, rate, pitch)) {
        finishOnce(generation, tr("The speech engine could not start this line."), true); return;
    }
    disconnect(m_poll, nullptr, this, nullptr);
    disconnect(m_timeout, nullptr, this, nullptr);
    connect(m_poll, &QTimer::timeout, this, [this, generation] {
        if (!m_active || generation != m_generation) return;
        const auto state = m_backend->poll();
        if (state != NarratorBackend::State::Speaking)
            finishOnce(generation, state == NarratorBackend::State::Failed ? tr("The speech engine stopped unexpectedly.") : m_status, true);
    });
    connect(m_timeout, &QTimer::timeout, this, [this, generation] {
        finishOnce(generation, tr("Narration timed out and was stopped."), true);
    });
    m_poll->start();
    m_timeout->start(m_timeoutMs);
}
void NarratorEngine::stop() { finishOnce(m_generation, tr("Narration was cancelled."), true); }

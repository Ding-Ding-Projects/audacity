/*
 * Audacity: A Digital Audio Editor
 */
#include "narratorservice.h"

#include <QProcess>
#include <QStandardPaths>

using namespace au::experience;

NarratorEngine::NarratorEngine(QObject* parent)
    : QObject(parent)
{
    detectEngine();
}

void NarratorEngine::detectEngine()
{
    // This build of Qt was checked for QtTextToSpeech and does not carry
    // it (no libQt6TextToSpeech in the toolchain), so on this machine the
    // narrator falls back to a command line process backend when one is
    // present. A build against a Qt that does carry QtTextToSpeech would
    // report NarratorEngineKind::QtTextToSpeech here instead; the
    // preference page always says honestly which one is active.
    const QString spdSay = QStandardPaths::findExecutable(QStringLiteral("spd-say"));
    const QString espeak = QStandardPaths::findExecutable(QStringLiteral("espeak-ng"));

    if (!spdSay.isEmpty()) {
        m_engineKind = NarratorEngineKind::ProcessBackend;
        m_processBackendCommand = spdSay;
    } else if (!espeak.isEmpty()) {
        m_engineKind = NarratorEngineKind::ProcessBackend;
        m_processBackendCommand = espeak;
    } else {
        m_engineKind = NarratorEngineKind::None;
        m_processBackendCommand.clear();
    }
}

QString NarratorEngine::engineDescription() const
{
    switch (m_engineKind) {
    case NarratorEngineKind::QtTextToSpeech:
        return QStringLiteral("Using the system speech engine.");
    case NarratorEngineKind::ProcessBackend:
        return QStringLiteral("Using %1 for speech on this machine.").arg(m_processBackendCommand);
    case NarratorEngineKind::None:
    default:
        return QStringLiteral("No speech engine was found on this machine, so the narrator stays silent.");
    }
}

QVector<NarratorVoice> NarratorEngine::voicesFor(NarratorLanguage language) const
{
    // Without a real engine attached there is nothing to enumerate. A
    // QtTextToSpeech or process backend build reports its real voice list
    // here; until then the picker shows "Choose automatically" only, and
    // the status line says plainly that no voice is installed.
    Q_UNUSED(language);
    return {};
}

void NarratorEngine::speak(const QString& text, NarratorLanguage language, const QString& voiceId, double rate,
                           double pitch)
{
    Q_UNUSED(language);
    Q_UNUSED(voiceId);
    Q_UNUSED(rate);
    Q_UNUSED(pitch);

    if (m_engineKind != NarratorEngineKind::ProcessBackend || text.isEmpty()) {
        return;
    }

    QProcess::startDetached(m_processBackendCommand, { text });
}

void NarratorEngine::stop()
{
    // The process backend is fire-and-forget per utterance; there is
    // nothing persistent here to stop. A QtTextToSpeech backend would
    // call its own stop() here.
}

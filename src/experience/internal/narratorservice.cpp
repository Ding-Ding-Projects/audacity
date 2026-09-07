/*
 * Audacity: A Digital Audio Editor
 */
#include "narratorservice.h"

#include <algorithm>

#include <QAccessible>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

using namespace au::experience;

NarratorEngine::NarratorEngine(QObject* parent)
    : QObject(parent)
{
    detectEngine();
    m_process = new QProcess(this);
    m_process->setReadBufferSize(4096);
    m_timeout = new QTimer(this);
    m_timeout->setSingleShot(true);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int, QProcess::ExitStatus) {
                m_timeout->stop();
                m_process->readAllStandardOutput();
                m_process->readAllStandardError();
                emit speechFinished();
            });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (m_process->state() == QProcess::NotRunning) {
            emit speechFinished();
        }
    });
    connect(m_timeout, &QTimer::timeout, this, &NarratorEngine::finishTimedOutProcess);
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

bool NarratorEngine::screenReaderActive()
{
    return QAccessible::isActive();
}

void NarratorEngine::speak(const QString& text, NarratorLanguage language, const QString& voiceId, double rate,
                           double pitch, bool quietMode)
{
    if (quietMode || screenReaderActive()) {
        // Quiet mode is the narrator's own reduced-sound setting: honour it
        // by staying silent. A screen reader already reading the interface
        // gets ducked under rather than talked over.
        emit speechFinished();
        return;
    }

    if (m_engineKind != NarratorEngineKind::ProcessBackend || text.isEmpty() || language == NarratorLanguage::Both) {
        emit speechFinished();
        return;
    }

    if (m_process->state() != QProcess::NotRunning) {
        return;
    }
    constexpr int MAX_TEXT_CODE_UNITS = 1000;
    const QString boundedText = text.left(MAX_TEXT_CODE_UNITS);
    const double boundedRate = std::clamp(rate, -1.0, 1.0);
    const double boundedPitch = std::clamp(pitch, -1.0, 1.0);
    QStringList arguments;
    if (m_processBackendCommand.endsWith(QStringLiteral("spd-say"))) {
        if (!voiceId.isEmpty()) {
            arguments << QStringLiteral("-y") << voiceId;
        }
        arguments << QStringLiteral("-r") << QString::number(qRound(boundedRate * 100.0));
    } else {
        if (!voiceId.isEmpty()) {
            arguments << QStringLiteral("-v") << voiceId;
        }
        arguments << QStringLiteral("-s") << QString::number(qBound(80, qRound(175.0 + boundedRate * 100.0), 450));
        arguments << QStringLiteral("-p") << QString::number(qBound(0, qRound(50.0 + boundedPitch * 49.0), 99));
    }
    arguments << boundedText;
    m_process->start(m_processBackendCommand, arguments);
    m_timeout->start(10000);
}

void NarratorEngine::finishTimedOutProcess()
{
    if (!m_process || m_process->state() == QProcess::NotRunning) {
        return;
    }
    m_process->terminate();
    QTimer::singleShot(250, this, [this]() {
        if (m_process && m_process->state() != QProcess::NotRunning) {
            m_process->kill();
        }
    });
}

void NarratorEngine::stop()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_timeout->stop();
        m_process->kill();
    }
}

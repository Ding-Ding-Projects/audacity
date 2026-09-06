/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "narratorqueue.h"

namespace au::experience {
//! Which speech backend the narrator is actually using on this machine.
enum class NarratorEngineKind {
    None = 0,
    QtTextToSpeech = 1,
    ProcessBackend = 2
};

//! One voice reported by whichever engine is active.
struct NarratorVoice
{
    //! A stable identifier persisted in settings. Never a display name,
    //! since two installed voices can share one.
    QString id;
    QString displayName;
    NarratorLanguage language = NarratorLanguage::English;
};

//! Speaks one line through whichever engine is available on this machine.
//! Detection is honest: if Qt was built without QtTextToSpeech, and no
//! command line speech backend (speech-dispatcher's spd-say, or
//! espeak-ng) is on the path, engineKind() reports None and speak() is a
//! safely ignored no-op that still drains the queue so callers do not
//! stall on it.
class NarratorEngine : public QObject
{
    Q_OBJECT

public:
    explicit NarratorEngine(QObject* parent = nullptr);

    NarratorEngineKind engineKind() const { return m_engineKind; }
    //! A short, already appropriate for display, description of the
    //! active engine (or the lack of one), for the preferences page.
    QString engineDescription() const;

    //! Voices for one language, as reported by the active engine. Empty
    //! when no engine is available or the engine has no voice for that
    //! language.
    QVector<NarratorVoice> voicesFor(NarratorLanguage language) const;

    //! Speaks one line, at the given rate ([-1, 1] engine convention where
    //! supported) and pitch. voiceId may be empty for "choose automatically".
    void speak(const QString& text, NarratorLanguage language, const QString& voiceId, double rate, double pitch);

    void stop();

private:
    void detectEngine();

    NarratorEngineKind m_engineKind = NarratorEngineKind::None;
    QString m_processBackendCommand;
};
}

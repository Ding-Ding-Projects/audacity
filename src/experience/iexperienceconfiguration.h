/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <vector>

#include "framework/global/modularity/imoduleinterface.h"
#include "framework/global/async/channel.h"
#include "framework/global/async/notification.h"

#include "experiencetypes.h"

namespace au::experience {
//! Every companion setting lives here and is persisted through muse settings.
class IExperienceConfiguration : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(au::experience::IExperienceConfiguration)

public:
    virtual ~IExperienceConfiguration() = default;

    // Language
    virtual LanguageMode languageMode() const = 0;
    virtual void setLanguageMode(LanguageMode mode) = 0;
    virtual muse::async::Channel<LanguageMode> languageModeChanged() const = 0;

    // Funny levels, 1 to 5, tone only
    virtual int englishFunnyLevel() const = 0;
    virtual void setEnglishFunnyLevel(int level) = 0;
    virtual int cantoneseFunnyLevel() const = 0;
    virtual void setCantoneseFunnyLevel(int level) = 0;
    virtual muse::async::Notification funnyLevelChanged() const = 0;

    // Emoji
    virtual bool emojiInDialogs() const = 0;
    virtual void setEmojiInDialogs(bool value) = 0;
    virtual muse::async::Channel<bool> emojiInDialogsChanged() const = 0;

    // Attention support modes
    virtual bool focusMode() const = 0;
    virtual void setFocusMode(bool value) = 0;
    virtual bool lowStimulationMode() const = 0;
    virtual void setLowStimulationMode(bool value) = 0;
    virtual bool timeAwarenessMode() const = 0;
    virtual void setTimeAwarenessMode(bool value) = 0;
    virtual bool oneThingAtATimeMode() const = 0;
    virtual void setOneThingAtATimeMode(bool value) = 0;
    virtual bool momentumMode() const = 0;
    virtual void setMomentumMode(bool value) = 0;
    virtual muse::async::Notification attentionModesChanged() const = 0;

    // Scheduled settings
    virtual std::vector<ScheduleEntry> schedule() const = 0;
    virtual void setSchedule(const std::vector<ScheduleEntry>& entries) = 0;
    virtual muse::async::Notification scheduleChanged() const = 0;

    //! The long lived access token used to read a Home Assistant boolean
    //! entity for a scheduled row. Kept in this module's own settings
    //! namespace, never logged, and excluded from every export and from
    //! local history. There is one token for the whole schedule table; every
    //! Home Assistant row is expected to talk to the same instance.
    virtual QString homeAssistantToken() const = 0;
    virtual void setHomeAssistantToken(const QString& token) = 0;

    // Personal vocabulary
    virtual QString vocabularyFileName() const = 0;
    virtual void setVocabularyFileName(const QString& name) = 0;
    //! Absolute path of the parsed vocabulary table inside the application data directory.
    virtual QString vocabularyStoragePath() const = 0;
    virtual muse::async::Notification vocabularyChanged() const = 0;

    // Narrator, off by default
    virtual bool narratorEnabled() const = 0;
    virtual void setNarratorEnabled(bool value) = 0;
    //! 0 = English, 1 = Cantonese, 2 = Both. Kept as int at this boundary so
    //! the header does not need to include narratorqueue.h.
    virtual int narratorLanguage() const = 0;
    virtual void setNarratorLanguage(int value) = 0;
    //! Stable engine reported voice id, or empty for "choose automatically".
    virtual QString narratorEnglishVoiceId() const = 0;
    virtual void setNarratorEnglishVoiceId(const QString& id) = 0;
    virtual QString narratorCantoneseVoiceId() const = 0;
    virtual void setNarratorCantoneseVoiceId(const QString& id) = 0;
    virtual double narratorRate() const = 0;
    virtual void setNarratorRate(double value) = 0;
    virtual double narratorPitch() const = 0;
    virtual void setNarratorPitch(double value) = 0;
    virtual muse::async::Notification narratorSettingsChanged() const = 0;
};
}

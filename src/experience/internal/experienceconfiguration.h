/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"
#include "framework/global/iglobalconfiguration.h"

#include "iexperienceconfiguration.h"

namespace au::experience {
class ExperienceConfiguration : public IExperienceConfiguration, public muse::async::Asyncable
{
    muse::GlobalInject<muse::IGlobalConfiguration> globalConfiguration;

public:
    ~ExperienceConfiguration() override = default;

    void init();

    LanguageMode languageMode() const override;
    void setLanguageMode(LanguageMode mode) override;
    muse::async::Channel<LanguageMode> languageModeChanged() const override;

    int englishFunnyLevel() const override;
    void setEnglishFunnyLevel(int level) override;
    int cantoneseFunnyLevel() const override;
    void setCantoneseFunnyLevel(int level) override;
    muse::async::Notification funnyLevelChanged() const override;

    bool emojiInDialogs() const override;
    void setEmojiInDialogs(bool value) override;
    muse::async::Channel<bool> emojiInDialogsChanged() const override;

    bool focusMode() const override;
    void setFocusMode(bool value) override;
    bool lowStimulationMode() const override;
    void setLowStimulationMode(bool value) override;
    bool timeAwarenessMode() const override;
    void setTimeAwarenessMode(bool value) override;
    bool oneThingAtATimeMode() const override;
    void setOneThingAtATimeMode(bool value) override;
    bool momentumMode() const override;
    void setMomentumMode(bool value) override;
    muse::async::Notification attentionModesChanged() const override;

    std::vector<ScheduleEntry> schedule() const override;
    void setSchedule(const std::vector<ScheduleEntry>& entries) override;
    muse::async::Notification scheduleChanged() const override;

    QString homeAssistantToken() const override;
    void setHomeAssistantToken(const QString& token) override;

    QString vocabularyFileName() const override;
    void setVocabularyFileName(const QString& name) override;
    QString vocabularyStoragePath() const override;
    muse::async::Notification vocabularyChanged() const override;

private:
    muse::async::Channel<LanguageMode> m_languageModeChanged;
    muse::async::Notification m_funnyLevelChanged;
    muse::async::Channel<bool> m_emojiInDialogsChanged;
    muse::async::Notification m_attentionModesChanged;
    muse::async::Notification m_scheduleChanged;
    muse::async::Notification m_vocabularyChanged;
};
}

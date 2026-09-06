/*
 * Audacity: A Digital Audio Editor
 */
#include "experiencesettingsmodel.h"

#include <QQmlEngine>

#include "framework/global/translation.h"

namespace au::experience {
ExperienceSettingsModel::ExperienceSettingsModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void ExperienceSettingsModel::load()
{
    configuration()->languageModeChanged().onReceive(this, [this](LanguageMode) {
        emit languageModeChanged();
    });
    configuration()->funnyLevelChanged().onNotify(this, [this]() {
        emit funnyLevelsChanged();
    });
    configuration()->emojiInDialogsChanged().onReceive(this, [this](bool) {
        emit emojiInDialogsChanged();
    });
    configuration()->attentionModesChanged().onNotify(this, [this]() {
        emit attentionModesChanged();
    });
    configuration()->vocabularyChanged().onNotify(this, [this]() {
        emit vocabularyChanged();
    });
    service()->restartRequiredChanged().onNotify(this, [this]() {
        emit restartRequiredChanged();
    });
    configuration()->narratorSettingsChanged().onNotify(this, [this]() {
        emit narratorSettingsChanged();
    });

    if (!m_schoolMode) {
        m_schoolMode = new SchoolModeService(this);
        connect(m_schoolMode, &SchoolModeService::stateChanged, this, &ExperienceSettingsModel::schoolModeChanged);
    }
    if (!m_narratorEngine) {
        m_narratorEngine = new NarratorEngine(this);
    }
}

int ExperienceSettingsModel::languageMode() const
{
    return static_cast<int>(configuration()->languageMode());
}

void ExperienceSettingsModel::setLanguageMode(int mode)
{
    if (mode < 0 || mode > static_cast<int>(LanguageMode::Bilingual) || mode == languageMode()) {
        return;
    }
    configuration()->setLanguageMode(static_cast<LanguageMode>(mode));
    retranslate();
}

int ExperienceSettingsModel::englishFunnyLevel() const
{
    return configuration()->englishFunnyLevel();
}

void ExperienceSettingsModel::setEnglishFunnyLevel(int level)
{
    if (level == englishFunnyLevel()) {
        return;
    }
    configuration()->setEnglishFunnyLevel(level);
}

int ExperienceSettingsModel::cantoneseFunnyLevel() const
{
    return configuration()->cantoneseFunnyLevel();
}

void ExperienceSettingsModel::setCantoneseFunnyLevel(int level)
{
    if (level == cantoneseFunnyLevel()) {
        return;
    }
    configuration()->setCantoneseFunnyLevel(level);
}

bool ExperienceSettingsModel::emojiInDialogs() const
{
    return configuration()->emojiInDialogs();
}

void ExperienceSettingsModel::setEmojiInDialogs(bool value)
{
    if (value == emojiInDialogs()) {
        return;
    }
    configuration()->setEmojiInDialogs(value);
}

bool ExperienceSettingsModel::focusMode() const
{
    return configuration()->focusMode();
}

void ExperienceSettingsModel::setFocusMode(bool value)
{
    configuration()->setFocusMode(value);
}

bool ExperienceSettingsModel::lowStimulationMode() const
{
    return configuration()->lowStimulationMode();
}

void ExperienceSettingsModel::setLowStimulationMode(bool value)
{
    configuration()->setLowStimulationMode(value);
}

bool ExperienceSettingsModel::timeAwarenessMode() const
{
    return configuration()->timeAwarenessMode();
}

void ExperienceSettingsModel::setTimeAwarenessMode(bool value)
{
    configuration()->setTimeAwarenessMode(value);
}

bool ExperienceSettingsModel::oneThingAtATimeMode() const
{
    return configuration()->oneThingAtATimeMode();
}

void ExperienceSettingsModel::setOneThingAtATimeMode(bool value)
{
    configuration()->setOneThingAtATimeMode(value);
}

bool ExperienceSettingsModel::momentumMode() const
{
    return configuration()->momentumMode();
}

void ExperienceSettingsModel::setMomentumMode(bool value)
{
    configuration()->setMomentumMode(value);
}

bool ExperienceSettingsModel::restartRequired() const
{
    return service()->restartRequired();
}

bool ExperienceSettingsModel::cantoneseCatalogueAvailable() const
{
    return service()->cantoneseCatalogueAvailable();
}

QString ExperienceSettingsModel::vocabularyFileName() const
{
    return configuration()->vocabularyFileName();
}

int ExperienceSettingsModel::vocabularyEntryCount() const
{
    return service()->vocabularyEntryCount();
}

QString ExperienceSettingsModel::vocabularyError() const
{
    return m_vocabularyError;
}

bool ExperienceSettingsModel::homeAssistantTokenSet() const
{
    return !configuration()->homeAssistantToken().isEmpty();
}

void ExperienceSettingsModel::setHomeAssistantToken(const QString& token)
{
    configuration()->setHomeAssistantToken(token);
    emit homeAssistantTokenChanged();
}

QString ExperienceSettingsModel::previewMessage(int kind, const QString& plainText) const
{
    return styler()->style(static_cast<MessageKind>(kind), plainText);
}

void ExperienceSettingsModel::chooseVocabularyFile()
{
    const std::vector<std::string> filter { muse::trc("experience", "JSON files") + " (*.json)" };
    const muse::io::path_t path = interactive()->selectOpeningFileSync(
        muse::trc("experience", "Choose a personal vocabulary file"), muse::io::path_t(), filter);

    if (path.empty()) {
        return;
    }

    const VocabularyLoadResult result = service()->loadVocabularyFile(path.toQString());
    m_vocabularyError = result.ok ? QString() : result.error;
    emit vocabularyChanged();

    if (result.ok) {
        notificationCenter()->push(NotificationType::Success,
                                   muse::qtrc("experience", "Personal vocabulary loaded"),
                                   muse::qtrc("experience", "%n term(s) will be used in the interface.", nullptr,
                                              result.entryCount));
        retranslate();
    }
}

void ExperienceSettingsModel::clearVocabulary()
{
    service()->clearVocabulary();
    m_vocabularyError.clear();
    emit vocabularyChanged();
    retranslate();
}

void ExperienceSettingsModel::showExampleNotification()
{
    notificationCenter()->push(NotificationType::Info,
                               muse::qtrc("experience", "Example notification"),
                               muse::qtrc("experience", "This is how a notification reads at your current settings."));
}

void ExperienceSettingsModel::retranslate()
{
    //! Qt can look the strings up again straight away, which covers every
    //! binding written with qsTrc. Text that was copied into a property once,
    //! and text owned by the platform, still needs a restart.
    if (uiEngine() && uiEngine()->qmlEngine()) {
        uiEngine()->qmlEngine()->retranslate();
    }
}

bool ExperienceSettingsModel::narratorEnabled() const
{
    return configuration()->narratorEnabled();
}

void ExperienceSettingsModel::setNarratorEnabled(bool value)
{
    configuration()->setNarratorEnabled(value);
}

int ExperienceSettingsModel::narratorLanguage() const
{
    return configuration()->narratorLanguage();
}

void ExperienceSettingsModel::setNarratorLanguage(int value)
{
    configuration()->setNarratorLanguage(value);
}

QString ExperienceSettingsModel::narratorEnglishVoiceId() const
{
    return configuration()->narratorEnglishVoiceId();
}

void ExperienceSettingsModel::setNarratorEnglishVoiceId(const QString& id)
{
    configuration()->setNarratorEnglishVoiceId(id);
}

QString ExperienceSettingsModel::narratorCantoneseVoiceId() const
{
    return configuration()->narratorCantoneseVoiceId();
}

void ExperienceSettingsModel::setNarratorCantoneseVoiceId(const QString& id)
{
    configuration()->setNarratorCantoneseVoiceId(id);
}

double ExperienceSettingsModel::narratorRate() const
{
    return configuration()->narratorRate();
}

void ExperienceSettingsModel::setNarratorRate(double value)
{
    configuration()->setNarratorRate(value);
}

double ExperienceSettingsModel::narratorPitch() const
{
    return configuration()->narratorPitch();
}

void ExperienceSettingsModel::setNarratorPitch(double value)
{
    configuration()->setNarratorPitch(value);
}

QString ExperienceSettingsModel::narratorEngineDescription() const
{
    return m_narratorEngine ? m_narratorEngine->engineDescription() : QString();
}

bool ExperienceSettingsModel::schoolModeOn() const
{
    return m_schoolMode && m_schoolMode->isOn();
}

QString ExperienceSettingsModel::schoolModeDisplayName() const
{
    return m_schoolMode ? m_schoolMode->displayName() : QStringLiteral("School mode");
}

bool ExperienceSettingsModel::schoolModeHasCredential() const
{
    return m_schoolMode && m_schoolMode->hasCredential();
}

bool ExperienceSettingsModel::turnSchoolModeOn(const QString& newCredential)
{
    return m_schoolMode && m_schoolMode->turnOn(newCredential);
}

bool ExperienceSettingsModel::turnSchoolModeOff(const QString& credential)
{
    return m_schoolMode && m_schoolMode->turnOff(credential);
}

void ExperienceSettingsModel::renameSchoolMode(const QString& newDisplayName)
{
    if (m_schoolMode) {
        m_schoolMode->rename(newDisplayName);
    }
}
}

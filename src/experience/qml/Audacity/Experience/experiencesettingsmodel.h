/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"
#include "framework/interactive/iinteractive.h"
#include "framework/ui/iuiengine.h"

#include "iexperienceconfiguration.h"
#include "iexperienceservice.h"
#include "imessagestyler.h"
#include "inotificationcenter.h"
#include "internal/narratorservice.h"
#include "internal/schoolmode.h"

namespace au::experience {
//! The model behind the Experience preferences page and the companion
//! overlays. Everything it exposes is persisted through muse settings.
class ExperienceSettingsModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int languageMode READ languageMode WRITE setLanguageMode NOTIFY languageModeChanged FINAL)
    Q_PROPERTY(int englishFunnyLevel READ englishFunnyLevel WRITE setEnglishFunnyLevel NOTIFY funnyLevelsChanged FINAL)
    Q_PROPERTY(int cantoneseFunnyLevel READ cantoneseFunnyLevel WRITE setCantoneseFunnyLevel NOTIFY funnyLevelsChanged FINAL)
    Q_PROPERTY(bool emojiInDialogs READ emojiInDialogs WRITE setEmojiInDialogs NOTIFY emojiInDialogsChanged FINAL)

    Q_PROPERTY(bool focusMode READ focusMode WRITE setFocusMode NOTIFY attentionModesChanged FINAL)
    Q_PROPERTY(bool lowStimulationMode READ lowStimulationMode WRITE setLowStimulationMode NOTIFY attentionModesChanged FINAL)
    Q_PROPERTY(bool timeAwarenessMode READ timeAwarenessMode WRITE setTimeAwarenessMode NOTIFY attentionModesChanged FINAL)
    Q_PROPERTY(bool oneThingAtATimeMode READ oneThingAtATimeMode WRITE setOneThingAtATimeMode NOTIFY attentionModesChanged FINAL)
    Q_PROPERTY(bool momentumMode READ momentumMode WRITE setMomentumMode NOTIFY attentionModesChanged FINAL)

    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY restartRequiredChanged FINAL)
    Q_PROPERTY(bool cantoneseCatalogueAvailable READ cantoneseCatalogueAvailable CONSTANT FINAL)

    Q_PROPERTY(QString vocabularyFileName READ vocabularyFileName NOTIFY vocabularyChanged FINAL)
    Q_PROPERTY(int vocabularyEntryCount READ vocabularyEntryCount NOTIFY vocabularyChanged FINAL)
    Q_PROPERTY(QString vocabularyError READ vocabularyError NOTIFY vocabularyChanged FINAL)

    //! Write only. The stored token is never read back into the interface,
    //! the same way a stored password never is; this always reports empty.
    Q_PROPERTY(bool homeAssistantTokenSet READ homeAssistantTokenSet NOTIFY homeAssistantTokenChanged FINAL)

    Q_PROPERTY(bool narratorEnabled READ narratorEnabled WRITE setNarratorEnabled NOTIFY narratorSettingsChanged FINAL)
    Q_PROPERTY(int narratorLanguage READ narratorLanguage WRITE setNarratorLanguage NOTIFY narratorSettingsChanged FINAL)
    Q_PROPERTY(
        QString narratorEnglishVoiceId READ narratorEnglishVoiceId WRITE setNarratorEnglishVoiceId NOTIFY narratorSettingsChanged FINAL)
    Q_PROPERTY(
        QString narratorCantoneseVoiceId READ narratorCantoneseVoiceId WRITE setNarratorCantoneseVoiceId NOTIFY narratorSettingsChanged FINAL)
    Q_PROPERTY(double narratorRate READ narratorRate WRITE setNarratorRate NOTIFY narratorSettingsChanged FINAL)
    Q_PROPERTY(double narratorPitch READ narratorPitch WRITE setNarratorPitch NOTIFY narratorSettingsChanged FINAL)
    //! Reduced-sound setting: while on, the narrator stays completely
    //! silent even when otherwise enabled.
    Q_PROPERTY(bool narratorQuietMode READ narratorQuietMode WRITE setNarratorQuietMode NOTIFY narratorSettingsChanged FINAL)
    Q_PROPERTY(QString narratorEngineDescription READ narratorEngineDescription CONSTANT FINAL)

    Q_PROPERTY(bool schoolModeOn READ schoolModeOn NOTIFY schoolModeChanged FINAL)
    Q_PROPERTY(bool schoolModeAvailable READ schoolModeAvailable NOTIFY schoolModeChanged FINAL)
    Q_PROPERTY(QString schoolModeError READ schoolModeError NOTIFY schoolModeChanged FINAL)
    Q_PROPERTY(QString schoolModeDisplayName READ schoolModeDisplayName NOTIFY schoolModeChanged FINAL)
    Q_PROPERTY(bool schoolModeHasCredential READ schoolModeHasCredential NOTIFY schoolModeChanged FINAL)

    muse::GlobalInject<IExperienceConfiguration> configuration;
    muse::GlobalInject<IExperienceService> service;
    muse::GlobalInject<IMessageStyler> styler;
    muse::GlobalInject<INotificationCenter> notificationCenter;
    muse::ContextInject<muse::IInteractive> interactive { this };
    muse::ContextInject<muse::ui::IUiEngine> uiEngine { this };

public:
    explicit ExperienceSettingsModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    int languageMode() const;
    void setLanguageMode(int mode);

    int englishFunnyLevel() const;
    void setEnglishFunnyLevel(int level);
    int cantoneseFunnyLevel() const;
    void setCantoneseFunnyLevel(int level);

    bool emojiInDialogs() const;
    void setEmojiInDialogs(bool value);

    bool focusMode() const;
    void setFocusMode(bool value);
    bool lowStimulationMode() const;
    void setLowStimulationMode(bool value);
    bool timeAwarenessMode() const;
    void setTimeAwarenessMode(bool value);
    bool oneThingAtATimeMode() const;
    void setOneThingAtATimeMode(bool value);
    bool momentumMode() const;
    void setMomentumMode(bool value);

    bool restartRequired() const;
    bool cantoneseCatalogueAvailable() const;

    QString vocabularyFileName() const;
    int vocabularyEntryCount() const;
    QString vocabularyError() const;

    bool homeAssistantTokenSet() const;
    //! Replaces the stored Home Assistant token. Passing an empty string clears it.
    Q_INVOKABLE void setHomeAssistantToken(const QString& token);

    //! A live example of what the funny levels do to a message body.
    Q_INVOKABLE QString previewMessage(int kind, const QString& plainText) const;

    //! Opens the file picker and reads the chosen personal vocabulary file.
    Q_INVOKABLE void chooseVocabularyFile();
    Q_INVOKABLE void clearVocabulary();

    //! Shows a demonstration toast, used by the settings page and the captures.
    Q_INVOKABLE void showExampleNotification();

    bool narratorEnabled() const;
    void setNarratorEnabled(bool value);
    int narratorLanguage() const;
    void setNarratorLanguage(int value);
    QString narratorEnglishVoiceId() const;
    void setNarratorEnglishVoiceId(const QString& id);
    QString narratorCantoneseVoiceId() const;
    void setNarratorCantoneseVoiceId(const QString& id);
    double narratorRate() const;
    void setNarratorRate(double value);
    double narratorPitch() const;
    void setNarratorPitch(double value);
    bool narratorQuietMode() const;
    void setNarratorQuietMode(bool value);
    QString narratorEngineDescription() const;

    bool schoolModeOn() const;
    bool schoolModeAvailable() const;
    QString schoolModeError() const;
    QString schoolModeDisplayName() const;
    bool schoolModeHasCredential() const;
    //! Turns School mode on. When no credential exists yet, newCredential
    //! becomes the unlock PIN or password. Returns false when a credential
    //! already exists and none was needed, or none was given for the first
    //! activation.
    Q_INVOKABLE bool turnSchoolModeOn(const QString& newCredential);
    //! Turns School mode off. Returns false when the credential is wrong.
    Q_INVOKABLE bool turnSchoolModeOff(const QString& credential);
    Q_INVOKABLE bool renameSchoolMode(const QString& newDisplayName);

signals:
    void languageModeChanged();
    void funnyLevelsChanged();
    void emojiInDialogsChanged();
    void attentionModesChanged();
    void restartRequiredChanged();
    void vocabularyChanged();
    void homeAssistantTokenChanged();
    void narratorSettingsChanged();
    void schoolModeChanged();

private:
    void retranslate();

    QString m_vocabularyError;
    SchoolModeService* m_schoolMode = nullptr;
    NarratorEngine* m_narratorEngine = nullptr;
};
}

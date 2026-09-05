/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QObject>

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"
#include "framework/interactive/iinteractive.h"
#include "framework/ui/iuiengine.h"

#include "iexperienceconfiguration.h"
#include "iexperienceservice.h"
#include "imessagestyler.h"
#include "inotificationcenter.h"

namespace au::experience {
//! The model behind the Experience preferences page and the companion
//! overlays. Everything it exposes is persisted through muse settings.
class ExperienceSettingsModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

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

    //! A live example of what the funny levels do to a message body.
    Q_INVOKABLE QString previewMessage(int kind, const QString& plainText) const;

    //! Opens the file picker and reads the chosen personal vocabulary file.
    Q_INVOKABLE void chooseVocabularyFile();
    Q_INVOKABLE void clearVocabulary();

    //! Shows a demonstration toast, used by the settings page and the captures.
    Q_INVOKABLE void showExampleNotification();

signals:
    void languageModeChanged();
    void funnyLevelsChanged();
    void emojiInDialogsChanged();
    void attentionModesChanged();
    void restartRequiredChanged();
    void vocabularyChanged();

private:
    void retranslate();

    QString m_vocabularyError;
};
}

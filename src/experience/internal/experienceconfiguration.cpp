/*
 * Audacity: A Digital Audio Editor
 */
#include "experienceconfiguration.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "framework/global/settings.h"

using namespace muse;

namespace au::experience {
namespace {
static const std::string moduleName("experience");

static const Settings::Key LANGUAGE_MODE(moduleName, "experience/language/mode");
static const Settings::Key FUNNY_ENGLISH(moduleName, "experience/funny/english");
static const Settings::Key FUNNY_CANTONESE(moduleName, "experience/funny/cantonese");
static const Settings::Key EMOJI_DIALOGS(moduleName, "experience/emoji/dialogs");

static const Settings::Key MODE_FOCUS(moduleName, "experience/modes/focus");
static const Settings::Key MODE_LOW_STIMULATION(moduleName, "experience/modes/lowStimulation");
static const Settings::Key MODE_TIME_AWARENESS(moduleName, "experience/modes/timeAwareness");
static const Settings::Key MODE_ONE_THING(moduleName, "experience/modes/oneThingAtATime");
static const Settings::Key MODE_MOMENTUM(moduleName, "experience/modes/momentum");

static const Settings::Key SCHEDULE(moduleName, "experience/schedule/entries");
static const Settings::Key VOCABULARY_FILE_NAME(moduleName, "experience/vocabulary/fileName");

static const Settings::Key NARRATOR_ENABLED(moduleName, "experience/narrator/enabled");
static const Settings::Key NARRATOR_LANGUAGE(moduleName, "experience/narrator/language");
static const Settings::Key NARRATOR_ENGLISH_VOICE(moduleName, "experience/narrator/englishVoiceId");
static const Settings::Key NARRATOR_CANTONESE_VOICE(moduleName, "experience/narrator/cantoneseVoiceId");
static const Settings::Key NARRATOR_RATE(moduleName, "experience/narrator/rate");
static const Settings::Key NARRATOR_PITCH(moduleName, "experience/narrator/pitch");

//! Kept in the local settings file only. Never included in export, sync, or
//! local history, and never printed or logged; a scheduled row that needs
//! it reads it directly through this configuration interface.
static const Settings::Key HOME_ASSISTANT_TOKEN(moduleName, "experience/schedule/homeAssistantToken");

static int clampLevel(int level)
{
    return level < 1 ? 1 : (level > 5 ? 5 : level);
}
}

void ExperienceConfiguration::init()
{
    settings()->setDefaultValue(LANGUAGE_MODE, Val(static_cast<int>(LanguageMode::English)));
    settings()->valueChanged(LANGUAGE_MODE).onReceive(this, [this](const Val& val) {
        m_languageModeChanged.send(static_cast<LanguageMode>(val.toInt()));
    });

    settings()->setDefaultValue(FUNNY_ENGLISH, Val(5));
    settings()->setDefaultValue(FUNNY_CANTONESE, Val(5));
    settings()->valueChanged(FUNNY_ENGLISH).onReceive(this, [this](const Val&) { m_funnyLevelChanged.notify(); });
    settings()->valueChanged(FUNNY_CANTONESE).onReceive(this, [this](const Val&) { m_funnyLevelChanged.notify(); });

    settings()->setDefaultValue(EMOJI_DIALOGS, Val(true));
    settings()->valueChanged(EMOJI_DIALOGS).onReceive(this, [this](const Val& val) {
        m_emojiInDialogsChanged.send(val.toBool());
    });

    const std::vector<Settings::Key> modeKeys {
        MODE_FOCUS, MODE_LOW_STIMULATION, MODE_TIME_AWARENESS, MODE_ONE_THING, MODE_MOMENTUM
    };
    for (const Settings::Key& key : modeKeys) {
        settings()->setDefaultValue(key, Val(false));
        settings()->valueChanged(key).onReceive(this, [this](const Val&) { m_attentionModesChanged.notify(); });
    }

    settings()->setDefaultValue(SCHEDULE, Val(std::string("[]")));
    settings()->valueChanged(SCHEDULE).onReceive(this, [this](const Val&) { m_scheduleChanged.notify(); });

    settings()->setDefaultValue(VOCABULARY_FILE_NAME, Val(std::string()));
    settings()->valueChanged(VOCABULARY_FILE_NAME).onReceive(this, [this](const Val&) { m_vocabularyChanged.notify(); });

    // The narrator is off by default; a user must explicitly turn it on.
    settings()->setDefaultValue(NARRATOR_ENABLED, Val(false));
    settings()->setDefaultValue(NARRATOR_LANGUAGE, Val(0));
    settings()->setDefaultValue(NARRATOR_ENGLISH_VOICE, Val(std::string()));
    settings()->setDefaultValue(NARRATOR_CANTONESE_VOICE, Val(std::string()));
    settings()->setDefaultValue(NARRATOR_RATE, Val(0.0));
    settings()->setDefaultValue(NARRATOR_PITCH, Val(0.0));

    const std::vector<Settings::Key> narratorKeys {
        NARRATOR_ENABLED, NARRATOR_LANGUAGE, NARRATOR_ENGLISH_VOICE, NARRATOR_CANTONESE_VOICE, NARRATOR_RATE,
        NARRATOR_PITCH
    };
    for (const Settings::Key& key : narratorKeys) {
        settings()->valueChanged(key).onReceive(this, [this](const Val&) { m_narratorSettingsChanged.notify(); });
    }
}

LanguageMode ExperienceConfiguration::languageMode() const
{
    const int raw = settings()->value(LANGUAGE_MODE).toInt();
    if (raw < 0 || raw > static_cast<int>(LanguageMode::Bilingual)) {
        return LanguageMode::English;
    }
    return static_cast<LanguageMode>(raw);
}

void ExperienceConfiguration::setLanguageMode(LanguageMode mode)
{
    settings()->setSharedValue(LANGUAGE_MODE, Val(static_cast<int>(mode)));
}

async::Channel<LanguageMode> ExperienceConfiguration::languageModeChanged() const
{
    return m_languageModeChanged;
}

int ExperienceConfiguration::englishFunnyLevel() const
{
    return clampLevel(settings()->value(FUNNY_ENGLISH).toInt());
}

void ExperienceConfiguration::setEnglishFunnyLevel(int level)
{
    settings()->setSharedValue(FUNNY_ENGLISH, Val(clampLevel(level)));
}

int ExperienceConfiguration::cantoneseFunnyLevel() const
{
    return clampLevel(settings()->value(FUNNY_CANTONESE).toInt());
}

void ExperienceConfiguration::setCantoneseFunnyLevel(int level)
{
    settings()->setSharedValue(FUNNY_CANTONESE, Val(clampLevel(level)));
}

async::Notification ExperienceConfiguration::funnyLevelChanged() const
{
    return m_funnyLevelChanged;
}

bool ExperienceConfiguration::emojiInDialogs() const
{
    return settings()->value(EMOJI_DIALOGS).toBool();
}

void ExperienceConfiguration::setEmojiInDialogs(bool value)
{
    settings()->setSharedValue(EMOJI_DIALOGS, Val(value));
}

async::Channel<bool> ExperienceConfiguration::emojiInDialogsChanged() const
{
    return m_emojiInDialogsChanged;
}

bool ExperienceConfiguration::focusMode() const
{
    return settings()->value(MODE_FOCUS).toBool();
}

void ExperienceConfiguration::setFocusMode(bool value)
{
    settings()->setSharedValue(MODE_FOCUS, Val(value));
}

bool ExperienceConfiguration::lowStimulationMode() const
{
    return settings()->value(MODE_LOW_STIMULATION).toBool();
}

void ExperienceConfiguration::setLowStimulationMode(bool value)
{
    settings()->setSharedValue(MODE_LOW_STIMULATION, Val(value));
}

bool ExperienceConfiguration::timeAwarenessMode() const
{
    return settings()->value(MODE_TIME_AWARENESS).toBool();
}

void ExperienceConfiguration::setTimeAwarenessMode(bool value)
{
    settings()->setSharedValue(MODE_TIME_AWARENESS, Val(value));
}

bool ExperienceConfiguration::oneThingAtATimeMode() const
{
    return settings()->value(MODE_ONE_THING).toBool();
}

void ExperienceConfiguration::setOneThingAtATimeMode(bool value)
{
    settings()->setSharedValue(MODE_ONE_THING, Val(value));
}

bool ExperienceConfiguration::momentumMode() const
{
    return settings()->value(MODE_MOMENTUM).toBool();
}

void ExperienceConfiguration::setMomentumMode(bool value)
{
    settings()->setSharedValue(MODE_MOMENTUM, Val(value));
}

async::Notification ExperienceConfiguration::attentionModesChanged() const
{
    return m_attentionModesChanged;
}

std::vector<ScheduleEntry> ExperienceConfiguration::schedule() const
{
    const QString raw = QString::fromStdString(settings()->value(SCHEDULE).toString());
    const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    std::vector<ScheduleEntry> entries;
    if (!doc.isArray()) {
        return entries;
    }

    const QJsonArray array = doc.array();
    entries.reserve(static_cast<size_t>(array.size()));
    for (const QJsonValue& value : array) {
        if (!value.isObject()) {
            continue;
        }
        const ScheduleEntry entry = ScheduleEntry::fromMap(value.toObject().toVariantMap());
        if (entry.isValid()) {
            entries.push_back(entry);
        }
    }
    return entries;
}

void ExperienceConfiguration::setSchedule(const std::vector<ScheduleEntry>& entries)
{
    QJsonArray array;
    for (const ScheduleEntry& entry : entries) {
        if (!entry.isValid()) {
            continue;
        }
        array.append(QJsonObject::fromVariantMap(entry.toMap()));
    }

    const QString raw = QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
    settings()->setSharedValue(SCHEDULE, Val(raw.toStdString()));
}

async::Notification ExperienceConfiguration::scheduleChanged() const
{
    return m_scheduleChanged;
}

QString ExperienceConfiguration::homeAssistantToken() const
{
    return QString::fromStdString(settings()->value(HOME_ASSISTANT_TOKEN).toString());
}

void ExperienceConfiguration::setHomeAssistantToken(const QString& token)
{
    settings()->setLocalValue(HOME_ASSISTANT_TOKEN, Val(token.toStdString()));
}

QString ExperienceConfiguration::vocabularyFileName() const
{
    return QString::fromStdString(settings()->value(VOCABULARY_FILE_NAME).toString());
}

void ExperienceConfiguration::setVocabularyFileName(const QString& name)
{
    settings()->setSharedValue(VOCABULARY_FILE_NAME, Val(name.toStdString()));
}

QString ExperienceConfiguration::vocabularyStoragePath() const
{
    return globalConfiguration()->userAppDataPath().toQString() + "/experience/vocabulary.json";
}

async::Notification ExperienceConfiguration::vocabularyChanged() const
{
    return m_vocabularyChanged;
}

bool ExperienceConfiguration::narratorEnabled() const
{
    return settings()->value(NARRATOR_ENABLED).toBool();
}

void ExperienceConfiguration::setNarratorEnabled(bool value)
{
    settings()->setSharedValue(NARRATOR_ENABLED, Val(value));
}

int ExperienceConfiguration::narratorLanguage() const
{
    const int raw = settings()->value(NARRATOR_LANGUAGE).toInt();
    return (raw < 0 || raw > 2) ? 0 : raw;
}

void ExperienceConfiguration::setNarratorLanguage(int value)
{
    settings()->setSharedValue(NARRATOR_LANGUAGE, Val((value < 0 || value > 2) ? 0 : value));
}

QString ExperienceConfiguration::narratorEnglishVoiceId() const
{
    return QString::fromStdString(settings()->value(NARRATOR_ENGLISH_VOICE).toString());
}

void ExperienceConfiguration::setNarratorEnglishVoiceId(const QString& id)
{
    settings()->setSharedValue(NARRATOR_ENGLISH_VOICE, Val(id.toStdString()));
}

QString ExperienceConfiguration::narratorCantoneseVoiceId() const
{
    return QString::fromStdString(settings()->value(NARRATOR_CANTONESE_VOICE).toString());
}

void ExperienceConfiguration::setNarratorCantoneseVoiceId(const QString& id)
{
    settings()->setSharedValue(NARRATOR_CANTONESE_VOICE, Val(id.toStdString()));
}

double ExperienceConfiguration::narratorRate() const
{
    return settings()->value(NARRATOR_RATE).toDouble();
}

void ExperienceConfiguration::setNarratorRate(double value)
{
    settings()->setSharedValue(NARRATOR_RATE, Val(value));
}

double ExperienceConfiguration::narratorPitch() const
{
    return settings()->value(NARRATOR_PITCH).toDouble();
}

void ExperienceConfiguration::setNarratorPitch(double value)
{
    settings()->setSharedValue(NARRATOR_PITCH, Val(value));
}

async::Notification ExperienceConfiguration::narratorSettingsChanged() const
{
    return m_narratorSettingsChanged;
}
}

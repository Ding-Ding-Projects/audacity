/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QMetaType>
#include <QString>
#include <QVariantMap>

namespace au::experience {
//! The three language modes offered by the Experience module.
enum class LanguageMode {
    English = 0,
    Cantonese = 1,
    Bilingual = 2
};

//! The kind of message a piece of text belongs to. The message styler uses it
//! to pick a decoration table. It never changes the facts of the text.
enum class MessageKind {
    Info = 0,
    Success = 1,
    Warning = 2,
    Error = 3,
    Tooltip = 4,
    Dialog = 5
};

//! Notification severity. Info and success dismiss themselves, warning and
//! error stay until the reader dismisses them.
enum class NotificationType {
    Info = 0,
    Success = 1,
    Warning = 2,
    Error = 3
};

struct Notification
{
    int id = 0;
    NotificationType type = NotificationType::Info;
    QString title;
    QString body;
    QString actionText;
    QString actionCode;
    qint64 timestamp = 0;
    bool dismissed = false;

    QVariantMap toMap() const;
};

//! Where a schedule row's value comes from.
namespace ScheduleSource {
static const QString Local = QStringLiteral("local");
static const QString HttpsApi = QStringLiteral("httpsApi");
static const QString HomeAssistant = QStringLiteral("homeAssistant");
}

//! One row of the scheduled settings table.
//!
//! A row with no end time fires once, at hour:minute, exactly as it always
//! did. A row with an end time is active for the whole window between the
//! start and the end time, on the days it is scheduled for, and the setting
//! it touches is restored to what it held before the window opened as soon
//! as the window closes. An optional start and end date bound the whole rule
//! to a date range; leaving either empty means unbounded on that side.
struct ScheduleEntry
{
    QString id;
    bool enabled = true;
    int hour = 9;
    int minute = 0;
    //! -1 means the row fires once at hour:minute rather than holding a window open.
    int endHour = -1;
    int endMinute = -1;
    //! ISO 8601 date (yyyy-MM-dd), or empty for no bound.
    QString startDate;
    QString endDate;
    //! Bit 0 is Monday, bit 6 is Sunday.
    int weekdayMask = 0b1111111;
    //! One of the keys listed in ScheduleKeys.
    QString key;
    //! Used directly when source is Local. Otherwise a starting point shown
    //! to the user until the remote source answers.
    QString value;
    //! One of the values in ScheduleSource.
    QString source = ScheduleSource::Local;
    //! Used when source is HttpsApi: a validated HTTPS endpoint returning a
    //! JSON object whose fields are allowlisted against ScheduleKeys.
    QString apiUrl;
    //! Used when source is HomeAssistant.
    QString haBaseUrl;
    QString haEntityId;

    bool hasEndTime() const { return endHour >= 0 && endMinute >= 0; }
    bool isValid() const;
    QVariantMap toMap() const;
    static ScheduleEntry fromMap(const QVariantMap& map);
};

//! The setting keys a schedule row is allowed to change.
namespace ScheduleKeys {
static const QString LanguageMode = QStringLiteral("languageMode");
static const QString Theme = QStringLiteral("theme");
static const QString Density = QStringLiteral("density");
static const QString SeedColor = QStringLiteral("seedColor");
static const QString FontFamily = QStringLiteral("fontFamily");
static const QString FontSize = QStringLiteral("fontSize");
static const QString ReducedMotion = QStringLiteral("reducedMotion");
}

//! The result of reading a personal vocabulary file.
struct VocabularyLoadResult
{
    bool ok = false;
    //! A human readable reason, already translated, when ok is false.
    QString error;
    int entryCount = 0;
    QString fileName;
};
}

Q_DECLARE_METATYPE(au::experience::LanguageMode)
Q_DECLARE_METATYPE(au::experience::MessageKind)
Q_DECLARE_METATYPE(au::experience::NotificationType)

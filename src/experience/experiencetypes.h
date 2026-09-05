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

//! One row of the scheduled settings table.
struct ScheduleEntry
{
    QString id;
    bool enabled = true;
    int hour = 9;
    int minute = 0;
    //! Bit 0 is Monday, bit 6 is Sunday.
    int weekdayMask = 0b1111111;
    //! One of the keys listed in ScheduleKeys.
    QString key;
    QString value;

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

/*
 * Audacity: A Digital Audio Editor
 */
#include "messagestyler.h"

#include <QStringList>

namespace au::experience {
namespace {
//! Three tone groups. Warnings and errors share the careful group so that a
//! problem never reads as a joke.
enum class ToneGroup {
    Neutral,
    Positive,
    Careful
};

ToneGroup groupOf(MessageKind kind)
{
    switch (kind) {
    case MessageKind::Success:
        return ToneGroup::Positive;
    case MessageKind::Warning:
    case MessageKind::Error:
        return ToneGroup::Careful;
    case MessageKind::Info:
    case MessageKind::Tooltip:
    case MessageKind::Dialog:
        break;
    }
    return ToneGroup::Neutral;
}

//! FNV-1a over the UTF-8 bytes. A stable hash keeps the styler deterministic
//! across runs and platforms, which qHash does not guarantee.
quint32 stableHash(const QString& text)
{
    quint32 hash = 2166136261u;
    const QByteArray utf8 = text.toUtf8();
    for (char byte : utf8) {
        hash ^= static_cast<quint8>(byte);
        hash *= 16777619u;
    }
    return hash;
}

int clampLevel(int level)
{
    return level < 1 ? 1 : (level > 5 ? 5 : level);
}

//! Bounded decoration tables. Index is [level - 2] because level 1 is plain.
const QStringList& englishTable(ToneGroup group, int level)
{
    static const QStringList neutral[4] = {
        { QStringLiteral("Just so you know:"), QStringLiteral("For reference:") },
        { QStringLiteral("Quick note:"), QStringLiteral("Heads up:") },
        { QStringLiteral("Here is the story:"), QStringLiteral("Right, listen up:") },
        { QStringLiteral("Big news from the studio:"), QStringLiteral("Stop the tape for a second:") }
    };
    static const QStringList positive[4] = {
        { QStringLiteral("Done:"), QStringLiteral("That worked:") },
        { QStringLiteral("Nicely done:"), QStringLiteral("All good:") },
        { QStringLiteral("Beautiful:"), QStringLiteral("Smooth as anything:") },
        { QStringLiteral("Chef's kiss:"), QStringLiteral("Absolutely nailed it:") }
    };
    static const QStringList careful[4] = {
        { QStringLiteral("Please note:"), QStringLiteral("One thing:") },
        { QStringLiteral("Careful here:"), QStringLiteral("Worth a look:") },
        { QStringLiteral("Hold on a moment:"), QStringLiteral("This one needs you:") },
        { QStringLiteral("Right, deep breath:"), QStringLiteral("Do not walk away yet:") }
    };

    const int index = clampLevel(level) - 2;
    switch (group) {
    case ToneGroup::Positive:
        return positive[index];
    case ToneGroup::Careful:
        return careful[index];
    case ToneGroup::Neutral:
        break;
    }
    return neutral[index];
}

const QStringList& cantoneseTable(ToneGroup group, int level)
{
    static const QStringList neutral[4] = {
        { QStringLiteral("話你知："), QStringLiteral("順帶一提：") },
        { QStringLiteral("同你講聲："), QStringLiteral("聽住先：") },
        { QStringLiteral("等我話你知啦："), QStringLiteral("嚟啦，睇下呢個：") },
        { QStringLiteral("喂喂喂，緊要嘢嚟㗎："), QStringLiteral("停一停，聽我講：") }
    };
    static const QStringList positive[4] = {
        { QStringLiteral("搞掂："), QStringLiteral("成功喇：") },
        { QStringLiteral("做得好："), QStringLiteral("冇問題喇：") },
        { QStringLiteral("正！"), QStringLiteral("順順利利：") },
        { QStringLiteral("勁揪呀你："), QStringLiteral("靚仔靚女，掂晒：") }
    };
    static const QStringList careful[4] = {
        { QStringLiteral("請注意："), QStringLiteral("有件事：") },
        { QStringLiteral("小心啲："), QStringLiteral("睇真啲先：") },
        { QStringLiteral("等陣先："), QStringLiteral("呢樣要你搞掂：") },
        { QStringLiteral("唞啖氣先："), QStringLiteral("咪走住呀：") }
    };

    const int index = clampLevel(level) - 2;
    switch (group) {
    case ToneGroup::Positive:
        return positive[index];
    case ToneGroup::Careful:
        return careful[index];
    case ToneGroup::Neutral:
        break;
    }
    return neutral[index];
}

QString emojiFor(MessageKind kind)
{
    switch (kind) {
    case MessageKind::Success:
        return QStringLiteral("\xF0\x9F\x8E\x89");
    case MessageKind::Warning:
        return QStringLiteral("\xE2\x9A\xA0\xEF\xB8\x8F");
    case MessageKind::Error:
        return QStringLiteral("\xF0\x9F\x9B\x91");
    case MessageKind::Info:
    case MessageKind::Dialog:
        return QStringLiteral("\xF0\x9F\x92\xAC");
    case MessageKind::Tooltip:
        break;
    }
    return QString();
}

//! Emoji belong in bodies only. A tooltip is help text next to a control, so
//! it stays clean.
bool emojiAllowed(MessageKind kind)
{
    return kind != MessageKind::Tooltip;
}

QString pick(const QStringList& phrases, const QString& plainText, int salt)
{
    if (phrases.isEmpty()) {
        return QString();
    }
    const quint32 hash = stableHash(plainText) + static_cast<quint32>(salt) * 2654435761u;
    return phrases.at(static_cast<int>(hash % static_cast<quint32>(phrases.size())));
}
}

QString MessageStyler::style(MessageKind kind, const QString& plainText) const
{
    return styleWith(kind, plainText,
                     configuration()->languageMode(),
                     configuration()->englishFunnyLevel(),
                     configuration()->cantoneseFunnyLevel(),
                     configuration()->emojiInDialogs());
}

QString MessageStyler::styleWith(MessageKind kind, const QString& plainText, LanguageMode mode, int englishLevel,
                                 int cantoneseLevel, bool emoji) const
{
    if (plainText.isEmpty()) {
        return plainText;
    }

    const ToneGroup group = groupOf(kind);
    const int english = clampLevel(englishLevel);
    const int cantonese = clampLevel(cantoneseLevel);

    QString result = plainText;

    switch (mode) {
    case LanguageMode::English: {
        if (english > 1) {
            const QString prefix = pick(englishTable(group, english), plainText, english);
            if (!prefix.isEmpty()) {
                result = prefix + QLatin1Char(' ') + result;
            }
        }
        break;
    }
    case LanguageMode::Cantonese: {
        if (cantonese > 1) {
            const QString prefix = pick(cantoneseTable(group, cantonese), plainText, cantonese);
            if (!prefix.isEmpty()) {
                result = prefix + result;
            }
        }
        break;
    }
    case LanguageMode::Bilingual: {
        //! In bilingual mode the text already carries both languages, so the
        //! English decoration goes in front and the Cantonese one behind.
        if (english > 1) {
            const QString prefix = pick(englishTable(group, english), plainText, english);
            if (!prefix.isEmpty()) {
                result = prefix + QLatin1Char(' ') + result;
            }
        }
        if (cantonese > 1) {
            const QString suffix = pick(cantoneseTable(group, cantonese), plainText, cantonese + 16);
            if (!suffix.isEmpty()) {
                //! The table entries end with a colon because they normally
                //! introduce the text. As a suffix the colon is dropped.
                QString trimmed = suffix;
                while (trimmed.endsWith(QChar(0xFF1A)) || trimmed.endsWith(QLatin1Char(':'))) {
                    trimmed.chop(1);
                }
                result = result + QLatin1Char(' ') + trimmed;
            }
        }
        break;
    }
    }

    if (emoji && emojiAllowed(kind)) {
        const QString glyph = emojiFor(kind);
        if (!glyph.isEmpty()) {
            result = glyph + QLatin1Char(' ') + result;
        }
    }

    return result;
}
}

/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include "internal/messagestyler.h"

using namespace au::experience;

namespace {
const QString FACTUAL = QStringLiteral("3 tracks were removed at 00:04.500, press Ctrl+Z to undo");
}

class MessageStylerTests : public ::testing::Test
{
protected:
    MessageStyler styler;
};

TEST_F(MessageStylerTests, LevelOneIsPlain)
{
    const QString styled = styler.styleWith(MessageKind::Info, FACTUAL, LanguageMode::English, 1, 1, false);
    EXPECT_EQ(styled, FACTUAL);
}

TEST_F(MessageStylerTests, EveryLevelKeepsTheFactsVerbatim)
{
    for (int level = 1; level <= 5; ++level) {
        for (int kind = 0; kind <= 5; ++kind) {
            const QString styled = styler.styleWith(static_cast<MessageKind>(kind), FACTUAL, LanguageMode::English, level, level,
                                                    true);
            EXPECT_TRUE(styled.contains(FACTUAL)) << "level " << level << " kind " << kind;
        }
    }
}

TEST_F(MessageStylerTests, CantoneseAndBilingualKeepTheFactsVerbatim)
{
    for (int level = 1; level <= 5; ++level) {
        const QString cantonese = styler.styleWith(MessageKind::Warning, FACTUAL, LanguageMode::Cantonese, level, level, true);
        EXPECT_TRUE(cantonese.contains(FACTUAL));

        const QString bilingual = styler.styleWith(MessageKind::Warning, FACTUAL, LanguageMode::Bilingual, level, level, true);
        EXPECT_TRUE(bilingual.contains(FACTUAL));
    }
}

TEST_F(MessageStylerTests, IsDeterministic)
{
    for (int level = 2; level <= 5; ++level) {
        const QString first = styler.styleWith(MessageKind::Success, FACTUAL, LanguageMode::Bilingual, level, level, true);
        for (int repeat = 0; repeat < 20; ++repeat) {
            EXPECT_EQ(styler.styleWith(MessageKind::Success, FACTUAL, LanguageMode::Bilingual, level, level, true), first);
        }
    }
}

TEST_F(MessageStylerTests, LevelsAreIndependentPerLanguage)
{
    const QString englishOnly = styler.styleWith(MessageKind::Info, FACTUAL, LanguageMode::Bilingual, 4, 1, false);
    const QString cantoneseOnly = styler.styleWith(MessageKind::Info, FACTUAL, LanguageMode::Bilingual, 1, 4, false);
    EXPECT_NE(englishOnly, cantoneseOnly);
    EXPECT_TRUE(englishOnly.contains(FACTUAL));
    EXPECT_TRUE(cantoneseOnly.contains(FACTUAL));
}

TEST_F(MessageStylerTests, EmojiOnlyWhenSwitchedOnAndNeverInTooltips)
{
    const QString withEmoji = styler.styleWith(MessageKind::Success, FACTUAL, LanguageMode::English, 1, 1, true);
    const QString withoutEmoji = styler.styleWith(MessageKind::Success, FACTUAL, LanguageMode::English, 1, 1, false);
    EXPECT_NE(withEmoji, withoutEmoji);
    EXPECT_EQ(withoutEmoji, FACTUAL);

    const QString tooltip = styler.styleWith(MessageKind::Tooltip, FACTUAL, LanguageMode::English, 1, 1, true);
    EXPECT_EQ(tooltip, FACTUAL);
}

TEST_F(MessageStylerTests, EmptyTextStaysEmpty)
{
    EXPECT_EQ(styler.styleWith(MessageKind::Info, QString(), LanguageMode::Bilingual, 5, 5, true), QString());
}

TEST_F(MessageStylerTests, LevelsOutsideTheRangeAreClamped)
{
    const QString low = styler.styleWith(MessageKind::Info, FACTUAL, LanguageMode::English, -3, 0, false);
    EXPECT_EQ(low, styler.styleWith(MessageKind::Info, FACTUAL, LanguageMode::English, 1, 1, false));

    const QString high = styler.styleWith(MessageKind::Info, FACTUAL, LanguageMode::English, 99, 99, false);
    EXPECT_EQ(high, styler.styleWith(MessageKind::Info, FACTUAL, LanguageMode::English, 5, 5, false));
}

/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include "internal/experiencetranslator.h"

using namespace au::experience;

TEST(ExperienceTranslatorTests, ComposesBothLanguages)
{
    EXPECT_EQ(ExperienceTranslator::compose(QStringLiteral("Add track"), QStringLiteral("加軌")),
              QStringLiteral("Add track / 加軌"));
}

TEST(ExperienceTranslatorTests, DoesNotRepeatAnIdenticalTranslation)
{
    EXPECT_EQ(ExperienceTranslator::compose(QStringLiteral("MP3"), QStringLiteral("MP3")), QStringLiteral("MP3"));
}

TEST(ExperienceTranslatorTests, FallsBackToEnglishWhenThereIsNoTranslation)
{
    EXPECT_EQ(ExperienceTranslator::compose(QStringLiteral("Add track"), QString()), QStringLiteral("Add track"));
}

TEST(ExperienceTranslatorTests, IsEmptyWithoutACatalogueOrAVocabulary)
{
    ExperienceTranslator translator;
    translator.setLanguageMode(LanguageMode::Bilingual);

    EXPECT_TRUE(translator.isEmpty());
}

TEST(ExperienceTranslatorTests, IsNotEmptyOnceAVocabularyIsSet)
{
    ExperienceTranslator translator;
    translator.setVocabulary({ { QStringLiteral("track"), QStringLiteral("lane") } });

    EXPECT_FALSE(translator.isEmpty());
}

TEST(ExperienceTranslatorTests, AppliesTheVocabularyToTheSourceText)
{
    ExperienceTranslator translator;
    translator.setVocabulary({ { QStringLiteral("track"), QStringLiteral("lane") } });

    EXPECT_EQ(translator.translate("appshell", "Add track", nullptr, -1), QStringLiteral("Add lane"));
}

TEST(ExperienceTranslatorTests, StaysOutOfTheWayWhenNothingMatches)
{
    ExperienceTranslator translator;
    translator.setVocabulary({ { QStringLiteral("track"), QStringLiteral("lane") } });

    EXPECT_TRUE(translator.translate("appshell", "Export audio", nullptr, -1).isEmpty());
}

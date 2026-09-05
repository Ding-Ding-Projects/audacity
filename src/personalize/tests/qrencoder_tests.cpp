/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include "internal/qrencoder.h"

using namespace au::personalize;

class QrEncoderTests : public ::testing::Test
{
};

TEST_F(QrEncoderTests, EncodesShortTextAtVersion1)
{
    QrEncoder::Result result = QrEncoder::encode("HELLO WORLD");
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.size, 21);
}

TEST_F(QrEncoderTests, FinderPatternsAreDrawnAtAllThreeCorners)
{
    QrEncoder::Result result = QrEncoder::encode("HELLO WORLD");
    ASSERT_TRUE(result.ok);

    // The outer ring of each 7x7 finder pattern is dark.
    EXPECT_TRUE(result.moduleAt(0, 0));
    EXPECT_TRUE(result.moduleAt(6, 0));
    EXPECT_TRUE(result.moduleAt(0, 6));
    EXPECT_TRUE(result.moduleAt(6, 6));
    // The inner 3x3 square is also dark.
    EXPECT_TRUE(result.moduleAt(3, 3));
    // The ring between the outer border and the inner square is light.
    EXPECT_FALSE(result.moduleAt(1, 1));

    int size = result.size;
    EXPECT_TRUE(result.moduleAt(size - 7, 0));
    EXPECT_TRUE(result.moduleAt(size - 1, 0));
    EXPECT_TRUE(result.moduleAt(0, size - 7));
    EXPECT_TRUE(result.moduleAt(0, size - 1));
}

TEST_F(QrEncoderTests, TimingPatternAlternates)
{
    QrEncoder::Result result = QrEncoder::encode("HELLO WORLD");
    ASSERT_TRUE(result.ok);

    for (int i = 8; i < result.size - 8; ++i) {
        bool expectedDark = (i % 2) == 0;
        EXPECT_EQ(result.moduleAt(i, 6), expectedDark) << "row 6 at column " << i;
        EXPECT_EQ(result.moduleAt(6, i), expectedDark) << "column 6 at row " << i;
    }
}

TEST_F(QrEncoderTests, RefusesTextThatExceedsSupportedCapacity)
{
    QString longText;
    for (int i = 0; i < 200; ++i) {
        longText += "x";
    }
    QrEncoder::Result result = QrEncoder::encode(longText.toUtf8());
    EXPECT_FALSE(result.ok);
}

TEST_F(QrEncoderTests, AcceptsTextRightAtTheDocumentedVersion5Ceiling)
{
    QString text;
    for (int i = 0; i < 106; ++i) {
        text += "B";
    }
    QrEncoder::Result result = QrEncoder::encode(text.toUtf8());
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.size, 37);
}

TEST_F(QrEncoderTests, TwoDifferentTextsProduceDifferentMatrices)
{
    QrEncoder::Result a = QrEncoder::encode("HELLO WORLD");
    QrEncoder::Result b = QrEncoder::encode("GOODBYE MOON");
    ASSERT_TRUE(a.ok);
    ASSERT_TRUE(b.ok);
    EXPECT_NE(a.modules, b.modules);
}

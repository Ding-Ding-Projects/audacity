/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include "internal/schoolmode.h"

using namespace au::experience;

TEST(SchoolModeStoreTests, ParsesAnEmptyFileAsOffWithTheShippedName)
{
    const SchoolModeStore::ParseResult result = SchoolModeStore::parse(QByteArray());
    ASSERT_TRUE(result.ok);
    EXPECT_FALSE(result.record.on);
    EXPECT_EQ(result.record.displayName, QStringLiteral("School mode"));
}

TEST(SchoolModeStoreTests, RoundTripsThroughSerialize)
{
    SchoolModeRecord record;
    record.on = true;
    record.displayName = QStringLiteral("Quiet time");
    record.credentialSaltHex = SchoolModeStore::newSaltHex();
    record.credentialHashHex = SchoolModeStore::hashCredential(QStringLiteral("1234"), record.credentialSaltHex);

    const QByteArray json = SchoolModeStore::serialize(record);
    const SchoolModeStore::ParseResult result = SchoolModeStore::parse(json);

    ASSERT_TRUE(result.ok);
    EXPECT_TRUE(result.record.on);
    EXPECT_EQ(result.record.displayName, QStringLiteral("Quiet time"));
    EXPECT_EQ(result.record.credentialHashHex, record.credentialHashHex);
}

TEST(SchoolModeStoreTests, RejectsMalformedJson)
{
    EXPECT_FALSE(SchoolModeStore::parse(QByteArray("{ not json")).ok);
}

TEST(SchoolModeStoreTests, RejectsAnEmptyDisplayName)
{
    const QByteArray json = R"({"on":false,"displayName":""})";
    EXPECT_FALSE(SchoolModeStore::parse(json).ok);
}

TEST(SchoolModeStoreTests, VerifiesTheRightCredential)
{
    const QString salt = SchoolModeStore::newSaltHex();
    const QString hash = SchoolModeStore::hashCredential(QStringLiteral("open-sesame"), salt);

    EXPECT_TRUE(SchoolModeStore::verifyCredential(QStringLiteral("open-sesame"), salt, hash));
    EXPECT_FALSE(SchoolModeStore::verifyCredential(QStringLiteral("wrong"), salt, hash));
}

TEST(SchoolModeStoreTests, RejectsVerificationWhenNoCredentialIsStoredYet)
{
    EXPECT_FALSE(SchoolModeStore::verifyCredential(QStringLiteral("anything"), QString(), QString()));
}

TEST(SchoolModeStoreTests, TwoSaltsAreNotTheSame)
{
    EXPECT_NE(SchoolModeStore::newSaltHex(), SchoolModeStore::newSaltHex());
}

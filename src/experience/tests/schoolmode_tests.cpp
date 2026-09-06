/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>

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

TEST(SchoolModeServiceTests, StartsOffWhenTheSharedRecordDoesNotExist)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    SchoolModeService service(nullptr, directory.filePath(QStringLiteral("school-mode.json")));

    EXPECT_TRUE(service.isAvailable());
    EXPECT_FALSE(service.isOn());
}

TEST(SchoolModeServiceTests, StartsFromAnOnRecordAndKeepsItsCredential)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("school-mode.json"));

    SchoolModeRecord record;
    record.on = true;
    record.displayName = QStringLiteral("Focus time");
    record.credentialSaltHex = SchoolModeStore::newSaltHex();
    record.credentialHashHex = SchoolModeStore::hashCredential(QStringLiteral("1234"), record.credentialSaltHex);
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    const QByteArray serialized = SchoolModeStore::serialize(record);
    ASSERT_EQ(file.write(serialized), serialized.size());
    file.close();

    SchoolModeService service(nullptr, path);
    EXPECT_TRUE(service.isAvailable());
    EXPECT_TRUE(service.isOn());
    EXPECT_EQ(service.displayName(), QStringLiteral("Focus time"));
    EXPECT_TRUE(service.hasCredential());
}

TEST(SchoolModeServiceTests, LiveOnOffPreservesTheStoredCredentialAndName)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("school-mode.json"));
    SchoolModeService service(nullptr, path);

    ASSERT_TRUE(service.turnOn(QStringLiteral("1234")));
    ASSERT_TRUE(service.isOn());
    service.rename(QStringLiteral("Focus time"));
    ASSERT_TRUE(service.turnOff(QStringLiteral("1234")));

    const SchoolModeStore::ParseResult stored = SchoolModeStore::readRecordFile(path);
    ASSERT_TRUE(stored.ok);
    EXPECT_FALSE(stored.record.on);
    EXPECT_EQ(stored.record.displayName, QStringLiteral("Focus time"));
    EXPECT_TRUE(SchoolModeStore::verifyCredential(QStringLiteral("1234"), stored.record.credentialSaltHex,
                                                  stored.record.credentialHashHex));
}

TEST(SchoolModeServiceTests, CorruptLiveRecordIsUnavailableAndKeepsTheLastKnownMode)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("school-mode.json"));
    SchoolModeService service(nullptr, path);
    ASSERT_TRUE(service.turnOn(QStringLiteral("1234")));

    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    ASSERT_EQ(file.write("{ malformed"), QByteArray("{ malformed").size());
    file.close();
    service.reload();

    EXPECT_FALSE(service.isAvailable());
    EXPECT_TRUE(service.isOn());
    EXPECT_FALSE(service.error().isEmpty());
}

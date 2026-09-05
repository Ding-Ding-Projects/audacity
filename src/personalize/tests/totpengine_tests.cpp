/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include "internal/totpengine.h"

using namespace au::personalize;

namespace {
//! The ASCII secrets used by the official RFC 6238 appendix B test vectors.
const QByteArray SECRET_SHA1 = QByteArray("12345678901234567890");
const QByteArray SECRET_SHA256 = QByteArray("12345678901234567890123456789012");
const QByteArray SECRET_SHA512
    = QByteArray("1234567890123456789012345678901234567890123456789012345678901234");
}

class TotpEngineTests : public ::testing::Test
{
};

TEST_F(TotpEngineTests, Rfc6238Sha1Vectors)
{
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA1, 59, 8, 30, TotpEngine::Algorithm::Sha1), "94287082");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA1, 1111111109, 8, 30, TotpEngine::Algorithm::Sha1), "07081804");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA1, 1111111111, 8, 30, TotpEngine::Algorithm::Sha1), "14050471");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA1, 1234567890, 8, 30, TotpEngine::Algorithm::Sha1), "89005924");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA1, 2000000000, 8, 30, TotpEngine::Algorithm::Sha1), "69279037");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA1, 20000000000LL, 8, 30, TotpEngine::Algorithm::Sha1), "65353130");
}

TEST_F(TotpEngineTests, Rfc6238Sha256Vectors)
{
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA256, 59, 8, 30, TotpEngine::Algorithm::Sha256), "46119246");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA256, 1111111109, 8, 30, TotpEngine::Algorithm::Sha256), "68084774");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA256, 1111111111, 8, 30, TotpEngine::Algorithm::Sha256), "67062674");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA256, 1234567890, 8, 30, TotpEngine::Algorithm::Sha256), "91819424");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA256, 2000000000, 8, 30, TotpEngine::Algorithm::Sha256), "90698825");
}

TEST_F(TotpEngineTests, Rfc6238Sha512Vectors)
{
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA512, 59, 8, 30, TotpEngine::Algorithm::Sha512), "90693936");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA512, 1111111109, 8, 30, TotpEngine::Algorithm::Sha512), "25091201");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA512, 1111111111, 8, 30, TotpEngine::Algorithm::Sha512), "99943326");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA512, 1234567890, 8, 30, TotpEngine::Algorithm::Sha512), "93441116");
    EXPECT_EQ(TotpEngine::totp(SECRET_SHA512, 2000000000, 8, 30, TotpEngine::Algorithm::Sha512), "38618901");
}

TEST_F(TotpEngineTests, VerifyAcceptsWithinSkewWindow)
{
    QString code = TotpEngine::totp(SECRET_SHA1, 1000000030, 6, 30, TotpEngine::Algorithm::Sha1);
    // 6 digit code truncated to the last 6 of the 8 digit computation is not
    // what totp() with digits=6 does; compute it directly at 6 digits.
    QString sixDigit = TotpEngine::totp(SECRET_SHA1, 1000000030, 6, 30, TotpEngine::Algorithm::Sha1);
    EXPECT_TRUE(TotpEngine::verify(SECRET_SHA1, sixDigit, 1000000030, 6, 30, TotpEngine::Algorithm::Sha1, 1));
    // One period earlier or later still verifies within the default skew.
    EXPECT_TRUE(TotpEngine::verify(SECRET_SHA1, sixDigit, 1000000030 + 30, 6, 30, TotpEngine::Algorithm::Sha1, 1));
    EXPECT_TRUE(TotpEngine::verify(SECRET_SHA1, sixDigit, 1000000030 - 30, 6, 30, TotpEngine::Algorithm::Sha1, 1));
    // Far outside the window fails.
    EXPECT_FALSE(TotpEngine::verify(SECRET_SHA1, sixDigit, 1000000030 + 300, 6, 30, TotpEngine::Algorithm::Sha1, 1));
    EXPECT_FALSE(code.isEmpty());
}

TEST_F(TotpEngineTests, Base32RoundTrips)
{
    QByteArray secret = TotpEngine::randomSecret(20);
    QString encoded = TotpEngine::base32Encode(secret);
    QByteArray decoded = TotpEngine::base32Decode(encoded);
    EXPECT_EQ(secret, decoded);
}

TEST_F(TotpEngineTests, OtpauthUriCarriesTheDeclaredParameters)
{
    QString uri = TotpEngine::otpauthUri("Material Audacity", "someone@example.com", SECRET_SHA1, 6, 30,
                                         TotpEngine::Algorithm::Sha1);
    EXPECT_TRUE(uri.startsWith("otpauth://totp/"));
    EXPECT_TRUE(uri.contains("secret="));
    EXPECT_TRUE(uri.contains("digits=6"));
    EXPECT_TRUE(uri.contains("period=30"));
    EXPECT_TRUE(uri.contains("algorithm=SHA1"));
}

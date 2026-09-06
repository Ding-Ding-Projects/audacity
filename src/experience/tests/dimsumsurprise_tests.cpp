/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include "internal/dimsumcatalog.h"
#include "internal/dimsumsurprise.h"

using namespace au::experience;

TEST(DimSumCatalogTests, ParsesAValidDocument)
{
    const QByteArray json
        =
            R"({"dishes":[
        {"id":"har-gow","name":{"en":"Shrimp dumpling","zhHant":"蝦餃"},"photoAsset":"har-gow.jpg"},
        {"id":"siu-mai","name":{"en":"Pork and shrimp dumpling","zhHant":"燒賣"},"photoAsset":"siu-mai.jpg"}
    ]})";

    const QVector<DimSumDish> dishes = DimSumCatalog::parse(json);

    ASSERT_EQ(dishes.size(), 2);
    EXPECT_EQ(dishes[0].id, QStringLiteral("har-gow"));
    EXPECT_EQ(dishes[0].bilingualLabel(), QStringLiteral("Shrimp dumpling · 蝦餃"));
}

TEST(DimSumCatalogTests, SkipsAnEntryMissingEitherName)
{
    const QByteArray json = R"({"dishes":[{"id":"broken","name":{"en":"Only English"}}]})";
    EXPECT_TRUE(DimSumCatalog::parse(json).isEmpty());
}

TEST(DimSumCatalogTests, RejectsMalformedJson)
{
    EXPECT_TRUE(DimSumCatalog::parse(QByteArray("not json")).isEmpty());
}

TEST(DimSumDrawTests, DrawsTrueUnderTheThreshold)
{
    DimSumDraw draw(0.10);
    EXPECT_TRUE(draw.drawWithSample(0.05));
}

TEST(DimSumDrawTests, DrawsFalseAtOrAboveTheThreshold)
{
    DimSumDraw draw(0.10);
    EXPECT_FALSE(draw.drawWithSample(0.10));
    DimSumDraw draw2(0.10);
    EXPECT_FALSE(draw2.drawWithSample(0.99));
}

TEST(DimSumDrawTests, NeverDrawsTwiceInOneLaunch)
{
    DimSumDraw draw(1.0);
    EXPECT_TRUE(draw.drawWithSample(0.0));
    // Same instance, same favourable sample: still false, because the
    // one-shot state was already consumed by the first call.
    EXPECT_FALSE(draw.drawWithSample(0.0));
    EXPECT_FALSE(draw.draw());
}

TEST(DimSumSurpriseServiceTests, AllowsTheExactRedirectHostsAGenuineDownloadUses)
{
    EXPECT_TRUE(DimSumSurpriseService::isAllowedRedirectTarget(
                    QUrl("https://objects.githubusercontent.com/some/signed/path?sig=abc")));
    EXPECT_TRUE(DimSumSurpriseService::isAllowedRedirectTarget(
                    QUrl("https://release-assets.githubusercontent.com/github-production-release-asset/1?sig=abc")));
    EXPECT_TRUE(DimSumSurpriseService::isAllowedRedirectTarget(QUrl("https://github.com/owner/repo/releases")));
    EXPECT_TRUE(DimSumSurpriseService::isAllowedRedirectTarget(
                    QUrl("https://raw.githubusercontent.com/owner/repo/main/file.json")));
}

TEST(DimSumSurpriseServiceTests, RefusesAnUnlistedHost)
{
    EXPECT_FALSE(DimSumSurpriseService::isAllowedRedirectTarget(QUrl("https://evil.example.com/payload.png")));
    EXPECT_FALSE(DimSumSurpriseService::isAllowedRedirectTarget(
                    QUrl("https://github.com.evil.example.com/payload.png")));
}

TEST(DimSumSurpriseServiceTests, RefusesPlainHttpEvenOnAnAllowedHost)
{
    EXPECT_FALSE(DimSumSurpriseService::isAllowedRedirectTarget(QUrl("http://github.com/owner/repo/releases")));
}

TEST(DimSumSurpriseServiceTests, RefusesAnInvalidUrl)
{
    EXPECT_FALSE(DimSumSurpriseService::isAllowedRedirectTarget(QUrl()));
    EXPECT_FALSE(DimSumSurpriseService::isAllowedRedirectTarget(QUrl("not a url")));
}

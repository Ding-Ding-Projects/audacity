/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include "internal/pullcartmodel.h"

using namespace au::toolkit;

TEST(PullCartModelTests, StartsEmpty)
{
    PullCartModel cart;
    EXPECT_TRUE(cart.items().isEmpty());
    EXPECT_EQ(cart.totalBytes(), 0);
}

TEST(PullCartModelTests, AddingAModelSchedulesItOnce)
{
    PullCartModel cart;
    cart.addModel(QStringLiteral("llama3:8b"), 4LL * 1024 * 1024 * 1024);
    cart.addModel(QStringLiteral("llama3:8b"), 4LL * 1024 * 1024 * 1024);

    EXPECT_EQ(cart.items().size(), 1);
    EXPECT_TRUE(cart.contains(QStringLiteral("llama3:8b")));
}

TEST(PullCartModelTests, RemovingAModelDropsItFromTheCart)
{
    PullCartModel cart;
    cart.addModel(QStringLiteral("llama3:8b"), 100);
    cart.removeModel(QStringLiteral("llama3:8b"));
    EXPECT_FALSE(cart.contains(QStringLiteral("llama3:8b")));
}

TEST(PullCartModelTests, TotalBytesSumsEveryQueuedModel)
{
    PullCartModel cart;
    cart.addModel(QStringLiteral("a"), 100);
    cart.addModel(QStringLiteral("b"), 250);
    EXPECT_EQ(cart.totalBytes(), 350);
}

// This is the load bearing test: the cart's data model must never carry a
// price, currency, payment, checkout or subscription field of any kind,
// because adding a model to the cart only ever schedules a local download.
TEST(PullCartModelTests, NoItemCarriesAnyPaymentShapedField)
{
    PullCartModel cart;
    cart.addModel(QStringLiteral("a"), 100);

    const QVariantMap item = cart.items().first().toMap();
    const QStringList forbiddenKeys = {
        QStringLiteral("price"), QStringLiteral("cost"), QStringLiteral("currency"),
        QStringLiteral("payment"), QStringLiteral("checkout"), QStringLiteral("subscription"),
        QStringLiteral("account"), QStringLiteral("purchase"), QStringLiteral("total_due")
    };

    for (const QString& key : forbiddenKeys) {
        EXPECT_FALSE(item.contains(key)) << "unexpected payment-shaped field: " << key.toStdString();
    }
}

TEST(PullCartModelTests, ClearEmptiesTheCart)
{
    PullCartModel cart;
    cart.addModel(QStringLiteral("a"), 100);
    cart.clear();
    EXPECT_TRUE(cart.items().isEmpty());
}

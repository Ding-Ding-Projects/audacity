/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include "regex/regexengine.h"

using namespace au::companion;

namespace {
QString descriptionOfFirstToken(const RegexEngine& engine)
{
    const QVariantList explanation = engine.explanation();
    if (explanation.isEmpty()) {
        return QString();
    }
    return explanation.first().toMap().value(QStringLiteral("description")).toString();
}
}

TEST(RegexEngineTests, AnEmptyPatternIsValidAndMatchesNothing)
{
    RegexEngine engine;
    EXPECT_TRUE(engine.valid());
    EXPECT_EQ(engine.matchCount(), 0);
    EXPECT_TRUE(engine.explanation().isEmpty());
}

TEST(RegexEngineTests, AnInvalidPatternReportsItselfWithoutThrowing)
{
    RegexEngine engine;
    engine.setPattern(QStringLiteral("(unclosed"));

    EXPECT_FALSE(engine.valid());
    EXPECT_FALSE(engine.errorString().isEmpty());
    EXPECT_EQ(engine.matchCount(), 0);
}

TEST(RegexEngineTests, MatchesAndCapturesAreReported)
{
    RegexEngine engine;
    engine.setSampleText(QStringLiteral("track-01.wav track-02.wav"));
    engine.setPattern(QStringLiteral("track-(\\d+)\\.wav"));

    ASSERT_EQ(engine.matchCount(), 2);

    const QVariantMap first = engine.matches().first().toMap();
    EXPECT_EQ(first.value(QStringLiteral("text")).toString(), QStringLiteral("track-01.wav"));

    const QVariantList captures = first.value(QStringLiteral("captures")).toList();
    ASSERT_EQ(captures.size(), 1);
    EXPECT_EQ(captures.first().toMap().value(QStringLiteral("text")).toString(), QStringLiteral("01"));
}

TEST(RegexEngineTests, NamedCapturesCarryTheirName)
{
    RegexEngine engine;
    engine.setSampleText(QStringLiteral("2026-09-05"));
    engine.setPattern(QStringLiteral("(?<year>\\d{4})-(?<month>\\d{2})"));

    ASSERT_EQ(engine.matchCount(), 1);
    const QVariantList captures = engine.matches().first().toMap()
                                  .value(QStringLiteral("captures")).toList();
    ASSERT_EQ(captures.size(), 2);
    EXPECT_EQ(captures.at(0).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("year"));
    EXPECT_EQ(captures.at(1).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("month"));
}

TEST(RegexEngineTests, FlagsChangeTheResult)
{
    RegexEngine engine;
    engine.setSampleText(QStringLiteral("Audacity"));
    engine.setPattern(QStringLiteral("audacity"));
    EXPECT_EQ(engine.matchCount(), 0);

    engine.setCaseInsensitive(true);
    EXPECT_EQ(engine.matchCount(), 1);
}

TEST(RegexEngineTests, DotAllChangesWhetherADotCrossesALine)
{
    RegexEngine engine;
    engine.setSampleText(QStringLiteral("a\nb"));
    engine.setPattern(QStringLiteral("a.b"));
    EXPECT_EQ(engine.matchCount(), 0);

    engine.setDotAll(true);
    EXPECT_EQ(engine.matchCount(), 1);
}

TEST(RegexEngineTests, TheExplanationNamesEachToken)
{
    RegexEngine engine;
    engine.setPattern(QStringLiteral("\\d+"));

    ASSERT_FALSE(engine.explanation().isEmpty());
    EXPECT_TRUE(descriptionOfFirstToken(engine).contains(QStringLiteral("any digit")));
    EXPECT_TRUE(descriptionOfFirstToken(engine).contains(QStringLiteral("one or more times")));
}

TEST(RegexEngineTests, TheExplanationNamesLazyAndPossessiveQuantifiers)
{
    RegexEngine lazy;
    lazy.setPattern(QStringLiteral("a*?"));
    EXPECT_TRUE(descriptionOfFirstToken(lazy).contains(QStringLiteral("as few times as possible")));

    RegexEngine possessive;
    possessive.setPattern(QStringLiteral("a*+"));
    EXPECT_TRUE(descriptionOfFirstToken(possessive)
                .contains(QStringLiteral("without giving anything back")));
}

TEST(RegexEngineTests, TheParseTreeNestsGroups)
{
    RegexEngine engine;
    engine.setPattern(QStringLiteral("(ab(cd))"));

    const QVariantMap tree = engine.parseTree();
    const QVariantList children = tree.value(QStringLiteral("children")).toList();
    ASSERT_EQ(children.size(), 1);

    const QVariantMap outer = children.first().toMap();
    EXPECT_EQ(outer.value(QStringLiteral("kind")).toString(), QStringLiteral("group"));

    const QVariantList inner = outer.value(QStringLiteral("children")).toList();
    ASSERT_FALSE(inner.isEmpty());

    bool foundNestedGroup = false;
    for (const QVariant& value : inner) {
        if (value.toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("group")) {
            foundNestedGroup = true;
        }
    }
    EXPECT_TRUE(foundNestedGroup);
    EXPECT_TRUE(tree.value(QStringLiteral("balanced")).toBool());
}

TEST(RegexEngineTests, UnbalancedBracketsAreReportedByTheParseTree)
{
    RegexEngine engine;
    engine.setPattern(QStringLiteral("(ab"));
    EXPECT_FALSE(engine.parseTree().value(QStringLiteral("balanced")).toBool());
}

TEST(RegexEngineTests, TheReplacementPreviewUsesCaptures)
{
    RegexEngine engine;
    engine.setSampleText(QStringLiteral("track-01.wav"));
    engine.setPattern(QStringLiteral("track-(\\d+)"));
    engine.setReplacement(QStringLiteral("take \\1"));

    EXPECT_EQ(engine.replacementPreview(), QStringLiteral("take 01.wav"));
}

TEST(RegexEngineTests, ANestedQuantifierIsReportedAsAHighRisk)
{
    RegexEngine engine;
    engine.setPattern(QStringLiteral("(a+)+"));

    EXPECT_EQ(engine.riskLevel(), RegexEngine::RiskHigh);
    ASSERT_FALSE(engine.risks().isEmpty());
    EXPECT_EQ(engine.risks().first().toMap().value(QStringLiteral("title")).toString(),
              QStringLiteral("Nested quantifier"));
}

TEST(RegexEngineTests, AnOverlappingAlternationUnderAQuantifierIsReported)
{
    RegexEngine engine;
    engine.setPattern(QStringLiteral("(a|a)*"));

    EXPECT_EQ(engine.riskLevel(), RegexEngine::RiskHigh);
}

TEST(RegexEngineTests, APlainPatternCarriesNoRisk)
{
    RegexEngine engine;
    engine.setPattern(QStringLiteral("^track-\\d{2}\\.wav$"));

    EXPECT_EQ(engine.riskLevel(), RegexEngine::RiskNone);
    EXPECT_TRUE(engine.risks().isEmpty());
}

TEST(RegexEngineTests, ALongSampleIsTruncatedSoTheRunStaysBounded)
{
    RegexEngine engine;
    engine.setSampleText(QString(RegexEngine::MAX_SAMPLE_CHARS + 100, QChar(u'a')));
    engine.setPattern(QStringLiteral("a"));

    EXPECT_TRUE(engine.truncated());
    EXPECT_LE(engine.matchCount(), RegexEngine::MAX_MATCHES);
}

TEST(RegexEngineTests, TheRunIsTimed)
{
    RegexEngine engine;
    engine.setSampleText(QStringLiteral("aaaaaaaaaa"));
    engine.setPattern(QStringLiteral("a"));

    EXPECT_GE(engine.lastRunMilliseconds(), 0.0);
}

TEST(RegexEngineTests, GuidedInsertionKeepsTheRawPatternInSync)
{
    RegexEngine engine;
    engine.insertFragment(QStringLiteral("\\d"));
    engine.insertFragment(QStringLiteral("+"));

    EXPECT_EQ(engine.pattern(), QStringLiteral("\\d+"));
}

TEST(RegexEngineTests, WrappingASelectionProducesTheRequestedGroup)
{
    RegexEngine engine;
    engine.setPattern(QStringLiteral("abc"));
    engine.wrapSelection(QStringLiteral("named"), QStringLiteral("word"), 0, 3);

    EXPECT_EQ(engine.pattern(), QStringLiteral("(?<word>abc)"));
}

TEST(RegexEngineTests, LiteralEscapingProducesAPatternThatMatchesItself)
{
    const QString literal = QStringLiteral("a+b(c)");
    RegexEngine engine;
    engine.setSampleText(literal);
    engine.setPattern(RegexEngine::escapeLiteral(literal));

    EXPECT_TRUE(engine.valid());
    EXPECT_EQ(engine.matchCount(), 1);
}

TEST(RegexEngineTests, TheDialectAndCapabilityMatrixAreReported)
{
    RegexEngine engine;
    EXPECT_TRUE(engine.dialect().startsWith(QStringLiteral("PCRE2 via QRegularExpression")));
    EXPECT_FALSE(engine.capabilities().isEmpty());
}

TEST(RegexEngineTests, ExportAndImportRoundTripTheWholeWorkbench)
{
    RegexEngine source;
    source.setPattern(QStringLiteral("(?<n>\\d+)"));
    source.setSampleText(QStringLiteral("42"));
    source.setReplacement(QStringLiteral("[\\1]"));
    source.setMultiline(true);

    const QString json = source.exportJson();

    RegexEngine target;
    ASSERT_TRUE(target.importJson(json));

    EXPECT_EQ(target.pattern(), source.pattern());
    EXPECT_EQ(target.sampleText(), source.sampleText());
    EXPECT_EQ(target.replacement(), source.replacement());
    EXPECT_TRUE(target.multiline());
}

TEST(RegexEngineTests, ImportingRubbishFailsWithoutChangingAnything)
{
    RegexEngine engine;
    engine.setPattern(QStringLiteral("abc"));

    EXPECT_FALSE(engine.importJson(QStringLiteral("not json at all")));
    EXPECT_EQ(engine.pattern(), QStringLiteral("abc"));
}

TEST(RegexEngineTests, TheTokenCatalogCoversEveryGuidedGroup)
{
    RegexEngine engine;
    const QVariantList catalog = engine.tokenCatalog();

    QStringList groups;
    for (const QVariant& value : catalog) {
        const QString group = value.toMap().value(QStringLiteral("group")).toString();
        if (!groups.contains(group)) {
            groups.append(group);
        }
    }

    for (const QString& expected : { QStringLiteral("Character classes"), QStringLiteral("Anchors"),
                                     QStringLiteral("Groups"), QStringLiteral("Quantifiers"),
                                     QStringLiteral("Alternation"), QStringLiteral("Lookaround"),
                                     QStringLiteral("References"), QStringLiteral("Modifiers") }) {
        EXPECT_TRUE(groups.contains(expected)) << expected.toStdString();
    }
}

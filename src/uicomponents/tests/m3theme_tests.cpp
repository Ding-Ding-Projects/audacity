/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include <QColor>

#include "components/m3hct.h"
#include "components/m3themeprovider.h"

namespace au::uicomponents {
namespace {
constexpr unsigned int DEFAULT_SEED = 0xFF926BFFu;

unsigned int toArgb(const QColor& color)
{
    return static_cast<unsigned int>(color.rgb()) | 0xFF000000u;
}

// The scheme is a map keyed by the snake_case role name.
unsigned int roleArgb(const std::map<std::string, QColor>& scheme, const std::string& role)
{
    auto it = scheme.find(role);
    EXPECT_NE(it, scheme.end()) << "missing role " << role;
    if (it == scheme.end()) {
        return 0xFF000000u;
    }
    return toArgb(it->second);
}

const std::vector<std::pair<std::string, std::string> >& onRolePairs()
{
    static const std::vector<std::pair<std::string, std::string> > pairs = {
        { "on_primary", "primary" },
        { "on_primary_container", "primary_container" },
        { "on_secondary", "secondary" },
        { "on_secondary_container", "secondary_container" },
        { "on_tertiary", "tertiary" },
        { "on_tertiary_container", "tertiary_container" },
        { "on_error", "error" },
        { "on_error_container", "error_container" },
        { "on_background", "background" },
        { "on_surface", "surface" },
        { "on_surface_variant", "surface_variant" },
        { "inverse_on_surface", "inverse_surface" },
    };
    return pairs;
}
}

// ---------------------------------------------------------------------------
// HCT
// ---------------------------------------------------------------------------

TEST(M3HctTests, RoundTripsTheSeedColour)
{
    const m3::Hct hct = m3::Hct::fromArgb(DEFAULT_SEED);
    EXPECT_EQ(hct.toArgb(), DEFAULT_SEED);
}

TEST(M3HctTests, RoundTripsASpreadOfColours)
{
    const unsigned int samples[] = {
        0xFF000000u, 0xFFFFFFFFu, 0xFFFF0000u, 0xFF00FF00u, 0xFF0000FFu,
        0xFF6750A4u, 0xFF18A999u, 0xFFEF476Fu, 0xFF808080u, 0xFF123456u
    };

    for (unsigned int argb : samples) {
        const m3::Hct hct = m3::Hct::fromArgb(argb);
        const unsigned int back = hct.toArgb();

        // The solver returns the closest representable colour, so allow a
        // single step per channel rather than demanding an exact match.
        for (int shift : { 16, 8, 0 }) {
            const int a = static_cast<int>((argb >> shift) & 255u);
            const int b = static_cast<int>((back >> shift) & 255u);
            EXPECT_LE(std::abs(a - b), 1)
                << "channel at shift " << shift << " for colour " << std::hex << argb;
        }
    }
}

TEST(M3HctTests, ToneMapsOntoLightness)
{
    for (double tone : { 0.0, 10.0, 40.0, 50.0, 90.0, 100.0 }) {
        const unsigned int argb = m3::solveToArgb(265.0, 36.0, tone);
        EXPECT_NEAR(m3::lstarFromArgb(argb), tone, 1.0) << "tone " << tone;
    }
}

TEST(M3HctTests, ContrastRatioMatchesTheKnownExtremes)
{
    EXPECT_NEAR(m3::contrastRatio(0xFFFFFFFFu, 0xFF000000u), 21.0, 0.01);
    EXPECT_NEAR(m3::contrastRatio(0xFF808080u, 0xFF808080u), 1.0, 0.01);
}

// ---------------------------------------------------------------------------
// Schemes
// ---------------------------------------------------------------------------

class M3SchemeTests : public ::testing::TestWithParam<m3::SchemeKind>
{
};

TEST_P(M3SchemeTests, EveryOnRolePairMeetsWcagAA)
{
    const std::map<std::string, QColor> scheme
        = m3::buildScheme(DEFAULT_SEED, m3::Variant::TonalSpot, GetParam());

    for (const auto& pair : onRolePairs()) {
        const double ratio = m3::contrastRatio(roleArgb(scheme, pair.first),
                                               roleArgb(scheme, pair.second));
        EXPECT_GE(ratio, 4.5) << pair.first << " on " << pair.second
                              << " has a contrast ratio of " << ratio;
    }
}

TEST_P(M3SchemeTests, ProducesEveryDeclaredRole)
{
    const std::map<std::string, QColor> scheme
        = m3::buildScheme(DEFAULT_SEED, m3::Variant::TonalSpot, GetParam());

    EXPECT_EQ(scheme.size(), m3::roleNames().size());
    for (const std::string& role : m3::roleNames()) {
        EXPECT_NE(scheme.find(role), scheme.end()) << "missing role " << role;
        EXPECT_TRUE(scheme.at(role).isValid()) << "invalid colour for role " << role;
    }
}

INSTANTIATE_TEST_SUITE_P(AllSchemes, M3SchemeTests,
                         ::testing::Values(m3::SchemeKind::Light,
                                           m3::SchemeKind::Dark,
                                           m3::SchemeKind::HighContrastWhite,
                                           m3::SchemeKind::HighContrastBlack));

TEST(M3SchemeTests2, EveryVariantMeetsWcagAAForTheMainOnRolePairs)
{
    const m3::Variant variants[] = {
        m3::Variant::TonalSpot, m3::Variant::Vibrant, m3::Variant::Expressive,
        m3::Variant::Neutral, m3::Variant::Monochrome, m3::Variant::Fidelity
    };

    for (m3::Variant variant : variants) {
        for (m3::SchemeKind kind : { m3::SchemeKind::Light, m3::SchemeKind::Dark }) {
            const std::map<std::string, QColor> scheme
                = m3::buildScheme(DEFAULT_SEED, variant, kind);

            for (const auto& pair : onRolePairs()) {
                const double ratio = m3::contrastRatio(roleArgb(scheme, pair.first),
                                                       roleArgb(scheme, pair.second));
                EXPECT_GE(ratio, 4.5)
                    << "variant " << m3::variantToString(variant).toStdString()
                    << ", " << pair.first << " on " << pair.second
                    << " has a contrast ratio of " << ratio;
            }
        }
    }
}

TEST(M3SchemeTests2, HighContrastSchemesUsePureBackgrounds)
{
    const std::map<std::string, QColor> white
        = m3::buildScheme(DEFAULT_SEED, m3::Variant::TonalSpot, m3::SchemeKind::HighContrastWhite);
    const std::map<std::string, QColor> black
        = m3::buildScheme(DEFAULT_SEED, m3::Variant::TonalSpot, m3::SchemeKind::HighContrastBlack);

    EXPECT_EQ(roleArgb(white, "surface"), 0xFFFFFFFFu);
    EXPECT_EQ(roleArgb(white, "on_surface"), 0xFF000000u);
    EXPECT_EQ(roleArgb(black, "surface"), 0xFF000000u);
    EXPECT_EQ(roleArgb(black, "on_surface"), 0xFFFFFFFFu);
}

TEST(M3SchemeTests2, VariantNamesRoundTrip)
{
    for (const QString& name : { QStringLiteral("tonal_spot"), QStringLiteral("vibrant"),
                                 QStringLiteral("expressive"), QStringLiteral("neutral"),
                                 QStringLiteral("monochrome"), QStringLiteral("fidelity") }) {
        EXPECT_EQ(m3::variantToString(m3::variantFromString(name)), name);
    }

    // An unknown name falls back rather than throwing.
    EXPECT_EQ(m3::variantFromString(QStringLiteral("not a variant")), m3::Variant::TonalSpot);
}

// ---------------------------------------------------------------------------
// Motion
// ---------------------------------------------------------------------------

TEST(M3MotionTests, ReportsTheSpecifiedDurations)
{
    M3MotionTokens motion;
    motion.setReducedMotion(false);

    EXPECT_EQ(motion.short1(), 50);
    EXPECT_EQ(motion.short4(), 200);
    EXPECT_EQ(motion.medium1(), 250);
    EXPECT_EQ(motion.medium4(), 400);
    EXPECT_EQ(motion.long1(), 450);
    EXPECT_EQ(motion.long4(), 600);
    EXPECT_EQ(motion.extraLong1(), 700);
    EXPECT_EQ(motion.extraLong4(), 1000);
    EXPECT_DOUBLE_EQ(motion.travel(24.0), 24.0);
}

TEST(M3MotionTests, ReducedMotionZeroesEveryDurationAndTravel)
{
    M3MotionTokens motion;
    motion.setReducedMotion(true);

    EXPECT_TRUE(motion.reducedMotion());

    EXPECT_EQ(motion.short1(), 0);
    EXPECT_EQ(motion.short2(), 0);
    EXPECT_EQ(motion.short3(), 0);
    EXPECT_EQ(motion.short4(), 0);
    EXPECT_EQ(motion.medium1(), 0);
    EXPECT_EQ(motion.medium2(), 0);
    EXPECT_EQ(motion.medium3(), 0);
    EXPECT_EQ(motion.medium4(), 0);
    EXPECT_EQ(motion.long1(), 0);
    EXPECT_EQ(motion.long2(), 0);
    EXPECT_EQ(motion.long3(), 0);
    EXPECT_EQ(motion.long4(), 0);
    EXPECT_EQ(motion.extraLong1(), 0);
    EXPECT_EQ(motion.extraLong2(), 0);
    EXPECT_EQ(motion.extraLong3(), 0);
    EXPECT_EQ(motion.extraLong4(), 0);

    EXPECT_DOUBLE_EQ(motion.travel(24.0), 0.0);
    EXPECT_DOUBLE_EQ(motion.travel(0.0), 0.0);
}

TEST(M3MotionTests, EasingCurvesAreCubicBeziers)
{
    M3MotionTokens motion;
    EXPECT_EQ(motion.standard().type(), QEasingCurve::BezierSpline);
    EXPECT_EQ(motion.emphasizedDecelerate().type(), QEasingCurve::BezierSpline);
    EXPECT_EQ(motion.standardBezier().size(), 6);
}

// ---------------------------------------------------------------------------
// Shape, state layers, elevation and density
// ---------------------------------------------------------------------------

TEST(M3TokenTests, ShapeScaleMatchesTheSpecification)
{
    M3ShapeTokens shape;
    EXPECT_DOUBLE_EQ(shape.none(), 0.0);
    EXPECT_DOUBLE_EQ(shape.extraSmall(), 4.0);
    EXPECT_DOUBLE_EQ(shape.small(), 8.0);
    EXPECT_DOUBLE_EQ(shape.medium(), 12.0);
    EXPECT_DOUBLE_EQ(shape.large(), 16.0);
    EXPECT_DOUBLE_EQ(shape.extraLarge(), 28.0);
}

TEST(M3TokenTests, StateLayerOpacitiesMatchTheSpecification)
{
    M3StateLayerTokens layer;
    EXPECT_DOUBLE_EQ(layer.hover(), 0.08);
    EXPECT_DOUBLE_EQ(layer.focus(), 0.10);
    EXPECT_DOUBLE_EQ(layer.pressed(), 0.10);
    EXPECT_DOUBLE_EQ(layer.dragged(), 0.16);
    EXPECT_DOUBLE_EQ(layer.disabledContent(), 0.38);
    EXPECT_DOUBLE_EQ(layer.disabledContainer(), 0.12);
}

TEST(M3TokenTests, ElevationLevelsAreOrderedAndClamped)
{
    M3ElevationTokens elevation;
    EXPECT_DOUBLE_EQ(elevation.dp(0), 0.0);
    EXPECT_DOUBLE_EQ(elevation.dp(5), 12.0);

    // Out of range levels clamp instead of reading past the table.
    EXPECT_DOUBLE_EQ(elevation.dp(-1), 0.0);
    EXPECT_DOUBLE_EQ(elevation.dp(99), 12.0);

    for (int level = 1; level <= 5; ++level) {
        EXPECT_GT(elevation.dp(level), elevation.dp(level - 1));
        EXPECT_GT(elevation.tintOpacity(level), elevation.tintOpacity(level - 1));
    }
}

TEST(M3TokenTests, DensityStepsByFourAndNeverGoesBelowTheMinimumTarget)
{
    M3DensityTokens density;
    EXPECT_EQ(density.level(), 0);
    EXPECT_DOUBLE_EQ(density.apply(40.0), 40.0);

    density.setLevel(-2);
    EXPECT_EQ(density.level(), -2);
    EXPECT_DOUBLE_EQ(density.offset(), -8.0);
    EXPECT_DOUBLE_EQ(density.apply(40.0), 32.0);

    density.setLevel(-3);
    EXPECT_DOUBLE_EQ(density.apply(28.0), 24.0);

    // The level is clamped to the Material range.
    density.setLevel(-99);
    EXPECT_EQ(density.level(), -3);
    density.setLevel(99);
    EXPECT_EQ(density.level(), 0);
}
}

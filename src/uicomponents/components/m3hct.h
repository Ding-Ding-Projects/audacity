/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <map>
#include <string>
#include <vector>

#include <QColor>
#include <QString>

/*!
 * Material Design 3 colour maths.
 *
 * A self contained port of the core of material-color-utilities: sRGB to CIE
 * XYZ, the CAM16 appearance model, the HCT colour space (hue, chroma, tone),
 * tonal palettes and the Material 3 scheme variants.
 *
 * The same maths is mirrored in buildscripts/tools/m3_hct.py, which generates
 * the four theme configuration files under src/app/configs. Any change here
 * must be made there as well, and the theme files regenerated.
 *
 * Reference: https://github.com/material-foundation/material-color-utilities
 * (Apache License 2.0, Google LLC).
 */
namespace au::uicomponents::m3 {
struct ViewingConditions
{
    double n = 0.0;
    double aw = 0.0;
    double nbb = 0.0;
    double ncb = 0.0;
    double c = 0.0;
    double nc = 0.0;
    double rgbD[3] = { 0.0, 0.0, 0.0 };
    double fl = 0.0;
    double flRoot = 0.0;
    double z = 0.0;

    static ViewingConditions make(double adaptingLuminance, double backgroundLstar, double surround, bool discountingIlluminant);
    static const ViewingConditions& sRgb();
};

struct Cam16
{
    double hue = 0.0;
    double chroma = 0.0;
    double j = 0.0;
    double q = 0.0;
    double m = 0.0;
    double s = 0.0;
    double jstar = 0.0;
    double astar = 0.0;
    double bstar = 0.0;

    double distance(const Cam16& other) const;

    static Cam16 fromInt(unsigned int argb);
    static Cam16 fromXyz(double x, double y, double z, const ViewingConditions& vc);
    static Cam16 fromJch(double j, double c, double h);

    unsigned int viewed(const ViewingConditions& vc) const;
};

//! HCT coordinates: hue in degrees, chroma, tone (CIE L*).
struct Hct
{
    double hue = 0.0;
    double chroma = 0.0;
    double tone = 0.0;

    static Hct fromArgb(unsigned int argb);
    unsigned int toArgb() const;
};

double linearized(double rgbComponent);
int delinearized(double rgbComponent);
double lstarFromY(double y);
double yFromLstar(double lstar);
double lstarFromArgb(unsigned int argb);
unsigned int argbFromLstar(double lstar);
double sanitizeDegrees(double degrees);

//! Solve for the sRGB colour closest to the requested HCT coordinates.
unsigned int solveToArgb(double hue, double chroma, double tone);

double relativeLuminance(unsigned int argb);
//! WCAG 2.x contrast ratio, between 1.0 and 21.0.
double contrastRatio(unsigned int argbA, unsigned int argbB);

//! One tonal palette: a fixed hue and chroma sampled at any tone 0 to 100.
class TonalPalette
{
public:
    TonalPalette() = default;
    TonalPalette(double hue, double chroma);

    static TonalPalette fromArgb(unsigned int argb);

    double hue() const { return m_hue; }
    double chroma() const { return m_chroma; }

    unsigned int tone(double tone) const;

private:
    double m_hue = 0.0;
    double m_chroma = 0.0;
    mutable std::map<int, unsigned int> m_cache;
};

enum class Variant {
    TonalSpot = 0,
    Vibrant,
    Expressive,
    Neutral,
    Monochrome,
    Fidelity
};

enum class SchemeKind {
    Light = 0,
    Dark,
    HighContrastWhite,
    HighContrastBlack
};

Variant variantFromString(const QString& name, Variant fallback = Variant::TonalSpot);
QString variantToString(Variant variant);

struct Palettes
{
    TonalPalette primary;
    TonalPalette secondary;
    TonalPalette tertiary;
    TonalPalette neutral;
    TonalPalette neutralVariant;
    TonalPalette error;
};

Palettes buildPalettes(unsigned int seedArgb, Variant variant);

//! All Material 3 colour role names, in the order they are declared.
const std::vector<std::string>& roleNames();

//! Build a full scheme. Keys are the snake_case role names from roleNames().
std::map<std::string, QColor> buildScheme(unsigned int seedArgb, Variant variant, SchemeKind kind);
}

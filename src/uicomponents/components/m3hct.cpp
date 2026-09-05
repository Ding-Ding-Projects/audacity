/*
* Audacity: A Digital Audio Editor
*/
#include "m3hct.h"

#include <algorithm>
#include <cmath>

namespace au::uicomponents::m3 {
namespace {
constexpr double kPi = 3.14159265358979323846;

const double kWhitePointD65[3] = { 95.047, 100.0, 108.883 };

inline double clampDouble(double low, double high, double value)
{
    return std::max(low, std::min(high, value));
}

inline int clampInt(int low, int high, int value)
{
    return std::max(low, std::min(high, value));
}

inline double lerp(double start, double stop, double amount)
{
    return (1.0 - amount) * start + amount * stop;
}

inline double labF(double t)
{
    static const double e = 216.0 / 24389.0;
    static const double kappa = 24389.0 / 27.0;
    if (t > e) {
        return std::cbrt(t);
    }
    return (kappa * t + 16.0) / 116.0;
}

inline double labInvF(double ft)
{
    static const double e = 216.0 / 24389.0;
    static const double kappa = 24389.0 / 27.0;
    const double ft3 = ft * ft * ft;
    if (ft3 > e) {
        return ft3;
    }
    return (116.0 * ft - 16.0) / kappa;
}

inline unsigned int argbFromRgb(int red, int green, int blue)
{
    return (255u << 24) | ((static_cast<unsigned int>(red) & 255u) << 16)
           | ((static_cast<unsigned int>(green) & 255u) << 8)
           | (static_cast<unsigned int>(blue) & 255u);
}

inline int redFromArgb(unsigned int argb) { return static_cast<int>((argb >> 16) & 255u); }
inline int greenFromArgb(unsigned int argb) { return static_cast<int>((argb >> 8) & 255u); }
inline int blueFromArgb(unsigned int argb) { return static_cast<int>(argb & 255u); }

unsigned int argbFromXyz(double x, double y, double z)
{
    const double r = 3.2413774792388685 * x - 1.5376652402851851 * y - 0.49885366846268053 * z;
    const double g = -0.9691452513005321 * x + 1.8758853451067872 * y + 0.04156585616912061 * z;
    const double b = 0.05562093689691305 * x - 0.20395524564742123 * y + 1.0571799111220335 * z;
    return argbFromRgb(delinearized(r), delinearized(g), delinearized(b));
}
}

double linearized(double rgbComponent)
{
    const double normalized = rgbComponent / 255.0;
    if (normalized <= 0.040449936) {
        return normalized / 12.92 * 100.0;
    }
    return std::pow((normalized + 0.055) / 1.055, 2.4) * 100.0;
}

int delinearized(double rgbComponent)
{
    const double normalized = rgbComponent / 100.0;
    double delin = 0.0;
    if (normalized <= 0.0031308) {
        delin = normalized * 12.92;
    } else {
        delin = 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
    }
    return clampInt(0, 255, static_cast<int>(std::lround(delin * 255.0)));
}

double lstarFromY(double y)
{
    return labF(y / 100.0) * 116.0 - 16.0;
}

double yFromLstar(double lstar)
{
    return 100.0 * labInvF((lstar + 16.0) / 116.0);
}

double lstarFromArgb(unsigned int argb)
{
    const double r = linearized(redFromArgb(argb));
    const double g = linearized(greenFromArgb(argb));
    const double b = linearized(blueFromArgb(argb));
    const double y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    return lstarFromY(y);
}

unsigned int argbFromLstar(double lstar)
{
    const int component = delinearized(yFromLstar(lstar));
    return argbFromRgb(component, component, component);
}

double sanitizeDegrees(double degrees)
{
    degrees = std::fmod(degrees, 360.0);
    if (degrees < 0.0) {
        degrees += 360.0;
    }
    return degrees;
}

// ---------------------------------------------------------------------------
// Viewing conditions
// ---------------------------------------------------------------------------

ViewingConditions ViewingConditions::make(double adaptingLuminance, double backgroundLstar,
                                          double surround, bool discountingIlluminant)
{
    backgroundLstar = std::max(0.1, backgroundLstar);

    const double xw = kWhitePointD65[0];
    const double yw = kWhitePointD65[1];
    const double zw = kWhitePointD65[2];

    const double rW = xw * 0.401288 + yw * 0.650173 + zw * -0.051461;
    const double gW = xw * -0.250268 + yw * 1.204414 + zw * 0.045854;
    const double bW = xw * -0.002079 + yw * 0.048952 + zw * 0.953127;

    const double f = 0.8 + surround / 10.0;
    const double c = (f >= 0.9) ? lerp(0.59, 0.69, (f - 0.9) * 10.0)
                    : lerp(0.525, 0.59, (f - 0.8) * 10.0);

    double d = discountingIlluminant
               ? 1.0
               : f * (1.0 - (1.0 / 3.6) * std::exp((-adaptingLuminance - 42.0) / 92.0));
    d = clampDouble(0.0, 1.0, d);

    ViewingConditions vc;
    vc.nc = f;
    vc.c = c;
    vc.rgbD[0] = d * (100.0 / rW) + 1.0 - d;
    vc.rgbD[1] = d * (100.0 / gW) + 1.0 - d;
    vc.rgbD[2] = d * (100.0 / bW) + 1.0 - d;

    const double k = 1.0 / (5.0 * adaptingLuminance + 1.0);
    const double k4 = k * k * k * k;
    const double k4f = 1.0 - k4;
    vc.fl = k4 * adaptingLuminance
            + 0.1 * k4f * k4f * std::cbrt(5.0 * adaptingLuminance);
    vc.flRoot = std::pow(vc.fl, 0.25);

    vc.n = yFromLstar(backgroundLstar) / kWhitePointD65[1];
    vc.z = 1.48 + std::sqrt(vc.n);
    vc.nbb = 0.725 / std::pow(vc.n, 0.2);
    vc.ncb = vc.nbb;

    const double w[3] = { rW, gW, bW };
    double rgbA[3];
    for (int i = 0; i < 3; ++i) {
        const double factor = std::pow(vc.fl * vc.rgbD[i] * w[i] / 100.0, 0.42);
        rgbA[i] = 400.0 * factor / (factor + 27.13);
    }
    vc.aw = (2.0 * rgbA[0] + rgbA[1] + 0.05 * rgbA[2]) * vc.nbb;
    return vc;
}

const ViewingConditions& ViewingConditions::sRgb()
{
    static const ViewingConditions vc = ViewingConditions::make(
        (200.0 / kPi) * yFromLstar(50.0) / 100.0, 50.0, 2.0, false);
    return vc;
}

// ---------------------------------------------------------------------------
// CAM16
// ---------------------------------------------------------------------------

double Cam16::distance(const Cam16& other) const
{
    const double dJ = jstar - other.jstar;
    const double dA = astar - other.astar;
    const double dB = bstar - other.bstar;
    const double dEPrime = std::sqrt(dJ * dJ + dA * dA + dB * dB);
    return 1.41 * std::pow(dEPrime, 0.63);
}

Cam16 Cam16::fromInt(unsigned int argb)
{
    const double r = linearized(redFromArgb(argb));
    const double g = linearized(greenFromArgb(argb));
    const double b = linearized(blueFromArgb(argb));
    const double x = 0.41233895 * r + 0.35762064 * g + 0.18051042 * b;
    const double y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    const double z = 0.01932141 * r + 0.11916382 * g + 0.95034478 * b;
    return fromXyz(x, y, z, ViewingConditions::sRgb());
}

Cam16 Cam16::fromXyz(double x, double y, double z, const ViewingConditions& vc)
{
    const double rC = 0.401288 * x + 0.650173 * y - 0.051461 * z;
    const double gC = -0.250268 * x + 1.204414 * y + 0.045854 * z;
    const double bC = -0.002079 * x + 0.048952 * y + 0.953127 * z;

    const double rD = vc.rgbD[0] * rC;
    const double gD = vc.rgbD[1] * gC;
    const double bD = vc.rgbD[2] * bC;

    const double rAf = std::pow(vc.fl * std::fabs(rD) / 100.0, 0.42);
    const double gAf = std::pow(vc.fl * std::fabs(gD) / 100.0, 0.42);
    const double bAf = std::pow(vc.fl * std::fabs(bD) / 100.0, 0.42);

    const double rA = std::copysign(400.0 * rAf / (rAf + 27.13), rD);
    const double gA = std::copysign(400.0 * gAf / (gAf + 27.13), gD);
    const double bA = std::copysign(400.0 * bAf / (bAf + 27.13), bD);

    const double a = (11.0 * rA - 12.0 * gA + bA) / 11.0;
    const double b = (rA + gA - 2.0 * bA) / 9.0;
    const double u = (20.0 * rA + 20.0 * gA + 21.0 * bA) / 20.0;
    const double p2 = (40.0 * rA + 20.0 * gA + bA) / 20.0;

    const double atanDegrees = std::atan2(b, a) * 180.0 / kPi;
    const double hue = sanitizeDegrees(atanDegrees);
    const double hueRadians = hue * kPi / 180.0;

    const double ac = p2 * vc.nbb;
    const double j = 100.0 * std::pow(ac / vc.aw, vc.c * vc.z);
    const double q = (4.0 / vc.c) * std::sqrt(j / 100.0) * (vc.aw + 4.0) * vc.flRoot;

    const double huePrime = (hue < 20.14) ? hue + 360.0 : hue;
    const double eHue = 0.25 * (std::cos(huePrime * kPi / 180.0 + 2.0) + 3.8);
    const double p1 = 50000.0 / 13.0 * eHue * vc.nc * vc.ncb;
    const double t = p1 * std::sqrt(a * a + b * b) / (u + 0.305);
    const double alpha = std::pow(t, 0.9) * std::pow(1.64 - std::pow(0.29, vc.n), 0.73);
    const double c = alpha * std::sqrt(j / 100.0);
    const double m = c * vc.flRoot;
    const double s = 50.0 * std::sqrt((alpha * vc.c) / (vc.aw + 4.0));

    Cam16 cam;
    cam.hue = hue;
    cam.chroma = c;
    cam.j = j;
    cam.q = q;
    cam.m = m;
    cam.s = s;
    cam.jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j);
    const double mstar = 1.0 / 0.0228 * std::log(1.0 + 0.0228 * m);
    cam.astar = mstar * std::cos(hueRadians);
    cam.bstar = mstar * std::sin(hueRadians);
    return cam;
}

Cam16 Cam16::fromJch(double j, double c, double h)
{
    const ViewingConditions& vc = ViewingConditions::sRgb();
    Cam16 cam;
    cam.j = j;
    cam.chroma = c;
    cam.hue = h;
    cam.q = (4.0 / vc.c) * std::sqrt(j / 100.0) * (vc.aw + 4.0) * vc.flRoot;
    cam.m = c * vc.flRoot;
    const double alpha = (j > 0.0) ? c / std::sqrt(j / 100.0) : 0.0;
    cam.s = 50.0 * std::sqrt((alpha * vc.c) / (vc.aw + 4.0));

    const double hueRadians = h * kPi / 180.0;
    cam.jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j);
    const double mstar = 1.0 / 0.0228 * std::log(1.0 + 0.0228 * cam.m);
    cam.astar = mstar * std::cos(hueRadians);
    cam.bstar = mstar * std::sin(hueRadians);
    return cam;
}

unsigned int Cam16::viewed(const ViewingConditions& vc) const
{
    const double alpha = (chroma == 0.0 || j == 0.0) ? 0.0 : chroma / std::sqrt(j / 100.0);
    const double t = std::pow(alpha / std::pow(1.64 - std::pow(0.29, vc.n), 0.73), 1.0 / 0.9);
    const double hRad = hue * kPi / 180.0;
    const double eHue = 0.25 * (std::cos(hRad + 2.0) + 3.8);
    const double ac = vc.aw * std::pow(j / 100.0, 1.0 / vc.c / vc.z);
    const double p1 = eHue * (50000.0 / 13.0) * vc.nc * vc.ncb;
    const double p2 = ac / vc.nbb;

    const double hSin = std::sin(hRad);
    const double hCos = std::cos(hRad);

    const double gamma = 23.0 * (p2 + 0.305) * t
                         / (23.0 * p1 + 11.0 * t * hCos + 108.0 * t * hSin);
    const double a = gamma * hCos;
    const double b = gamma * hSin;

    const double rA = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0;
    const double gA = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0;
    const double bA = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0;

    const double rCBase = std::max(0.0, (27.13 * std::fabs(rA)) / (400.0 - std::fabs(rA)));
    const double rC = std::copysign((100.0 / vc.fl) * std::pow(rCBase, 1.0 / 0.42), rA);
    const double gCBase = std::max(0.0, (27.13 * std::fabs(gA)) / (400.0 - std::fabs(gA)));
    const double gC = std::copysign((100.0 / vc.fl) * std::pow(gCBase, 1.0 / 0.42), gA);
    const double bCBase = std::max(0.0, (27.13 * std::fabs(bA)) / (400.0 - std::fabs(bA)));
    const double bC = std::copysign((100.0 / vc.fl) * std::pow(bCBase, 1.0 / 0.42), bA);

    const double rF = rC / vc.rgbD[0];
    const double gF = gC / vc.rgbD[1];
    const double bF = bC / vc.rgbD[2];

    const double x = 1.86206786 * rF - 1.01125463 * gF + 0.14918677 * bF;
    const double y = 0.38752654 * rF + 0.62144744 * gF - 0.00897398 * bF;
    const double z = -0.01584150 * rF - 0.03412294 * gF + 1.04996444 * bF;

    return argbFromXyz(x, y, z);
}

// ---------------------------------------------------------------------------
// HCT
// ---------------------------------------------------------------------------

namespace {
bool findCamByJ(double hue, double chroma, double tone, Cam16& out)
{
    double low = 0.0;
    double high = 100.0;
    double bestDl = 1000.0;
    double bestDe = 1000.0;
    bool found = false;
    Cam16 best;

    while (std::fabs(low - high) > 0.01) {
        const double mid = low + (high - low) / 2.0;
        const Cam16 camBeforeClip = Cam16::fromJch(mid, chroma, hue);
        const unsigned int clipped = camBeforeClip.viewed(ViewingConditions::sRgb());
        const double clippedLstar = lstarFromArgb(clipped);
        const double dL = std::fabs(tone - clippedLstar);

        if (dL < 0.2) {
            const Cam16 camClipped = Cam16::fromInt(clipped);
            const double dE = camClipped.distance(
                Cam16::fromJch(camClipped.j, camClipped.chroma, hue));
            if (dE <= 1.0 && dE <= bestDe) {
                bestDl = dL;
                bestDe = dE;
                best = camClipped;
                found = true;
            }
        }

        if (bestDl == 0.0 && bestDe == 0.0) {
            break;
        }

        if (clippedLstar < tone) {
            low = mid;
        } else {
            high = mid;
        }
    }

    if (found) {
        out = best;
    }
    return found;
}
}

unsigned int solveToArgb(double hue, double chroma, double tone)
{
    if (chroma < 1.0 || std::lround(tone) <= 0 || std::lround(tone) >= 100) {
        return argbFromLstar(tone);
    }

    hue = sanitizeDegrees(hue);

    Cam16 answer;
    if (findCamByJ(hue, chroma, tone, answer)) {
        return answer.viewed(ViewingConditions::sRgb());
    }

    double low = 0.0;
    double high = chroma;
    bool found = false;
    while (high - low > 0.4) {
        const double mid = low + (high - low) / 2.0;
        Cam16 candidate;
        if (findCamByJ(hue, mid, tone, candidate)) {
            low = mid;
            answer = candidate;
            found = true;
        } else {
            high = mid;
        }
    }

    if (!found) {
        return argbFromLstar(tone);
    }
    return answer.viewed(ViewingConditions::sRgb());
}

Hct Hct::fromArgb(unsigned int argb)
{
    const Cam16 cam = Cam16::fromInt(argb);
    Hct hct;
    hct.hue = cam.hue;
    hct.chroma = cam.chroma;
    hct.tone = lstarFromArgb(argb);
    return hct;
}

unsigned int Hct::toArgb() const
{
    return solveToArgb(hue, chroma, tone);
}

double relativeLuminance(unsigned int argb)
{
    const double r = linearized(redFromArgb(argb)) / 100.0;
    const double g = linearized(greenFromArgb(argb)) / 100.0;
    const double b = linearized(blueFromArgb(argb)) / 100.0;
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

double contrastRatio(unsigned int argbA, unsigned int argbB)
{
    double l1 = relativeLuminance(argbA);
    double l2 = relativeLuminance(argbB);
    if (l1 < l2) {
        std::swap(l1, l2);
    }
    return (l1 + 0.05) / (l2 + 0.05);
}

// ---------------------------------------------------------------------------
// Tonal palettes and schemes
// ---------------------------------------------------------------------------

TonalPalette::TonalPalette(double hue, double chroma)
    : m_hue(hue), m_chroma(chroma)
{
}

TonalPalette TonalPalette::fromArgb(unsigned int argb)
{
    const Hct hct = Hct::fromArgb(argb);
    return TonalPalette(hct.hue, hct.chroma);
}

unsigned int TonalPalette::tone(double tone) const
{
    tone = clampDouble(0.0, 100.0, tone);
    const int key = static_cast<int>(std::lround(tone * 100.0));
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        return it->second;
    }
    const unsigned int argb = solveToArgb(m_hue, m_chroma, tone);
    m_cache[key] = argb;
    return argb;
}

Variant variantFromString(const QString& name, Variant fallback)
{
    const QString key = name.trimmed().toLower();
    if (key == "tonal_spot" || key == "tonalspot") {
        return Variant::TonalSpot;
    }
    if (key == "vibrant") {
        return Variant::Vibrant;
    }
    if (key == "expressive") {
        return Variant::Expressive;
    }
    if (key == "neutral") {
        return Variant::Neutral;
    }
    if (key == "monochrome") {
        return Variant::Monochrome;
    }
    if (key == "fidelity") {
        return Variant::Fidelity;
    }
    return fallback;
}

QString variantToString(Variant variant)
{
    switch (variant) {
    case Variant::TonalSpot: return QStringLiteral("tonal_spot");
    case Variant::Vibrant: return QStringLiteral("vibrant");
    case Variant::Expressive: return QStringLiteral("expressive");
    case Variant::Neutral: return QStringLiteral("neutral");
    case Variant::Monochrome: return QStringLiteral("monochrome");
    case Variant::Fidelity: return QStringLiteral("fidelity");
    }
    return QStringLiteral("tonal_spot");
}

Palettes buildPalettes(unsigned int seedArgb, Variant variant)
{
    const Hct source = Hct::fromArgb(seedArgb);
    const double h = source.hue;
    const double c = source.chroma;

    auto rotate = [](double hue, double rotation) {
        return sanitizeDegrees(hue + rotation);
    };

    Palettes p;
    p.error = TonalPalette(25.0, 84.0);

    switch (variant) {
    case Variant::TonalSpot:
        p.primary = TonalPalette(h, 36.0);
        p.secondary = TonalPalette(h, 16.0);
        p.tertiary = TonalPalette(rotate(h, 60.0), 24.0);
        p.neutral = TonalPalette(h, 6.0);
        p.neutralVariant = TonalPalette(h, 8.0);
        break;
    case Variant::Vibrant:
        p.primary = TonalPalette(h, 200.0);
        p.secondary = TonalPalette(rotate(h, 15.0), 24.0);
        p.tertiary = TonalPalette(rotate(h, 45.0), 32.0);
        p.neutral = TonalPalette(h, 10.0);
        p.neutralVariant = TonalPalette(h, 12.0);
        break;
    case Variant::Expressive:
        p.primary = TonalPalette(rotate(h, 240.0), 40.0);
        p.secondary = TonalPalette(rotate(h, 300.0), 24.0);
        p.tertiary = TonalPalette(rotate(h, 30.0), 32.0);
        p.neutral = TonalPalette(rotate(h, 15.0), 8.0);
        p.neutralVariant = TonalPalette(rotate(h, 15.0), 12.0);
        break;
    case Variant::Neutral:
        p.primary = TonalPalette(h, 12.0);
        p.secondary = TonalPalette(h, 8.0);
        p.tertiary = TonalPalette(h, 16.0);
        p.neutral = TonalPalette(h, 2.0);
        p.neutralVariant = TonalPalette(h, 2.0);
        break;
    case Variant::Monochrome:
        p.primary = TonalPalette(h, 0.0);
        p.secondary = TonalPalette(h, 0.0);
        p.tertiary = TonalPalette(h, 0.0);
        p.neutral = TonalPalette(h, 0.0);
        p.neutralVariant = TonalPalette(h, 0.0);
        break;
    case Variant::Fidelity:
        p.primary = TonalPalette(h, c);
        p.secondary = TonalPalette(h, std::max(c - 32.0, c * 0.5));
        p.tertiary = TonalPalette(rotate(h, 60.0), c * 0.75);
        p.neutral = TonalPalette(h, c / 8.0);
        p.neutralVariant = TonalPalette(h, c / 8.0 + 4.0);
        break;
    }
    return p;
}

namespace {
enum class PaletteKey { Primary, Secondary, Tertiary, Error, Neutral, NeutralVariant };

struct RoleTone
{
    const char* role;
    PaletteKey palette;
    double tones[4]; // light, dark, high contrast white, high contrast black
};

// Keep this table byte for byte in step with ROLE_TONES in
// buildscripts/tools/m3_hct.py.
const RoleTone kRoleTones[] = {
    { "primary", PaletteKey::Primary, { 40, 80, 25, 95 } },
    { "on_primary", PaletteKey::Primary, { 100, 20, 100, 10 } },
    { "primary_container", PaletteKey::Primary, { 90, 30, 80, 20 } },
    { "on_primary_container", PaletteKey::Primary, { 10, 90, 0, 100 } },
    { "inverse_primary", PaletteKey::Primary, { 80, 40, 90, 30 } },
    { "primary_fixed", PaletteKey::Primary, { 90, 90, 80, 90 } },
    { "primary_fixed_dim", PaletteKey::Primary, { 80, 80, 70, 80 } },
    { "on_primary_fixed", PaletteKey::Primary, { 10, 10, 0, 10 } },
    { "on_primary_fixed_variant", PaletteKey::Primary, { 30, 30, 20, 30 } },

    { "secondary", PaletteKey::Secondary, { 40, 80, 25, 95 } },
    { "on_secondary", PaletteKey::Secondary, { 100, 20, 100, 10 } },
    { "secondary_container", PaletteKey::Secondary, { 90, 30, 80, 20 } },
    { "on_secondary_container", PaletteKey::Secondary, { 10, 90, 0, 100 } },
    { "secondary_fixed", PaletteKey::Secondary, { 90, 90, 80, 90 } },
    { "secondary_fixed_dim", PaletteKey::Secondary, { 80, 80, 70, 80 } },
    { "on_secondary_fixed", PaletteKey::Secondary, { 10, 10, 0, 10 } },
    { "on_secondary_fixed_variant", PaletteKey::Secondary, { 30, 30, 20, 30 } },

    { "tertiary", PaletteKey::Tertiary, { 40, 80, 25, 95 } },
    { "on_tertiary", PaletteKey::Tertiary, { 100, 20, 100, 10 } },
    { "tertiary_container", PaletteKey::Tertiary, { 90, 30, 80, 20 } },
    { "on_tertiary_container", PaletteKey::Tertiary, { 10, 90, 0, 100 } },
    { "tertiary_fixed", PaletteKey::Tertiary, { 90, 90, 80, 90 } },
    { "tertiary_fixed_dim", PaletteKey::Tertiary, { 80, 80, 70, 80 } },
    { "on_tertiary_fixed", PaletteKey::Tertiary, { 10, 10, 0, 10 } },
    { "on_tertiary_fixed_variant", PaletteKey::Tertiary, { 30, 30, 20, 30 } },

    { "error", PaletteKey::Error, { 40, 80, 25, 95 } },
    { "on_error", PaletteKey::Error, { 100, 20, 100, 10 } },
    { "error_container", PaletteKey::Error, { 90, 30, 80, 20 } },
    { "on_error_container", PaletteKey::Error, { 10, 90, 0, 100 } },

    { "background", PaletteKey::Neutral, { 98, 6, 100, 0 } },
    { "on_background", PaletteKey::Neutral, { 10, 90, 0, 100 } },
    { "surface", PaletteKey::Neutral, { 98, 6, 100, 0 } },
    { "surface_dim", PaletteKey::Neutral, { 87, 6, 90, 0 } },
    { "surface_bright", PaletteKey::Neutral, { 98, 24, 100, 30 } },
    { "surface_container_lowest", PaletteKey::Neutral, { 100, 4, 100, 0 } },
    { "surface_container_low", PaletteKey::Neutral, { 96, 10, 98, 4 } },
    { "surface_container", PaletteKey::Neutral, { 94, 12, 96, 8 } },
    { "surface_container_high", PaletteKey::Neutral, { 92, 17, 94, 12 } },
    { "surface_container_highest", PaletteKey::Neutral, { 90, 22, 92, 17 } },
    { "on_surface", PaletteKey::Neutral, { 10, 90, 0, 100 } },
    { "surface_variant", PaletteKey::NeutralVariant, { 90, 30, 92, 20 } },
    { "on_surface_variant", PaletteKey::NeutralVariant, { 30, 80, 10, 95 } },
    { "outline", PaletteKey::NeutralVariant, { 50, 60, 25, 85 } },
    { "outline_variant", PaletteKey::NeutralVariant, { 80, 30, 50, 60 } },
    { "inverse_surface", PaletteKey::Neutral, { 20, 90, 10, 100 } },
    { "inverse_on_surface", PaletteKey::Neutral, { 95, 20, 100, 0 } },
    { "surface_tint", PaletteKey::Primary, { 40, 80, 25, 95 } },
    { "scrim", PaletteKey::Neutral, { 0, 0, 0, 0 } },
    { "shadow", PaletteKey::Neutral, { 0, 0, 0, 0 } },
};

const TonalPalette& paletteFor(const Palettes& p, PaletteKey key)
{
    switch (key) {
    case PaletteKey::Primary: return p.primary;
    case PaletteKey::Secondary: return p.secondary;
    case PaletteKey::Tertiary: return p.tertiary;
    case PaletteKey::Error: return p.error;
    case PaletteKey::Neutral: return p.neutral;
    case PaletteKey::NeutralVariant: return p.neutralVariant;
    }
    return p.neutral;
}
}

const std::vector<std::string>& roleNames()
{
    static const std::vector<std::string> names = []() {
        std::vector<std::string> result;
        result.reserve(sizeof(kRoleTones) / sizeof(kRoleTones[0]));
        for (const RoleTone& entry : kRoleTones) {
            result.emplace_back(entry.role);
        }
        return result;
    }();
    return names;
}

std::map<std::string, QColor> buildScheme(unsigned int seedArgb, Variant variant, SchemeKind kind)
{
    const Palettes palettes = buildPalettes(seedArgb, variant);
    const int index = static_cast<int>(kind);

    std::map<std::string, QColor> scheme;
    for (const RoleTone& entry : kRoleTones) {
        const unsigned int argb = paletteFor(palettes, entry.palette).tone(entry.tones[index]);
        scheme[entry.role] = QColor(redFromArgb(argb), greenFromArgb(argb), blueFromArgb(argb));
    }
    return scheme;
}
}

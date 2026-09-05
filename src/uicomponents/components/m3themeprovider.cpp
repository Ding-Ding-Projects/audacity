/*
* Audacity: A Digital Audio Editor
*/
#include "m3themeprovider.h"

#include <QFile>
#include <QGuiApplication>
#include <QSettings>
#include <QStyleHints>
#include <QTextStream>
#include <QVariant>

#include "settings.h"
#include "log.h"

using namespace au::uicomponents;
using namespace muse;

namespace {
const Settings::Key SEED_COLOR_KEY("ui", "ui/m3/seedColor");
const Settings::Key VARIANT_KEY("ui", "ui/m3/variant");
const Settings::Key REDUCED_MOTION_KEY("ui", "ui/m3/reducedMotion");
const Settings::Key DENSITY_KEY("ui", "ui/m3/density");

const char* DEFAULT_SEED = "#926BFF";
const char* DEFAULT_VARIANT = "tonal_spot";

QString snakeToCamel(const std::string& snake)
{
    QString out;
    bool upper = false;
    for (char ch : snake) {
        if (ch == '_') {
            upper = true;
            continue;
        }
        out.append(upper ? QChar(ch).toUpper() : QChar(ch));
        upper = false;
    }
    return out;
}
}

// ---------------------------------------------------------------------------
// Colours
// ---------------------------------------------------------------------------

M3ColorTokens::M3ColorTokens(QObject* parent)
    : QObject(parent)
{
}

QColor M3ColorTokens::role(const QString& name) const
{
    return m_scheme.value(name, QColor());
}

QStringList M3ColorTokens::roleNames() const
{
    QStringList names = m_scheme.keys();
    names.sort();
    return names;
}

void M3ColorTokens::setScheme(const QHash<QString, QColor>& scheme)
{
    if (m_scheme == scheme) {
        return;
    }
    m_scheme = scheme;
    emit changed();
}

// ---------------------------------------------------------------------------
// Typography
// ---------------------------------------------------------------------------

M3TypographyTokens::M3TypographyTokens(QObject* parent)
    : QObject(parent)
{
}

const M3TypographyTokens::Spec* M3TypographyTokens::spec(const QString& role) const
{
    // The Material 3 type scale: size, line height, tracking, weight.
    static const QHash<QString, Spec> specs = {
        { QStringLiteral("displayLarge"), { 57.0, 64.0, -0.25, QFont::Normal } },
        { QStringLiteral("displayMedium"), { 45.0, 52.0, 0.0, QFont::Normal } },
        { QStringLiteral("displaySmall"), { 36.0, 44.0, 0.0, QFont::Normal } },
        { QStringLiteral("headlineLarge"), { 32.0, 40.0, 0.0, QFont::Normal } },
        { QStringLiteral("headlineMedium"), { 28.0, 36.0, 0.0, QFont::Normal } },
        { QStringLiteral("headlineSmall"), { 24.0, 32.0, 0.0, QFont::Normal } },
        { QStringLiteral("titleLarge"), { 22.0, 28.0, 0.0, QFont::Normal } },
        { QStringLiteral("titleMedium"), { 16.0, 24.0, 0.15, QFont::Medium } },
        { QStringLiteral("titleSmall"), { 14.0, 20.0, 0.1, QFont::Medium } },
        { QStringLiteral("bodyLarge"), { 16.0, 24.0, 0.5, QFont::Normal } },
        { QStringLiteral("bodyMedium"), { 14.0, 20.0, 0.25, QFont::Normal } },
        { QStringLiteral("bodySmall"), { 12.0, 16.0, 0.4, QFont::Normal } },
        { QStringLiteral("labelLarge"), { 14.0, 20.0, 0.1, QFont::Medium } },
        { QStringLiteral("labelMedium"), { 12.0, 16.0, 0.5, QFont::Medium } },
        { QStringLiteral("labelSmall"), { 11.0, 16.0, 0.5, QFont::Medium } },
    };

    auto it = specs.find(role);
    return it == specs.end() ? nullptr : &it.value();
}

QFont M3TypographyTokens::font(const QString& role) const
{
    const Spec* s = spec(role);
    QFont font;
    if (!m_family.isEmpty()) {
        font.setFamily(m_family);
    }
    if (!s) {
        return font;
    }
    font.setPixelSize(qRound(s->size * m_scale));
    font.setWeight(static_cast<QFont::Weight>(s->weight));
    font.setLetterSpacing(QFont::AbsoluteSpacing, s->letterSpacing);
    return font;
}

qreal M3TypographyTokens::lineHeight(const QString& role) const
{
    const Spec* s = spec(role);
    return s ? s->lineHeight * m_scale : 0.0;
}

qreal M3TypographyTokens::letterSpacing(const QString& role) const
{
    const Spec* s = spec(role);
    return s ? s->letterSpacing : 0.0;
}

QStringList M3TypographyTokens::roleNames() const
{
    return {
        QStringLiteral("displayLarge"), QStringLiteral("displayMedium"), QStringLiteral("displaySmall"),
        QStringLiteral("headlineLarge"), QStringLiteral("headlineMedium"), QStringLiteral("headlineSmall"),
        QStringLiteral("titleLarge"), QStringLiteral("titleMedium"), QStringLiteral("titleSmall"),
        QStringLiteral("bodyLarge"), QStringLiteral("bodyMedium"), QStringLiteral("bodySmall"),
        QStringLiteral("labelLarge"), QStringLiteral("labelMedium"), QStringLiteral("labelSmall")
    };
}

void M3TypographyTokens::setFamily(const QString& family)
{
    if (m_family == family) {
        return;
    }
    m_family = family;
    emit changed();
}

void M3TypographyTokens::setScale(qreal scale)
{
    if (qFuzzyCompare(m_scale, scale)) {
        return;
    }
    m_scale = scale;
    emit changed();
}

// ---------------------------------------------------------------------------
// Shape, state layers, elevation, density
// ---------------------------------------------------------------------------

M3ShapeTokens::M3ShapeTokens(QObject* parent)
    : QObject(parent)
{
}

M3StateLayerTokens::M3StateLayerTokens(QObject* parent)
    : QObject(parent)
{
}

M3ElevationTokens::M3ElevationTokens(QObject* parent)
    : QObject(parent)
{
}

qreal M3ElevationTokens::dp(int level) const
{
    static const qreal levels[6] = { 0.0, 1.0, 3.0, 6.0, 8.0, 12.0 };
    return levels[qBound(0, level, 5)];
}

qreal M3ElevationTokens::keyBlur(int level) const
{
    static const qreal blurs[6] = { 0.0, 3.0, 6.0, 8.0, 10.0, 14.0 };
    return blurs[qBound(0, level, 5)];
}

qreal M3ElevationTokens::keyOffset(int level) const
{
    static const qreal offsets[6] = { 0.0, 1.0, 1.0, 2.0, 2.0, 4.0 };
    return offsets[qBound(0, level, 5)];
}

qreal M3ElevationTokens::ambientBlur(int level) const
{
    static const qreal blurs[6] = { 0.0, 2.0, 4.0, 6.0, 8.0, 12.0 };
    return blurs[qBound(0, level, 5)];
}

qreal M3ElevationTokens::tintOpacity(int level) const
{
    static const qreal opacities[6] = { 0.0, 0.05, 0.08, 0.11, 0.12, 0.14 };
    return opacities[qBound(0, level, 5)];
}

M3DensityTokens::M3DensityTokens(QObject* parent)
    : QObject(parent)
{
}

void M3DensityTokens::setLevel(int level)
{
    level = qBound(-3, level, 0);
    if (m_level == level) {
        return;
    }
    m_level = level;
    emit changed();
}

qreal M3DensityTokens::apply(qreal baseHeight) const
{
    return qMax(24.0, baseHeight + m_level * 4.0);
}

// ---------------------------------------------------------------------------
// Motion
// ---------------------------------------------------------------------------

M3MotionTokens::M3MotionTokens(QObject* parent)
    : QObject(parent)
{
}

void M3MotionTokens::setReducedMotion(bool value)
{
    if (m_reducedMotion == value) {
        return;
    }
    m_reducedMotion = value;
    emit changed();
}

namespace {
QEasingCurve bezierCurve(qreal x1, qreal y1, qreal x2, qreal y2)
{
    QEasingCurve curve(QEasingCurve::BezierSpline);
    curve.addCubicBezierSegment(QPointF(x1, y1), QPointF(x2, y2), QPointF(1.0, 1.0));
    return curve;
}

QVariantList bezierList(qreal x1, qreal y1, qreal x2, qreal y2)
{
    return { x1, y1, x2, y2, 1.0, 1.0 };
}
}

QEasingCurve M3MotionTokens::standard() const { return bezierCurve(0.2, 0.0, 0.0, 1.0); }
QEasingCurve M3MotionTokens::standardAccelerate() const { return bezierCurve(0.3, 0.0, 1.0, 1.0); }
QEasingCurve M3MotionTokens::standardDecelerate() const { return bezierCurve(0.0, 0.0, 0.0, 1.0); }
QEasingCurve M3MotionTokens::emphasized() const { return bezierCurve(0.2, 0.0, 0.0, 1.0); }
QEasingCurve M3MotionTokens::emphasizedAccelerate() const { return bezierCurve(0.3, 0.0, 0.8, 0.15); }
QEasingCurve M3MotionTokens::emphasizedDecelerate() const { return bezierCurve(0.05, 0.7, 0.1, 1.0); }

QVariantList M3MotionTokens::standardBezier() const { return bezierList(0.2, 0.0, 0.0, 1.0); }
QVariantList M3MotionTokens::standardAccelerateBezier() const { return bezierList(0.3, 0.0, 1.0, 1.0); }
QVariantList M3MotionTokens::standardDecelerateBezier() const { return bezierList(0.0, 0.0, 0.0, 1.0); }
QVariantList M3MotionTokens::emphasizedBezier() const { return bezierList(0.2, 0.0, 0.0, 1.0); }
QVariantList M3MotionTokens::emphasizedAccelerateBezier() const { return bezierList(0.3, 0.0, 0.8, 0.15); }
QVariantList M3MotionTokens::emphasizedDecelerateBezier() const { return bezierList(0.05, 0.7, 0.1, 1.0); }

qreal M3MotionTokens::travel(qreal distance) const
{
    return m_reducedMotion ? 0.0 : distance;
}

// ---------------------------------------------------------------------------
// Provider
// ---------------------------------------------------------------------------

M3ThemeProvider::M3ThemeProvider(QObject* parent)
    : QObject(parent)
{
    m_color = new M3ColorTokens(this);
    m_typography = new M3TypographyTokens(this);
    m_shape = new M3ShapeTokens(this);
    m_motion = new M3MotionTokens(this);
    m_stateLayer = new M3StateLayerTokens(this);
    m_elevation = new M3ElevationTokens(this);
    m_density = new M3DensityTokens(this);

    m_seed = QColor(QString::fromLatin1(DEFAULT_SEED));
}

void M3ThemeProvider::init()
{
    if (m_inited) {
        return;
    }
    m_inited = true;

    settings()->setDefaultValue(SEED_COLOR_KEY, Val(std::string(DEFAULT_SEED)));
    settings()->setDefaultValue(VARIANT_KEY, Val(std::string(DEFAULT_VARIANT)));
    settings()->setDefaultValue(REDUCED_MOTION_KEY, Val(false));
    settings()->setDefaultValue(DENSITY_KEY, Val(0));

    settings()->valueChanged(SEED_COLOR_KEY).onReceive(this, [this](const Val&) { rebuild(); });
    settings()->valueChanged(VARIANT_KEY).onReceive(this, [this](const Val&) { rebuild(); });
    settings()->valueChanged(REDUCED_MOTION_KEY).onReceive(this, [this](const Val&) { rebuild(); });
    settings()->valueChanged(DENSITY_KEY).onReceive(this, [this](const Val&) { rebuild(); });

    uiConfiguration()->currentThemeChanged().onNotify(this, [this]() { rebuild(); });
    uiConfiguration()->fontChanged().onNotify(this, [this]() { rebuild(); });

    rebuild();
}

m3::SchemeKind M3ThemeProvider::currentSchemeKind() const
{
    const muse::ui::ThemeCode code = uiConfiguration()->currentTheme().codeKey;
    if (code == muse::ui::HIGH_CONTRAST_WHITE_THEME_CODE) {
        return m3::SchemeKind::HighContrastWhite;
    }
    if (code == muse::ui::HIGH_CONTRAST_BLACK_THEME_CODE) {
        return m3::SchemeKind::HighContrastBlack;
    }
    if (code == muse::ui::DARK_THEME_CODE) {
        return m3::SchemeKind::Dark;
    }
    return m3::SchemeKind::Light;
}

void M3ThemeProvider::rebuild()
{
    const QString seedText = QString::fromStdString(settings()->value(SEED_COLOR_KEY).toString());
    QColor seed(seedText);
    if (!seed.isValid()) {
        seed = QColor(QString::fromLatin1(DEFAULT_SEED));
    }
    m_seed = seed;

    m_variant = m3::variantFromString(
        QString::fromStdString(settings()->value(VARIANT_KEY).toString()));

    const unsigned int seedArgb = static_cast<unsigned int>(m_seed.rgb()) | 0xFF000000u;
    const std::map<std::string, QColor> scheme
        = m3::buildScheme(seedArgb, m_variant, currentSchemeKind());

    QHash<QString, QColor> camel;
    camel.reserve(static_cast<int>(scheme.size()));
    for (const auto& entry : scheme) {
        camel.insert(snakeToCamel(entry.first), entry.second);
    }
    m_color->setScheme(camel);

    m_typography->setFamily(QString::fromStdString(uiConfiguration()->fontFamily()));

    const bool reduced = settings()->value(REDUCED_MOTION_KEY).toBool() || detectReducedMotion();
    m_motion->setReducedMotion(reduced);

    m_density->setLevel(settings()->value(DENSITY_KEY).toInt());

    emit themeChanged();
}

QColor M3ThemeProvider::seedColor() const
{
    return m_seed;
}

void M3ThemeProvider::setSeedColor(const QColor& color)
{
    if (!color.isValid() || color == m_seed) {
        return;
    }
    settings()->setSharedValue(SEED_COLOR_KEY, Val(color.name().toStdString()));
    if (!m_inited) {
        m_seed = color;
        emit themeChanged();
    }
}

QString M3ThemeProvider::variant() const
{
    return m3::variantToString(m_variant);
}

void M3ThemeProvider::setVariant(const QString& variant)
{
    const m3::Variant parsed = m3::variantFromString(variant, m_variant);
    if (parsed == m_variant) {
        return;
    }
    settings()->setSharedValue(VARIANT_KEY, Val(m3::variantToString(parsed).toStdString()));
    if (!m_inited) {
        m_variant = parsed;
        emit themeChanged();
    }
}

QStringList M3ThemeProvider::variants() const
{
    return { QStringLiteral("tonal_spot"), QStringLiteral("vibrant"), QStringLiteral("expressive"),
             QStringLiteral("neutral"), QStringLiteral("monochrome"), QStringLiteral("fidelity") };
}

bool M3ThemeProvider::isDark() const
{
    const m3::SchemeKind kind = currentSchemeKind();
    return kind == m3::SchemeKind::Dark || kind == m3::SchemeKind::HighContrastBlack;
}

bool M3ThemeProvider::isHighContrast() const
{
    const m3::SchemeKind kind = currentSchemeKind();
    return kind == m3::SchemeKind::HighContrastWhite || kind == m3::SchemeKind::HighContrastBlack;
}

QString M3ThemeProvider::schemeName() const
{
    switch (currentSchemeKind()) {
    case m3::SchemeKind::Light: return QStringLiteral("light");
    case m3::SchemeKind::Dark: return QStringLiteral("dark");
    case m3::SchemeKind::HighContrastWhite: return QStringLiteral("high_contrast_white");
    case m3::SchemeKind::HighContrastBlack: return QStringLiteral("high_contrast_black");
    }
    return QStringLiteral("light");
}

QColor M3ThemeProvider::blend(const QColor& over, const QColor& base, qreal opacity) const
{
    if (!over.isValid() || !base.isValid()) {
        return base;
    }
    const qreal a = qBound(0.0, opacity, 1.0);
    return QColor::fromRgbF(base.redF() * (1.0 - a) + over.redF() * a,
                            base.greenF() * (1.0 - a) + over.greenF() * a,
                            base.blueF() * (1.0 - a) + over.blueF() * a);
}

QColor M3ThemeProvider::surfaceAt(int elevationLevel) const
{
    const QColor base = m_color->surface();
    if (elevationLevel <= 0) {
        return base;
    }
    return blend(m_color->surfaceTint(), base, m_elevation->tintOpacity(elevationLevel));
}

qreal M3ThemeProvider::contrastRatio(const QColor& a, const QColor& b) const
{
    if (!a.isValid() || !b.isValid()) {
        return 1.0;
    }
    const unsigned int argbA = static_cast<unsigned int>(a.rgb()) | 0xFF000000u;
    const unsigned int argbB = static_cast<unsigned int>(b.rgb()) | 0xFF000000u;
    return m3::contrastRatio(argbA, argbB);
}

QString M3ThemeProvider::captureRoute() const
{
    return QString::fromLocal8Bit(qgetenv("AU_M3_GALLERY_ROUTE"));
}

bool M3ThemeProvider::detectReducedMotion()
{
    if (qEnvironmentVariableIntValue("QT_M3_REDUCED_MOTION") == 1) {
        return true;
    }

    // Qt may gain a first class hint for this. Read it reflectively so that the
    // build keeps working on the Qt versions that do not have it yet.
    if (QGuiApplication::styleHints()) {
        const QVariant hint = QGuiApplication::styleHints()->property("reducedMotion");
        if (hint.isValid() && hint.toBool()) {
            return true;
        }
    }

#ifdef Q_OS_LINUX
    if (qEnvironmentVariableIsSet("GTK_ENABLE_ANIMATIONS")
        && qEnvironmentVariableIntValue("GTK_ENABLE_ANIMATIONS") == 0) {
        return true;
    }

    // Fall back to the GTK settings file, which is what the desktop portal
    // writes when the user turns animations off.
    const QString configHome = qEnvironmentVariableIsSet("XDG_CONFIG_HOME")
                               ? QString::fromLocal8Bit(qgetenv("XDG_CONFIG_HOME"))
                               : QString::fromLocal8Bit(qgetenv("HOME")) + QStringLiteral("/.config");

    for (const QString& relative : { QStringLiteral("/gtk-4.0/settings.ini"),
                                     QStringLiteral("/gtk-3.0/settings.ini") }) {
        const QString path = configHome + relative;
        if (!QFile::exists(path)) {
            continue;
        }
        QSettings gtk(path, QSettings::IniFormat);
        const QVariant value = gtk.value(QStringLiteral("Settings/gtk-enable-animations"));
        if (value.isValid()) {
            return !value.toBool();
        }
    }
#endif

    return false;
}

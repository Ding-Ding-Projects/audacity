/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QHash>
#include <QObject>
#include <QVariantList>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "ui/iuiconfiguration.h"

#include "m3hct.h"

namespace au::uicomponents {
/*!
 * Convenience macro for a read only colour role.
 * The QML name is camelCase, the lookup key is the same string.
 */
#define M3_COLOR_ROLE(NAME) \
    Q_PROPERTY(QColor NAME READ NAME NOTIFY changed) \
public: \
    QColor NAME() const { return role(QStringLiteral(#NAME)); } \
private:

/*!
 * Every Material 3 system colour role for the theme that is currently active.
 * Reached from QML as M3.color.primary and so on.
 */
class M3ColorTokens : public QObject
{
    Q_OBJECT

    M3_COLOR_ROLE(primary)
    M3_COLOR_ROLE(onPrimary)
    M3_COLOR_ROLE(primaryContainer)
    M3_COLOR_ROLE(onPrimaryContainer)
    M3_COLOR_ROLE(inversePrimary)
    M3_COLOR_ROLE(primaryFixed)
    M3_COLOR_ROLE(primaryFixedDim)
    M3_COLOR_ROLE(onPrimaryFixed)
    M3_COLOR_ROLE(onPrimaryFixedVariant)

    M3_COLOR_ROLE(secondary)
    M3_COLOR_ROLE(onSecondary)
    M3_COLOR_ROLE(secondaryContainer)
    M3_COLOR_ROLE(onSecondaryContainer)
    M3_COLOR_ROLE(secondaryFixed)
    M3_COLOR_ROLE(secondaryFixedDim)
    M3_COLOR_ROLE(onSecondaryFixed)
    M3_COLOR_ROLE(onSecondaryFixedVariant)

    M3_COLOR_ROLE(tertiary)
    M3_COLOR_ROLE(onTertiary)
    M3_COLOR_ROLE(tertiaryContainer)
    M3_COLOR_ROLE(onTertiaryContainer)
    M3_COLOR_ROLE(tertiaryFixed)
    M3_COLOR_ROLE(tertiaryFixedDim)
    M3_COLOR_ROLE(onTertiaryFixed)
    M3_COLOR_ROLE(onTertiaryFixedVariant)

    M3_COLOR_ROLE(error)
    M3_COLOR_ROLE(onError)
    M3_COLOR_ROLE(errorContainer)
    M3_COLOR_ROLE(onErrorContainer)

    M3_COLOR_ROLE(background)
    M3_COLOR_ROLE(onBackground)
    M3_COLOR_ROLE(surface)
    M3_COLOR_ROLE(surfaceDim)
    M3_COLOR_ROLE(surfaceBright)
    M3_COLOR_ROLE(surfaceContainerLowest)
    M3_COLOR_ROLE(surfaceContainerLow)
    M3_COLOR_ROLE(surfaceContainer)
    M3_COLOR_ROLE(surfaceContainerHigh)
    M3_COLOR_ROLE(surfaceContainerHighest)
    M3_COLOR_ROLE(onSurface)
    M3_COLOR_ROLE(surfaceVariant)
    M3_COLOR_ROLE(onSurfaceVariant)
    M3_COLOR_ROLE(outline)
    M3_COLOR_ROLE(outlineVariant)
    M3_COLOR_ROLE(inverseSurface)
    M3_COLOR_ROLE(inverseOnSurface)
    M3_COLOR_ROLE(surfaceTint)
    M3_COLOR_ROLE(scrim)
    M3_COLOR_ROLE(shadow)

public:
    explicit M3ColorTokens(QObject* parent = nullptr);

    /*!
     * Every role, keyed by its camelCase name.
     *
     * Use this from QML when the role is chosen at run time, for example
     * \c {M3.color.roles[name]}. It notifies on a theme change, so bindings
     * that read it stay correct, which a plain method call cannot do.
     */
    Q_PROPERTY(QVariantMap roles READ roles NOTIFY changed)

    //! Look up any role by its camelCase name. Returns an invalid colour if unknown.
    //! Not reactive, so prefer the roles map in a binding.
    Q_INVOKABLE QColor role(const QString& name) const;

    QVariantMap roles() const;

    //! All known role names, useful for the developer gallery.
    Q_INVOKABLE QStringList roleNames() const;

    void setScheme(const QHash<QString, QColor>& scheme);

signals:
    void changed();

private:
    QHash<QString, QColor> m_scheme;
};

#undef M3_COLOR_ROLE

/*!
 * The Material 3 type scale. Every role is a ready to use QFont, with the
 * matching line height in device independent pixels and tracking in pixels.
 */
class M3TypographyTokens : public QObject
{
    Q_OBJECT

#define M3_TYPE_ROLE(NAME) \
    Q_PROPERTY(QFont NAME READ NAME NOTIFY changed) \
    Q_PROPERTY(qreal NAME##LineHeight READ NAME##LineHeight NOTIFY changed) \
    Q_PROPERTY(qreal NAME##LetterSpacing READ NAME##LetterSpacing NOTIFY changed) \
public: \
    QFont NAME() const { return font(QStringLiteral(#NAME)); \
    } \
    qreal NAME##LineHeight() const { return lineHeight(QStringLiteral(#NAME)); } \
    qreal NAME##LetterSpacing() const { return letterSpacing(QStringLiteral(#NAME)); } \
private:

    M3_TYPE_ROLE(displayLarge)
    M3_TYPE_ROLE(displayMedium)
    M3_TYPE_ROLE(displaySmall)
    M3_TYPE_ROLE(headlineLarge)
    M3_TYPE_ROLE(headlineMedium)
    M3_TYPE_ROLE(headlineSmall)
    M3_TYPE_ROLE(titleLarge)
    M3_TYPE_ROLE(titleMedium)
    M3_TYPE_ROLE(titleSmall)
    M3_TYPE_ROLE(bodyLarge)
    M3_TYPE_ROLE(bodyMedium)
    M3_TYPE_ROLE(bodySmall)
    M3_TYPE_ROLE(labelLarge)
    M3_TYPE_ROLE(labelMedium)
    M3_TYPE_ROLE(labelSmall)

#undef M3_TYPE_ROLE

public:
    explicit M3TypographyTokens(QObject* parent = nullptr);

    Q_INVOKABLE QFont font(const QString& role) const;
    Q_INVOKABLE qreal lineHeight(const QString& role) const;
    Q_INVOKABLE qreal letterSpacing(const QString& role) const;
    Q_INVOKABLE QStringList roleNames() const;

    void setFamily(const QString& family);
    void setScale(qreal scale);

signals:
    void changed();

private:
    struct Spec {
        qreal size = 14.0;
        qreal lineHeight = 20.0;
        qreal letterSpacing = 0.0;
        int weight = QFont::Normal;
    };

    const Spec* spec(const QString& role) const;

    QString m_family;
    qreal m_scale = 1.0;
};

/*!
 * Material 3 corner radii in device independent pixels. "full" is a large
 * number that any component can clamp to half of its own height.
 */
class M3ShapeTokens : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal none READ none CONSTANT)
    Q_PROPERTY(qreal extraSmall READ extraSmall CONSTANT)
    Q_PROPERTY(qreal small READ small CONSTANT)
    Q_PROPERTY(qreal medium READ medium CONSTANT)
    Q_PROPERTY(qreal large READ large CONSTANT)
    Q_PROPERTY(qreal extraLarge READ extraLarge CONSTANT)
    Q_PROPERTY(qreal full READ full CONSTANT)

public:
    explicit M3ShapeTokens(QObject* parent = nullptr);

    qreal none() const { return 0.0; }
    qreal extraSmall() const { return 4.0; }
    qreal small() const { return 8.0; }
    qreal medium() const { return 12.0; }
    qreal large() const { return 16.0; }
    qreal extraLarge() const { return 28.0; }
    qreal full() const { return 9999.0; }
};

/*!
 * Material 3 motion. Every duration reports zero while reduced motion is on,
 * so a plain Behavior or NumberAnimation needs no extra guard.
 */
class M3MotionTokens : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool reducedMotion READ reducedMotion NOTIFY changed)

#define M3_DURATION(NAME, VALUE) \
    Q_PROPERTY(int NAME READ NAME NOTIFY changed) \
public: \
    int NAME() const { return m_reducedMotion ? 0 : VALUE; \
    } \
private:

    M3_DURATION(short1, 50)
    M3_DURATION(short2, 100)
    M3_DURATION(short3, 150)
    M3_DURATION(short4, 200)
    M3_DURATION(medium1, 250)
    M3_DURATION(medium2, 300)
    M3_DURATION(medium3, 350)
    M3_DURATION(medium4, 400)
    M3_DURATION(long1, 450)
    M3_DURATION(long2, 500)
    M3_DURATION(long3, 550)
    M3_DURATION(long4, 600)
    M3_DURATION(extraLong1, 700)
    M3_DURATION(extraLong2, 800)
    M3_DURATION(extraLong3, 900)
    M3_DURATION(extraLong4, 1000)

#undef M3_DURATION

    Q_PROPERTY(QEasingCurve standard READ standard CONSTANT)
    Q_PROPERTY(QEasingCurve standardAccelerate READ standardAccelerate CONSTANT)
    Q_PROPERTY(QEasingCurve standardDecelerate READ standardDecelerate CONSTANT)
    Q_PROPERTY(QEasingCurve emphasized READ emphasized CONSTANT)
    Q_PROPERTY(QEasingCurve emphasizedAccelerate READ emphasizedAccelerate CONSTANT)
    Q_PROPERTY(QEasingCurve emphasizedDecelerate READ emphasizedDecelerate CONSTANT)

    Q_PROPERTY(QVariantList standardBezier READ standardBezier CONSTANT)
    Q_PROPERTY(QVariantList standardAccelerateBezier READ standardAccelerateBezier CONSTANT)
    Q_PROPERTY(QVariantList standardDecelerateBezier READ standardDecelerateBezier CONSTANT)
    Q_PROPERTY(QVariantList emphasizedBezier READ emphasizedBezier CONSTANT)
    Q_PROPERTY(QVariantList emphasizedAccelerateBezier READ emphasizedAccelerateBezier CONSTANT)
    Q_PROPERTY(QVariantList emphasizedDecelerateBezier READ emphasizedDecelerateBezier CONSTANT)

public:
    explicit M3MotionTokens(QObject* parent = nullptr);

    bool reducedMotion() const { return m_reducedMotion; }
    void setReducedMotion(bool value);

    QEasingCurve standard() const;
    QEasingCurve standardAccelerate() const;
    QEasingCurve standardDecelerate() const;
    QEasingCurve emphasized() const;
    QEasingCurve emphasizedAccelerate() const;
    QEasingCurve emphasizedDecelerate() const;

    QVariantList standardBezier() const;
    QVariantList standardAccelerateBezier() const;
    QVariantList standardDecelerateBezier() const;
    QVariantList emphasizedBezier() const;
    QVariantList emphasizedAccelerateBezier() const;
    QVariantList emphasizedDecelerateBezier() const;

    /*!
     * The distance a decorative element should travel. Returns zero while
     * reduced motion is on so that transitions become plain cross fades.
     */
    Q_INVOKABLE qreal travel(qreal distance) const;

signals:
    void changed();

private:
    bool m_reducedMotion = false;
};

//! Material 3 state layer opacities.
class M3StateLayerTokens : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal hover READ hover CONSTANT)
    Q_PROPERTY(qreal focus READ focus CONSTANT)
    Q_PROPERTY(qreal pressed READ pressed CONSTANT)
    Q_PROPERTY(qreal dragged READ dragged CONSTANT)
    Q_PROPERTY(qreal disabledContent READ disabledContent CONSTANT)
    Q_PROPERTY(qreal disabledContainer READ disabledContainer CONSTANT)

public:
    explicit M3StateLayerTokens(QObject* parent = nullptr);

    qreal hover() const { return 0.08; }
    qreal focus() const { return 0.10; }
    qreal pressed() const { return 0.10; }
    qreal dragged() const { return 0.16; }
    qreal disabledContent() const { return 0.38; }
    qreal disabledContainer() const { return 0.12; }
};

/*!
 * Material 3 elevation. Each level carries a resting height in device
 * independent pixels plus the two shadow layers Material draws for it.
 */
class M3ElevationTokens : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal level0 READ level0 CONSTANT)
    Q_PROPERTY(qreal level1 READ level1 CONSTANT)
    Q_PROPERTY(qreal level2 READ level2 CONSTANT)
    Q_PROPERTY(qreal level3 READ level3 CONSTANT)
    Q_PROPERTY(qreal level4 READ level4 CONSTANT)
    Q_PROPERTY(qreal level5 READ level5 CONSTANT)

public:
    explicit M3ElevationTokens(QObject* parent = nullptr);

    qreal level0() const { return 0.0; }
    qreal level1() const { return 1.0; }
    qreal level2() const { return 3.0; }
    qreal level3() const { return 6.0; }
    qreal level4() const { return 8.0; }
    qreal level5() const { return 12.0; }

    //! Resting height in device independent pixels for a level between 0 and 5.
    Q_INVOKABLE qreal dp(int level) const;
    //! Key shadow blur radius for a level.
    Q_INVOKABLE qreal keyBlur(int level) const;
    //! Key shadow vertical offset for a level.
    Q_INVOKABLE qreal keyOffset(int level) const;
    //! Ambient shadow blur radius for a level.
    Q_INVOKABLE qreal ambientBlur(int level) const;
    //! Opacity of the tonal surface tint overlay for a level.
    Q_INVOKABLE qreal tintOpacity(int level) const;
};

/*!
 * Material 3 density. Level 0 is the comfortable default and each step down to
 * -3 removes four device independent pixels from the height of a control.
 */
class M3DensityTokens : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int level READ level NOTIFY changed)
    Q_PROPERTY(qreal step READ step CONSTANT)
    Q_PROPERTY(qreal offset READ offset NOTIFY changed)

public:
    explicit M3DensityTokens(QObject* parent = nullptr);

    int level() const { return m_level; }
    void setLevel(int level);

    qreal step() const { return 4.0; }
    qreal offset() const { return m_level * 4.0; }

    //! Apply the current density to a base height, never going below 24.
    Q_INVOKABLE qreal apply(qreal baseHeight) const;

signals:
    void changed();

private:
    int m_level = 0;
};

/*!
 * The Material 3 token engine.
 *
 * Reached from QML as the singleton M3 in the module Audacity.M3:
 *
 *     import Audacity.M3
 *     Rectangle { color: M3.color.surfaceContainer; radius: M3.shape.medium }
 *
 * The active scheme follows the muse theme selection, so the existing light,
 * dark and high contrast switching keeps working. The seed colour, scheme
 * variant, reduced motion preference and density are persisted through muse
 * settings under the keys ui/m3/seedColor, ui/m3/variant, ui/m3/reducedMotion
 * and ui/m3/density.
 */
class M3ThemeProvider : public QObject, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(M3ColorTokens * color READ color CONSTANT)
    Q_PROPERTY(M3TypographyTokens * typography READ typography CONSTANT)
    Q_PROPERTY(M3ShapeTokens * shape READ shape CONSTANT)
    Q_PROPERTY(M3MotionTokens * motion READ motion CONSTANT)
    Q_PROPERTY(M3StateLayerTokens * stateLayer READ stateLayer CONSTANT)
    Q_PROPERTY(M3ElevationTokens * elevation READ elevation CONSTANT)
    Q_PROPERTY(M3DensityTokens * density READ density CONSTANT)

    Q_PROPERTY(QColor seedColor READ seedColor WRITE setSeedColor NOTIFY themeChanged)
    Q_PROPERTY(QString variant READ variant WRITE setVariant NOTIFY themeChanged)
    Q_PROPERTY(QStringList variants READ variants CONSTANT)
    Q_PROPERTY(bool isDark READ isDark NOTIFY themeChanged)
    Q_PROPERTY(bool isHighContrast READ isHighContrast NOTIFY themeChanged)
    Q_PROPERTY(QString schemeName READ schemeName NOTIFY themeChanged)

    //! The focus indicator is three pixels wide and sits two pixels outside the shape.
    Q_PROPERTY(qreal focusIndicatorThickness READ focusIndicatorThickness CONSTANT)
    Q_PROPERTY(qreal focusIndicatorOffset READ focusIndicatorOffset CONSTANT)

    muse::GlobalInject<muse::ui::IUiConfiguration> uiConfiguration;

public:
    explicit M3ThemeProvider(QObject* parent = nullptr);

    //! Called once during module start up, before QML is loaded.
    void init();

    M3ColorTokens* color() const { return m_color; }
    M3TypographyTokens* typography() const { return m_typography; }
    M3ShapeTokens* shape() const { return m_shape; }
    M3MotionTokens* motion() const { return m_motion; }
    M3StateLayerTokens* stateLayer() const { return m_stateLayer; }
    M3ElevationTokens* elevation() const { return m_elevation; }
    M3DensityTokens* density() const { return m_density; }

    QColor seedColor() const;
    void setSeedColor(const QColor& color);

    QString variant() const;
    void setVariant(const QString& variant);

    QStringList variants() const;

    bool isDark() const;
    bool isHighContrast() const;
    QString schemeName() const;

    qreal focusIndicatorThickness() const { return 3.0; }
    qreal focusIndicatorOffset() const { return 2.0; }

    /*!
     * Blend a colour over a surface at the given opacity. Used for state
     * layers and for the tonal surface tint at each elevation level.
     */
    Q_INVOKABLE QColor blend(const QColor& over, const QColor& base, qreal opacity) const;

    //! The tonal surface colour for an elevation level, tinted with surfaceTint.
    Q_INVOKABLE QColor surfaceAt(int elevationLevel) const;

    //! WCAG 2.x contrast ratio between two colours, between 1.0 and 21.0.
    Q_INVOKABLE qreal contrastRatio(const QColor& a, const QColor& b) const;

    //! True if the platform or the user asked for reduced motion.
    static bool detectReducedMotion();

    /*!
     * The value of AU_M3_GALLERY_ROUTE, or an empty string when it is not set.
     *
     * The developer gallery uses it to select one component, state, theme and
     * scale so that a capture harness gets the same frame every time. QML
     * cannot read the environment on its own, so it is surfaced here.
     */
    Q_INVOKABLE QString captureRoute() const;

    /*!
     * Switch the application to the theme a capture route asked for.
     *
     * \a name is one of light, dark, high_contrast_white or
     * high_contrast_black. Anything else is ignored and false is returned, so
     * a route with no theme part leaves the user's own theme alone. The
     * scheme follows the framework theme, so this writes the framework
     * setting rather than holding a scheme of its own.
     */
    Q_INVOKABLE bool applyScheme(const QString& name);

signals:
    void themeChanged();

private:
    void rebuild();
    m3::SchemeKind currentSchemeKind() const;

    M3ColorTokens* m_color = nullptr;
    M3TypographyTokens* m_typography = nullptr;
    M3ShapeTokens* m_shape = nullptr;
    M3MotionTokens* m_motion = nullptr;
    M3StateLayerTokens* m_stateLayer = nullptr;
    M3ElevationTokens* m_elevation = nullptr;
    M3DensityTokens* m_density = nullptr;

    QColor m_seed;
    m3::Variant m_variant = m3::Variant::TonalSpot;
    bool m_inited = false;
};
}

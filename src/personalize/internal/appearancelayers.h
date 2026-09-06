/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>

namespace au::personalize {
/*!
 * \brief The per element, per state layered style model.
 *
 * Where AppearanceOverrides holds one flat set of properties per element and
 * state (a colour, a radius, a typeface), AppearanceLayers holds an ordered
 * stack of layers on top of that: fills (solid, gradient, pattern or image),
 * strokes, shadows and glows, blur and backdrop effects, tonal adjustments,
 * transforms and masks, each with its own blend mode and opacity, and each
 * itself carrying an optional override per interaction state (normal, hover,
 * focus, pressed, selected, disabled, dragged, error, loading, success,
 * warning). A state with no layers of its own inherits the normal stack.
 *
 * Every layer is one QVariantMap with at least:
 *   id (string, stable), type (string), name (string), visible (bool),
 *   locked (bool), opacity (real 0..1), blendMode (string),
 *   properties (a QVariantMap whose shape depends on type)
 *
 * Layer types: "fill", "stroke", "shadow", "glow", "blur", "adjustment",
 * "transform", "mask".
 *
 * The whole document is persisted as versioned JSON in the application data
 * directory, in the same "personalize" subdirectory AppearanceOverrides
 * uses, under a separate file. Nothing here ever leaves the local machine.
 */
class AppearanceLayers : public QObject
{
    Q_OBJECT

public:
    explicit AppearanceLayers(QObject* parent = nullptr);

    static constexpr int SCHEMA_VERSION = 1;

    Q_INVOKABLE QVariantList layers(const QString& elementId, const QString& state = QString()) const;
    Q_INVOKABLE void setLayers(const QString& elementId, const QString& state, const QVariantList& layers);
    //! True when the given state has its own explicit stack rather than
    //! inheriting the normal one.
    Q_INVOKABLE bool hasOwnState(const QString& elementId, const QString& state) const;
    Q_INVOKABLE void clearState(const QString& elementId, const QString& state);

    Q_INVOKABLE QString addLayer(const QString& elementId, const QString& state, const QString& type);
    Q_INVOKABLE void removeLayer(const QString& elementId, const QString& state, const QString& layerId);
    Q_INVOKABLE void duplicateLayer(const QString& elementId, const QString& state, const QString& layerId);
    Q_INVOKABLE void moveLayer(const QString& elementId, const QString& state, const QString& layerId, int newIndex);
    Q_INVOKABLE void setLayerVisible(const QString& elementId, const QString& state, const QString& layerId, bool visible);
    Q_INVOKABLE void setLayerLocked(const QString& elementId, const QString& state, const QString& layerId, bool locked);
    Q_INVOKABLE void renameLayer(const QString& elementId, const QString& state, const QString& layerId, const QString& name);
    Q_INVOKABLE void setLayerBlendMode(const QString& elementId, const QString& state, const QString& layerId,
                                       const QString& blendMode);
    Q_INVOKABLE void setLayerOpacity(const QString& elementId, const QString& state, const QString& layerId, qreal opacity);
    Q_INVOKABLE void setLayerProperty(const QString& elementId, const QString& state, const QString& layerId,
                                      const QString& property, const QVariant& value);

    Q_INVOKABLE void resetElement(const QString& elementId);
    Q_INVOKABLE void resetAll();

    //! The exact blend modes the renderer can actually draw, in order. Any
    //! other name is accepted and stored but rendered as "normal" with a
    //! visible capability note in the editor.
    Q_INVOKABLE QStringList supportedBlendModes() const;
    Q_INVOKABLE QStringList allBlendModes() const;

    Q_INVOKABLE QString exportElement(const QString& elementId) const;
    Q_INVOKABLE bool importElement(const QString& elementId, const QString& json);

    //! One-way pull of a legacy flat AppearanceOverrides colour/radius pair
    //! into a starter "Fill" and "Stroke" layer, so an element already
    //! customised through the old editor keeps looking the same the first
    //! time it is opened in the layered one. Never overwrites an element
    //! that already has layers of its own.
    Q_INVOKABLE bool migrateFromFlatColor(const QString& elementId, const QColor& fillColor, qreal radius);

signals:
    void layersChanged(const QString& elementId, const QString& state);

private:
    QString storePath() const;
    void load();
    void save() const;
    QVariantMap elementDoc(const QString& elementId) const;
    void setElementDoc(const QString& elementId, const QVariantMap& doc);
    QVariantList layerStack(const QVariantMap& doc, const QString& state, bool& ownState) const;

    QVariantMap m_elements;
};
}

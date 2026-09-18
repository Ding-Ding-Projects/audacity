/*
* Audacity: A Digital Audio Editor
*/

#include "appearancelayers.h"

#include <functional>

#include <QColor>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

using namespace au::personalize;

namespace {
const QString NORMAL_STATE = QStringLiteral("normal");

QString newLayerId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QVariantMap defaultLayerProperties(const QString& type)
{
    QVariantMap p;
    if (type == "fill") {
        p["kind"] = "solid";
        p["color"] = QColor(0x60, 0x60, 0x60).name(QColor::HexArgb);
        p["gradientStops"] = QVariantList();
        p["gradientAngle"] = 0;
        p["imagePath"] = QString();
    } else if (type == "stroke") {
        p["color"] = QColor(0x00, 0x00, 0x00, 0x80).name(QColor::HexArgb);
        p["width"] = 1.0;
    } else if (type == "shadow" || type == "glow") {
        p["color"] = QColor(0x00, 0x00, 0x00, 0x80).name(QColor::HexArgb);
        p["offsetX"] = 0.0;
        p["offsetY"] = type == "shadow" ? 2.0 : 0.0;
        p["blurRadius"] = 8.0;
        p["spread"] = 0.0;
        p["inner"] = false;
    } else if (type == "blur") {
        p["radius"] = 8.0;
        p["backdrop"] = false;
    } else if (type == "adjustment") {
        p["brightness"] = 0.0;
        p["contrast"] = 0.0;
        p["saturation"] = 0.0;
        p["hue"] = 0.0;
        p["colorizeColor"] = QString();
    } else if (type == "transform") {
        p["translateX"] = 0.0;
        p["translateY"] = 0.0;
        p["rotation"] = 0.0;
        p["scaleX"] = 1.0;
        p["scaleY"] = 1.0;
        p["skewX"] = 0.0;
        p["originX"] = 0.5;
        p["originY"] = 0.5;
    } else if (type == "mask") {
        p["shape"] = "rectangle";
        p["radius"] = 0.0;
        p["path"] = QVariantList();
    }
    return p;
}

QVariantMap newLayer(const QString& type)
{
    QVariantMap layer;
    layer["id"] = newLayerId();
    layer["type"] = type;
    layer["name"] = type;
    layer["visible"] = true;
    layer["locked"] = false;
    layer["opacity"] = 1.0;
    layer["blendMode"] = "normal";
    layer["properties"] = defaultLayerProperties(type);
    return layer;
}
} // namespace

AppearanceLayers::AppearanceLayers(QObject* parent)
    : QObject(parent)
{
    load();
}

QString AppearanceLayers::storePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/personalize";
    QDir().mkpath(dir);
    return dir + "/appearance-layers.json";
}

void AppearanceLayers::load()
{
    QFile file(storePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }
    QJsonObject root = doc.object();
    // A future schema bump would migrate here before assigning; version 1
    // is the only shape written so far.
    m_elements = root.value("elements").toObject().toVariantMap();
}

void AppearanceLayers::save() const
{
    QJsonObject root;
    root["schemaVersion"] = SCHEMA_VERSION;
    root["elements"] = QJsonObject::fromVariantMap(m_elements);

    QFile file(storePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QVariantMap AppearanceLayers::elementDoc(const QString& elementId) const
{
    return m_elements.value(elementId).toMap();
}

void AppearanceLayers::setElementDoc(const QString& elementId, const QVariantMap& doc)
{
    m_elements[elementId] = doc;
    save();
}

QVariantList AppearanceLayers::layerStack(const QVariantMap& doc, const QString& state, bool& ownState) const
{
    QVariantMap states = doc.value("states").toMap();
    QString key = state.isEmpty() ? NORMAL_STATE : state;
    if (states.contains(key)) {
        ownState = true;
        return states.value(key).toList();
    }
    ownState = false;
    return states.value(NORMAL_STATE).toList();
}

QVariantList AppearanceLayers::layers(const QString& elementId, const QString& state) const
{
    bool ownState = false;
    return layerStack(elementDoc(elementId), state, ownState);
}

bool AppearanceLayers::hasOwnState(const QString& elementId, const QString& state) const
{
    bool ownState = false;
    layerStack(elementDoc(elementId), state, ownState);
    return ownState;
}

void AppearanceLayers::clearState(const QString& elementId, const QString& state)
{
    QVariantMap doc = elementDoc(elementId);
    QVariantMap states = doc.value("states").toMap();
    QString key = state.isEmpty() ? NORMAL_STATE : state;
    if (key != NORMAL_STATE) {
        states.remove(key);
        doc["states"] = states;
        setElementDoc(elementId, doc);
    }
    emit layersChanged(elementId, state);
}

void AppearanceLayers::setLayers(const QString& elementId, const QString& state, const QVariantList& newLayers)
{
    QVariantMap doc = elementDoc(elementId);
    QVariantMap states = doc.value("states").toMap();
    QString key = state.isEmpty() ? NORMAL_STATE : state;
    states[key] = newLayers;
    doc["states"] = states;
    setElementDoc(elementId, doc);
    emit layersChanged(elementId, state);
}

QString AppearanceLayers::addLayer(const QString& elementId, const QString& state, const QString& type)
{
    QVariantList stack = layers(elementId, state);
    QVariantMap layer = newLayer(type);
    stack.append(layer);
    setLayers(elementId, state, stack);
    return layer.value("id").toString();
}

void AppearanceLayers::removeLayer(const QString& elementId, const QString& state, const QString& layerId)
{
    QVariantList stack = layers(elementId, state);
    for (int i = 0; i < stack.size(); ++i) {
        if (stack.at(i).toMap().value("id").toString() == layerId) {
            stack.removeAt(i);
            break;
        }
    }
    setLayers(elementId, state, stack);
}

void AppearanceLayers::duplicateLayer(const QString& elementId, const QString& state, const QString& layerId)
{
    QVariantList stack = layers(elementId, state);
    for (int i = 0; i < stack.size(); ++i) {
        QVariantMap layer = stack.at(i).toMap();
        if (layer.value("id").toString() == layerId) {
            QVariantMap copy = layer;
            copy["id"] = newLayerId();
            copy["name"] = layer.value("name").toString() + " copy";
            stack.insert(i + 1, copy);
            break;
        }
    }
    setLayers(elementId, state, stack);
}

void AppearanceLayers::moveLayer(const QString& elementId, const QString& state, const QString& layerId, int newIndex)
{
    QVariantList stack = layers(elementId, state);
    int from = -1;
    for (int i = 0; i < stack.size(); ++i) {
        if (stack.at(i).toMap().value("id").toString() == layerId) {
            from = i;
            break;
        }
    }
    if (from < 0) {
        return;
    }
    newIndex = qBound(0, newIndex, stack.size() - 1);
    QVariant layer = stack.takeAt(from);
    stack.insert(newIndex, layer);
    setLayers(elementId, state, stack);
}

static QVariantList mutateLayer(const QVariantList& stack, const QString& layerId,
                                 const std::function<void (QVariantMap&)>& mutate)
{
    QVariantList result = stack;
    for (int i = 0; i < result.size(); ++i) {
        QVariantMap layer = result.at(i).toMap();
        if (layer.value("id").toString() == layerId) {
            mutate(layer);
            result[i] = layer;
            break;
        }
    }
    return result;
}

void AppearanceLayers::setLayerVisible(const QString& elementId, const QString& state, const QString& layerId, bool visible)
{
    QVariantList stack = mutateLayer(layers(elementId, state), layerId, [visible](QVariantMap& l) { l["visible"] = visible; });
    setLayers(elementId, state, stack);
}

void AppearanceLayers::setLayerLocked(const QString& elementId, const QString& state, const QString& layerId, bool locked)
{
    QVariantList stack = mutateLayer(layers(elementId, state), layerId, [locked](QVariantMap& l) { l["locked"] = locked; });
    setLayers(elementId, state, stack);
}

void AppearanceLayers::renameLayer(const QString& elementId, const QString& state, const QString& layerId, const QString& name)
{
    QVariantList stack = mutateLayer(layers(elementId, state), layerId, [name](QVariantMap& l) { l["name"] = name; });
    setLayers(elementId, state, stack);
}

void AppearanceLayers::setLayerBlendMode(const QString& elementId, const QString& state, const QString& layerId,
                                         const QString& blendMode)
{
    QVariantList stack = mutateLayer(layers(elementId, state), layerId,
                                      [blendMode](QVariantMap& l) { l["blendMode"] = blendMode; });
    setLayers(elementId, state, stack);
}

void AppearanceLayers::setLayerOpacity(const QString& elementId, const QString& state, const QString& layerId, qreal opacity)
{
    opacity = qBound(0.0, opacity, 1.0);
    QVariantList stack = mutateLayer(layers(elementId, state), layerId,
                                      [opacity](QVariantMap& l) { l["opacity"] = opacity; });
    setLayers(elementId, state, stack);
}

void AppearanceLayers::setLayerProperty(const QString& elementId, const QString& state, const QString& layerId,
                                        const QString& property, const QVariant& value)
{
    QVariantList stack = mutateLayer(layers(elementId, state), layerId, [&](QVariantMap& l) {
        QVariantMap props = l.value("properties").toMap();
        props[property] = value;
        l["properties"] = props;
    });
    setLayers(elementId, state, stack);
}

void AppearanceLayers::resetElement(const QString& elementId)
{
    m_elements.remove(elementId);
    save();
    emit layersChanged(elementId, QString());
}

void AppearanceLayers::resetAll()
{
    m_elements.clear();
    save();
    emit layersChanged(QString(), QString());
}

QStringList AppearanceLayers::supportedBlendModes() const
{
    // Every mode QtQuick MultiEffect and the fallback blend shader in
    // M3AppearanceLayers.qml can actually draw. Kept in sync with that file
    // by the docs/features/appearance-editor.md capability matrix and by the
    // layer model tests.
    return { "normal", "multiply", "screen", "overlay", "darken", "lighten" };
}

QStringList AppearanceLayers::allBlendModes() const
{
    // The remaining names are accepted and stored (so a document imported
    // from elsewhere is never rejected) but render as "normal" with a
    // capability note in the editor: colorDodge, colorBurn, hardLight,
    // softLight, difference, exclusion, hue, saturation, color, luminosity.
    QStringList all = supportedBlendModes();
    all += { "colorDodge", "colorBurn", "hardLight", "softLight", "difference", "exclusion",
             "hue", "saturation", "color", "luminosity" };
    return all;
}

QString AppearanceLayers::exportElement(const QString& elementId) const
{
    QJsonObject root;
    root["schemaVersion"] = SCHEMA_VERSION;
    root["elementId"] = elementId;
    root["document"] = QJsonObject::fromVariantMap(elementDoc(elementId));
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool AppearanceLayers::importElement(const QString& elementId, const QString& json)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }
    QJsonObject root = doc.object();
    setElementDoc(elementId, root.value("document").toObject().toVariantMap());
    emit layersChanged(elementId, QString());
    return true;
}

bool AppearanceLayers::migrateFromFlatColor(const QString& elementId, const QColor& fillColor, qreal radius)
{
    if (m_elements.contains(elementId)) {
        return false;
    }
    QVariantMap fill = newLayer("fill");
    QVariantMap fillProps = fill.value("properties").toMap();
    fillProps["color"] = fillColor.name(QColor::HexArgb);
    fill["properties"] = fillProps;
    fill["name"] = "Migrated fill";

    QVariantList stack;
    stack.append(fill);
    if (radius > 0) {
        QVariantMap mask = newLayer("mask");
        QVariantMap maskProps = mask.value("properties").toMap();
        maskProps["shape"] = "rounded";
        maskProps["radius"] = radius;
        mask["properties"] = maskProps;
        mask["name"] = "Migrated radius";
        stack.append(mask);
    }
    setLayers(elementId, QString(), stack);
    return true;
}

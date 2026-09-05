/*
* Audacity: A Digital Audio Editor
*/

#include "appearanceoverrides.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

using namespace au::personalize;

AppearanceOverrides::AppearanceOverrides(QObject* parent)
    : QObject(parent)
{
    load();
}

QString AppearanceOverrides::storePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/personalize";
    QDir().mkpath(dir);
    return dir + "/appearance-overrides.json";
}

void AppearanceOverrides::load()
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
    m_elements = root.value("elements").toObject().toVariantMap();
    m_presets = root.value("presets").toObject().toVariantMap();
    m_rainbowSpeedLevel = root.value("rainbowSpeedLevel").toInt(3);
}

void AppearanceOverrides::save() const
{
    QJsonObject root;
    root["elements"] = QJsonObject::fromVariantMap(m_elements);
    root["presets"] = QJsonObject::fromVariantMap(m_presets);
    root["rainbowSpeedLevel"] = m_rainbowSpeedLevel;

    QFile file(storePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

static QString stateKey(const QString& property, const QString& state)
{
    return state.isEmpty() ? property : property + "@" + state;
}

void AppearanceOverrides::setProperty(const QString& elementId, const QString& property, const QVariant& value,
                                      const QString& state)
{
    QVariantMap element = m_elements.value(elementId).toMap();
    element[stateKey(property, state)] = value;
    m_elements[elementId] = element;
    save();
    emit elementChanged(elementId);
}

QVariant AppearanceOverrides::getProperty(const QString& elementId, const QString& property, const QString& state) const
{
    QVariantMap element = m_elements.value(elementId).toMap();
    QVariant value = element.value(stateKey(property, state));
    if (!value.isValid() && !state.isEmpty()) {
        // A state with no override of its own inherits the normal one.
        value = element.value(stateKey(property, QString()));
    }
    return value;
}

bool AppearanceOverrides::hasProperty(const QString& elementId, const QString& property, const QString& state) const
{
    return getProperty(elementId, property, state).isValid();
}

QVariant AppearanceOverrides::resolve(const QString& elementId, const QString& state, const QString& property,
                                      const QVariant& fallback) const
{
    if (elementId.isEmpty()) {
        return fallback;
    }
    QVariant value = getProperty(elementId, property, state);
    return value.isValid() ? value : fallback;
}

void AppearanceOverrides::resetProperty(const QString& elementId, const QString& property, const QString& state)
{
    QVariantMap element = m_elements.value(elementId).toMap();
    element.remove(stateKey(property, state));
    m_elements[elementId] = element;
    save();
    emit elementChanged(elementId);
}

void AppearanceOverrides::resetElement(const QString& elementId)
{
    m_elements.remove(elementId);
    save();
    emit elementChanged(elementId);
}

void AppearanceOverrides::resetAll()
{
    m_elements.clear();
    save();
    emit elementChanged(QString());
}

QStringList AppearanceOverrides::installedFontFamilies() const
{
    QStringList families = QFontDatabase::families();
    families.sort(Qt::CaseInsensitive);
    return families;
}

QStringList AppearanceOverrides::presetNames() const
{
    QStringList names = m_presets.keys();
    names.sort(Qt::CaseInsensitive);
    return names;
}

void AppearanceOverrides::savePreset(const QString& name, const QString& elementId)
{
    if (name.isEmpty()) {
        return;
    }
    m_presets[name] = m_elements.value(elementId);
    save();
}

void AppearanceOverrides::applyPreset(const QString& name, const QString& elementId)
{
    if (!m_presets.contains(name)) {
        return;
    }
    m_elements[elementId] = m_presets.value(name);
    save();
    emit elementChanged(elementId);
}

void AppearanceOverrides::deletePreset(const QString& name)
{
    m_presets.remove(name);
    save();
}

QString AppearanceOverrides::copyStyle(const QString& elementId) const
{
    QJsonDocument doc = QJsonDocument::fromVariant(m_elements.value(elementId));
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void AppearanceOverrides::pasteStyle(const QString& elementId, const QString& styleJson)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(styleJson.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        return;
    }
    m_elements[elementId] = doc.toVariant();
    save();
    emit elementChanged(elementId);
}

QString AppearanceOverrides::exportAll() const
{
    QJsonObject root;
    root["elements"] = QJsonObject::fromVariantMap(m_elements);
    root["presets"] = QJsonObject::fromVariantMap(m_presets);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool AppearanceOverrides::importAll(const QString& json)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }
    QJsonObject root = doc.object();
    m_elements = root.value("elements").toObject().toVariantMap();
    m_presets = root.value("presets").toObject().toVariantMap();
    save();
    emit elementChanged(QString());
    return true;
}

int AppearanceOverrides::rainbowSpeedLevel() const
{
    return m_rainbowSpeedLevel;
}

void AppearanceOverrides::setRainbowSpeedLevel(int level)
{
    level = qBound(1, level, 5);
    if (m_rainbowSpeedLevel == level) {
        return;
    }
    m_rainbowSpeedLevel = level;
    save();
    emit rainbowSpeedLevelChanged();
}

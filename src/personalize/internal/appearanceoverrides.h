/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QFontDatabase>
#include <QObject>
#include <QVariantMap>
#include <QStringList>

namespace au::personalize {
/*!
 * \brief The per element appearance override store.
 *
 * Every override is addressed by an element identifier chosen by the QML
 * that owns the element, a property name, and an optional state name
 * ("normal", "hover", "focus", "pressed", "selected", "disabled"). A state
 * left unset falls back to "normal" for that same element and property, so a
 * hover colour can be set without also having to repeat the normal one.
 *
 * Overrides are persisted as a single JSON document under the application's
 * user data directory, in a "personalize" subdirectory, and are read back on
 * construction. Nothing here ever leaves the local machine.
 */
class AppearanceOverrides : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int rainbowSpeedLevel READ rainbowSpeedLevel WRITE setRainbowSpeedLevel NOTIFY rainbowSpeedLevelChanged)

public:
    explicit AppearanceOverrides(QObject* parent = nullptr);

    Q_INVOKABLE void setProperty(const QString& elementId, const QString& property, const QVariant& value,
                                  const QString& state = QString());
    Q_INVOKABLE QVariant getProperty(const QString& elementId, const QString& property,
                                      const QString& state = QString()) const;
    Q_INVOKABLE bool hasProperty(const QString& elementId, const QString& property, const QString& state = QString()) const;

    Q_INVOKABLE void resetProperty(const QString& elementId, const QString& property, const QString& state = QString());
    Q_INVOKABLE void resetElement(const QString& elementId);
    Q_INVOKABLE void resetAll();

    Q_INVOKABLE QStringList installedFontFamilies() const;

    Q_INVOKABLE QStringList presetNames() const;
    Q_INVOKABLE void savePreset(const QString& name, const QString& elementId);
    Q_INVOKABLE void applyPreset(const QString& name, const QString& elementId);
    Q_INVOKABLE void deletePreset(const QString& name);

    Q_INVOKABLE QString copyStyle(const QString& elementId) const;
    Q_INVOKABLE void pasteStyle(const QString& elementId, const QString& styleJson);

    Q_INVOKABLE QString exportAll() const;
    Q_INVOKABLE bool importAll(const QString& json);

    int rainbowSpeedLevel() const;
    void setRainbowSpeedLevel(int level);

    QString appDisplayName() const;
    void setAppDisplayName(const QString& name);

signals:
    void elementChanged(const QString& elementId);
    void rainbowSpeedLevelChanged();
    void appDisplayNameChanged();

private:
    QString storePath() const;
    void load();
    void save() const;

    QVariantMap m_elements;
    QVariantMap m_presets;
    int m_rainbowSpeedLevel = 3;
};
}

/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>

namespace au::personalize {
//! The default display name shown by an unrenamed build.
extern const char* DEFAULT_APP_DISPLAY_NAME;

/*!
 * \brief Lets the person using the application rename it, in its own copy
 * only.
 *
 * The rename changes what the title bar, the About surface and the
 * notifications say. It never touches the application data directory, the
 * package or installer identity, or the update feed, all of which stay
 * derived from the constant \c DEFAULT_APP_DISPLAY_NAME rather than from this
 * setting. A crash report or other diagnostic sent for the developers should
 * keep saying the real name, not the one the person chose.
 */
class DisplayNameSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString defaultDisplayName READ defaultDisplayName CONSTANT)

public:
    explicit DisplayNameSettings(QObject* parent = nullptr);

    QString displayName() const;
    void setDisplayName(const QString& name);

    QString defaultDisplayName() const;

    Q_INVOKABLE void resetToDefault();

signals:
    void displayNameChanged();

private:
    QString storePath() const;
    QString m_displayName;
};
}

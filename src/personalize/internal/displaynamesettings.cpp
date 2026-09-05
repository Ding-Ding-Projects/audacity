/*
* Audacity: A Digital Audio Editor
*/

#include "displaynamesettings.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

using namespace au::personalize;

const char* au::personalize::DEFAULT_APP_DISPLAY_NAME = "Material Audacity";

DisplayNameSettings::DisplayNameSettings(QObject* parent)
    : QObject(parent)
{
    QFile file(storePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString stored = QString::fromUtf8(file.readAll()).trimmed();
        if (!stored.isEmpty()) {
            m_displayName = stored;
        }
    }
    if (m_displayName.isEmpty()) {
        m_displayName = QString::fromUtf8(DEFAULT_APP_DISPLAY_NAME);
    }
}

QString DisplayNameSettings::storePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/personalize";
    QDir().mkpath(dir);
    return dir + "/display-name.txt";
}

QString DisplayNameSettings::displayName() const
{
    return m_displayName;
}

void DisplayNameSettings::setDisplayName(const QString& name)
{
    QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        trimmed = QString::fromUtf8(DEFAULT_APP_DISPLAY_NAME);
    }
    if (m_displayName == trimmed) {
        return;
    }
    m_displayName = trimmed;

    QFile file(storePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream out(&file);
        out << m_displayName;
    }

    emit displayNameChanged();
}

QString DisplayNameSettings::defaultDisplayName() const
{
    return QString::fromUtf8(DEFAULT_APP_DISPLAY_NAME);
}

void DisplayNameSettings::resetToDefault()
{
    setDisplayName(QString::fromUtf8(DEFAULT_APP_DISPLAY_NAME));
}

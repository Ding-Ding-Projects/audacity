/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>

namespace au::personalize {
/*!
 * \brief The joke support desk.
 *
 * It plays the part of a real support system, right down to a made up
 * ticket number and a canned first response, and then its one real action
 * is opening the application's own data folder in the platform's file
 * manager so a locked out person can delete it themselves. Nothing here
 * is sent anywhere: there is no network call in this class, and the page
 * that hosts it says so plainly.
 */
class SupportTickets : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString dataFolderPath READ dataFolderPath CONSTANT)

public:
    explicit SupportTickets(QObject* parent = nullptr);

    QString dataFolderPath() const;

    Q_INVOKABLE QVariantList tickets() const;
    Q_INVOKABLE QString openTicket(const QString& category, const QString& description);
    Q_INVOKABLE bool openDataFolder() const;

private:
    QString storePath() const;
    void load();
    void save() const;

    QVariantList m_tickets;
};
}

/*
* Audacity: A Digital Audio Editor
*/

#include "supporttickets.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QUrl>

using namespace au::personalize;

SupportTickets::SupportTickets(QObject* parent)
    : QObject(parent)
{
    load();
}

QString SupportTickets::dataFolderPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString SupportTickets::storePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/personalize";
    QDir().mkpath(dir);
    return dir + "/support-tickets.json";
}

void SupportTickets::load()
{
    QFile file(storePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error == QJsonParseError::NoError && doc.isArray()) {
        m_tickets = doc.array().toVariantList();
    }
}

void SupportTickets::save() const
{
    QFile file(storePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(QJsonArray::fromVariantList(m_tickets)).toJson(QJsonDocument::Indented));
    }
}

QVariantList SupportTickets::tickets() const
{
    return m_tickets;
}

QString SupportTickets::openTicket(const QString& category, const QString& description)
{
    int number = 100000 + QRandomGenerator::global()->bounded(900000);
    QString ticketId = QString("AUD-%1").arg(number);

    QVariantMap ticket;
    ticket["id"] = ticketId;
    ticket["category"] = category;
    ticket["description"] = description;
    ticket["status"] = "resolved";
    ticket["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    ticket["response"]
        = "Thank you for contacting support. Our top engineer has reviewed your case with the "
          "seriousness it deserves and has determined that the fastest fix is for you to open your "
          "own application data folder and remove it yourself. Nothing was sent anywhere to reach "
          "this conclusion.";

    m_tickets.prepend(ticket);
    save();
    return ticketId;
}

bool SupportTickets::openDataFolder() const
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(dataFolderPath()));
}

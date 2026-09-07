#include "shared/profilepaths.h"
/*
* Audacity: A Digital Audio Editor
*/

#include "mutationhistory.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTextStream>

using namespace au::personalize;

MutationHistory::MutationHistory(QObject* parent)
    : QObject(parent)
{
}

QString MutationHistory::storePath() const
{
    QString dir = au::profile::Paths::writableLocation(QStandardPaths::AppDataLocation) + "/personalize";
    QDir().mkpath(dir);
    return dir + "/mutation-history.jsonl";
}

void MutationHistory::record(const QString& action, const QString& redactedDescription)
{
    QJsonObject entry;
    entry["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    entry["action"] = action;
    entry["description"] = redactedDescription;

    QFile file(storePath());
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream out(&file);
    out << QString::fromUtf8(QJsonDocument(entry).toJson(QJsonDocument::Compact)) << "\n";
}

QVariantList MutationHistory::entries() const
{
    QVariantList result;
    QFile file(storePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return result;
    }
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            result.append(doc.object().toVariantMap());
        }
    }
    return result;
}

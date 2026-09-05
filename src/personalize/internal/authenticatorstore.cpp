/*
* Audacity: A Digital Audio Editor
*/

#include "authenticatorstore.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>

using namespace au::personalize;

AuthenticatorStore::AuthenticatorStore()
{
}

QString AuthenticatorStore::storePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/personalize";
    QDir().mkpath(dir);
    return dir + "/authenticator.dat";
}

QByteArray AuthenticatorStore::obscureKey() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/personalize";
    QDir().mkpath(dir);
    QString keyPath = dir + "/authenticator.key";

    QFile keyFile(keyPath);
    if (keyFile.exists() && keyFile.open(QIODevice::ReadOnly)) {
        QByteArray key = keyFile.readAll();
        if (!key.isEmpty()) {
            return key;
        }
    }

    QByteArray newKey(32, '\0');
    for (int i = 0; i < newKey.size(); ++i) {
        newKey[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
    }

    QFile writeFile(keyPath);
    if (writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        writeFile.write(newKey);
        writeFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
    return newKey;
}

QByteArray AuthenticatorStore::xorWithKeystream(const QByteArray& data, const QByteArray& key) const
{
    QByteArray result(data.size(), '\0');
    QByteArray keystream;
    quint32 counter = 0;
    int keystreamOffset = 0;

    for (int i = 0; i < data.size(); ++i) {
        if (keystreamOffset >= keystream.size()) {
            QByteArray block = key;
            block.append(reinterpret_cast<const char*>(&counter), sizeof(counter));
            keystream = QCryptographicHash::hash(block, QCryptographicHash::Sha256);
            ++counter;
            keystreamOffset = 0;
        }
        result[i] = static_cast<char>(data.at(i) ^ keystream.at(keystreamOffset));
        ++keystreamOffset;
    }
    return result;
}

QList<AuthenticatorEntry> AuthenticatorStore::load() const
{
    QList<AuthenticatorEntry> result;

    QFile file(storePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return result;
    }
    QByteArray obscured = file.readAll();
    if (obscured.isEmpty()) {
        return result;
    }

    QByteArray json = xorWithKeystream(obscured, obscureKey());
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        return result;
    }

    for (const QJsonValue& value : doc.array()) {
        QJsonObject obj = value.toObject();
        AuthenticatorEntry entry;
        entry.id = obj.value("id").toString();
        entry.issuer = obj.value("issuer").toString();
        entry.account = obj.value("account").toString();
        entry.secret = QByteArray::fromBase64(obj.value("secret").toString().toUtf8());
        entry.digits = obj.value("digits").toInt(6);
        entry.periodSeconds = obj.value("period").toInt(30);
        entry.algorithm = obj.value("algorithm").toInt(0);
        result.append(entry);
    }
    return result;
}

void AuthenticatorStore::save(const QList<AuthenticatorEntry>& entries) const
{
    QJsonArray array;
    for (const AuthenticatorEntry& entry : entries) {
        QJsonObject obj;
        obj["id"] = entry.id;
        obj["issuer"] = entry.issuer;
        obj["account"] = entry.account;
        obj["secret"] = QString::fromUtf8(entry.secret.toBase64());
        obj["digits"] = entry.digits;
        obj["period"] = entry.periodSeconds;
        obj["algorithm"] = entry.algorithm;
        array.append(obj);
    }

    QByteArray json = QJsonDocument(array).toJson(QJsonDocument::Compact);
    QByteArray obscured = xorWithKeystream(json, obscureKey());

    QFile file(storePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(obscured);
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
}

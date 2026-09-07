#include "shared/profilepaths.h"
/*
 * Audacity: A Digital Audio Editor
 */
#include "schoolmode.h"

#include <random>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

using namespace au::experience;

namespace {
QString bytesToHex(const QByteArray& bytes)
{
    return QString::fromLatin1(bytes.toHex());
}
}

SchoolModeStore::ParseResult SchoolModeStore::parse(const QByteArray& json)
{
    ParseResult result;

    if (json.isEmpty()) {
        // An absent or empty file simply means the mode has never been
        // configured on this machine. That is not an error.
        result.ok = true;
        result.record = SchoolModeRecord();
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.error = QStringLiteral("The shared School mode record is not valid JSON.");
        return result;
    }

    const QJsonObject obj = doc.object();

    SchoolModeRecord record;
    record.on = obj.value(QStringLiteral("on")).toBool(false);
    record.displayName = obj.value(QStringLiteral("displayName")).toString(QStringLiteral("School mode"));
    record.credentialHashHex = obj.value(QStringLiteral("credentialHashHex")).toString();
    record.credentialSaltHex = obj.value(QStringLiteral("credentialSaltHex")).toString();

    if (!record.isValid()) {
        result.error = QStringLiteral("The shared School mode record has an empty display name.");
        return result;
    }

    result.ok = true;
    result.record = record;
    return result;
}

QByteArray SchoolModeStore::serialize(const SchoolModeRecord& record)
{
    QJsonObject obj;
    obj[QStringLiteral("on")] = record.on;
    obj[QStringLiteral("displayName")] = record.displayName;
    obj[QStringLiteral("credentialHashHex")] = record.credentialHashHex;
    obj[QStringLiteral("credentialSaltHex")] = record.credentialSaltHex;
    return QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

QString SchoolModeStore::newSaltHex()
{
    static thread_local std::mt19937 engine(std::random_device {}());
    static thread_local std::uniform_int_distribution<int> dist(0, 255);

    QByteArray salt;
    salt.resize(16);
    for (int i = 0; i < salt.size(); ++i) {
        salt[i] = static_cast<char>(dist(engine));
    }
    return bytesToHex(salt);
}

QString SchoolModeStore::hashCredential(const QString& credential, const QString& saltHex)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(QByteArray::fromHex(saltHex.toLatin1()));
    hash.addData(credential.toUtf8());
    return bytesToHex(hash.result());
}

bool SchoolModeStore::verifyCredential(const QString& candidate, const QString& saltHex, const QString& storedHashHex)
{
    if (storedHashHex.isEmpty()) {
        return false;
    }
    return hashCredential(candidate, saltHex) == storedHashHex;
}

QString SchoolModeStore::sharedFilePath()
{
    const QString appData = au::profile::Paths::writableLocation(QStandardPaths::AppDataLocation);
    // appData is normally "<parent>/<OrgName>/<AppName>" (or similar); walk
    // up one level to the shared parent every one of this user's apps sits
    // under, then into a "shared" subdirectory.
    QDir dir(appData);
    dir.cdUp();
    return dir.absoluteFilePath(QStringLiteral("shared/school-mode.json"));
}

SchoolModeService::SchoolModeService(QObject* parent)
    : QObject(parent), m_watcher(new QFileSystemWatcher(this))
{
    reload();

    const QString path = SchoolModeStore::sharedFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (QFile::exists(path)) {
        m_watcher->addPath(path);
    }
    m_watcher->addPath(QFileInfo(path).absolutePath());

    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &SchoolModeService::onFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &SchoolModeService::onFileChanged);
}

void SchoolModeService::reload()
{
    QFile file(SchoolModeStore::sharedFilePath());
    QByteArray data;
    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
    }

    const SchoolModeStore::ParseResult result = SchoolModeStore::parse(data);
    if (result.ok) {
        m_record = result.record;
    }
}

void SchoolModeService::save()
{
    const QString path = SchoolModeStore::sharedFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(SchoolModeStore::serialize(m_record));
    }

    if (!m_watcher->files().contains(path)) {
        m_watcher->addPath(path);
    }

    emit stateChanged();
}

bool SchoolModeService::turnOn(const QString& newCredential)
{
    if (m_record.credentialHashHex.isEmpty()) {
        if (newCredential.isEmpty()) {
            return false;
        }
        m_record.credentialSaltHex = SchoolModeStore::newSaltHex();
        m_record.credentialHashHex = SchoolModeStore::hashCredential(newCredential, m_record.credentialSaltHex);
    }

    m_record.on = true;
    save();
    return true;
}

bool SchoolModeService::turnOff(const QString& credential)
{
    if (!SchoolModeStore::verifyCredential(credential, m_record.credentialSaltHex, m_record.credentialHashHex)) {
        return false;
    }

    m_record.on = false;
    save();
    return true;
}

void SchoolModeService::rename(const QString& newDisplayName)
{
    if (newDisplayName.isEmpty()) {
        return;
    }
    m_record.displayName = newDisplayName;
    save();
}

void SchoolModeService::onFileChanged()
{
    reload();
    emit stateChanged();
}

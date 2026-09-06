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
#include <QSaveFile>
#include <QStandardPaths>

using namespace au::experience;

namespace {
QString bytesToHex(const QByteArray& bytes)
{
    return QString::fromLatin1(bytes.toHex());
}

bool isLowerHex(const QString& value, int expectedLength)
{
    if (value.size() != expectedLength) {
        return false;
    }
    for (const QChar ch : value) {
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
              || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) {
            return false;
        }
    }
    return true;
}
}

SchoolModeStore::ParseResult SchoolModeStore::parse(const QByteArray& json)
{
    ParseResult result;

    constexpr qsizetype MAX_RECORD_BYTES = 16 * 1024;
    if (json.size() > MAX_RECORD_BYTES) {
        result.error = QStringLiteral("The shared School mode record is too large.");
        return result;
    }

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

    const QJsonValue version = obj.value(QStringLiteral("version"));
    const QJsonValue on = obj.value(QStringLiteral("on"));
    const QJsonValue displayName = obj.value(QStringLiteral("displayName"));
    const QJsonValue credentialHash = obj.value(QStringLiteral("credentialHashHex"));
    const QJsonValue credentialSalt = obj.value(QStringLiteral("credentialSaltHex"));
    if (!version.isDouble() || version.toDouble() != 1.0 || !on.isBool() || !displayName.isString()
        || !credentialHash.isString() || !credentialSalt.isString()) {
        result.error = QStringLiteral("The shared School mode record has an unsupported schema.");
        return result;
    }

    SchoolModeRecord record;
    record.on = on.toBool();
    record.displayName = displayName.toString();
    record.credentialHashHex = credentialHash.toString();
    record.credentialSaltHex = credentialSalt.toString();

    if (!record.isValid()) {
        result.error = QStringLiteral("The shared School mode record has an invalid display name.");
        return result;
    }

    const bool hasCredentialHash = !record.credentialHashHex.isEmpty();
    const bool hasCredentialSalt = !record.credentialSaltHex.isEmpty();
    if (hasCredentialHash != hasCredentialSalt
        || (hasCredentialHash && (!isLowerHex(record.credentialHashHex, 64) || !isLowerHex(record.credentialSaltHex, 32)))
        || (record.on && !hasCredentialHash)) {
        result.error = QStringLiteral("The shared School mode record has inconsistent credential data.");
        return result;
    }

    result.ok = true;
    result.record = record;
    return result;
}

QByteArray SchoolModeStore::serialize(const SchoolModeRecord& record)
{
    QJsonObject obj;
    obj[QStringLiteral("version")] = 1;
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
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // appData is normally "<parent>/<OrgName>/<AppName>" (or similar); walk
    // up one level to the shared parent every one of this user's apps sits
    // under, then into a "shared" subdirectory.
    QDir dir(appData);
    dir.cdUp();
    return dir.absoluteFilePath(QStringLiteral("shared/school-mode.json"));
}

SchoolModeStore::ParseResult SchoolModeStore::readRecordFile(const QString& path)
{
    QFile file(path);
    if (!file.exists()) {
        return parse(QByteArray());
    }
    if (file.size() > 16 * 1024) {
        ParseResult result;
        result.error = QStringLiteral("The shared School mode record is too large.");
        return result;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        ParseResult result;
        result.error = QStringLiteral("The shared School mode record could not be read.");
        return result;
    }
    return parse(file.readAll());
}

SchoolModeStore::SharedRecordResult SchoolModeStore::sharedRecord()
{
    static SchoolModeRecord lastKnownRecord;
    static bool hasKnownRecord = false;

    const ParseResult result = readRecordFile(sharedFilePath());
    if (result.ok) {
        lastKnownRecord = result.record;
        hasKnownRecord = true;
        return { true, true, QString(), result.record };
    }

    return { false, hasKnownRecord, result.error, lastKnownRecord };
}

SchoolModeService::SchoolModeService(QObject* parent, const QString& recordPath)
    : QObject(parent), m_recordPath(recordPath.isEmpty() ? SchoolModeStore::sharedFilePath() : recordPath),
      m_watcher(new QFileSystemWatcher(this))
{
    reload();

    const QString path = m_recordPath;
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
    const SchoolModeStore::ParseResult result = SchoolModeStore::readRecordFile(m_recordPath);
    if (result.ok) {
        m_record = result.record;
        m_available = true;
        m_hasKnownRecord = true;
        m_error.clear();
        if (m_recordPath == SchoolModeStore::sharedFilePath()) {
            // Prime the process-wide reader used by synchronous presentation
            // helpers, so a later corrupt live read retains this same record.
            SchoolModeStore::sharedRecord();
        }
        return;
    }

    m_available = false;
    m_error = result.error;
    if (!m_hasKnownRecord) {
        m_record = SchoolModeRecord();
    }
}

bool SchoolModeService::save(const SchoolModeRecord& record)
{
    const QString path = m_recordPath;
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        m_available = false;
        m_error = QStringLiteral("The shared School mode record directory could not be created.");
        emit stateChanged();
        return false;
    }

    QSaveFile file(path);
    const QByteArray serialized = SchoolModeStore::serialize(record);
    if (!file.open(QIODevice::WriteOnly) || file.write(serialized) != serialized.size() || !file.commit()) {
        m_available = false;
        m_error = QStringLiteral("The shared School mode record could not be written.");
        emit stateChanged();
        return false;
    }

    m_record = record;
    m_available = true;
    m_hasKnownRecord = true;
    m_error.clear();

    if (!m_watcher->files().contains(path)) {
        m_watcher->addPath(path);
    }

    emit stateChanged();
    return true;
}

bool SchoolModeService::turnOn(const QString& newCredential)
{
    SchoolModeRecord updated = m_record;
    if (updated.credentialHashHex.isEmpty()) {
        if (newCredential.isEmpty()) {
            return false;
        }
        updated.credentialSaltHex = SchoolModeStore::newSaltHex();
        updated.credentialHashHex = SchoolModeStore::hashCredential(newCredential, updated.credentialSaltHex);
    }

    updated.on = true;
    return save(updated);
}

bool SchoolModeService::turnOff(const QString& credential)
{
    if (!SchoolModeStore::verifyCredential(credential, m_record.credentialSaltHex, m_record.credentialHashHex)) {
        return false;
    }

    SchoolModeRecord updated = m_record;
    updated.on = false;
    return save(updated);
}

bool SchoolModeService::rename(const QString& newDisplayName)
{
    if (newDisplayName.isEmpty() || newDisplayName.size() > 80) {
        return false;
    }
    SchoolModeRecord updated = m_record;
    updated.displayName = newDisplayName;
    return save(updated);
}

void SchoolModeService::onFileChanged()
{
    reload();
    emit stateChanged();
}

/*
* Audacity: A Digital Audio Editor
*/

#include "lockregistry.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>

#include "mutationhistory.h"
#include "totpengine.h"

using namespace au::personalize;

namespace {
const int MAX_ATTEMPTS_BEFORE_COOLDOWN = 5;

QByteArray randomSalt()
{
    QByteArray salt(16, '\0');
    for (int i = 0; i < salt.size(); ++i) {
        salt[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
    }
    return salt;
}

QString hashSecret(const QString& secret, const QByteArray& salt)
{
    QByteArray input = salt + secret.toUtf8();
    // A single PBKDF2 pass is out of reach without an extra dependency this
    // module does not otherwise need; repeated SHA-256 rounds over a random
    // per-lock salt is the honest middle ground given that this is a for
    // fun lock and never a security boundary.
    QByteArray hash = input;
    for (int round = 0; round < 10000; ++round) {
        hash = QCryptographicHash::hash(hash + input, QCryptographicHash::Sha256);
    }
    return QString::fromUtf8(hash.toBase64());
}

bool policyNeedsPin(LockPolicy policy)
{
    return policy == LockPolicy::Pin || policy == LockPolicy::PinAndPassword || policy == LockPolicy::PinAndTotp
           || policy == LockPolicy::PasswordAndPinAndTotp;
}

bool policyNeedsPassword(LockPolicy policy)
{
    return policy == LockPolicy::Password || policy == LockPolicy::PinAndPassword || policy == LockPolicy::PasswordAndTotp
           || policy == LockPolicy::PasswordAndPinAndTotp;
}

bool policyNeedsTotp(LockPolicy policy)
{
    return policy == LockPolicy::PasswordAndTotp || policy == LockPolicy::PinAndTotp
           || policy == LockPolicy::PasswordAndPinAndTotp;
}
}

LockRegistry::LockRegistry(QObject* parent)
    : QObject(parent)
    , m_history(new MutationHistory(this))
{
    load();
}

QString LockRegistry::storePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/personalize";
    QDir().mkpath(dir);
    return dir + "/locks.json";
}

QString LockRegistry::dataFolderPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

void LockRegistry::load()
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
    m_locks = doc.object().toVariantMap();
}

void LockRegistry::save() const
{
    QFile file(storePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(QJsonObject::fromVariantMap(m_locks)).toJson(QJsonDocument::Indented));
}

QStringList LockRegistry::lockedElementIds() const
{
    return m_locks.keys();
}

bool LockRegistry::isLocked(const QString& elementId) const
{
    return m_locks.contains(elementId);
}

bool LockRegistry::isActivelyLocked(const QString& elementId) const
{
    if (!isLocked(elementId)) {
        return false;
    }
    if (!m_unlockedUntil.contains(elementId)) {
        return true;
    }
    qint64 until = m_unlockedUntil.value(elementId);
    if (until < 0) {
        return false; // unlocked until the app closes
    }
    return QDateTime::currentSecsSinceEpoch() >= until;
}

void LockRegistry::createLock(const QString& elementId, int policyInt, const QString& pin, const QString& password,
                              const QString& totpSecretBase32, int unlockDurationMinutes, bool lockedOnLaunch)
{
    LockPolicy policy = static_cast<LockPolicy>(policyInt);
    QVariantMap lock;
    lock["policy"] = policyInt;
    lock["unlockDurationMinutes"] = unlockDurationMinutes;
    lock["lockedOnLaunch"] = lockedOnLaunch;

    if (policyNeedsPin(policy) && !pin.isEmpty()) {
        QByteArray salt = randomSalt();
        lock["pinSalt"] = QString::fromUtf8(salt.toBase64());
        lock["pinHash"] = hashSecret(pin, salt);
    }
    if (policyNeedsPassword(policy) && !password.isEmpty()) {
        QByteArray salt = randomSalt();
        lock["passwordSalt"] = QString::fromUtf8(salt.toBase64());
        lock["passwordHash"] = hashSecret(password, salt);
    }
    if (policyNeedsTotp(policy) && !totpSecretBase32.isEmpty()) {
        lock["totpSecret"] = totpSecretBase32;
    }

    m_locks[elementId] = lock;
    m_failedAttempts[elementId] = 0;
    save();

    m_history->record("lock.created", QString("Created a lock on \"%1\".").arg(elementId));
    emit locksChanged();
}

void LockRegistry::removeLock(const QString& elementId)
{
    if (!m_locks.contains(elementId)) {
        return;
    }
    m_locks.remove(elementId);
    m_unlockedUntil.remove(elementId);
    m_failedAttempts.remove(elementId);
    save();

    m_history->record("lock.removed", QString("Removed the lock on \"%1\".").arg(elementId));
    emit locksChanged();
}

bool LockRegistry::tryUnlock(const QString& elementId, const QString& pin, const QString& password, const QString& totpCode)
{
    if (!m_locks.contains(elementId)) {
        return true;
    }

    if (m_failedAttempts.value(elementId, 0) >= MAX_ATTEMPTS_BEFORE_COOLDOWN) {
        return false;
    }

    QVariantMap lock = m_locks.value(elementId).toMap();
    LockPolicy policy = static_cast<LockPolicy>(lock.value("policy").toInt());

    bool pinOk = true;
    bool passwordOk = true;
    bool totpOk = true;

    if (policyNeedsPin(policy)) {
        QByteArray salt = QByteArray::fromBase64(lock.value("pinSalt").toString().toUtf8());
        pinOk = hashSecret(pin, salt) == lock.value("pinHash").toString();
    }
    if (policyNeedsPassword(policy)) {
        QByteArray salt = QByteArray::fromBase64(lock.value("passwordSalt").toString().toUtf8());
        passwordOk = hashSecret(password, salt) == lock.value("passwordHash").toString();
    }
    if (policyNeedsTotp(policy)) {
        QByteArray secret = TotpEngine::base32Decode(lock.value("totpSecret").toString());
        totpOk = TotpEngine::verify(secret, totpCode, QDateTime::currentSecsSinceEpoch(), 6, 30,
                                    TotpEngine::Algorithm::Sha1, 1);
    }

    bool ok = pinOk && passwordOk && totpOk;
    if (!ok) {
        m_failedAttempts[elementId] = m_failedAttempts.value(elementId, 0) + 1;
        return false;
    }

    m_failedAttempts[elementId] = 0;

    int durationMinutes = lock.value("unlockDurationMinutes").toInt();
    if (durationMinutes <= 0) {
        m_unlockedUntil[elementId] = -1; // until the app closes
    } else {
        m_unlockedUntil[elementId] = QDateTime::currentSecsSinceEpoch() + static_cast<qint64>(durationMinutes) * 60;
    }

    emit locksChanged();
    return true;
}

void LockRegistry::lockAgain(const QString& elementId)
{
    m_unlockedUntil.remove(elementId);
    emit locksChanged();
}

QVariantMap LockRegistry::lockInfo(const QString& elementId) const
{
    QVariantMap lock = m_locks.value(elementId).toMap();
    QVariantMap info;
    info["policy"] = lock.value("policy");
    info["unlockDurationMinutes"] = lock.value("unlockDurationMinutes");
    info["lockedOnLaunch"] = lock.value("lockedOnLaunch");
    info["failedAttempts"] = m_failedAttempts.value(elementId, 0);
    return info;
}

/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace au::personalize {
class MutationHistory;

//! The six lock policies a toy lock can use, matching the six combinations
//! named in the shared instructions.
enum class LockPolicy {
    Pin,
    Password,
    PinAndPassword,
    PasswordAndTotp,
    PinAndTotp,
    PasswordAndPinAndTotp
};

/*!
 * \brief The toy lock registry.
 *
 * A lock here is a self imposed speed bump on one named element, never a
 * security boundary: it does not encrypt anything, it does not stop
 * another program running as the same user, and its own recovery route is
 * simply deleting the application's data folder. Every surface that offers
 * to create one says so.
 *
 * Each lock keeps its own credential, its own attempt budget, and its own
 * unlock state; there is no master credential that unlocks more than one
 * element at a time.
 */
class LockRegistry : public QObject
{
    Q_OBJECT

public:
    explicit LockRegistry(QObject* parent = nullptr);

    Q_INVOKABLE QStringList lockedElementIds() const;
    Q_INVOKABLE bool isLocked(const QString& elementId) const;
    //! True once a lock exists and has not been unlocked for its configured
    //! duration in this session.
    Q_INVOKABLE bool isActivelyLocked(const QString& elementId) const;

    Q_INVOKABLE void createLock(const QString& elementId, int policy, const QString& pin, const QString& password,
                                const QString& totpSecretBase32, int unlockDurationMinutes, bool lockedOnLaunch);
    Q_INVOKABLE void removeLock(const QString& elementId);

    //! Attempts an unlock with whichever of pin/password/totp the policy
    //! needs. Empty fields are ignored for a policy that does not need them.
    //! Returns true and clears the active-lock state for `unlockDurationMinutes`
    //! (or until the app closes, if that value is 0 and `untilClose` is true).
    Q_INVOKABLE bool tryUnlock(const QString& elementId, const QString& pin, const QString& password, const QString& totpCode);
    Q_INVOKABLE void lockAgain(const QString& elementId);

    Q_INVOKABLE QVariantMap lockInfo(const QString& elementId) const;
    Q_INVOKABLE QString dataFolderPath() const;

signals:
    void locksChanged();

private:
    QString storePath() const;
    void load();
    void save() const;

    QVariantMap m_locks;
    //! elementId -> the timestamp (seconds since epoch) the temporary
    //! unlock expires, or -1 for "until the app closes".
    QMap<QString, qint64> m_unlockedUntil;
    QMap<QString, int> m_failedAttempts;
    MutationHistory* m_history = nullptr;
};
}

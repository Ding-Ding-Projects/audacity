/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>

#include "authenticatorstore.h"

namespace au::personalize {
class MutationHistory;

/*!
 * \brief The model behind the built in, offline authenticator page.
 *
 * Every code is computed locally from the machine clock and the stored
 * secret. There is no network call anywhere in this class.
 */
class AuthenticatorModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(qint64 secondsRemainingInPeriod READ secondsRemainingInPeriod NOTIFY tick)
    Q_PROPERTY(bool clockLooksSkewed READ clockLooksSkewed NOTIFY tick)

public:
    explicit AuthenticatorModel(QObject* parent = nullptr);

    QVariantList entries() const;
    qint64 secondsRemainingInPeriod() const;
    bool clockLooksSkewed() const;

    Q_INVOKABLE QString newSecretBase32() const;
    Q_INVOKABLE QString otpauthUriFor(const QString& issuer, const QString& account, const QString& secretBase32, int digits,
                                      int periodSeconds, int algorithm) const;

    Q_INVOKABLE bool addFromOtpauthUri(const QString& uri);
    Q_INVOKABLE QString addManual(const QString& issuer, const QString& account, const QString& secretBase32, int digits, int periodSeconds,
                                  int algorithm);
    Q_INVOKABLE void removeEntry(const QString& id);

    Q_INVOKABLE QString currentCode(const QString& id) const;
    Q_INVOKABLE QString nextCode(const QString& id) const;

    Q_INVOKABLE void tickNow();

signals:
    void entriesChanged();
    void tick();

private:
    void load();
    void persist();

    AuthenticatorStore m_store;
    QList<AuthenticatorEntry> m_entries;
    MutationHistory* m_history = nullptr;
};
}

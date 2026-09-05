/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QByteArray>
#include <QString>
#include <QVariantList>

namespace au::personalize {
/*!
 * \brief One authenticator entry: an issuer, an account, and the secret
 * needed to compute its codes.
 */
struct AuthenticatorEntry {
    QString id;
    QString issuer;
    QString account;
    QByteArray secret;
    int digits = 6;
    int periodSeconds = 30;
    int algorithm = 0; // TotpEngine::Algorithm
};

/*!
 * \brief The local, offline store for authenticator entries.
 *
 * Entries are kept in a single file under the application's user data
 * directory, obscured with a key derived from a per install secret file
 * created with restrictive permissions the first time this store runs.
 * This is honestly not the same guarantee an operating system credential
 * vault gives: it keeps a casual read of the file from being useful, it
 * does not defend against another program running as the same user. The
 * authenticator page says so plainly.
 *
 * Ordinary export routes never read this file: it lives outside the
 * general application settings and appearance export paths on purpose.
 */
class AuthenticatorStore
{
public:
    AuthenticatorStore();

    QList<AuthenticatorEntry> load() const;
    void save(const QList<AuthenticatorEntry>& entries) const;

private:
    QString storePath() const;
    QByteArray obscureKey() const;
    QByteArray xorWithKeystream(const QByteArray& data, const QByteArray& key) const;
};
}

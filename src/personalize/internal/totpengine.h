/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QByteArray>
#include <QString>
#include <QCryptographicHash>

namespace au::personalize {
/*!
 * \brief A local, offline TOTP (RFC 6238) and HOTP (RFC 4226) implementation.
 *
 * Everything here runs on the machine's own clock and the secret the caller
 * supplies. There is no network call anywhere in this class.
 */
class TotpEngine
{
public:
    enum class Algorithm { Sha1, Sha256, Sha512 };

    //! Decodes a base32 secret (RFC 4648, case insensitive, padding optional).
    static QByteArray base32Decode(const QString& base32);
    //! Encodes raw bytes as base32 (RFC 4648, upper case, no padding).
    static QString base32Encode(const QByteArray& bytes);

    //! Generates a random secret of the given byte length (default 20, the
    //! size most authenticator apps expect for SHA-1).
    static QByteArray randomSecret(int byteLength = 20);

    //! HOTP (RFC 4226), returned as a zero padded decimal string.
    static QString hotp(const QByteArray& secret, quint64 counter, int digits, Algorithm algorithm);

    //! TOTP (RFC 6238) at the given Unix timestamp.
    static QString totp(const QByteArray& secret, qint64 unixSeconds, int digits, int periodSeconds, Algorithm algorithm);

    //! True if `code` matches the TOTP at `unixSeconds` within `skewSteps`
    //! periods either side, allowing for ordinary clock drift.
    static bool verify(const QByteArray& secret, const QString& code, qint64 unixSeconds, int digits, int periodSeconds,
                        Algorithm algorithm, int skewSteps = 1);

    //! Builds an otpauth://totp/ URI for QR pairing.
    static QString otpauthUri(const QString& issuer, const QString& account, const QByteArray& secret, int digits,
                               int periodSeconds, Algorithm algorithm);

    static QCryptographicHash::Algorithm qtAlgorithm(Algorithm algorithm);
    static QString algorithmName(Algorithm algorithm);
};
}

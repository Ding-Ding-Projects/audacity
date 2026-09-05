/*
* Audacity: A Digital Audio Editor
*/

#include "totpengine.h"

#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QUrl>
#include <QUrlQuery>

using namespace au::personalize;

static const char* BASE32_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

QByteArray TotpEngine::base32Decode(const QString& base32)
{
    QString cleaned;
    for (QChar c : base32) {
        if (c == '=' || c.isSpace() || c == '-') {
            continue;
        }
        cleaned += c.toUpper();
    }

    QByteArray result;
    int buffer = 0;
    int bitsLeft = 0;
    for (QChar c : cleaned) {
        int value = -1;
        for (int i = 0; i < 32; ++i) {
            if (BASE32_ALPHABET[i] == c.toLatin1()) {
                value = i;
                break;
            }
        }
        if (value < 0) {
            continue;
        }
        buffer = (buffer << 5) | value;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            bitsLeft -= 8;
            result.append(static_cast<char>((buffer >> bitsLeft) & 0xFF));
        }
    }
    return result;
}

QString TotpEngine::base32Encode(const QByteArray& bytes)
{
    QString result;
    int buffer = 0;
    int bitsLeft = 0;
    for (unsigned char byte : bytes) {
        buffer = (buffer << 8) | byte;
        bitsLeft += 8;
        while (bitsLeft >= 5) {
            bitsLeft -= 5;
            result += QChar(BASE32_ALPHABET[(buffer >> bitsLeft) & 0x1F]);
        }
    }
    if (bitsLeft > 0) {
        result += QChar(BASE32_ALPHABET[(buffer << (5 - bitsLeft)) & 0x1F]);
    }
    return result;
}

QByteArray TotpEngine::randomSecret(int byteLength)
{
    QByteArray secret;
    secret.resize(byteLength);
    for (int i = 0; i < byteLength; ++i) {
        secret[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
    }
    return secret;
}

QCryptographicHash::Algorithm TotpEngine::qtAlgorithm(Algorithm algorithm)
{
    switch (algorithm) {
    case Algorithm::Sha256: return QCryptographicHash::Sha256;
    case Algorithm::Sha512: return QCryptographicHash::Sha512;
    case Algorithm::Sha1:
    default: return QCryptographicHash::Sha1;
    }
}

QString TotpEngine::algorithmName(Algorithm algorithm)
{
    switch (algorithm) {
    case Algorithm::Sha256: return "SHA256";
    case Algorithm::Sha512: return "SHA512";
    case Algorithm::Sha1:
    default: return "SHA1";
    }
}

QString TotpEngine::hotp(const QByteArray& secret, quint64 counter, int digits, Algorithm algorithm)
{
    QByteArray counterBytes(8, '\0');
    for (int i = 7; i >= 0; --i) {
        counterBytes[i] = static_cast<char>(counter & 0xFF);
        counter >>= 8;
    }

    QMessageAuthenticationCode mac(qtAlgorithm(algorithm));
    mac.setKey(secret);
    mac.addData(counterBytes);
    QByteArray hash = mac.result();

    if (hash.isEmpty()) {
        return QString();
    }

    int offset = hash.at(hash.size() - 1) & 0x0F;
    quint32 binary = ((static_cast<quint32>(hash.at(offset)) & 0x7F) << 24)
                     | ((static_cast<quint32>(hash.at(offset + 1)) & 0xFF) << 16)
                     | ((static_cast<quint32>(hash.at(offset + 2)) & 0xFF) << 8)
                     | (static_cast<quint32>(hash.at(offset + 3)) & 0xFF);

    quint32 mod = 1;
    for (int i = 0; i < digits; ++i) {
        mod *= 10;
    }

    quint32 code = binary % mod;
    return QString("%1").arg(code, digits, 10, QChar('0'));
}

QString TotpEngine::totp(const QByteArray& secret, qint64 unixSeconds, int digits, int periodSeconds, Algorithm algorithm)
{
    quint64 counter = static_cast<quint64>(unixSeconds / periodSeconds);
    return hotp(secret, counter, digits, algorithm);
}

bool TotpEngine::verify(const QByteArray& secret, const QString& code, qint64 unixSeconds, int digits, int periodSeconds,
                        Algorithm algorithm, int skewSteps)
{
    QString trimmed = code.trimmed();
    if (trimmed.size() != digits) {
        return false;
    }
    for (int step = -skewSteps; step <= skewSteps; ++step) {
        qint64 shifted = unixSeconds + static_cast<qint64>(step) * periodSeconds;
        if (shifted < 0) {
            continue;
        }
        if (totp(secret, shifted, digits, periodSeconds, algorithm) == trimmed) {
            return true;
        }
    }
    return false;
}

QString TotpEngine::otpauthUri(const QString& issuer, const QString& account, const QByteArray& secret, int digits,
                               int periodSeconds, Algorithm algorithm)
{
    QString label = issuer.isEmpty() ? account : (issuer + ":" + account);

    QUrl url;
    url.setScheme("otpauth");
    url.setHost("totp");
    url.setPath("/" + label);

    QUrlQuery query;
    query.addQueryItem("secret", base32Encode(secret));
    if (!issuer.isEmpty()) {
        query.addQueryItem("issuer", issuer);
    }
    query.addQueryItem("algorithm", algorithmName(algorithm));
    query.addQueryItem("digits", QString::number(digits));
    query.addQueryItem("period", QString::number(periodSeconds));
    url.setQuery(query);

    return QString::fromUtf8(url.toEncoded());
}

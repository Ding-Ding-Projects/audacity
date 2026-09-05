/*
* Audacity: A Digital Audio Editor
*/

#include "authenticatormodel.h"

#include <QDateTime>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

#include "mutationhistory.h"
#include "totpengine.h"

using namespace au::personalize;

AuthenticatorModel::AuthenticatorModel(QObject* parent)
    : QObject(parent)
    , m_history(new MutationHistory(this))
{
    load();
}

void AuthenticatorModel::load()
{
    m_entries = m_store.load();
}

void AuthenticatorModel::persist()
{
    m_store.save(m_entries);
    emit entriesChanged();
}

QVariantList AuthenticatorModel::entries() const
{
    QVariantList list;
    for (const AuthenticatorEntry& entry : m_entries) {
        QVariantMap map;
        map["id"] = entry.id;
        map["issuer"] = entry.issuer;
        map["account"] = entry.account;
        map["digits"] = entry.digits;
        map["period"] = entry.periodSeconds;
        map["algorithm"] = entry.algorithm;
        map["currentCode"] = currentCode(entry.id);
        map["nextCode"] = nextCode(entry.id);
        list.append(map);
    }
    return list;
}

qint64 AuthenticatorModel::secondsRemainingInPeriod() const
{
    // Every entry may have its own period, but 30 seconds is by far the
    // common case and this drives one shared countdown display.
    qint64 now = QDateTime::currentSecsSinceEpoch();
    return 30 - (now % 30);
}

bool AuthenticatorModel::clockLooksSkewed() const
{
    // There is no network time source available here, so this is only a
    // crude sanity check: a clock reporting a year well outside any
    // plausible range for this build is worth a warning, a clock that is
    // merely a few minutes off from a real time server is not detectable
    // this way at all. The authenticator page says so plainly.
    int year = QDateTime::currentDateTimeUtc().date().year();
    return year < 2024 || year > 2100;
}

QString AuthenticatorModel::newSecretBase32() const
{
    return TotpEngine::base32Encode(TotpEngine::randomSecret(20));
}

QString AuthenticatorModel::otpauthUriFor(const QString& issuer, const QString& account, const QString& secretBase32,
                                          int digits, int periodSeconds, int algorithm) const
{
    QByteArray secret = TotpEngine::base32Decode(secretBase32);
    return TotpEngine::otpauthUri(issuer, account, secret, digits, periodSeconds,
                                  static_cast<TotpEngine::Algorithm>(algorithm));
}

bool AuthenticatorModel::addFromOtpauthUri(const QString& uriText)
{
    QUrl url(uriText);
    if (url.scheme() != "otpauth" || url.host() != "totp") {
        return false;
    }

    QString label = url.path();
    if (label.startsWith("/")) {
        label = label.mid(1);
    }
    QString issuer;
    QString account = label;
    int colon = label.indexOf(':');
    if (colon >= 0) {
        issuer = label.left(colon);
        account = label.mid(colon + 1);
    }

    QUrlQuery query(url);
    if (query.hasQueryItem("issuer")) {
        issuer = query.queryItemValue("issuer", QUrl::FullyDecoded);
    }
    QString secretBase32 = query.queryItemValue("secret", QUrl::FullyDecoded);
    if (secretBase32.isEmpty()) {
        return false;
    }
    int digits = query.hasQueryItem("digits") ? query.queryItemValue("digits").toInt() : 6;
    int period = query.hasQueryItem("period") ? query.queryItemValue("period").toInt() : 30;
    TotpEngine::Algorithm algorithm = TotpEngine::Algorithm::Sha1;
    QString algoText = query.queryItemValue("algorithm").toUpper();
    if (algoText == "SHA256") {
        algorithm = TotpEngine::Algorithm::Sha256;
    } else if (algoText == "SHA512") {
        algorithm = TotpEngine::Algorithm::Sha512;
    }

    QString id = addManual(issuer, QUrl::fromPercentEncoding(account.toUtf8()), secretBase32, digits, period,
                           static_cast<int>(algorithm));
    return !id.isEmpty();
}

QString AuthenticatorModel::addManual(const QString& issuer, const QString& account, const QString& secretBase32, int digits,
                                      int periodSeconds, int algorithm)
{
    QByteArray secret = TotpEngine::base32Decode(secretBase32);
    if (secret.isEmpty()) {
        return QString();
    }

    AuthenticatorEntry entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.issuer = issuer;
    entry.account = account;
    entry.secret = secret;
    entry.digits = digits > 0 ? digits : 6;
    entry.periodSeconds = periodSeconds > 0 ? periodSeconds : 30;
    entry.algorithm = algorithm;

    m_entries.append(entry);
    persist();

    m_history->record("authenticator.added", QString("Added an authenticator entry for \"%1\".").arg(issuer.isEmpty()
                                                                                                     ? account : issuer));
    return entry.id;
}

void AuthenticatorModel::removeEntry(const QString& id)
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).id == id) {
            QString label = m_entries.at(i).issuer.isEmpty() ? m_entries.at(i).account : m_entries.at(i).issuer;
            m_entries.removeAt(i);
            persist();
            m_history->record("authenticator.removed", QString("Removed the authenticator entry for \"%1\".").arg(label));
            return;
        }
    }
}

static const AuthenticatorEntry* findEntry(const QList<AuthenticatorEntry>& entries, const QString& id)
{
    for (const AuthenticatorEntry& entry : entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

QString AuthenticatorModel::currentCode(const QString& id) const
{
    const AuthenticatorEntry* entry = findEntry(m_entries, id);
    if (!entry) {
        return QString();
    }
    return TotpEngine::totp(entry->secret, QDateTime::currentSecsSinceEpoch(), entry->digits, entry->periodSeconds,
                            static_cast<TotpEngine::Algorithm>(entry->algorithm));
}

QString AuthenticatorModel::nextCode(const QString& id) const
{
    const AuthenticatorEntry* entry = findEntry(m_entries, id);
    if (!entry) {
        return QString();
    }
    qint64 nextPeriodStart = QDateTime::currentSecsSinceEpoch() + entry->periodSeconds;
    return TotpEngine::totp(entry->secret, nextPeriodStart, entry->digits, entry->periodSeconds,
                            static_cast<TotpEngine::Algorithm>(entry->algorithm));
}

void AuthenticatorModel::tickNow()
{
    emit tick();
    emit entriesChanged();
}

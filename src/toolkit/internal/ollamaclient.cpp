/*
* Audacity: A Digital Audio Editor
*/

#include "ollamaclient.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHostAddress>

using namespace au::toolkit;

OllamaClient::OllamaClient(QObject* parent)
    : QObject(parent)
{
}

QString OllamaClient::host() const
{
    return m_host;
}

void OllamaClient::setHost(const QString& host)
{
    QString hostOnly = host;
    const int colon = hostOnly.lastIndexOf(':');
    if (colon > 0) {
        hostOnly = hostOnly.left(colon);
    }

    if (!isAllowedHost(hostOnly)) {
        emit requestFailed(QStringLiteral("set host"),
                            QStringLiteral("Only a loopback or private network address is allowed for the local model manager."));
        return;
    }

    if (m_host == host) {
        return;
    }
    m_host = host;
    emit hostChanged();
}

bool OllamaClient::isAllowedHost(const QString& candidateHost) const
{
    if (candidateHost.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0) {
        return true;
    }

    QHostAddress address(candidateHost);
    if (address.isNull()) {
        return false;
    }

    if (address.isLoopback() || address.isLinkLocal()) {
        return true;
    }

    // RFC 1918 private ranges.
    const quint32 v4 = address.toIPv4Address();
    if (v4 != 0) {
        if ((v4 & 0xFF000000) == 0x0A000000) { // 10.0.0.0/8
            return true;
        }
        if ((v4 & 0xFFF00000) == 0xAC100000) { // 172.16.0.0/12
            return true;
        }
        if ((v4 & 0xFFFF0000) == 0xC0A80000) { // 192.168.0.0/16
            return true;
        }
    }

    return false;
}

bool OllamaClient::reachable() const
{
    return m_reachable;
}

QString OllamaClient::version() const
{
    return m_version;
}

QUrl OllamaClient::endpoint(const QString& path) const
{
    return QUrl(QStringLiteral("http://") + m_host + path);
}

void OllamaClient::refreshHealth()
{
    QNetworkRequest request(endpoint(QStringLiteral("/api/version")));
    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const bool ok = (reply->error() == QNetworkReply::NoError);
        if (m_reachable != ok) {
            m_reachable = ok;
            emit reachableChanged();
        }
        if (ok) {
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            const QString version = doc.object().value(QStringLiteral("version")).toString();
            if (m_version != version) {
                m_version = version;
                emit versionChanged();
            }
        } else {
            emit requestFailed(QStringLiteral("health check"), reply->errorString());
        }
    });
}

void OllamaClient::refreshInstalledModels()
{
    QNetworkRequest request(endpoint(QStringLiteral("/api/tags")));
    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(QStringLiteral("list installed models"), reply->errorString());
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray models = doc.object().value(QStringLiteral("models")).toArray();
        QVariantList list;
        for (const QJsonValue& v : models) {
            list << v.toObject().toVariantMap();
        }
        emit installedModelsChanged(list);
    });
}

void OllamaClient::pullModel(const QString& modelTag)
{
    if (m_activePulls.contains(modelTag)) {
        return;
    }

    QNetworkRequest request(endpoint(QStringLiteral("/api/pull")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject body;
    body[QStringLiteral("model")] = modelTag;
    body[QStringLiteral("stream")] = true;

    QNetworkReply* reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_activePulls.insert(modelTag, reply);

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, modelTag]() {
        const QByteArrayList lines = reply->readAll().split('\n');
        for (const QByteArray& line : lines) {
            if (line.trimmed().isEmpty()) {
                continue;
            }
            const QJsonObject obj = QJsonDocument::fromJson(line).object();
            const QString status = obj.value(QStringLiteral("status")).toString();
            const qint64 completed = obj.value(QStringLiteral("completed")).toVariant().toLongLong();
            const qint64 total = obj.value(QStringLiteral("total")).toVariant().toLongLong();
            emit pullProgress(modelTag, completed, total, status);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, modelTag]() {
        m_activePulls.remove(modelTag);
        const bool ok = (reply->error() == QNetworkReply::NoError);
        emit pullFinished(modelTag, ok, ok ? QString() : reply->errorString());
        reply->deleteLater();
    });
}

void OllamaClient::cancelPull(const QString& modelTag)
{
    if (QNetworkReply* reply = m_activePulls.value(modelTag)) {
        reply->abort();
    }
}

void OllamaClient::sendChatMessage(const QString& model, const QVariantList& messages, const QVariantMap& parameters)
{
    QNetworkRequest request(endpoint(QStringLiteral("/api/chat")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject body;
    body[QStringLiteral("model")] = model;
    body[QStringLiteral("stream")] = true;

    QJsonArray messagesJson;
    for (const QVariant& m : messages) {
        messagesJson.append(QJsonObject::fromVariantMap(m.toMap()));
    }
    body[QStringLiteral("messages")] = messagesJson;

    if (!parameters.isEmpty()) {
        body[QStringLiteral("options")] = QJsonObject::fromVariantMap(parameters);
    }

    QNetworkReply* reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        const QByteArrayList lines = reply->readAll().split('\n');
        for (const QByteArray& line : lines) {
            if (line.trimmed().isEmpty()) {
                continue;
            }
            const QJsonObject obj = QJsonDocument::fromJson(line).object();
            const bool done = obj.value(QStringLiteral("done")).toBool();
            const QString content = obj.value(QStringLiteral("message")).toObject().value(QStringLiteral("content")).toString();
            emit chatToken(content, done);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit chatError(reply->errorString());
        }
        reply->deleteLater();
    });
}

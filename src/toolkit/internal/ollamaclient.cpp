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
#include <QSettings>

using namespace au::toolkit;

OllamaClient::OllamaClient(QObject* parent)
    : QObject(parent)
{
    restorePullQueue();
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

QVariantList OllamaClient::installedModels() const
{
    return m_installedModels;
}

bool OllamaClient::chatInFlight() const
{
    return m_chatInFlight;
}

int OllamaClient::pullConcurrency() const
{
    return m_pullConcurrency;
}

void OllamaClient::setPullConcurrency(int value)
{
    value = qBound(1, value, 4);
    if (m_pullConcurrency == value) {
        return;
    }
    m_pullConcurrency = value;
    emit pullConcurrencyChanged();
    startQueuedPulls();
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
            startQueuedPulls();
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
        m_installedModels = list;
        emit installedModelsChanged(list);
        emit installedModelsListChanged();
    });
}

void OllamaClient::pullModel(const QString& modelTag)
{
    const QString tag = modelTag.trimmed();
    if (tag.isEmpty() || m_activePulls.contains(tag) || m_pullQueue.contains(tag)) {
        return;
    }
    m_pullQueue.enqueue(tag);
    persistPullQueue();
    startQueuedPulls();
}

void OllamaClient::startQueuedPulls()
{
    while (m_activePulls.size() < m_pullConcurrency && !m_pullQueue.isEmpty()) {
        startPull(m_pullQueue.dequeue());
    }
    persistPullQueue();
}

void OllamaClient::startPull(const QString& modelTag)
{

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
        startQueuedPulls();
    });
}

void OllamaClient::cancelPull(const QString& modelTag)
{
    if (QNetworkReply* reply = m_activePulls.value(modelTag)) {
        reply->abort();
        return;
    }
    m_pullQueue.removeAll(modelTag);
    persistPullQueue();
}

void OllamaClient::sendChatMessage(const QString& model, const QVariantList& messages, const QVariantMap& parameters)
{
    if (m_chatInFlight || model.trimmed().isEmpty() || messages.isEmpty()) {
        return;
    }
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

    m_chatBuffer.clear();
    m_chatCancelled = false;
    m_chatReply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QNetworkReply* reply = m_chatReply;
    setChatInFlight(true);

    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        m_chatBuffer += reply->readAll();
        QByteArrayList lines = m_chatBuffer.split('\n');
        m_chatBuffer = lines.takeLast();
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
        const bool cancelled = m_chatCancelled || reply->error() == QNetworkReply::OperationCanceledError;
        if (!cancelled && reply->error() != QNetworkReply::NoError) {
            emit chatError(reply->errorString());
        }
        if (reply == m_chatReply) {
            m_chatReply = nullptr;
        }
        m_chatBuffer.clear();
        setChatInFlight(false);
        emit chatFinished(cancelled);
        reply->deleteLater();
    });
}

void OllamaClient::cancelChat()
{
    if (m_chatReply) {
        m_chatCancelled = true;
        m_chatReply->abort();
    }
}

void OllamaClient::persistPullQueue() const
{
    QSettings settings;
    QStringList queue;
    for (const QString& tag : m_pullQueue) {
        queue << tag;
    }
    // An in-flight pull is persisted too. If the process exits, Ollama's
    // pull operation is safe to ask for again and the next health check
    // reconciles it with the runtime instead of silently losing the task.
    for (auto it = m_activePulls.cbegin(); it != m_activePulls.cend(); ++it) {
        if (!queue.contains(it.key())) {
            queue << it.key();
        }
    }
    settings.setValue(QStringLiteral("ollama/pullQueue"), queue);
    QVariantList state;
    for (const QString& tag : queue) {
        state << QVariantMap { { QStringLiteral("tag"), tag }, { QStringLiteral("state"), QStringLiteral("queued") } };
    }
    Q_EMIT const_cast<OllamaClient*>(this)->queuedPullsChanged(state);
}

void OllamaClient::restorePullQueue()
{
    QSettings settings;
    const QStringList saved = settings.value(QStringLiteral("ollama/pullQueue")).toStringList();
    for (const QString& tag : saved) {
        if (!tag.trimmed().isEmpty() && !m_pullQueue.contains(tag)) {
            m_pullQueue.enqueue(tag);
        }
    }
    persistPullQueue();
}

void OllamaClient::setChatInFlight(bool value)
{
    if (m_chatInFlight == value) {
        return;
    }
    m_chatInFlight = value;
    emit chatInFlightChanged();
}

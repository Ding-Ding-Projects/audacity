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
#include <QFile>
#include <QDateTime>
#include <QUuid>
#include <QSet>
#include <QRegularExpression>

using namespace au::toolkit;

OllamaClient::OllamaClient(QObject* parent)
    : QObject(parent)
{
    restorePullQueue();
    refreshChatSessions();
    QSettings settings;
    m_catalogSnapshot = settings.value(QStringLiteral("ollama/catalogSnapshot")).toMap();
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

int OllamaClient::capabilityRevision() const
{
    return m_capabilityRevision;
}

QVariantList OllamaClient::chatSessions() const
{
    return m_chatSessions;
}

QVariantMap OllamaClient::catalogSnapshot() const
{
    return m_catalogSnapshot;
}

QVariantList OllamaClient::queuedPulls() const
{
    return m_queuedPulls;
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
    for (int index = 0; index < messages.size(); ++index) {
        QJsonObject message = QJsonObject::fromVariantMap(messages.at(index).toMap());
        if (index == messages.size() - 1 && message.value(QStringLiteral("role")).toString() == QStringLiteral("user")) {
            const QStringList images = m_pendingImages.take(model);
            if (!images.isEmpty()) {
                QJsonArray encodedImages;
                for (const QString& image : images) {
                    encodedImages.append(image);
                }
                message.insert(QStringLiteral("images"), encodedImages);
            }
        }
        messagesJson.append(message);
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

void OllamaClient::inspectModel(const QString& modelTag)
{
    const QString tag = modelTag.trimmed();
    if (tag.isEmpty()) {
        return;
    }
    QNetworkRequest request(endpoint(QStringLiteral("/api/show")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    const QJsonObject body { { QStringLiteral("model"), tag } };
    QNetworkReply* reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, tag]() {
        bool supportsImages = false;
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonArray capabilities = QJsonDocument::fromJson(reply->readAll()).object().value(QStringLiteral("capabilities")).toArray();
            for (const QJsonValue& capability : capabilities) {
                if (capability.toString() == QStringLiteral("vision")) {
                    supportsImages = true;
                    break;
                }
            }
        } else {
            emit requestFailed(QStringLiteral("inspect model capabilities"), reply->errorString());
        }
        m_imageCapabilities.insert(tag, supportsImages);
        ++m_capabilityRevision;
        emit capabilityRevisionChanged();
        emit modelInspected(tag, supportsImages);
        reply->deleteLater();
    });
}

bool OllamaClient::supportsImageAttachments(const QString& modelTag) const
{
    return m_imageCapabilities.value(modelTag.trimmed(), false);
}

bool OllamaClient::attachImage(const QString& modelTag, const QUrl& fileUrl)
{
    const QString tag = modelTag.trimmed();
    if (!supportsImageAttachments(tag)) {
        emit attachmentRejected(QStringLiteral("The selected model has not reported image capability through /api/show."));
        return false;
    }
    if (!fileUrl.isLocalFile()) {
        emit attachmentRejected(QStringLiteral("Only a local image file may be attached."));
        return false;
    }
    QFile file(fileUrl.toLocalFile());
    constexpr qint64 maximumImageBytes = 5LL * 1024 * 1024;
    if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > maximumImageBytes) {
        emit attachmentRejected(QStringLiteral("The image must be a readable local file of at most 5 MiB."));
        return false;
    }
    const QByteArray bytes = file.readAll();
    const bool png = bytes.startsWith("\x89PNG\r\n\x1a\n");
    const bool jpeg = bytes.startsWith("\xff\xd8\xff");
    const bool webp = bytes.startsWith("RIFF") && bytes.mid(8, 4) == "WEBP";
    if (!png && !jpeg && !webp) {
        emit attachmentRejected(QStringLiteral("Only PNG, JPEG, and WebP image files are supported."));
        return false;
    }
    QStringList images = m_pendingImages.value(tag);
    if (images.size() >= 4) {
        emit attachmentRejected(QStringLiteral("At most four image attachments may be sent with one message."));
        return false;
    }
    images << QString::fromLatin1(bytes.toBase64());
    m_pendingImages.insert(tag, images);
    return true;
}

void OllamaClient::clearAttachments(const QString& modelTag)
{
    m_pendingImages.remove(modelTag.trimmed());
}

QString OllamaClient::saveChatSession(const QString& title, const QString& systemPrompt, const QVariantList& messages)
{
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVariantMap session {
        { QStringLiteral("id"), id },
        { QStringLiteral("title"), title.left(120) },
        { QStringLiteral("systemPrompt"), systemPrompt.left(4096) },
        { QStringLiteral("messages"), messages.mid(qMax(0, messages.size() - 200)) },
        { QStringLiteral("updatedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate) }
    };
    QSettings settings;
    QVariantList stored = settings.value(QStringLiteral("ollama/chatSessions")).toList();
    stored << session;
    while (stored.size() > 50) {
        stored.removeFirst();
    }
    settings.setValue(QStringLiteral("ollama/chatSessions"), stored);
    refreshChatSessions();
    return id;
}

QVariantMap OllamaClient::loadChatSession(const QString& id) const
{
    for (const QVariant& entry : m_chatSessions) {
        const QVariantMap session = entry.toMap();
        if (session.value(QStringLiteral("id")).toString() == id) {
            return session;
        }
    }
    return {};
}

void OllamaClient::deleteChatSession(const QString& id)
{
    QSettings settings;
    QVariantList stored = settings.value(QStringLiteral("ollama/chatSessions")).toList();
    for (auto it = stored.begin(); it != stored.end();) {
        if (it->toMap().value(QStringLiteral("id")).toString() == id) {
            it = stored.erase(it);
        } else {
            ++it;
        }
    }
    settings.setValue(QStringLiteral("ollama/chatSessions"), stored);
    refreshChatSessions();
}

QString OllamaClient::exportChatSession(const QString& id) const
{
    const QVariantMap session = loadChatSession(id);
    if (session.isEmpty()) {
        return {};
    }
    QJsonObject exported;
    exported.insert(QStringLiteral("title"), session.value(QStringLiteral("title")).toString());
    exported.insert(QStringLiteral("systemPrompt"), session.value(QStringLiteral("systemPrompt")).toString());
    QJsonArray messages;
    for (const QVariant& value : session.value(QStringLiteral("messages")).toList()) {
        QJsonObject message = QJsonObject::fromVariantMap(value.toMap());
        message.remove(QStringLiteral("images"));
        messages.append(message);
    }
    exported.insert(QStringLiteral("messages"), messages);
    exported.insert(QStringLiteral("exportedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return QString::fromUtf8(QJsonDocument(exported).toJson(QJsonDocument::Indented));
}

bool OllamaClient::writeChatSessionExport(const QString& id, const QUrl& fileUrl) const
{
    if (!fileUrl.isLocalFile()) {
        return false;
    }
    const QString content = exportChatSession(id);
    if (content.isEmpty()) {
        return false;
    }
    QFile output(fileUrl.toLocalFile());
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return output.write(content.toUtf8()) == content.toUtf8().size();
}

bool OllamaClient::importCatalogSnapshot(const QUrl& fileUrl)
{
    if (!fileUrl.isLocalFile()) {
        emit requestFailed(QStringLiteral("import catalog snapshot"), QStringLiteral("Only a local verified snapshot file is allowed."));
        return false;
    }
    QFile file(fileUrl.toLocalFile());
    constexpr qint64 maximumSnapshotBytes = 16LL * 1024 * 1024;
    if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > maximumSnapshotBytes) {
        emit requestFailed(QStringLiteral("import catalog snapshot"), QStringLiteral("Snapshot must be a readable file of at most 16 MiB."));
        return false;
    }
    const QJsonObject snapshot = QJsonDocument::fromJson(file.readAll()).object();
    const QString origin = snapshot.value(QStringLiteral("origin")).toString();
    const QString revision = snapshot.value(QStringLiteral("revision")).toString();
    const int pageCount = snapshot.value(QStringLiteral("pageCount")).toInt(-1);
    const QJsonArray models = snapshot.value(QStringLiteral("models")).toArray();
    const QJsonArray pages = snapshot.value(QStringLiteral("pages")).toArray();
    if (origin != QStringLiteral("https://ollama.com/library") || revision.isEmpty() || revision.size() > 128 || pageCount < 1 || pageCount > 1000 || models.isEmpty() || models.size() > 10000
        || pages.size() != pageCount || snapshot.value(QStringLiteral("completeness")).toString() != QStringLiteral("model-and-tag-terminal-verified")) {
        emit requestFailed(QStringLiteral("import catalog snapshot"), QStringLiteral("Snapshot lacks verified terminal model and tag coverage."));
        return false;
    }
    QVariantList reconstructedModels;
    QVariantList reconstructedPages;
    for (const QJsonValue& page : pages) {
        const QJsonObject receipt = page.toObject();
        const QString url = receipt.value(QStringLiteral("url")).toString();
        const QString hash = receipt.value(QStringLiteral("sha256")).toString();
        if (!url.startsWith(QStringLiteral("https://ollama.com/library")) || hash.size() != 64 || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(hash).hasMatch()) return false;
        reconstructedPages << QVariantMap{{QStringLiteral("url"), url}, {QStringLiteral("sha256"), hash}};
    }
    QSet<QString> names;
    for (const QJsonValue& model : models) {
        const QJsonObject object = model.toObject();
        const QString name = object.value(QStringLiteral("name")).toString();
        const QJsonArray tags = object.value(QStringLiteral("tags")).toArray();
        if (name.isEmpty() || name.size() > 256 || name.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f]"))) || names.contains(name)
            || tags.isEmpty() || tags.size() > 10000) {
            emit requestFailed(QStringLiteral("import catalog snapshot"), QStringLiteral("Snapshot has a model without verified tag receipts."));
            return false;
        }
        names.insert(name);
        QVariantList safeTags; QSet<QString> seenTags;
        for (const QJsonValue& tag : tags) {
            const QString value = tag.toString();
            if (value.isEmpty() || value.size() > 256 || value.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f]"))) || seenTags.contains(value)) return false;
            seenTags.insert(value);
            safeTags << value;
        }
        reconstructedModels << QVariantMap{{QStringLiteral("name"), name}, {QStringLiteral("tags"), safeTags}};
    }
    m_catalogSnapshot = QVariantMap{{QStringLiteral("origin"), origin}, {QStringLiteral("revision"), revision}, {QStringLiteral("pageCount"), pageCount}, {QStringLiteral("pages"), reconstructedPages}, {QStringLiteral("models"), reconstructedModels}, {QStringLiteral("completeness"), QStringLiteral("untrusted-local-import")}};
    m_catalogSnapshot.insert(QStringLiteral("importedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    QSettings settings;
    settings.setValue(QStringLiteral("ollama/catalogSnapshot"), m_catalogSnapshot);
    emit catalogSnapshotChanged();
    return true;
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
    OllamaClient* self = const_cast<OllamaClient*>(this);
    self->m_queuedPulls = state;
    Q_EMIT self->queuedPullsChanged(state);
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

void OllamaClient::refreshChatSessions()
{
    QSettings settings;
    m_chatSessions = settings.value(QStringLiteral("ollama/chatSessions")).toList();
    emit chatSessionsChanged();
}

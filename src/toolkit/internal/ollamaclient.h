/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>
#include <QQueue>

namespace au::toolkit {
//! A thin client over the local Ollama HTTP API. The host defaults to
//! Ollama's own loopback default and is configurable, but only to another
//! loopback or private network address: this client never talks to an
//! arbitrary public host, because the whole point of the suite manager is
//! local, offline model management.
class OllamaClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(bool reachable READ reachable NOTIFY reachableChanged)
    Q_PROPERTY(QString version READ version NOTIFY versionChanged)
    Q_PROPERTY(QVariantList installedModels READ installedModels NOTIFY installedModelsListChanged)
    Q_PROPERTY(bool chatInFlight READ chatInFlight NOTIFY chatInFlightChanged)
    Q_PROPERTY(int pullConcurrency READ pullConcurrency WRITE setPullConcurrency NOTIFY pullConcurrencyChanged)
    Q_PROPERTY(int capabilityRevision READ capabilityRevision NOTIFY capabilityRevisionChanged)

public:
    explicit OllamaClient(QObject* parent = nullptr);

    QString host() const;
    void setHost(const QString& host);
    bool reachable() const;
    QString version() const;
    QVariantList installedModels() const;
    bool chatInFlight() const;
    int pullConcurrency() const;
    int capabilityRevision() const;
    void setPullConcurrency(int value);

    //! Returns true only when the host is loopback or a private network
    //! address (RFC 1918 / link-local / loopback). A public address is
    //! refused and the host is left unchanged.
    Q_INVOKABLE bool isAllowedHost(const QString& candidateHost) const;

    Q_INVOKABLE void refreshHealth();
    Q_INVOKABLE void refreshInstalledModels();
    Q_INVOKABLE void pullModel(const QString& modelTag);
    Q_INVOKABLE void cancelPull(const QString& modelTag);
    Q_INVOKABLE void sendChatMessage(const QString& model, const QVariantList& messages, const QVariantMap& parameters);
    Q_INVOKABLE void cancelChat();
    Q_INVOKABLE void inspectModel(const QString& modelTag);
    Q_INVOKABLE bool supportsImageAttachments(const QString& modelTag) const;
    Q_INVOKABLE bool attachImage(const QString& modelTag, const QUrl& fileUrl);
    Q_INVOKABLE void clearAttachments(const QString& modelTag);

signals:
    void hostChanged();
    void reachableChanged();
    void versionChanged();
    void installedModelsChanged(const QVariantList& models);
    void installedModelsListChanged();
    void pullProgress(const QString& modelTag, qint64 completedBytes, qint64 totalBytes, const QString& status);
    void pullFinished(const QString& modelTag, bool success, const QString& error);
    void chatToken(const QString& token, bool done);
    void chatError(const QString& error);
    void chatFinished(bool cancelled);
    void chatInFlightChanged();
    void pullConcurrencyChanged();
    void capabilityRevisionChanged();
    void modelInspected(const QString& modelTag, bool supportsImages);
    void attachmentRejected(const QString& reason);
    void queuedPullsChanged(const QVariantList& pulls);
    void requestFailed(const QString& what, const QString& reason);

private:
    QUrl endpoint(const QString& path) const;
    void startQueuedPulls();
    void startPull(const QString& modelTag);
    void persistPullQueue() const;
    void restorePullQueue();
    void setChatInFlight(bool value);

    QNetworkAccessManager m_network;
    QString m_host = QStringLiteral("127.0.0.1:11434");
    bool m_reachable = false;
    QString m_version;
    QVariantList m_installedModels;
    QHash<QString, QNetworkReply*> m_activePulls;
    QQueue<QString> m_pullQueue;
    QNetworkReply* m_chatReply = nullptr;
    QByteArray m_chatBuffer;
    bool m_chatInFlight = false;
    bool m_chatCancelled = false;
    int m_pullConcurrency = 2;
    int m_capabilityRevision = 0;
    QHash<QString, bool> m_imageCapabilities;
    QHash<QString, QStringList> m_pendingImages;
};
}

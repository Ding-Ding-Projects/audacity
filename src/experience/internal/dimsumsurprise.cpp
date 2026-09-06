#include "shared/profilepaths.h"
/*
 * Audacity: A Digital Audio Editor
 */
#include "dimsumsurprise.h"

#include <random>

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTimer>

using namespace au::experience;

namespace {
double randomUnitSample()
{
    static thread_local std::mt19937 engine(std::random_device {}());
    static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(engine);
}
}

DimSumDraw::DimSumDraw(double probability)
    : m_probability(probability)
{
}

bool DimSumDraw::draw()
{
    return drawWithSample(randomUnitSample());
}

bool DimSumDraw::drawWithSample(double sample)
{
    if (m_consumed) {
        return false;
    }
    m_consumed = true;
    return sample < m_probability;
}

QString DimSumSurpriseService::catalogUrl()
{
    return QStringLiteral(
        "https://raw.githubusercontent.com/Ding-Ding-Projects/dim-sum-photos/main/catalog/index.json");
}

QString DimSumSurpriseService::releaseAssetUrl(const QString& assetFileName)
{
    if (assetFileName.isEmpty()) {
        return QString();
    }
    return QStringLiteral("https://github.com/Ding-Ding-Projects/dim-sum-photos/releases/download/catalog-v1/")
           + assetFileName;
}

bool DimSumSurpriseService::isAllowedRedirectTarget(const QUrl& url)
{
    if (!url.isValid() || url.scheme() != QStringLiteral("https")) {
        return false;
    }

    // The exact hosts a genuine GitHub release asset download can redirect
    // through. github.com and raw.githubusercontent.com are included for
    // completeness even though the observed chain only ever needs the two
    // signed object storage hosts; nothing outside this exact set is ever
    // trusted, regardless of what a redirect response claims.
    static const QVector<QString> allowedHosts = {
        QStringLiteral("github.com"),
        QStringLiteral("objects.githubusercontent.com"),
        QStringLiteral("release-assets.githubusercontent.com"),
        QStringLiteral("raw.githubusercontent.com"),
    };

    return allowedHosts.contains(url.host());
}

DimSumSurpriseService::DimSumSurpriseService(QObject* parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
    // Honour the desktop's own proxy configuration (an http_proxy or
    // https_proxy environment variable, or the platform's system proxy
    // settings), exactly as an ordinary desktop browser or download tool
    // would. Without this, a machine that only reaches the public internet
    // through a configured proxy would silently fail every fetch here.
    QNetworkProxyFactory::setUseSystemConfiguration(true);
}

QString DimSumSurpriseService::cacheDirectory() const
{
    const QString base = au::profile::Paths::writableLocation(QStandardPaths::AppDataLocation);
    const QString dir = base + QStringLiteral("/dimsum-cache");
    QDir().mkpath(dir);
    return dir;
}

QVector<DimSumDish> DimSumSurpriseService::cachedCatalog() const
{
    QFile file(cacheDirectory() + QStringLiteral("/catalog.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return DimSumCatalog::parse(file.readAll());
}

void DimSumSurpriseService::refreshCatalogAsync()
{
    if (au::profile::Paths::active()) { emit catalogRefreshed(false); return; }
    if (m_refreshInFlight) {
        return;
    }
    m_refreshInFlight = true;

    QNetworkRequest request { QUrl(catalogUrl()) };
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setMaximumRedirectsAllowed(0);

    QNetworkReply* reply = m_network->get(request);

    QTimer* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, &QNetworkReply::abort);
    timeoutTimer->start(TIMEOUT_MS);

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleCatalogReply(reply);
    });
}

void DimSumSurpriseService::handleCatalogReply(QNetworkReply* reply)
{
    m_refreshInFlight = false;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit catalogRefreshed(false);
        return;
    }

    const QByteArray body = reply->read(MAX_RESPONSE_BYTES + 1);
    if (body.size() > MAX_RESPONSE_BYTES) {
        emit catalogRefreshed(false);
        return;
    }

    const QVector<DimSumDish> dishes = DimSumCatalog::parse(body);
    if (dishes.isEmpty()) {
        emit catalogRefreshed(false);
        return;
    }

    QDir().mkpath(cacheDirectory());
    QFile file(cacheDirectory() + QStringLiteral("/catalog.json"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(body);
    }

    emit catalogRefreshed(true);
}

QString DimSumSurpriseService::photoCachePath(const DimSumDish& dish) const
{
    if (dish.id.isEmpty()) {
        return QString();
    }
    const QString photosDir = cacheDirectory() + QStringLiteral("/photos");
    QDir().mkpath(photosDir);

    // The dish id is the project's own stable, filesystem-safe identifier
    // (for example "hk-dish-0001"); the extension follows the asset the
    // catalog actually named, defaulting to png.
    QString extension = QFileInfo(dish.photoAsset).suffix();
    if (extension.isEmpty()) {
        extension = QStringLiteral("png");
    }
    return photosDir + QStringLiteral("/") + dish.id + QStringLiteral(".") + extension;
}

QString DimSumSurpriseService::cachedPhotoPath(const DimSumDish& dish) const
{
    const QString path = photoCachePath(dish);
    if (path.isEmpty() || !QFile::exists(path)) {
        return QString();
    }
    return QStringLiteral("file://") + path;
}

void DimSumSurpriseService::refreshPhotoAsync(const DimSumDish& dish)
{
    if (dish.photoAsset.isEmpty() || m_photoFetchesInFlight.contains(dish.id)) {
        return;
    }
    m_photoFetchesInFlight.push_back(dish.id);

    startPhotoRequest(QUrl(releaseAssetUrl(dish.photoAsset)), dish.id, photoCachePath(dish), MAX_REDIRECTS);
}

void DimSumSurpriseService::startPhotoRequest(const QUrl& url, const QString& dishId,
                                              const QString& destinationPath, int redirectsRemaining)
{
    if (au::profile::Paths::active()) {
        m_photoFetchesInFlight.removeAll(dishId);
        emit photoRefreshed(dishId, false);
        return;
    }
    QNetworkRequest request { url };
    // Redirects are followed manually, one hop at a time, only after the
    // target has been checked against the exact allowed host list below.
    // Qt's own automatic policy has no such allowlist, so it is never used
    // here even though it would happily also fetch the redirected asset.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setMaximumRedirectsAllowed(0);

    QNetworkReply* reply = m_network->get(request);

    QTimer* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, &QNetworkReply::abort);
    timeoutTimer->start(TIMEOUT_MS);

    QObject::connect(reply, &QNetworkReply::finished, this,
                     [this, reply, dishId, destinationPath, redirectsRemaining]() {
        handlePhotoReply(reply, dishId, destinationPath, redirectsRemaining);
    });
}

void DimSumSurpriseService::handlePhotoReply(QNetworkReply* reply, const QString& dishId,
                                             const QString& destinationPath, int redirectsRemaining)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_photoFetchesInFlight.removeAll(dishId);
        emit photoRefreshed(dishId, false);
        return;
    }

    const QVariant redirectAttribute = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectAttribute.isValid()) {
        const QUrl redirectTarget = reply->url().resolved(redirectAttribute.toUrl());
        if (redirectsRemaining <= 0 || !isAllowedRedirectTarget(redirectTarget)) {
            m_photoFetchesInFlight.removeAll(dishId);
            emit photoRefreshed(dishId, false);
            return;
        }
        startPhotoRequest(redirectTarget, dishId, destinationPath, redirectsRemaining - 1);
        return;
    }

    // A photo is a larger asset than the catalog document itself; bound it
    // generously but still refuse an unbounded response.
    static constexpr int MAX_PHOTO_BYTES = 8 * 1024 * 1024;
    const QByteArray body = reply->read(MAX_PHOTO_BYTES + 1);
    m_photoFetchesInFlight.removeAll(dishId);
    if (body.isEmpty() || body.size() > MAX_PHOTO_BYTES) {
        emit photoRefreshed(dishId, false);
        return;
    }

    QFile file(destinationPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(body) != body.size()) {
        emit photoRefreshed(dishId, false);
        return;
    }

    emit photoRefreshed(dishId, true);
}

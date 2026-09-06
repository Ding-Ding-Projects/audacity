/*
 * Audacity: A Digital Audio Editor
 */
#include "dimsumsurprise.h"

#include <random>

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
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

DimSumSurpriseService::DimSumSurpriseService(QObject* parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

QString DimSumSurpriseService::cacheDirectory() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
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

    QNetworkRequest request { QUrl(releaseAssetUrl(dish.photoAsset)) };
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setMaximumRedirectsAllowed(0);

    QNetworkReply* reply = m_network->get(request);

    QTimer* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, &QNetworkReply::abort);
    timeoutTimer->start(TIMEOUT_MS);

    const QString destinationPath = photoCachePath(dish);
    const QString dishId = dish.id;
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, dishId, destinationPath]() {
        handlePhotoReply(reply, dishId, destinationPath);
    });
}

void DimSumSurpriseService::handlePhotoReply(QNetworkReply* reply, const QString& dishId,
                                             const QString& destinationPath)
{
    m_photoFetchesInFlight.removeAll(dishId);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit photoRefreshed(dishId, false);
        return;
    }

    // A photo is a larger asset than the catalog document itself; bound it
    // generously but still refuse an unbounded response.
    static constexpr int MAX_PHOTO_BYTES = 8 * 1024 * 1024;
    const QByteArray body = reply->read(MAX_PHOTO_BYTES + 1);
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

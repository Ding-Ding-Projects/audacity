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

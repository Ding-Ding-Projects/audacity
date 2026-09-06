/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include "dimsumcatalog.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace au::experience {
//! Decides whether this launch shows the dim sum surprise, and if so, picks
//! one dish. A fresh draw happens once per process; the same instance never
//! draws twice, so calling shouldShow() a second time in one launch always
//! answers false once it has already answered true once (or been asked
//! once at all - the draw is consumed on first use).
//!
//! There is deliberately no setting anywhere that can turn this off. The
//! only thing callers may do is decide not to ask (first run, error path,
//! active dialog or task), which is a display-timing decision, not an
//! opt-out.
class DimSumDraw
{
public:
    static constexpr double PROBABILITY = 0.10;

    explicit DimSumDraw(double probability = PROBABILITY);

    //! Consumes this launch's single draw and returns whether the surprise
    //! should show. Every call after the first returns false.
    bool draw();

    //! Same as draw() but with an injected random value in [0, 1) so tests
    //! are deterministic. Still consumes the one-shot state.
    bool drawWithSample(double sample);

private:
    double m_probability;
    bool m_consumed = false;
};

//! Fetches the public dim sum catalog and caches it plus one selected photo
//! in the application data directory. Bounded size and timeout, at most two
//! redirects and only onto an explicitly allowlisted https host, no third
//! party mirrors. Never vendors an image into this repository; this class
//! only ever writes into the local cache directory.
class DimSumSurpriseService : public QObject
{
    Q_OBJECT

public:
    // The real published catalog (2,866 dishes, each with a paragraph of
    // generation prompt text) is a little over 8 MB, well past a
    // conservative 2 MB guess; this leaves headroom for the catalog to grow
    // further while still refusing anything unbounded.
    static constexpr int MAX_RESPONSE_BYTES = 16 * 1024 * 1024;
    static constexpr int TIMEOUT_MS = 6000;
    //! A release asset download always answers with one redirect from
    //! github.com to a signed, time-limited object storage URL. Two hops
    //! covers that plus one extra without opening the door to an open
    //! redirect chain.
    static constexpr int MAX_REDIRECTS = 2;

    static QString catalogUrl();
    //! Builds the direct asset URL for a published catalog-v1* release asset
    //! file name. The caller already knows the file name from the catalog.
    static QString releaseAssetUrl(const QString& assetFileName);

    //! True only when a redirect target is safe to follow: an https URL
    //! whose host is one of the exact hosts a genuine GitHub release
    //! download can redirect through. Anything else, including plain http,
    //! an unlisted host, or a malformed URL, is refused.
    static bool isAllowedRedirectTarget(const QUrl& url);

    explicit DimSumSurpriseService(QObject* parent = nullptr);

    //! Directory the catalog and photos are cached into, created on demand.
    QString cacheDirectory() const;

    //! Reads a previously cached catalog file, if one exists and parses.
    QVector<DimSumDish> cachedCatalog() const;

    //! Starts a bounded background refresh of the catalog. Safe to call
    //! repeatedly; a refresh already in flight is not duplicated.
    void refreshCatalogAsync();

    //! The local file a dish's photo would be cached at, whether or not it
    //! has actually been fetched yet.
    QString photoCachePath(const DimSumDish& dish) const;

    //! Returns that path only when the file genuinely exists on disk, empty
    //! otherwise. Never claims a photo is available on the strength of
    //! catalog metadata alone.
    QString cachedPhotoPath(const DimSumDish& dish) const;

    //! Starts a bounded background fetch of one dish's photo from its
    //! published catalog-v1* release asset. Safe to call repeatedly for the
    //! same dish; a fetch already in flight for it is not duplicated.
    void refreshPhotoAsync(const DimSumDish& dish);

signals:
    void catalogRefreshed(bool ok);
    void photoRefreshed(QString dishId, bool ok);

private:
    void handleCatalogReply(QNetworkReply* reply);
    void startPhotoRequest(const QUrl& url, const QString& dishId, const QString& destinationPath, int redirectsRemaining);
    void handlePhotoReply(QNetworkReply* reply, const QString& dishId, const QString& destinationPath, int redirectsRemaining);

    QNetworkAccessManager* m_network = nullptr;
    bool m_refreshInFlight = false;
    QVector<QString> m_photoFetchesInFlight;
};
}

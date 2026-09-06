/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QObject>
#include <QString>

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
//! in the application data directory. Bounded size and timeout, no
//! redirects, no third party mirrors. Never vendors an image into this
//! repository; this class only ever writes into the local cache directory.
class DimSumSurpriseService : public QObject
{
    Q_OBJECT

public:
    static constexpr int MAX_RESPONSE_BYTES = 2 * 1024 * 1024;
    static constexpr int TIMEOUT_MS = 6000;

    static QString catalogUrl();
    //! Builds the direct asset URL for a published catalog-v1* release asset
    //! file name. The caller already knows the file name from the catalog.
    static QString releaseAssetUrl(const QString& assetFileName);

    explicit DimSumSurpriseService(QObject* parent = nullptr);

    //! Directory the catalog and photos are cached into, created on demand.
    QString cacheDirectory() const;

    //! Reads a previously cached catalog file, if one exists and parses.
    QVector<DimSumDish> cachedCatalog() const;

    //! Starts a bounded background refresh of the catalog. Safe to call
    //! repeatedly; a refresh already in flight is not duplicated.
    void refreshCatalogAsync();

signals:
    void catalogRefreshed(bool ok);

private:
    void handleCatalogReply(QNetworkReply* reply);

    QNetworkAccessManager* m_network = nullptr;
    bool m_refreshInFlight = false;
};
}

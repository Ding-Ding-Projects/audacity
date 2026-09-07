/*
 * Audacity: A Digital Audio Editor
 */
#include "dimsumsurprisemodel.h"

#include <random>

using namespace au::experience;

DimSumSurpriseModel::DimSumSurpriseModel(QObject* parent)
    : QObject(parent)
{
}

bool DimSumSurpriseModel::isFirstRun() const
{
    if (!appShellConfiguration()) {
        // The app shell configuration could not be reached at all. Fail
        // safe: treat an uncertain state as a first run so the surprise
        // never appears before we can honestly tell.
        return true;
    }
    return !appShellConfiguration()->hasCompletedFirstLaunchSetup();
}

void DimSumSurpriseModel::offerIfDue()
{
    // School mode suppresses the surprise entirely, as if it were not
    // installed: no draw happens.
    const SchoolModeStore::SharedRecordResult schoolMode = SchoolModeStore::sharedRecord();
    if ((!schoolMode.available && !schoolMode.hasKnownRecord) || (schoolMode.hasKnownRecord && schoolMode.record.on)) {
        return;
    }

    if (isFirstRun()) {
        return;
    }

    const bool forced = qEnvironmentVariableIntValue("AU_DIM_SUM_FORCE") != 0;
    const bool won = forced ? true : m_draw.draw();
    if (!won) {
        return;
    }

    const QVector<DimSumDish> dishes = m_service.cachedCatalog();
    if (dishes.isEmpty()) {
        // Nothing cached yet and no network fetch has completed: there is
        // nothing honest to show, so stay quiet rather than inventing a
        // dish. A future launch, once the cache has been refreshed at
        // least once, will be able to draw for real.
        m_service.refreshCatalogAsync();
        return;
    }

    static thread_local std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> pick(0, dishes.size() - 1);
    m_dish = dishes.at(pick(engine));
    // Only claim a photo is available once a locally cached copy of it
    // actually exists; the catalog naming a photo asset is not the same
    // as this machine having fetched it yet.
    m_photoPath = m_service.cachedPhotoPath(m_dish);
    if (m_photoPath.isEmpty()) {
        // Fetch it in the background so a future launch can show it; this
        // launch still shows the honest offline placeholder.
        m_service.refreshPhotoAsync(m_dish);
    }
    m_visible = true;
    emit revealed();
}

/*
 * Audacity: A Digital Audio Editor
 */
#include "dimsumsurprisemodel.h"

#include <cstdlib>
#include <random>

#include <QFile>

using namespace au::experience;

DimSumSurpriseModel::DimSumSurpriseModel(QObject* parent)
    : QObject(parent)
{
}

void DimSumSurpriseModel::offerIfDue(bool isFirstRun)
{
    // School mode suppresses the surprise entirely, as if it were not
    // installed: no draw happens.
    QFile schoolModeFile(SchoolModeStore::sharedFilePath());
    QByteArray schoolModeData;
    if (schoolModeFile.open(QIODevice::ReadOnly)) {
        schoolModeData = schoolModeFile.readAll();
    }
    m_schoolMode = SchoolModeStore::parse(schoolModeData);
    if (m_schoolMode.ok && m_schoolMode.record.on) {
        return;
    }

    if (isFirstRun) {
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

    static thread_local std::mt19937 engine(std::random_device {}());
    std::uniform_int_distribution<int> pick(0, dishes.size() - 1);
    m_dish = dishes.at(pick(engine));
    m_photoPath = QString();
    m_visible = true;
    emit revealed();
}

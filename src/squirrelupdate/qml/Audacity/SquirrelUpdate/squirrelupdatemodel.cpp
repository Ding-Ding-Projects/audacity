/*
* Audacity: A Digital Audio Editor
*/
#include "squirrelupdatemodel.h"

#include "framework/global/translation.h"

#include "internal/restartcoordinator.h"

namespace au::squirrelupdate {
SquirrelUpdateModel::SquirrelUpdateModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void SquirrelUpdateModel::load()
{
    service()->availableUpdateChanged().onNotify(this, [this]() {
        if (!service()->isChecking()) {
            m_lastCheckAt = QDateTime::currentDateTime();
        }
        emit stateChanged();
    });

    configuration()->changed().onNotify(this, [this]() {
        emit settingsChanged();
    });
}

bool SquirrelUpdateModel::isWindows() const
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool SquirrelUpdateModel::isSupported() const
{
    return service()->isSupported();
}

int SquirrelUpdateModel::state() const
{
    if (demoBannerForced()) {
        return static_cast<int>(UpdateBannerState::Ready);
    }

    const UpdateBannerState computed = computeUpdateBannerState(
        isWindows(),
        service()->isSupported(),
        service()->isChecking(),
        service()->availableUpdate().available,
        !service()->lastError().isEmpty());

    return static_cast<int>(computed);
}

QString SquirrelUpdateModel::installedVersion() const
{
    return service()->installedVersion();
}

QString SquirrelUpdateModel::availableVersion() const
{
    if (demoBannerForced() && service()->availableUpdate().version.isEmpty()) {
        return QStringLiteral("4.1.0-demo");
    }
    return service()->availableUpdate().version;
}

QString SquirrelUpdateModel::lastError() const
{
    return service()->lastError();
}

bool SquirrelUpdateModel::bannerVisible() const
{
    return service()->availableUpdate().available || demoBannerForced();
}

bool SquirrelUpdateModel::demoBannerForced() const
{
    return qEnvironmentVariableIntValue("AU_SQUIRREL_DEMO_BANNER") != 0;
}

QString SquirrelUpdateModel::lastCheckDisplay() const
{
    if (!m_lastCheckAt.isValid()) {
        return muse::qtrc("squirrelupdate", "No check has run yet in this session.");
    }
    return QLocale().toString(m_lastCheckAt, QLocale::ShortFormat);
}

bool SquirrelUpdateModel::enabled() const
{
    return configuration()->isEnabled();
}

void SquirrelUpdateModel::setEnabled(bool value)
{
    configuration()->setEnabled(value);
}

QString SquirrelUpdateModel::feedUrl() const
{
    return configuration()->feedUrl();
}

int SquirrelUpdateModel::checkIntervalHours() const
{
    return configuration()->checkIntervalHours();
}

void SquirrelUpdateModel::checkForUpdate()
{
    service()->checkForUpdate();
    emit stateChanged();
}

void SquirrelUpdateModel::restartToUpdate()
{
    // The lambdas keep RestartCoordinator itself free of Qt and of the
    // modularity container, so its actual decision is unit tested directly.
    // closeOpenedProject(false) is the exact same call
    // ApplicationActionController::quit and ::restart already make before
    // doing anything irreversible: it saves, offers to discard, or lets the
    // user cancel, and returns false only for a cancel.
    auto closeOpenedProjects = [this]() {
        auto pfc = projectFilesController();
        return !pfc || pfc->closeOpenedProject(false);
    };
    auto applyUpdate = [this]() {
        return service()->restartToUpdate();
    };

    const muse::Ret result = RestartCoordinator::attemptRestart(closeOpenedProjects, applyUpdate);
    if (!result) {
        emit restartBlocked(QString::fromStdString(result.toString()));
    }
}

void SquirrelUpdateModel::dismiss()
{
    service()->dismiss();
    emit stateChanged();
}
}

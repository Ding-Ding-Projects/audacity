/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QObject>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"

#include "isquirrelupdateconfiguration.h"
#include "isquirrelupdateservice.h"

namespace au::squirrelupdate {
//! The one visible state a user needs to see, computed from the service.
//! Kept as a plain enum reachable from QML and, through the pure function
//! below, testable with no Qt event loop and no injected dependencies.
enum class UpdateBannerState {
    Hidden,
    NoUpdate,
    Checking,
    Available,
    Downloading,
    Ready,
    Failed,
    Offline,
    InvalidMetadata,
    CorruptAsset,
    Cancelled,
    Rollback,
    NotApplicable
};

//! The model behind the Help > Check for updates action, the Updates
//! preferences page and the ready to restart banner.
class SquirrelUpdateModel : public QObject, public muse::async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isWindows READ isWindows CONSTANT FINAL)
    Q_PROPERTY(bool isSupported READ isSupported NOTIFY stateChanged FINAL)
    Q_PROPERTY(int state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString installedVersion READ installedVersion CONSTANT FINAL)
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool bannerVisible READ bannerVisible NOTIFY stateChanged FINAL)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY settingsChanged FINAL)
    Q_PROPERTY(QString feedUrl READ feedUrl NOTIFY settingsChanged FINAL)
    Q_PROPERTY(int checkIntervalHours READ checkIntervalHours NOTIFY settingsChanged FINAL)
    Q_PROPERTY(QString lastCheckDisplay READ lastCheckDisplay NOTIFY stateChanged FINAL)

    muse::GlobalInject<ISquirrelUpdateConfiguration> configuration;
    muse::GlobalInject<ISquirrelUpdateService> service;

public:
    explicit SquirrelUpdateModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    bool isWindows() const;
    bool isSupported() const;
    int state() const;
    QString installedVersion() const;
    QString availableVersion() const;
    QString lastError() const;
    bool bannerVisible() const;
    QString lastCheckDisplay() const;

    bool enabled() const;
    void setEnabled(bool value);
    QString feedUrl() const;
    int checkIntervalHours() const;

    Q_INVOKABLE void checkForUpdate();
    Q_INVOKABLE void restartToUpdate();
    Q_INVOKABLE void dismiss();

    //! Pure computation of the banner state from the service's observable
    //! facts, kept free of Qt signals and settings so it can be unit tested
    //! directly, and reused by the real property above.
    static UpdateBannerState computeState(bool isWindowsPlatform, bool isSupportedPlatform, bool checking,
                                          bool available, bool lastErrorPresent);

signals:
    void stateChanged();
    void settingsChanged();

private:
    QDateTime m_lastCheckAt;
};
}

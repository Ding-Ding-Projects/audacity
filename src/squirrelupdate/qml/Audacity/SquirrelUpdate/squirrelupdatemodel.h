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
#include "updatebannerstate.h"

namespace au::squirrelupdate {
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
    Q_PROPERTY(bool demoBannerForced READ demoBannerForced CONSTANT FINAL)

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
    //! True when AU_SQUIRREL_DEMO_BANNER=1 is set in the environment. Used
    //! only to force the banner and the preferences ready state visible for
    //! screenshots and manual review, with no real network access.
    bool demoBannerForced() const;
    QString lastCheckDisplay() const;

    bool enabled() const;
    void setEnabled(bool value);
    QString feedUrl() const;
    int checkIntervalHours() const;

    Q_INVOKABLE void checkForUpdate();
    Q_INVOKABLE void restartToUpdate();
    Q_INVOKABLE void dismiss();

signals:
    void stateChanged();
    void settingsChanged();

private:
    QDateTime m_lastCheckAt;
};
}

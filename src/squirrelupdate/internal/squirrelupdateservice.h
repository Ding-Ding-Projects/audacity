/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QObject>
#include <QString>

#include "framework/global/modularity/ioc.h"
#include "framework/global/async/asyncable.h"
#include "framework/network/inetworkmanagercreator.h"

#include "isquirrelupdateconfiguration.h"
#include "isquirrelupdateservice.h"

class QTimer;

namespace au::squirrelupdate {
class SquirrelUpdateService : public QObject, public ISquirrelUpdateService, public muse::async::Asyncable
{
    Q_OBJECT

    muse::GlobalInject<ISquirrelUpdateConfiguration> configuration;
    muse::GlobalInject<muse::network::INetworkManagerCreator> networkManagerCreator;

public:
    explicit SquirrelUpdateService(QObject* parent = nullptr);
    ~SquirrelUpdateService() override;

    void init();

    bool isSupported() const override;
    QString installedVersion() const override;

    void checkForUpdate() override;
    bool isChecking() const override;

    AvailableUpdate availableUpdate() const override;
    muse::async::Notification availableUpdateChanged() const override;

    void dismiss() override;

    muse::Ret restartToUpdate() override;

    QString lastError() const override;

private:
    void onFeedDownloaded(const QByteArray& feed);
    void downloadPackage(const ReleaseEntry& entry);
    void onPackageDownloaded(const ReleaseEntry& entry, const QByteArray& bytes);

    void finishWithError(const QString& error);
    void restartTimer();

    //! %LOCALAPPDATA%\SquirrelTemp on Windows, the temporary directory
    //! elsewhere. Squirrel's own updater uses the same folder.
    QString squirrelTempDir() const;

    QString updaterPath() const;

    muse::network::INetworkManagerPtr m_networkManager;

    QTimer* m_timer = nullptr;

    bool m_checking = false;
    bool m_dismissed = false;

    AvailableUpdate m_available;
    QString m_lastError;

    muse::async::Notification m_availableUpdateChanged;
};
}

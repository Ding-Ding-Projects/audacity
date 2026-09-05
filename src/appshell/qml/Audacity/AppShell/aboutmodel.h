/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include "framework/global/modularity/ioc.h"
#include "iappshellconfiguration.h"
#include "framework/global/iglobalconfiguration.h"
#include "framework/global/iapplication.h"
#include "framework/update/iupdateconfiguration.h"
#include "framework/global/io/ifilesystem.h"

class QUrl;

namespace au::appshell {
class AboutModel : public QObject, public muse::Contextable
{
    Q_OBJECT
    QML_ELEMENT

    muse::GlobalInject<IAppShellConfiguration> configuration;
    muse::GlobalInject<muse::IGlobalConfiguration> globalConfiguration;
    muse::GlobalInject<muse::IApplication> application;
    muse::GlobalInject<muse::update::IUpdateConfiguration> updateConfiguration;
    muse::GlobalInject<muse::io::IFileSystem> fileSystem;

public:
    explicit AboutModel(QObject* parent = nullptr);

    Q_INVOKABLE QString appVersion() const;

    //! NOTE Build provenance, fixed when the build was configured. Never the
    //! time the application was started.
    Q_INVOKABLE QString buildVersion() const;
    Q_INVOKABLE QString buildUpdatedAtUtc() const;
    Q_INVOKABLE QString buildUpdatedAtLocal() const;
    Q_INVOKABLE QString appRevision() const;
    Q_INVOKABLE QVariantMap appUrl() const;
    Q_INVOKABLE QVariantMap forumUrl() const;
    Q_INVOKABLE QVariantMap contributionUrl() const;
    Q_INVOKABLE QVariantMap privacyPolicyUrl() const;
    Q_INVOKABLE QVariantList creditList() const;
    Q_INVOKABLE QString gplText() const;

    Q_INVOKABLE void copyRevisionToClipboard() const;

    Q_INVOKABLE void toggleDevMode();

private:
    QVariantMap makeUrl(const QUrl& url, bool showPath = true) const;
};
}

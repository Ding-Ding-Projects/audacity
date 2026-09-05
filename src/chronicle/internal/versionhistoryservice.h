/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

#include "framework/global/modularity/ioc.h"
#include "framework/global/async/asyncable.h"
#include "framework/global/iglobalconfiguration.h"

#include "iversionhistoryservice.h"
#include "isnapshotstore.h"

namespace au::chronicle {
/*!
 * The default implementation of the local version history.
 *
 * Everything a snapshot records is first staged into a directory the service
 * owns, at <appDataDir>/history/stage/<project-id>. The staging directory
 * holds a copy of the project file, the settings file and the preset files, so
 * a revision is one coherent picture and the diff summary names files the user
 * recognises. The user's own project folder is only ever read.
 */
class VersionHistoryService : public QObject, public IVersionHistoryService, public muse::async::Asyncable
{
public:
    VersionHistoryService();

    void init();

    QString storeKind() const override;

    void setCurrentProject(const QString& projectId, const QString& projectFilePath) override;
    QString currentProjectId() const override { return m_projectId; }

    QString recordSnapshot(const QString& action, const QString& detail = QString(), const QString& explicitLabel = QString()) override;

    QList<Revision> revisions() const override;
    QList<RevisionFile> files(const QString& revisionId) const override;

    bool restore(const QString& revisionId) override;
    bool exportRevision(const QString& revisionId, const QString& destinationDir) override;
    bool setLabel(const QString& revisionId, const QString& label) override;

    int prune() override;

    int retentionCount() const override;
    void setRetentionCount(int value) override;
    int retentionDays() const override;
    void setRetentionDays(int value) override;

    muse::async::Notification revisionsChanged() const override { return m_revisionsChanged; }

    //! Derives the label shown in the panel from the action and its detail.
    //! Public so that the behaviour can be asserted directly by the tests.
    static QString deriveLabel(const QString& action, const QString& detail);

    //! Test seam: use this store and this root instead of the ones the
    //! application would choose.
    void setStoreForTesting(const ISnapshotStorePtr& store, const QString& root);

private:
    muse::GlobalInject<muse::IGlobalConfiguration> globalConfiguration;

    QString historyRoot() const;
    QString storePathFor(const QString& projectId) const;
    QString stagePathFor(const QString& projectId) const;

    bool openStore(const QString& projectId);
    void stage();
    void stageProjectFile();
    void stageSettingsFile();
    void stagePresets();
    static bool copyInto(const QString& sourceFile, const QString& destinationFile);

    ISnapshotStorePtr m_store;
    QString m_projectId;
    QString m_projectFilePath;
    QString m_rootOverride;

    QFileSystemWatcher* m_settingsWatcher = nullptr;
    QTimer* m_settingsDebounce = nullptr;

    mutable muse::async::Notification m_revisionsChanged;
};

using VersionHistoryServicePtr = std::shared_ptr<VersionHistoryService>;
}

/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "framework/global/modularity/imoduleinterface.h"
#include "framework/global/async/notification.h"

#include "types/chronicletypes.h"

namespace au::chronicle {
/*!
 * The local, Git backed version history.
 *
 * The history lives in an isolated repository beside the application data
 * directory, at <appDataDir>/history/<project-id>.git. It is never placed
 * inside the user's project folder, so opening a project never adds anything
 * the user did not ask for.
 *
 * The history is append only. A restore writes the old content back and is
 * itself recorded as a new revision, so the sequence of what happened is never
 * rewritten and a restore can always be undone by restoring the revision
 * before it.
 */
class IVersionHistoryService : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(au::chronicle::IVersionHistoryService)

public:
    virtual ~IVersionHistoryService() = default;

    //! "git" when the git executable was found on PATH, otherwise
    //! "content-addressed" for the built in fallback store.
    virtual QString storeKind() const = 0;

    //! Selects the history of one project. An empty id selects the history
    //! that is not tied to a project, which still records settings changes.
    virtual void setCurrentProject(const QString& projectId, const QString& projectFilePath) = 0;
    virtual QString currentProjectId() const = 0;

    //! Records a snapshot. The label is derived from the action and the detail
    //! when no explicit label is given, for example "Deleted track Vocals".
    virtual QString recordSnapshot(const QString& action, const QString& detail = QString(), const QString& explicitLabel = QString()) = 0;

    virtual QList<Revision> revisions() const = 0;
    virtual QList<RevisionFile> files(const QString& revisionId) const = 0;

    //! Restores a revision and records the restore as a new revision.
    virtual bool restore(const QString& revisionId) = 0;
    virtual bool exportRevision(const QString& revisionId, const QString& destinationDir) = 0;
    virtual bool setLabel(const QString& revisionId, const QString& label) = 0;

    //! Applies the retention settings. Returns the number of revisions pruned.
    virtual int prune() = 0;

    virtual int retentionCount() const = 0;
    virtual void setRetentionCount(int value) = 0;
    virtual int retentionDays() const = 0;
    virtual void setRetentionDays(int value) = 0;

    virtual muse::async::Notification revisionsChanged() const = 0;
};
}

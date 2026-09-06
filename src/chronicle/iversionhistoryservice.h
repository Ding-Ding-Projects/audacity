/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QByteArray>

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
    //! Purely decorative: does not affect retention.
    virtual bool setStarred(const QString& revisionId, bool starred) = 0;
    //! A pinned revision is excluded from retention, exactly like the
    //! newest revision itself.
    virtual bool setPinned(const QString& revisionId, bool pinned) = 0;

    //! Applies the retention settings. Returns the number of revisions pruned.
    virtual int prune() = 0;

    virtual int retentionCount() const = 0;
    virtual void setRetentionCount(int value) = 0;
    virtual int retentionDays() const = 0;
    virtual void setRetentionDays(int value) = 0;

    //! When true (the default), a revision is recorded for every undoable
    //! action pushed onto the project's undo stack, not only on save. A
    //! continuous drag stays one revision because Audacity itself
    //! consolidates it into one undo entry.
    virtual bool commitOnEveryAction() const = 0;
    virtual void setCommitOnEveryAction(bool value) = 0;

    //! When true (the default), the whole local history is packed and
    //! embedded into the project's own save file on every save, so the
    //! history follows the file to another machine.
    virtual bool embedHistoryInSaveFile() const = 0;
    virtual void setEmbedHistoryInSaveFile(bool value) = 0;

    //! Packs the current project's history for embedding into its save
    //! file. Returns an empty array when there is nothing to pack yet or
    //! packing fails; a caller must never fail the save over that.
    virtual QByteArray packHistoryForEmbedding() const = 0;

    //! The format string to record alongside a call to
    //! packHistoryForEmbedding(), naming which backend produced the bytes.
    virtual QString embeddedHistoryFormat() const = 0;

    //! Absorbs history embedded in a save file, adopting it only when it is
    //! strictly ahead of what is already recorded here. Returns true when it
    //! was adopted.
    virtual bool absorbEmbeddedHistory(const QByteArray& data) = 0;

    virtual muse::async::Notification revisionsChanged() const = 0;

    //! The directory every project's history repository lives beside, for
    //! the storage panel to measure disk usage from. Never inside a user's
    //! own project folder.
    virtual QString historyRootPath() const = 0;
};
}

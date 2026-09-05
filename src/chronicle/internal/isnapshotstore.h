/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <memory>

#include "types/chronicletypes.h"

namespace au::chronicle {
/*!
 * The storage behind the local version history.
 *
 * Two implementations satisfy this interface and they are interchangeable:
 * GitSnapshotStore drives the git executable found on PATH, and
 * FileSnapshotStore is a content addressed store used when git is missing.
 *
 * Every store is append only. A restore never rewrites or removes a
 * revision; it writes the restored content back into the working tree and the
 * caller records that as a new revision.
 */
class ISnapshotStore
{
public:
    virtual ~ISnapshotStore() = default;

    //! "git" or "content-addressed". Reported in the panel so the user knows
    //! which backend their history is kept in.
    virtual QString kind() const = 0;

    //! Prepares the store at storePath. The path is never inside the user's
    //! project folder.
    virtual bool open(const QString& storePath) = 0;
    virtual bool isOpen() const = 0;

    //! Records the current content of workTree as a new revision.
    //! Returns the new revision id, or an empty string on failure.
    virtual QString commit(const QString& workTree, const QString& label, const QString& action, const QDateTime& timestamp) = 0;

    //! Every revision, newest first.
    virtual QList<Revision> revisions() const = 0;

    //! The file list of one revision, with sizes and the status against the
    //! previous revision.
    virtual QList<RevisionFile> files(const QString& revisionId) const = 0;

    //! Writes the content of one revision into workTree. The caller records a
    //! new revision afterwards, so the history stays append only.
    virtual bool checkout(const QString& revisionId, const QString& workTree) = 0;

    //! Writes the content of one revision into an arbitrary folder.
    virtual bool exportTo(const QString& revisionId, const QString& destinationDir) = 0;

    //! Replaces the label of a revision. Labels live beside the history
    //! rather than inside it, so editing one never rewrites a revision.
    virtual bool setLabel(const QString& revisionId, const QString& label) = 0;

    //! Marks revisions outside the retention window as pruned and removes
    //! their payload. Returns the number of revisions pruned. Never prunes
    //! the newest revision.
    virtual int prune(int keepCount, int keepDays) = 0;
};

using ISnapshotStorePtr = std::shared_ptr<ISnapshotStore>;
}

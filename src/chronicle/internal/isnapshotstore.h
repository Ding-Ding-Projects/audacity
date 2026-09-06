/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <memory>

#include <QByteArray>

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

    //! Marks or unmarks a revision as starred. Purely decorative: it plays
    //! no part in retention.
    virtual bool setStarred(const QString& revisionId, bool starred) = 0;

    //! Marks or unmarks a revision as pinned. A pinned revision is excluded
    //! from retention exactly like the newest revision itself.
    virtual bool setPinned(const QString& revisionId, bool pinned) = 0;

    //! Marks revisions outside the retention window as pruned and removes
    //! their payload. Returns the number of revisions pruned. Never prunes
    //! the newest revision.
    virtual int prune(int keepCount, int keepDays) = 0;

    //! Packs the entire history into one self-contained byte array, suitable
    //! for embedding into the project's own save file so the history can
    //! travel with it to another machine. Returns an empty array when there
    //! is nothing to pack yet or packing itself fails; a caller must treat
    //! either as "there is nothing to embed right now", never as a reason to
    //! fail the save that asked for it.
    virtual QByteArray packHistory() const = 0;

    //! Absorbs history packed by packHistory() into this store. The merge is
    //! fast forward only: it adopts the packed history when it is at least
    //! as advanced as what this store already has, and otherwise leaves the
    //! store untouched, so an older or unrelated bundle can never discard a
    //! local revision.
    //!
    //! The return value means "no local revision was lost", not "something
    //! new was gained": the content addressed store can tell in advance that
    //! a bundle carries nothing new and returns false for it, while the git
    //! backend cannot tell the difference between that case and a real
    //! advance without inspecting a result its own tooling does not expose,
    //! so it returns true for both. Either way, calling this again with a
    //! bundle this store has already absorbed never loses anything.
    virtual bool unpackHistory(const QByteArray& data) = 0;
};

using ISnapshotStorePtr = std::shared_ptr<ISnapshotStore>;
}

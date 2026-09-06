/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QHash>
#include <QStringList>

#include "isnapshotstore.h"

namespace au::chronicle {
/*!
 * The version history store backed by an isolated bare git repository.
 *
 * The repository lives beside the application data directory and never inside
 * the user's project folder, so nothing is added to the folder the user sees.
 * Every operation goes through the git executable found on PATH, driven with
 * QProcess. No git library is linked.
 *
 * The history is append only. A restore writes an old tree back into the
 * working tree with read-tree and checkout-index, which does not move any
 * reference, and the caller then records the restore as a new revision.
 * Labels are stored in a small JSON file beside the repository rather than in
 * the commit messages that are already written, so editing a label never
 * rewrites a commit.
 */
class GitSnapshotStore : public ISnapshotStore
{
public:
    //! Reports whether a usable git executable is on PATH. The result is
    //! cached for the lifetime of the process.
    static bool isGitAvailable();
    //! Test hook: forgets the cached probe result.
    static void resetGitProbe();

    QString kind() const override { return QStringLiteral("git"); }

    bool open(const QString& storePath) override;
    bool isOpen() const override { return m_open; }

    QString commit(const QString& workTree, const QString& label, const QString& action, const QDateTime& timestamp) override;

    QList<Revision> revisions() const override;
    QList<RevisionFile> files(const QString& revisionId) const override;

    bool checkout(const QString& revisionId, const QString& workTree) override;
    bool exportTo(const QString& revisionId, const QString& destinationDir) override;
    bool setLabel(const QString& revisionId, const QString& label) override;
    int prune(int keepCount, int keepDays) override;

    QByteArray packHistory() const override;
    bool unpackHistory(const QByteArray& data) override;

private:
    bool run(const QStringList& arguments, QString* output = nullptr,
             const QString& workTree = QString(), const QStringList& extraEnvironment = QStringList()) const;

    QString labelsFilePath() const;
    QString prunedFilePath() const;
    QHash<QString, QString> readLabels() const;
    void writeLabels(const QHash<QString, QString>& labels) const;
    QStringList readPruned() const;
    void writePruned(const QStringList& ids) const;

    QString m_storePath;
    bool m_open = false;
};
}

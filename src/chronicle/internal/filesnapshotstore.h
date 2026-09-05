/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QJsonArray>

#include "isnapshotstore.h"

namespace au::chronicle {
/*!
 * The fallback version history store, used when no git executable is on PATH.
 *
 * It is content addressed: every file is stored once under the SHA-256 of its
 * content, and a revision is a manifest naming the files it contains. That
 * gives the same deduplication and the same append only guarantee as the git
 * store behind the same interface, so nothing above this layer needs to know
 * which backend is in use.
 */
class FileSnapshotStore : public ISnapshotStore
{
public:
    QString kind() const override { return QStringLiteral("content-addressed"); }

    bool open(const QString& storePath) override;
    bool isOpen() const override { return m_open; }

    QString commit(const QString& workTree, const QString& label, const QString& action, const QDateTime& timestamp) override;

    QList<Revision> revisions() const override;
    QList<RevisionFile> files(const QString& revisionId) const override;

    bool checkout(const QString& revisionId, const QString& workTree) override;
    bool exportTo(const QString& revisionId, const QString& destinationDir) override;
    bool setLabel(const QString& revisionId, const QString& label) override;
    int prune(int keepCount, int keepDays) override;

private:
    QString manifestPath() const;
    QJsonArray readManifest() const;
    void writeManifest(const QJsonArray& manifest) const;

    QString objectPath(const QString& hash) const;
    QString storeObject(const QByteArray& content) const;
    QJsonObject revisionObject(const QString& revisionId, int* indexOut = nullptr) const;
    void collectGarbage(const QJsonArray& manifest) const;

    QString m_storePath;
    bool m_open = false;
};
}

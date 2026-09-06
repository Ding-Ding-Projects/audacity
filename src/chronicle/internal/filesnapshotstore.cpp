/*
* Audacity: A Digital Audio Editor
*/
#include "filesnapshotstore.h"

#include <algorithm>

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include "log.h"

using namespace au::chronicle;

bool FileSnapshotStore::open(const QString& storePath)
{
    m_storePath = QDir::cleanPath(storePath);
    m_open = QDir().mkpath(m_storePath) && QDir().mkpath(m_storePath + QStringLiteral("/objects"));
    return m_open;
}

QString FileSnapshotStore::manifestPath() const
{
    return m_storePath + QStringLiteral("/revisions.json");
}

QJsonArray FileSnapshotStore::readManifest() const
{
    QFile file(manifestPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonArray();
    }
    return QJsonDocument::fromJson(file.readAll()).array();
}

void FileSnapshotStore::writeManifest(const QJsonArray& manifest) const
{
    QFile file(manifestPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOGE() << "could not write the history manifest at " << manifestPath().toStdString();
        return;
    }
    file.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
}

QString FileSnapshotStore::objectPath(const QString& hash) const
{
    return m_storePath + QStringLiteral("/objects/") + hash.left(2) + QChar(u'/') + hash.mid(2);
}

QString FileSnapshotStore::storeObject(const QByteArray& content) const
{
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
    const QString path = objectPath(hash);
    if (QFileInfo::exists(path)) {
        return hash;
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QString();
    }
    file.write(content);
    return hash;
}

QString FileSnapshotStore::commit(const QString& workTree, const QString& label, const QString& action,
                                  const QDateTime& timestamp)
{
    if (!m_open || workTree.isEmpty() || !QFileInfo::exists(workTree)) {
        return QString();
    }

    const QDir root(workTree);
    QJsonArray fileEntries;
    QStringList relativePaths;

    QDirIterator iterator(workTree, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString absolute = iterator.next();
        const QString relative = root.relativeFilePath(absolute);
        if (relative.startsWith(QStringLiteral(".git"))) {
            continue;
        }
        relativePaths.append(relative);
    }
    relativePaths.sort();

    for (const QString& relative : relativePaths) {
        QFile file(root.absoluteFilePath(relative));
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QByteArray content = file.readAll();
        const QString hash = storeObject(content);
        if (hash.isEmpty()) {
            continue;
        }
        QJsonObject entry;
        entry.insert(QStringLiteral("path"), relative);
        entry.insert(QStringLiteral("hash"), hash);
        entry.insert(QStringLiteral("size"), static_cast<double>(content.size()));
        fileEntries.append(entry);
    }

    const QDateTime stamp = timestamp.isValid() ? timestamp.toUTC() : QDateTime::currentDateTimeUtc();

    QJsonArray manifest = readManifest();

    // The identifier is the hash of the revision itself, including the
    // previous identifier, so the chain is verifiable and every revision is
    // distinct even when two snapshots hold identical content.
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    hasher.addData(manifest.isEmpty() ? QByteArray() : manifest.last().toObject()
                   .value(QStringLiteral("id")).toString().toUtf8());
    hasher.addData(stamp.toString(Qt::ISODateWithMs).toUtf8());
    hasher.addData(action.toUtf8());
    hasher.addData(QString::number(manifest.size()).toUtf8());
    hasher.addData(QJsonDocument(fileEntries).toJson(QJsonDocument::Compact));
    const QString id = QString::fromLatin1(hasher.result().toHex());

    QJsonObject revision;
    revision.insert(QStringLiteral("id"), id);
    revision.insert(QStringLiteral("timestamp"), stamp.toString(Qt::ISODate));
    revision.insert(QStringLiteral("label"), label.isEmpty() ? QStringLiteral("Snapshot") : label);
    revision.insert(QStringLiteral("action"), action);
    revision.insert(QStringLiteral("files"), fileEntries);

    manifest.append(revision);
    writeManifest(manifest);

    return id;
}

QList<Revision> FileSnapshotStore::revisions() const
{
    QList<Revision> result;
    if (!m_open) {
        return result;
    }

    const QJsonArray manifest = readManifest();
    // The manifest is oldest first; the interface reports newest first.
    for (int i = manifest.size() - 1; i >= 0; --i) {
        const QJsonObject object = manifest.at(i).toObject();
        Revision revision;
        revision.id = object.value(QStringLiteral("id")).toString();
        revision.timestamp = QDateTime::fromString(object.value(QStringLiteral("timestamp")).toString(), Qt::ISODate);
        revision.label = object.value(QStringLiteral("label")).toString();
        revision.action = object.value(QStringLiteral("action")).toString();
        if (revision.action.isEmpty()) {
            revision.action = actions::Manual;
        }
        revision.starred = object.value(QStringLiteral("starred")).toBool();
        revision.pinned = object.value(QStringLiteral("pinned")).toBool();
        result.append(revision);
    }
    return result;
}

QJsonObject FileSnapshotStore::revisionObject(const QString& revisionId, int* indexOut) const
{
    const QJsonArray manifest = readManifest();
    for (int i = 0; i < manifest.size(); ++i) {
        const QJsonObject object = manifest.at(i).toObject();
        if (object.value(QStringLiteral("id")).toString() == revisionId) {
            if (indexOut) {
                *indexOut = i;
            }
            return object;
        }
    }
    if (indexOut) {
        *indexOut = -1;
    }
    return QJsonObject();
}

QList<RevisionFile> FileSnapshotStore::files(const QString& revisionId) const
{
    QList<RevisionFile> result;
    if (!m_open) {
        return result;
    }

    int index = -1;
    const QJsonObject object = revisionObject(revisionId, &index);
    if (index < 0) {
        return result;
    }

    QHash<QString, QString> previous;
    if (index > 0) {
        const QJsonArray manifest = readManifest();
        const QJsonArray previousFiles = manifest.at(index - 1).toObject().value(QStringLiteral("files")).toArray();
        for (const QJsonValue& value : previousFiles) {
            const QJsonObject entry = value.toObject();
            previous.insert(entry.value(QStringLiteral("path")).toString(),
                            entry.value(QStringLiteral("hash")).toString());
        }
    }

    QSet<QString> seen;
    const QJsonArray fileEntries = object.value(QStringLiteral("files")).toArray();
    for (const QJsonValue& value : fileEntries) {
        const QJsonObject entry = value.toObject();
        RevisionFile file;
        file.path = entry.value(QStringLiteral("path")).toString();
        file.size = static_cast<qint64>(entry.value(QStringLiteral("size")).toDouble());
        const QString hash = entry.value(QStringLiteral("hash")).toString();
        seen.insert(file.path);

        if (!previous.contains(file.path)) {
            file.status = QStringLiteral("added");
        } else if (previous.value(file.path) != hash) {
            file.status = QStringLiteral("modified");
        } else {
            file.status = QStringLiteral("unchanged");
        }
        result.append(file);
    }

    for (auto it = previous.begin(); it != previous.end(); ++it) {
        if (!seen.contains(it.key())) {
            RevisionFile file;
            file.path = it.key();
            file.size = 0;
            file.status = QStringLiteral("deleted");
            result.append(file);
        }
    }

    std::sort(result.begin(), result.end(), [](const RevisionFile& a, const RevisionFile& b) {
        return a.path < b.path;
    });

    return result;
}

bool FileSnapshotStore::checkout(const QString& revisionId, const QString& workTree)
{
    if (!m_open || workTree.isEmpty()) {
        return false;
    }

    int index = -1;
    const QJsonObject object = revisionObject(revisionId, &index);
    if (index < 0) {
        return false;
    }

    QDir().mkpath(workTree);
    const QDir root(workTree);

    QSet<QString> wanted;
    const QJsonArray fileEntries = object.value(QStringLiteral("files")).toArray();
    for (const QJsonValue& value : fileEntries) {
        const QJsonObject entry = value.toObject();
        const QString relative = entry.value(QStringLiteral("path")).toString();
        wanted.insert(relative);

        QFile source(objectPath(entry.value(QStringLiteral("hash")).toString()));
        if (!source.open(QIODevice::ReadOnly)) {
            LOGE() << "missing history object for " << relative.toStdString();
            return false;
        }
        const QString destination = root.absoluteFilePath(relative);
        QDir().mkpath(QFileInfo(destination).absolutePath());
        QFile target(destination);
        if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }
        target.write(source.readAll());
    }

    QDirIterator iterator(workTree, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QStringList extras;
    while (iterator.hasNext()) {
        const QString absolute = iterator.next();
        const QString relative = root.relativeFilePath(absolute);
        if (relative.startsWith(QStringLiteral(".git"))) {
            continue;
        }
        if (!wanted.contains(relative)) {
            extras.append(absolute);
        }
    }
    for (const QString& extra : extras) {
        QFile::remove(extra);
    }

    return true;
}

bool FileSnapshotStore::exportTo(const QString& revisionId, const QString& destinationDir)
{
    if (!m_open || destinationDir.isEmpty()) {
        return false;
    }

    int index = -1;
    const QJsonObject object = revisionObject(revisionId, &index);
    if (index < 0) {
        return false;
    }

    QDir().mkpath(destinationDir);
    const QDir root(destinationDir);

    const QJsonArray fileEntries = object.value(QStringLiteral("files")).toArray();
    for (const QJsonValue& value : fileEntries) {
        const QJsonObject entry = value.toObject();
        QFile source(objectPath(entry.value(QStringLiteral("hash")).toString()));
        if (!source.open(QIODevice::ReadOnly)) {
            return false;
        }
        const QString destination = root.absoluteFilePath(entry.value(QStringLiteral("path")).toString());
        QDir().mkpath(QFileInfo(destination).absolutePath());
        QFile target(destination);
        if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }
        target.write(source.readAll());
    }

    return true;
}

bool FileSnapshotStore::setLabel(const QString& revisionId, const QString& label)
{
    if (!m_open) {
        return false;
    }

    QJsonArray manifest = readManifest();
    for (int i = 0; i < manifest.size(); ++i) {
        QJsonObject object = manifest.at(i).toObject();
        if (object.value(QStringLiteral("id")).toString() != revisionId) {
            continue;
        }
        // Only the label changes. The recorded content and its identifier are
        // never touched, so the history stays append only.
        object.insert(QStringLiteral("label"), label);
        manifest.replace(i, object);
        writeManifest(manifest);
        return true;
    }
    return false;
}

bool FileSnapshotStore::setStarred(const QString& revisionId, bool starred)
{
    return setBoolField(revisionId, QStringLiteral("starred"), starred);
}

bool FileSnapshotStore::setPinned(const QString& revisionId, bool pinned)
{
    return setBoolField(revisionId, QStringLiteral("pinned"), pinned);
}

bool FileSnapshotStore::setBoolField(const QString& revisionId, const QString& field, bool value)
{
    if (!m_open) {
        return false;
    }

    QJsonArray manifest = readManifest();
    for (int i = 0; i < manifest.size(); ++i) {
        QJsonObject object = manifest.at(i).toObject();
        if (object.value(QStringLiteral("id")).toString() != revisionId) {
            continue;
        }
        object.insert(field, value);
        manifest.replace(i, object);
        writeManifest(manifest);
        return true;
    }
    return false;
}

void FileSnapshotStore::collectGarbage(const QJsonArray& manifest) const
{
    QSet<QString> referenced;
    for (const QJsonValue& value : manifest) {
        const QJsonArray fileEntries = value.toObject().value(QStringLiteral("files")).toArray();
        for (const QJsonValue& entry : fileEntries) {
            referenced.insert(entry.toObject().value(QStringLiteral("hash")).toString());
        }
    }

    QDirIterator iterator(m_storePath + QStringLiteral("/objects"), QDir::Files, QDirIterator::Subdirectories);
    QStringList unreferenced;
    while (iterator.hasNext()) {
        const QString absolute = iterator.next();
        const QFileInfo info(absolute);
        const QString hash = info.dir().dirName() + info.fileName();
        if (!referenced.contains(hash)) {
            unreferenced.append(absolute);
        }
    }
    for (const QString& path : unreferenced) {
        QFile::remove(path);
    }
}

int FileSnapshotStore::prune(int keepCount, int keepDays)
{
    if (!m_open) {
        return 0;
    }

    QJsonArray manifest = readManifest();
    if (manifest.size() <= 1) {
        return 0;
    }

    const QDateTime cutoff = keepDays > 0 ? QDateTime::currentDateTimeUtc().addDays(-keepDays) : QDateTime();

    QJsonArray kept;
    int pruned = 0;
    const int total = manifest.size();
    for (int i = 0; i < total; ++i) {
        const QJsonObject object = manifest.at(i).toObject();
        // The manifest is oldest first, so the distance from the end is the
        // age rank. The newest revision is never pruned.
        const int rank = total - 1 - i;
        const QDateTime timestamp = QDateTime::fromString(object.value(QStringLiteral("timestamp")).toString(),
                                                          Qt::ISODate);
        const bool overCount = keepCount > 0 && rank >= keepCount;
        const bool tooOld = cutoff.isValid() && timestamp.isValid() && timestamp < cutoff;
        const bool pinned = object.value(QStringLiteral("pinned")).toBool();
        if (rank > 0 && !pinned && (overCount || tooOld)) {
            ++pruned;
            continue;
        }
        kept.append(object);
    }

    if (pruned > 0) {
        writeManifest(kept);
        collectGarbage(kept);
    }

    return pruned;
}

QByteArray FileSnapshotStore::packHistory() const
{
    if (!m_open) {
        return QByteArray();
    }

    QFile manifestFile(manifestPath());
    QString manifestJson;
    if (manifestFile.open(QIODevice::ReadOnly)) {
        manifestJson = QString::fromUtf8(manifestFile.readAll());
    }
    if (manifestJson.trimmed().isEmpty()) {
        // Nothing has been recorded yet, so there is nothing worth embedding.
        return QByteArray();
    }

    QStringList relativePaths;
    const QDir objectsDir(m_storePath + QStringLiteral("/objects"));
    QDirIterator iterator(objectsDir.path(), QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        relativePaths.append(objectsDir.relativeFilePath(iterator.filePath()));
    }
    // Sorted so that packing the same store twice produces the same bytes.
    relativePaths.sort();

    QJsonObject objects;
    for (const QString& relative : relativePaths) {
        QFile object(objectsDir.filePath(relative));
        if (!object.open(QIODevice::ReadOnly)) {
            continue;
        }
        objects.insert(relative, QString::fromLatin1(object.readAll().toBase64()));
    }

    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("chronicle-file-store-v1"));
    root.insert(QStringLiteral("manifest"), manifestJson);
    root.insert(QStringLiteral("objects"), objects);

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool FileSnapshotStore::unpackHistory(const QByteArray& data)
{
    if (!m_open || data.isEmpty()) {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(data);
    if (!document.isObject()) {
        return false;
    }
    const QJsonObject root = document.object();

    const QJsonDocument incomingManifestDoc = QJsonDocument::fromJson(
        root.value(QStringLiteral("manifest")).toString().toUtf8());
    if (!incomingManifestDoc.isArray()) {
        return false;
    }
    const QJsonArray incoming = incomingManifestDoc.array();
    const QJsonArray local = readManifest();

    // Fast forward only: the incoming manifest is adopted only when it opens
    // with every revision the local manifest already has, in the same order,
    // and adds at least one more. Anything else (an older bundle, an
    // unrelated one, or a local history that has already moved on) leaves
    // this store exactly as it was.
    if (incoming.size() <= local.size()) {
        return false;
    }
    for (int i = 0; i < local.size(); ++i) {
        const QString localId = local.at(i).toObject().value(QStringLiteral("id")).toString();
        const QString incomingId = incoming.at(i).toObject().value(QStringLiteral("id")).toString();
        if (localId != incomingId) {
            return false;
        }
    }

    // Objects are content addressed, so writing one that is already there
    // under the same name is always safe; this never loses local content.
    const QJsonObject objects = root.value(QStringLiteral("objects")).toObject();
    for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
        const QString destination = m_storePath + QStringLiteral("/objects/") + it.key();
        if (QFileInfo::exists(destination)) {
            continue;
        }
        QDir().mkpath(QFileInfo(destination).absolutePath());
        QFile file(destination);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QByteArray::fromBase64(it.value().toString().toLatin1()));
        }
    }

    writeManifest(incoming);
    return true;
}

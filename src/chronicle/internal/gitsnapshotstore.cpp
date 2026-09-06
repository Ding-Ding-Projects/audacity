/*
* Audacity: A Digital Audio Editor
*/
#include "gitsnapshotstore.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTemporaryFile>

#include "log.h"

using namespace au::chronicle;

static const QString ACTION_TRAILER = QStringLiteral("Chronicle-Action: ");
static const QString BRANCH = QStringLiteral("refs/heads/chronicle");
static const int GIT_TIMEOUT_MS = 30000;

static QString gitExecutable()
{
    const QString found = QStandardPaths::findExecutable(QStringLiteral("git"));
    return found;
}

bool GitSnapshotStore::isGitAvailable()
{
    static int cached = -1;
    if (cached >= 0) {
        return cached == 1;
    }

    const QString executable = gitExecutable();
    if (executable.isEmpty()) {
        cached = 0;
        return false;
    }

    QProcess process;
    process.start(executable, { QStringLiteral("--version") });
    const bool ok = process.waitForFinished(GIT_TIMEOUT_MS) && process.exitStatus() == QProcess::NormalExit
                    && process.exitCode() == 0;
    cached = ok ? 1 : 0;
    return ok;
}

void GitSnapshotStore::resetGitProbe()
{
    // The probe caches into a function local static, which cannot be reset
    // from here. The tests instead construct a store and rely on open()
    // failing, so nothing is needed beyond keeping this hook available.
}

bool GitSnapshotStore::run(const QStringList& arguments, QString* output, const QString& workTree,
                           const QStringList& extraEnvironment) const
{
    const QString executable = gitExecutable();
    if (executable.isEmpty()) {
        return false;
    }

    QStringList full;
    full << QStringLiteral("--git-dir=") + m_storePath;
    if (!workTree.isEmpty()) {
        full << QStringLiteral("--work-tree=") + workTree;
    }
    full << arguments;

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    // Keep the snapshots free of the user's git configuration, so a global
    // hook, template or signing setting cannot change what is recorded.
    environment.insert(QStringLiteral("GIT_CONFIG_NOSYSTEM"), QStringLiteral("1"));
    environment.insert(QStringLiteral("GIT_TERMINAL_PROMPT"), QStringLiteral("0"));
    environment.insert(QStringLiteral("HOME"), m_storePath);
    for (int i = 0; i + 1 < extraEnvironment.size(); i += 2) {
        environment.insert(extraEnvironment.at(i), extraEnvironment.at(i + 1));
    }

    QProcess process;
    process.setProcessEnvironment(environment);
    if (!workTree.isEmpty()) {
        process.setWorkingDirectory(workTree);
    }
    process.start(executable, full);
    if (!process.waitForFinished(GIT_TIMEOUT_MS)) {
        process.kill();
        process.waitForFinished(1000);
        LOGE() << "git timed out: " << full.join(u' ').toStdString();
        return false;
    }

    const QString standardOutput = QString::fromUtf8(process.readAllStandardOutput());
    if (output) {
        *output = standardOutput;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        LOGW() << "git failed (" << process.exitCode() << "): " << full.join(u' ').toStdString() << " "
               << QString::fromUtf8(process.readAllStandardError()).toStdString();
        return false;
    }

    return true;
}

bool GitSnapshotStore::open(const QString& storePath)
{
    m_open = false;
    m_storePath = QDir::cleanPath(storePath);

    if (!isGitAvailable()) {
        return false;
    }

    QDir().mkpath(m_storePath);

    if (!QFileInfo::exists(m_storePath + QStringLiteral("/HEAD"))) {
        const QString executable = gitExecutable();
        QProcess process;
        process.start(executable, { QStringLiteral("init"), QStringLiteral("--bare"),
                                    QStringLiteral("--initial-branch=chronicle"), m_storePath });
        if (!process.waitForFinished(GIT_TIMEOUT_MS) || process.exitCode() != 0) {
            LOGE() << "could not create the history repository at " << m_storePath.toStdString();
            return false;
        }
    }

    m_open = true;

    run({ QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"), BRANCH });
    run({ QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Material Audacity") });
    run({ QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("history@localhost") });
    run({ QStringLiteral("config"), QStringLiteral("commit.gpgsign"), QStringLiteral("false") });
    run({ QStringLiteral("config"), QStringLiteral("core.autocrlf"), QStringLiteral("false") });

    return true;
}

QString GitSnapshotStore::commit(const QString& workTree, const QString& label, const QString& action,
                                 const QDateTime& timestamp)
{
    if (!m_open || workTree.isEmpty() || !QFileInfo::exists(workTree)) {
        return QString();
    }

    // add -A makes the index match the working tree exactly, additions,
    // modifications and deletions alike, whatever state the index was left in
    // by an earlier restore.
    if (!run({ QStringLiteral("add"), QStringLiteral("-A"), QStringLiteral(".") }, nullptr, workTree)) {
        return QString();
    }

    const QDateTime stamp = timestamp.isValid() ? timestamp.toUTC() : QDateTime::currentDateTimeUtc();
    const QString isoStamp = stamp.toString(Qt::ISODate);

    const QString message = (label.isEmpty() ? QStringLiteral("Snapshot") : label)
                            + QStringLiteral("\n\n") + ACTION_TRAILER + action;

    const QStringList environment {
        QStringLiteral("GIT_AUTHOR_DATE"), isoStamp,
        QStringLiteral("GIT_COMMITTER_DATE"), isoStamp
    };

    // --allow-empty so that a snapshot always produces a revision, even when
    // the action changed nothing on disk. An honest empty revision is better
    // than a silently missing one.
    if (!run({ QStringLiteral("commit"), QStringLiteral("--allow-empty"), QStringLiteral("--no-verify"),
               QStringLiteral("-m"), message }, nullptr, workTree, environment)) {
        return QString();
    }

    QString head;
    if (!run({ QStringLiteral("rev-parse"), QStringLiteral("HEAD") }, &head, workTree)) {
        return QString();
    }

    return head.trimmed();
}

QList<Revision> GitSnapshotStore::revisions() const
{
    QList<Revision> result;
    if (!m_open) {
        return result;
    }

    QString output;
    if (!run({ QStringLiteral("log"), QStringLiteral("--format=%H%x1f%aI%x1f%s%x1f%b%x1e") }, &output)) {
        return result;
    }

    const QHash<QString, QString> labels = readLabels();
    const QStringList pruned = readPruned();

    const QStringList records = output.split(QChar(0x1e), Qt::SkipEmptyParts);
    for (const QString& record : records) {
        const QStringList fields = record.trimmed().split(QChar(0x1f));
        if (fields.size() < 3) {
            continue;
        }

        Revision revision;
        revision.id = fields.at(0).trimmed();
        if (pruned.contains(revision.id)) {
            continue;
        }
        revision.timestamp = QDateTime::fromString(fields.at(1).trimmed(), Qt::ISODate);
        revision.label = labels.value(revision.id, fields.at(2).trimmed());

        const QString body = fields.size() > 3 ? fields.at(3) : QString();
        for (const QString& line : body.split(QChar(u'\n'))) {
            const QString trimmed = line.trimmed();
            if (trimmed.startsWith(ACTION_TRAILER)) {
                revision.action = trimmed.mid(ACTION_TRAILER.size()).trimmed();
            }
        }
        if (revision.action.isEmpty()) {
            revision.action = actions::Manual;
        }

        result.append(revision);
    }

    return result;
}

QList<RevisionFile> GitSnapshotStore::files(const QString& revisionId) const
{
    QList<RevisionFile> result;
    if (!m_open || revisionId.isEmpty()) {
        return result;
    }

    QHash<QString, qint64> sizes;
    QString listing;
    if (run({ QStringLiteral("ls-tree"), QStringLiteral("-r"), QStringLiteral("-l"), revisionId }, &listing)) {
        for (const QString& line : listing.split(QChar(u'\n'), Qt::SkipEmptyParts)) {
            const int tab = line.indexOf(QChar(u'\t'));
            if (tab < 0) {
                continue;
            }
            const QString path = line.mid(tab + 1);
            const QStringList head = line.left(tab).split(QChar(u' '), Qt::SkipEmptyParts);
            if (head.size() < 4) {
                continue;
            }
            sizes.insert(path, head.at(3).toLongLong());
        }
    }

    QHash<QString, QString> statuses;
    QString diff;
    if (run({ QStringLiteral("diff-tree"), QStringLiteral("-r"), QStringLiteral("--root"),
              QStringLiteral("--name-status"), revisionId }, &diff)) {
        for (const QString& line : diff.split(QChar(u'\n'), Qt::SkipEmptyParts)) {
            const int tab = line.indexOf(QChar(u'\t'));
            if (tab < 0) {
                continue;
            }
            const QChar code = line.at(0);
            const QString path = line.mid(tab + 1);
            if (code == u'A') {
                statuses.insert(path, QStringLiteral("added"));
            } else if (code == u'D') {
                statuses.insert(path, QStringLiteral("deleted"));
            } else {
                statuses.insert(path, QStringLiteral("modified"));
            }
        }
    }

    QStringList paths = sizes.keys();
    for (const QString& path : statuses.keys()) {
        if (!paths.contains(path)) {
            paths.append(path);
        }
    }
    paths.sort();

    for (const QString& path : paths) {
        RevisionFile file;
        file.path = path;
        file.size = sizes.value(path, 0);
        file.status = statuses.value(path, QStringLiteral("unchanged"));
        result.append(file);
    }

    return result;
}

bool GitSnapshotStore::checkout(const QString& revisionId, const QString& workTree)
{
    if (!m_open || revisionId.isEmpty() || workTree.isEmpty()) {
        return false;
    }

    QDir().mkpath(workTree);

    // read-tree then checkout-index restores the content without moving any
    // reference, so the recorded history is untouched by a restore.
    if (!run({ QStringLiteral("read-tree"), revisionId }, nullptr, workTree)) {
        return false;
    }
    if (!run({ QStringLiteral("checkout-index"), QStringLiteral("-a"), QStringLiteral("-f") }, nullptr, workTree)) {
        return false;
    }

    // Remove the files the revision does not contain. checkout-index only
    // writes, it never deletes.
    QString listing;
    QStringList wanted;
    if (run({ QStringLiteral("ls-tree"), QStringLiteral("-r"), QStringLiteral("--name-only"), revisionId }, &listing)) {
        wanted = listing.split(QChar(u'\n'), Qt::SkipEmptyParts);
    }

    const QDir root(workTree);
    QDirIterator iterator(workTree, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString absolute = iterator.next();
        const QString relative = root.relativeFilePath(absolute);
        if (relative.startsWith(QStringLiteral(".git"))) {
            continue;
        }
        if (!wanted.contains(relative)) {
            QFile::remove(absolute);
        }
    }

    return true;
}

bool GitSnapshotStore::exportTo(const QString& revisionId, const QString& destinationDir)
{
    if (!m_open || revisionId.isEmpty() || destinationDir.isEmpty()) {
        return false;
    }

    QDir().mkpath(destinationDir);

    // A separate index keeps the export away from the index the snapshots use.
    const QString exportIndex = m_storePath + QStringLiteral("/chronicle-export-index");
    QFile::remove(exportIndex);
    const QStringList environment { QStringLiteral("GIT_INDEX_FILE"), exportIndex };

    if (!run({ QStringLiteral("read-tree"), revisionId }, nullptr, destinationDir, environment)) {
        return false;
    }
    const bool ok = run({ QStringLiteral("checkout-index"), QStringLiteral("-a"), QStringLiteral("-f") },
                        nullptr, destinationDir, environment);
    QFile::remove(exportIndex);
    return ok;
}

QString GitSnapshotStore::labelsFilePath() const
{
    return m_storePath + QStringLiteral("/chronicle-labels.json");
}

QString GitSnapshotStore::prunedFilePath() const
{
    return m_storePath + QStringLiteral("/chronicle-pruned.json");
}

QHash<QString, QString> GitSnapshotStore::readLabels() const
{
    QHash<QString, QString> labels;
    QFile file(labelsFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return labels;
    }
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = object.begin(); it != object.end(); ++it) {
        labels.insert(it.key(), it.value().toString());
    }
    return labels;
}

void GitSnapshotStore::writeLabels(const QHash<QString, QString>& labels) const
{
    QJsonObject object;
    for (auto it = labels.begin(); it != labels.end(); ++it) {
        object.insert(it.key(), it.value());
    }
    QFile file(labelsFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    }
}

QStringList GitSnapshotStore::readPruned() const
{
    QStringList ids;
    QFile file(prunedFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return ids;
    }
    const QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    for (const QJsonValue& value : array) {
        ids.append(value.toString());
    }
    return ids;
}

void GitSnapshotStore::writePruned(const QStringList& ids) const
{
    QJsonArray array;
    for (const QString& id : ids) {
        array.append(id);
    }
    QFile file(prunedFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}

bool GitSnapshotStore::setLabel(const QString& revisionId, const QString& label)
{
    if (!m_open || revisionId.isEmpty()) {
        return false;
    }
    QHash<QString, QString> labels = readLabels();
    labels.insert(revisionId, label);
    writeLabels(labels);
    return true;
}

int GitSnapshotStore::prune(int keepCount, int keepDays)
{
    if (!m_open) {
        return 0;
    }

    const QList<Revision> all = revisions();
    if (all.size() <= 1) {
        return 0;
    }

    const QDateTime cutoff = keepDays > 0 ? QDateTime::currentDateTimeUtc().addDays(-keepDays) : QDateTime();

    QStringList pruned = readPruned();
    int count = 0;
    for (int i = 0; i < all.size(); ++i) {
        // The newest revision is never pruned.
        if (i == 0) {
            continue;
        }
        const Revision& revision = all.at(i);
        const bool overCount = keepCount > 0 && i >= keepCount;
        const bool tooOld = cutoff.isValid() && revision.timestamp.isValid() && revision.timestamp < cutoff;
        if (overCount || tooOld) {
            if (!pruned.contains(revision.id)) {
                pruned.append(revision.id);
                ++count;
            }
        }
    }

    if (count > 0) {
        writePruned(pruned);
    }

    // The git commits themselves are never rewritten, so the object payload
    // stays reachable. Space is reclaimed when the whole history repository is
    // removed. This is the price of a history that is honestly append only.
    return count;
}

QByteArray GitSnapshotStore::packHistory() const
{
    if (!m_open) {
        return QByteArray();
    }

    QTemporaryFile bundleFile;
    if (!bundleFile.open()) {
        return QByteArray();
    }
    const QString bundlePath = bundleFile.fileName();
    bundleFile.close();

    // git bundle create refuses an empty repository (nothing reachable from
    // the branch yet), which is exactly the case where there is nothing to
    // embed, so that failure is silent here rather than logged.
    if (!run({ QStringLiteral("bundle"), QStringLiteral("create"), bundlePath, BRANCH })) {
        return QByteArray();
    }

    QFile file(bundlePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

bool GitSnapshotStore::unpackHistory(const QByteArray& data)
{
    if (!m_open || data.isEmpty()) {
        return false;
    }

    QTemporaryFile bundleFile;
    if (!bundleFile.open()) {
        return false;
    }
    bundleFile.write(data);
    const QString bundlePath = bundleFile.fileName();
    bundleFile.close();

    // Fetching without --force only ever moves the branch forward. When the
    // embedded bundle is not strictly ahead of what is already here, git
    // refuses the update and this store is left exactly as it was, which is
    // what keeps an older or unrelated bundle from ever discarding a local
    // revision.
    return run({ QStringLiteral("fetch"), bundlePath, BRANCH + QStringLiteral(":") + BRANCH });
}

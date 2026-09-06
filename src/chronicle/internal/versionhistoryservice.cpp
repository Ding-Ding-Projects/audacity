/*
* Audacity: A Digital Audio Editor
*/
#include "versionhistoryservice.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include "settings.h"

#include "gitsnapshotstore.h"
#include "filesnapshotstore.h"

#include "log.h"

using namespace au::chronicle;

static const std::string MODULE_NAME("chronicle");
static const muse::Settings::Key RETENTION_COUNT(MODULE_NAME, "chronicle/retentionCount");
static const muse::Settings::Key RETENTION_DAYS(MODULE_NAME, "chronicle/retentionDays");
static const muse::Settings::Key COMMIT_ON_EVERY_ACTION(MODULE_NAME, "chronicle/commitOnEveryAction");
static const muse::Settings::Key EMBED_HISTORY_IN_SAVE_FILE(MODULE_NAME, "chronicle/embedHistoryInSaveFile");

static const int DEFAULT_RETENTION_COUNT = 200;
static const int DEFAULT_RETENTION_DAYS = 90;
static const bool DEFAULT_COMMIT_ON_EVERY_ACTION = true;
static const bool DEFAULT_EMBED_HISTORY_IN_SAVE_FILE = true;

static const int SETTINGS_DEBOUNCE_MS = 1500;

QString au::chronicle::actionTitle(const QString& action)
{
    if (action == actions::ProjectSave) {
        return QStringLiteral("Project saved");
    }
    if (action == actions::SettingsChange) {
        return QStringLiteral("Settings changed");
    }
    if (action == actions::PresetSave) {
        return QStringLiteral("Preset saved");
    }
    if (action == actions::PresetDelete) {
        return QStringLiteral("Preset deleted");
    }
    if (action == actions::Restore) {
        return QStringLiteral("Restored");
    }
    if (action == actions::DiscardUnsaved) {
        return QStringLiteral("Discarded unsaved work");
    }
    return QStringLiteral("Snapshot");
}

QString au::chronicle::actionFamily(const QString& action)
{
    if (action == actions::ProjectSave) {
        return QStringLiteral("save");
    }
    if (action == actions::Restore || action == actions::DiscardUnsaved) {
        return QStringLiteral("restore");
    }
    if (action == actions::SettingsChange || action == actions::PresetSave || action == actions::PresetDelete) {
        return QStringLiteral("project-settings");
    }

    // Everything below this line is a free form action name coming from the
    // undo stack itself, for example "Cut", "Move clip", "Amplify" or
    // "Add track". Audacity does not carry a stable identifier for these, so
    // the family is guessed from keywords in the name. A name that matches
    // nothing recognised is kept in "edit" rather than dropped, so it still
    // shows up somewhere in the filter chips.
    const QString lower = action.toLower();

    static const QStringList trackWords{ QStringLiteral("track") };
    static const QStringList clipWords{
        QStringLiteral("clip"), QStringLiteral("move"), QStringLiteral("split"), QStringLiteral("trim"),
        QStringLiteral("join"), QStringLiteral("stretch"), QStringLiteral("duplicate")
    };
    static const QStringList generateWords{
        QStringLiteral("generate"), QStringLiteral("tone"), QStringLiteral("noise"), QStringLiteral("silence"),
        QStringLiteral("chirp"), QStringLiteral("dtmf")
    };
    static const QStringList labelWords{ QStringLiteral("label") };
    static const QStringList envelopeWords{ QStringLiteral("envelope") };
    static const QStringList effectWords{
        QStringLiteral("effect"), QStringLiteral("amplify"), QStringLiteral("compress"), QStringLiteral("reverb"),
        QStringLiteral("equal"), QStringLiteral("normali"), QStringLiteral("limiter"), QStringLiteral("filter"),
        QStringLiteral("fade"), QStringLiteral("nyquist"), QStringLiteral("apply"), QStringLiteral("reduction")
    };

    auto matchesAny = [&lower](const QStringList& words) {
        for (const QString& word : words) {
            if (lower.contains(word)) {
                return true;
            }
        }
        return false;
    };

    if (matchesAny(trackWords)) {
        return QStringLiteral("track");
    }
    if (matchesAny(effectWords)) {
        return QStringLiteral("effect");
    }
    if (matchesAny(generateWords)) {
        return QStringLiteral("generate");
    }
    if (matchesAny(labelWords)) {
        return QStringLiteral("label");
    }
    if (matchesAny(envelopeWords)) {
        return QStringLiteral("envelope");
    }
    if (matchesAny(clipWords)) {
        return QStringLiteral("clip");
    }

    return QStringLiteral("edit");
}

QString au::chronicle::actionFamilyTitle(const QString& family)
{
    if (family == QStringLiteral("edit")) {
        return QStringLiteral("Edit");
    }
    if (family == QStringLiteral("clip")) {
        return QStringLiteral("Clip");
    }
    if (family == QStringLiteral("track")) {
        return QStringLiteral("Track");
    }
    if (family == QStringLiteral("effect")) {
        return QStringLiteral("Effect");
    }
    if (family == QStringLiteral("generate")) {
        return QStringLiteral("Generate");
    }
    if (family == QStringLiteral("label")) {
        return QStringLiteral("Label");
    }
    if (family == QStringLiteral("envelope")) {
        return QStringLiteral("Envelope");
    }
    if (family == QStringLiteral("project-settings")) {
        return QStringLiteral("Project settings");
    }
    if (family == QStringLiteral("save")) {
        return QStringLiteral("Save");
    }
    if (family == QStringLiteral("restore")) {
        return QStringLiteral("Restore");
    }
    return QStringLiteral("Edit");
}

bool au::chronicle::isMilestoneAction(const QString& action)
{
    if (action == actions::ProjectSave || action == actions::Restore) {
        return true;
    }

    // Export and render have no fixed identifier of their own here: they
    // reach this history, if at all, as the free form name the undo stack or
    // a future caller gives them, exactly like the action-family keywords
    // above. Matched by keyword rather than assuming a name that has not
    // been wired up anywhere yet.
    const QString lower = action.toLower();
    return lower.contains(QStringLiteral("export")) || lower.contains(QStringLiteral("render"));
}

VersionHistoryService::VersionHistoryService()
    : QObject()
{
}

QString VersionHistoryService::deriveLabel(const QString& action, const QString& detail)
{
    const QString trimmed = detail.trimmed();

    if (action == actions::ProjectSave) {
        return trimmed.isEmpty() ? QStringLiteral("Saved the project")
               : QStringLiteral("Saved %1").arg(trimmed);
    }
    if (action == actions::SettingsChange) {
        return trimmed.isEmpty() ? QStringLiteral("Changed settings")
               : QStringLiteral("Changed %1").arg(trimmed);
    }
    if (action == actions::PresetSave) {
        return trimmed.isEmpty() ? QStringLiteral("Saved a preset")
               : QStringLiteral("Saved preset %1").arg(trimmed);
    }
    if (action == actions::PresetDelete) {
        return trimmed.isEmpty() ? QStringLiteral("Deleted a preset")
               : QStringLiteral("Deleted preset %1").arg(trimmed);
    }
    if (action == actions::Restore) {
        return trimmed.isEmpty() ? QStringLiteral("Restored an earlier revision")
               : QStringLiteral("Restored %1").arg(trimmed);
    }
    if (action == actions::DiscardUnsaved) {
        return trimmed.isEmpty() ? QStringLiteral("Discarded unsaved work")
               : QStringLiteral("Discarded unsaved work in %1").arg(trimmed);
    }
    return trimmed.isEmpty() ? QStringLiteral("Snapshot") : trimmed;
}

void VersionHistoryService::init()
{
    muse::settings()->setDefaultValue(RETENTION_COUNT, muse::Val(DEFAULT_RETENTION_COUNT));
    muse::settings()->setDefaultValue(RETENTION_DAYS, muse::Val(DEFAULT_RETENTION_DAYS));
    muse::settings()->setDefaultValue(COMMIT_ON_EVERY_ACTION, muse::Val(DEFAULT_COMMIT_ON_EVERY_ACTION));
    muse::settings()->setDefaultValue(EMBED_HISTORY_IN_SAVE_FILE, muse::Val(DEFAULT_EMBED_HISTORY_IN_SAVE_FILE));

    // The settings file is watched rather than every individual key, so any
    // preference change produces exactly one snapshot rather than one per key.
    m_settingsDebounce = new QTimer(this);
    m_settingsDebounce->setSingleShot(true);
    m_settingsDebounce->setInterval(SETTINGS_DEBOUNCE_MS);
    QObject::connect(m_settingsDebounce, &QTimer::timeout, this, [this]() {
        recordSnapshot(actions::SettingsChange);
    });

    const QString settingsFile = QString::fromStdString(muse::settings()->filePath().toStdString());
    if (!settingsFile.isEmpty() && QFileInfo::exists(settingsFile)) {
        m_settingsWatcher = new QFileSystemWatcher(this);
        m_settingsWatcher->addPath(settingsFile);
        QObject::connect(m_settingsWatcher, &QFileSystemWatcher::fileChanged, this,
                         [this, settingsFile](const QString&) {
            // QSettings rewrites the file, which drops the watch on some
            // platforms, so the path is re-added every time.
            if (m_settingsWatcher && !m_settingsWatcher->files().contains(settingsFile)) {
                m_settingsWatcher->addPath(settingsFile);
            }
            m_settingsDebounce->start();
        });
    }

    // Open the history that is not tied to a project, so settings changes made
    // before a project is opened are still recorded.
    openStore(QString());
}

QString VersionHistoryService::historyRoot() const
{
    if (!m_rootOverride.isEmpty()) {
        return m_rootOverride;
    }
    const QString appData = QString::fromStdString(globalConfiguration()->userAppDataPath().toStdString());
    return QDir::cleanPath(appData + QStringLiteral("/history"));
}

QString VersionHistoryService::storePathFor(const QString& projectId) const
{
    const QString id = projectId.isEmpty() ? QStringLiteral("workspace") : projectId;
    return historyRoot() + QChar(u'/') + id + QStringLiteral(".git");
}

QString VersionHistoryService::stagePathFor(const QString& projectId) const
{
    const QString id = projectId.isEmpty() ? QStringLiteral("workspace") : projectId;
    return historyRoot() + QStringLiteral("/stage/") + id;
}

void VersionHistoryService::setStoreForTesting(const ISnapshotStorePtr& store, const QString& root)
{
    m_rootOverride = root;
    m_store = store;
    m_projectId = QString();
    if (m_store) {
        m_store->open(storePathFor(m_projectId));
    }
    QDir().mkpath(stagePathFor(m_projectId));
}

bool VersionHistoryService::openStore(const QString& projectId)
{
    ISnapshotStorePtr store;
    if (GitSnapshotStore::isGitAvailable()) {
        store = std::make_shared<GitSnapshotStore>();
        if (!store->open(storePathFor(projectId))) {
            LOGW() << "the git history store could not be opened, falling back to the built in store";
            store.reset();
        }
    }

    if (!store) {
        store = std::make_shared<FileSnapshotStore>();
        if (!store->open(storePathFor(projectId))) {
            LOGE() << "no version history store could be opened";
            m_store.reset();
            return false;
        }
    }

    m_store = store;
    QDir().mkpath(stagePathFor(projectId));
    return true;
}

QString VersionHistoryService::storeKind() const
{
    return m_store ? m_store->kind() : QStringLiteral("unavailable");
}

void VersionHistoryService::setCurrentProject(const QString& projectId, const QString& projectFilePath)
{
    if (projectId == m_projectId && projectFilePath == m_projectFilePath) {
        return;
    }

    m_projectId = projectId;
    m_projectFilePath = projectFilePath;
    openStore(projectId);
    m_revisionsChanged.notify();
}

bool VersionHistoryService::copyInto(const QString& sourceFile, const QString& destinationFile)
{
    if (sourceFile.isEmpty() || !QFileInfo::exists(sourceFile)) {
        return false;
    }
    QDir().mkpath(QFileInfo(destinationFile).absolutePath());
    QFile::remove(destinationFile);
    return QFile::copy(sourceFile, destinationFile);
}

void VersionHistoryService::stageProjectFile()
{
    if (m_projectFilePath.isEmpty()) {
        return;
    }
    const QString name = QFileInfo(m_projectFilePath).fileName();
    copyInto(m_projectFilePath, stagePathFor(m_projectId) + QStringLiteral("/project/") + name);
}

void VersionHistoryService::stageSettingsFile()
{
    const QString settingsFile = QString::fromStdString(muse::settings()->filePath().toStdString());
    if (settingsFile.isEmpty()) {
        return;
    }
    copyInto(settingsFile, stagePathFor(m_projectId) + QStringLiteral("/settings/")
             + QFileInfo(settingsFile).fileName());
}

void VersionHistoryService::stagePresets()
{
    const QString presetsRoot = QString::fromStdString(globalConfiguration()->userAppDataPath().toStdString())
                                + QStringLiteral("/presets");
    if (!QFileInfo::exists(presetsRoot)) {
        return;
    }

    const QDir root(presetsRoot);
    const QString destinationRoot = stagePathFor(m_projectId) + QStringLiteral("/presets");

    // The staged copy is rebuilt each time so that a deleted preset really
    // disappears from the next revision.
    QDir(destinationRoot).removeRecursively();

    QDirIterator iterator(presetsRoot, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString absolute = iterator.next();
        copyInto(absolute, destinationRoot + QChar(u'/') + root.relativeFilePath(absolute));
    }
}

void VersionHistoryService::stage()
{
    QDir().mkpath(stagePathFor(m_projectId));
    stageProjectFile();
    stageSettingsFile();
    stagePresets();
}

QString VersionHistoryService::recordSnapshot(const QString& action, const QString& detail,
                                              const QString& explicitLabel)
{
    if (!m_store || !m_store->isOpen()) {
        return QString();
    }

    stage();

    const QString label = explicitLabel.isEmpty() ? deriveLabel(action, detail) : explicitLabel;
    const QString id = m_store->commit(stagePathFor(m_projectId), label, action, QDateTime::currentDateTimeUtc());

    if (!id.isEmpty()) {
        m_revisionsChanged.notify();
    }

    return id;
}

QList<Revision> VersionHistoryService::revisions() const
{
    return m_store ? m_store->revisions() : QList<Revision>();
}

QList<RevisionFile> VersionHistoryService::files(const QString& revisionId) const
{
    return m_store ? m_store->files(revisionId) : QList<RevisionFile>();
}

bool VersionHistoryService::restore(const QString& revisionId)
{
    if (!m_store || !m_store->isOpen() || revisionId.isEmpty()) {
        return false;
    }

    if (!m_store->checkout(revisionId, stagePathFor(m_projectId))) {
        return false;
    }

    // Every restore is itself a revision, so the history stays append only and
    // the state before the restore can always be reached again.
    const QString shortId = revisionId.left(10);
    const QString id = m_store->commit(stagePathFor(m_projectId),
                                       deriveLabel(actions::Restore, shortId),
                                       actions::Restore, QDateTime::currentDateTimeUtc());

    m_revisionsChanged.notify();
    return !id.isEmpty();
}

bool VersionHistoryService::exportRevision(const QString& revisionId, const QString& destinationDir)
{
    return m_store && m_store->exportTo(revisionId, destinationDir);
}

bool VersionHistoryService::setLabel(const QString& revisionId, const QString& label)
{
    if (!m_store || !m_store->setLabel(revisionId, label)) {
        return false;
    }
    m_revisionsChanged.notify();
    return true;
}

bool VersionHistoryService::setStarred(const QString& revisionId, bool starred)
{
    if (!m_store || !m_store->setStarred(revisionId, starred)) {
        return false;
    }
    m_revisionsChanged.notify();
    return true;
}

bool VersionHistoryService::setPinned(const QString& revisionId, bool pinned)
{
    if (!m_store || !m_store->setPinned(revisionId, pinned)) {
        return false;
    }
    m_revisionsChanged.notify();
    return true;
}

int VersionHistoryService::prune()
{
    if (!m_store) {
        return 0;
    }
    const int count = m_store->prune(retentionCount(), retentionDays());
    if (count > 0) {
        m_revisionsChanged.notify();
    }
    return count;
}

int VersionHistoryService::retentionCount() const
{
    return muse::settings()->value(RETENTION_COUNT).toInt();
}

void VersionHistoryService::setRetentionCount(int value)
{
    muse::settings()->setSharedValue(RETENTION_COUNT, muse::Val(value));
}

int VersionHistoryService::retentionDays() const
{
    return muse::settings()->value(RETENTION_DAYS).toInt();
}

void VersionHistoryService::setRetentionDays(int value)
{
    muse::settings()->setSharedValue(RETENTION_DAYS, muse::Val(value));
}

bool VersionHistoryService::commitOnEveryAction() const
{
    return muse::settings()->value(COMMIT_ON_EVERY_ACTION).toBool();
}

void VersionHistoryService::setCommitOnEveryAction(bool value)
{
    muse::settings()->setSharedValue(COMMIT_ON_EVERY_ACTION, muse::Val(value));
}

bool VersionHistoryService::embedHistoryInSaveFile() const
{
    return muse::settings()->value(EMBED_HISTORY_IN_SAVE_FILE).toBool();
}

void VersionHistoryService::setEmbedHistoryInSaveFile(bool value)
{
    muse::settings()->setSharedValue(EMBED_HISTORY_IN_SAVE_FILE, muse::Val(value));
}

QByteArray VersionHistoryService::packHistoryForEmbedding() const
{
    if (!m_store || !m_store->isOpen()) {
        return QByteArray();
    }
    return m_store->packHistory();
}

QString VersionHistoryService::embeddedHistoryFormat() const
{
    if (!m_store) {
        return QString();
    }
    return m_store->kind() == QStringLiteral("git")
           ? QStringLiteral("git-bundle")
           : QStringLiteral("chronicle-file-store-v1");
}

bool VersionHistoryService::absorbEmbeddedHistory(const QByteArray& data)
{
    if (!m_store || !m_store->isOpen() || data.isEmpty()) {
        return false;
    }
    const bool adopted = m_store->unpackHistory(data);
    if (adopted) {
        m_revisionsChanged.notify();
    }
    return adopted;
}

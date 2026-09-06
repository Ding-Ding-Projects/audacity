/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QFileInfo>
#include <QTemporaryDir>

#include "internal/filesnapshotstore.h"
#include "internal/gitsnapshotstore.h"
#include "internal/versionhistoryservice.h"

using namespace au::chronicle;

namespace {
void writeFile(const QString& path, const QByteArray& content)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(content);
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

//! Runs the whole store contract against whichever backend is given. Both
//! backends must behave the same, which is what makes the fallback usable.
void checkStoreContract(ISnapshotStore& store, const QString& storePath, const QString& workTree)
{
    ASSERT_TRUE(store.open(storePath));
    ASSERT_TRUE(store.isOpen());

    QDir().mkpath(workTree);
    writeFile(workTree + "/project/song.aup4", "first");
    writeFile(workTree + "/settings/settings.ini", "theme=light");

    const QString first = store.commit(workTree, "Saved the project", actions::ProjectSave,
                                       QDateTime::currentDateTimeUtc());
    ASSERT_FALSE(first.isEmpty());

    writeFile(workTree + "/project/song.aup4", "second and longer");
    QFile::remove(workTree + "/settings/settings.ini");
    writeFile(workTree + "/settings/settings.ini", "theme=dark");

    const QString second = store.commit(workTree, "Changed theme to dark", actions::SettingsChange,
                                        QDateTime::currentDateTimeUtc());
    ASSERT_FALSE(second.isEmpty());
    EXPECT_NE(first, second);

    // Newest first, with the labels and actions that were recorded.
    QList<Revision> revisions = store.revisions();
    ASSERT_EQ(revisions.size(), 2);
    EXPECT_EQ(revisions.at(0).id, second);
    EXPECT_EQ(revisions.at(0).label, QString("Changed theme to dark"));
    EXPECT_EQ(revisions.at(0).action, actions::SettingsChange);
    EXPECT_EQ(revisions.at(1).id, first);
    EXPECT_EQ(revisions.at(1).action, actions::ProjectSave);

    // The diff summary names the files with their sizes.
    const QList<RevisionFile> files = store.files(second);
    ASSERT_EQ(files.size(), 2);
    EXPECT_EQ(files.at(0).path, QString("project/song.aup4"));
    EXPECT_EQ(files.at(0).size, 17);
    EXPECT_EQ(files.at(0).status, QString("modified"));

    // Restoring writes the old content back without touching the history.
    ASSERT_TRUE(store.checkout(first, workTree));
    EXPECT_EQ(readFile(workTree + "/project/song.aup4"), QByteArray("first"));
    EXPECT_EQ(readFile(workTree + "/settings/settings.ini"), QByteArray("theme=light"));
    EXPECT_EQ(store.revisions().size(), 2);

    // The restore is recorded as a new revision rather than a rewrite.
    const QString third = store.commit(workTree, "Restored an earlier revision", actions::Restore,
                                       QDateTime::currentDateTimeUtc());
    ASSERT_FALSE(third.isEmpty());
    revisions = store.revisions();
    ASSERT_EQ(revisions.size(), 3);
    EXPECT_EQ(revisions.at(0).id, third);
    EXPECT_EQ(revisions.at(0).action, actions::Restore);
    // The two older revisions keep their identifiers, so nothing was rewritten.
    EXPECT_EQ(revisions.at(1).id, second);
    EXPECT_EQ(revisions.at(2).id, first);

    // A label edit changes only the label.
    ASSERT_TRUE(store.setLabel(second, "Changed the theme to dark"));
    revisions = store.revisions();
    ASSERT_EQ(revisions.size(), 3);
    EXPECT_EQ(revisions.at(1).id, second);
    EXPECT_EQ(revisions.at(1).label, QString("Changed the theme to dark"));

    // Export writes the content of a revision into a folder of its own.
    const QString exportDir = storePath + "-export";
    ASSERT_TRUE(store.exportTo(second, exportDir));
    EXPECT_EQ(readFile(exportDir + "/project/song.aup4"), QByteArray("second and longer"));
    EXPECT_EQ(readFile(exportDir + "/settings/settings.ini"), QByteArray("theme=dark"));

    // Retention hides the older revisions and never the newest.
    const int pruned = store.prune(1, 0);
    EXPECT_EQ(pruned, 2);
    revisions = store.revisions();
    ASSERT_EQ(revisions.size(), 1);
    EXPECT_EQ(revisions.at(0).id, third);
}
}

TEST(ChronicleSnapshotStore, ContentAddressedFallbackSatisfiesTheContract)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());

    FileSnapshotStore store;
    checkStoreContract(store, temporary.path() + "/history.store", temporary.path() + "/stage");
}

TEST(ChronicleSnapshotStore, GitBackedStoreSatisfiesTheContract)
{
    if (!GitSnapshotStore::isGitAvailable()) {
        GTEST_SKIP() << "git is not on PATH, the fallback store carries this contract instead";
    }

    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());

    GitSnapshotStore store;
    checkStoreContract(store, temporary.path() + "/history.git", temporary.path() + "/stage");
}

TEST(ChronicleSnapshotStore, FallbackStoreDeduplicatesIdenticalContent)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());

    const QString storePath = temporary.path() + "/history.store";
    const QString workTree = temporary.path() + "/stage";

    FileSnapshotStore store;
    ASSERT_TRUE(store.open(storePath));

    writeFile(workTree + "/a.txt", "same content");
    writeFile(workTree + "/b.txt", "same content");
    ASSERT_FALSE(store.commit(workTree, "Two identical files", actions::Manual,
                              QDateTime::currentDateTimeUtc()).isEmpty());

    // Content addressing means one object, not two.
    int objectCount = 0;
    QDirIterator iterator(storePath + "/objects", QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        ++objectCount;
    }
    EXPECT_EQ(objectCount, 1);
}

TEST(ChronicleSnapshotStore, FallbackStorePackAndUnpackRoundTripsTheWholeHistory)
{
    QTemporaryDir sourceDir;
    QTemporaryDir targetDir;
    ASSERT_TRUE(sourceDir.isValid());
    ASSERT_TRUE(targetDir.isValid());

    const QString workTree = sourceDir.path() + "/stage";

    FileSnapshotStore source;
    ASSERT_TRUE(source.open(sourceDir.path() + "/history.store"));
    writeFile(workTree + "/song.aup4", "first");
    source.commit(workTree, "First save", actions::ProjectSave, QDateTime::currentDateTimeUtc());
    writeFile(workTree + "/song.aup4", "second, now longer");
    const QString head = source.commit(workTree, "Second save", actions::ProjectSave, QDateTime::currentDateTimeUtc());
    ASSERT_FALSE(head.isEmpty());

    const QByteArray packed = source.packHistory();
    ASSERT_FALSE(packed.isEmpty());

    FileSnapshotStore target;
    ASSERT_TRUE(target.open(targetDir.path() + "/history.store"));
    EXPECT_TRUE(target.revisions().isEmpty());

    ASSERT_TRUE(target.unpackHistory(packed));

    const QList<Revision> sourceRevisions = source.revisions();
    const QList<Revision> targetRevisions = target.revisions();
    ASSERT_EQ(sourceRevisions.size(), targetRevisions.size());
    for (int i = 0; i < sourceRevisions.size(); ++i) {
        EXPECT_EQ(sourceRevisions.at(i).id, targetRevisions.at(i).id);
        EXPECT_EQ(sourceRevisions.at(i).label, targetRevisions.at(i).label);
    }

    // The restored history is actually usable, not just listed: exporting the
    // newest revision on the receiving end produces the real file content.
    const QString exportDir = targetDir.path() + "-export";
    ASSERT_TRUE(target.exportTo(head, exportDir));
    EXPECT_EQ(readFile(exportDir + "/song.aup4"), QByteArray("second, now longer"));

    // Unpacking an unrelated or already-current history is a safe no-op: it
    // never discards what the receiving store already has.
    EXPECT_FALSE(target.unpackHistory(packed));
    EXPECT_EQ(target.revisions().size(), targetRevisions.size());
}

TEST(ChronicleSnapshotStore, GitBackedStorePackAndUnpackRoundTripsTheWholeHistory)
{
    if (!GitSnapshotStore::isGitAvailable()) {
        GTEST_SKIP() << "git is not on PATH, the fallback store carries this contract instead";
    }

    QTemporaryDir sourceDir;
    QTemporaryDir targetDir;
    ASSERT_TRUE(sourceDir.isValid());
    ASSERT_TRUE(targetDir.isValid());

    const QString workTree = sourceDir.path() + "/stage";

    GitSnapshotStore source;
    ASSERT_TRUE(source.open(sourceDir.path() + "/history.git"));
    writeFile(workTree + "/song.aup4", "first");
    source.commit(workTree, "First save", actions::ProjectSave, QDateTime::currentDateTimeUtc());
    writeFile(workTree + "/song.aup4", "second, now longer");
    const QString head = source.commit(workTree, "Second save", actions::ProjectSave, QDateTime::currentDateTimeUtc());
    ASSERT_FALSE(head.isEmpty());

    const QByteArray bundle = source.packHistory();
    ASSERT_FALSE(bundle.isEmpty());

    GitSnapshotStore target;
    ASSERT_TRUE(target.open(targetDir.path() + "/history.git"));
    EXPECT_TRUE(target.revisions().isEmpty());

    ASSERT_TRUE(target.unpackHistory(bundle));

    const QList<Revision> sourceRevisions = source.revisions();
    const QList<Revision> targetRevisions = target.revisions();
    ASSERT_EQ(sourceRevisions.size(), targetRevisions.size());
    for (int i = 0; i < sourceRevisions.size(); ++i) {
        EXPECT_EQ(sourceRevisions.at(i).id, targetRevisions.at(i).id);
    }

    const QString exportDir = targetDir.path() + "-export";
    ASSERT_TRUE(target.exportTo(head, exportDir));
    EXPECT_EQ(readFile(exportDir + "/song.aup4"), QByteArray("second, now longer"));

    // Unbundling the exact same history again is a fast forward no-op for
    // git (already up to date counts as success, the same as a real
    // advance), and nothing here is lost or duplicated either way.
    target.unpackHistory(bundle);
    EXPECT_EQ(target.revisions().size(), targetRevisions.size());
}

TEST(ChronicleVersionHistory, LabelsDescribeWhatChanged)
{
    EXPECT_EQ(VersionHistoryService::deriveLabel(actions::ProjectSave, "Interview take 2"),
              QString("Saved Interview take 2"));
    EXPECT_EQ(VersionHistoryService::deriveLabel(actions::SettingsChange, "theme to dark"),
              QString("Changed theme to dark"));
    EXPECT_EQ(VersionHistoryService::deriveLabel(actions::PresetDelete, "Vocals"),
              QString("Deleted preset Vocals"));
    EXPECT_EQ(VersionHistoryService::deriveLabel(actions::DiscardUnsaved, "Interview take 2"),
              QString("Discarded unsaved work in Interview take 2"));
    // With no detail the label still says what happened rather than nothing.
    EXPECT_EQ(VersionHistoryService::deriveLabel(actions::Restore, QString()),
              QString("Restored an earlier revision"));
}

TEST(ChronicleVersionHistory, ActionTitlesAreHumanReadable)
{
    EXPECT_EQ(actionTitle(actions::ProjectSave), QString("Project saved"));
    EXPECT_EQ(actionTitle(actions::DiscardUnsaved), QString("Discarded unsaved work"));
    EXPECT_EQ(actionTitle("something-else"), QString("Snapshot"));
}

TEST(ChronicleVersionHistory, ActionFamilyGroupsFixedActionsTheWayAUserThinksAboutThem)
{
    EXPECT_EQ(actionFamily(actions::ProjectSave), QString("save"));
    EXPECT_EQ(actionFamily(actions::Restore), QString("restore"));
    EXPECT_EQ(actionFamily(actions::DiscardUnsaved), QString("restore"));
    EXPECT_EQ(actionFamily(actions::SettingsChange), QString("project-settings"));
    EXPECT_EQ(actionFamily(actions::PresetSave), QString("project-settings"));
    EXPECT_EQ(actionFamily(actions::PresetDelete), QString("project-settings"));
}

TEST(ChronicleVersionHistory, ActionFamilyGuessesTheFamilyOfAFreeFormUndoActionName)
{
    // These are the kind of names Audacity's own undo stack actually carries,
    // rather than the fixed identifiers this module defines itself.
    EXPECT_EQ(actionFamily("Cut"), QString("edit"));
    EXPECT_EQ(actionFamily("Paste"), QString("edit"));
    EXPECT_EQ(actionFamily("Move clip \"Vocal take 2\""), QString("clip"));
    EXPECT_EQ(actionFamily("Split"), QString("clip"));
    EXPECT_EQ(actionFamily("Add track"), QString("track"));
    EXPECT_EQ(actionFamily("Remove Track"), QString("track"));
    EXPECT_EQ(actionFamily("Apply Amplify"), QString("effect"));
    EXPECT_EQ(actionFamily("Noise Reduction"), QString("effect"));
    EXPECT_EQ(actionFamily("Generate Tone"), QString("generate"));
    EXPECT_EQ(actionFamily("Edit Labels"), QString("label"));
    EXPECT_EQ(actionFamily("Envelope point added"), QString("envelope"));
}

TEST(ChronicleVersionHistory, ActionFamilyTitlesAreHumanReadable)
{
    EXPECT_EQ(actionFamilyTitle("clip"), QString("Clip"));
    EXPECT_EQ(actionFamilyTitle("project-settings"), QString("Project settings"));
    EXPECT_EQ(actionFamilyTitle("save"), QString("Save"));
    EXPECT_EQ(actionFamilyTitle("restore"), QString("Restore"));
    EXPECT_EQ(actionFamilyTitle("unknown-family"), QString("Edit"));
}

TEST(ChronicleVersionHistory, OnlySaveAndRestoreAreMilestones)
{
    EXPECT_TRUE(isMilestoneAction(actions::ProjectSave));
    EXPECT_TRUE(isMilestoneAction(actions::Restore));
    EXPECT_FALSE(isMilestoneAction(actions::DiscardUnsaved));
    EXPECT_FALSE(isMilestoneAction("Cut"));
}

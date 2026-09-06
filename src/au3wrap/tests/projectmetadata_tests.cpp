/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include <sqlite3.h>

#include "au3-sqlite-helpers/sqlite/SQLiteUtils.h"

#include "au3wrap/au3projectmetadata.h"

using namespace au::au3;

namespace {
class ScopedInMemoryDb
{
public:
    ScopedInMemoryDb()
    {
        // au3's own SQLite is built with SQLITE_OMIT_AUTOINIT, so nothing
        // opens correctly until this has run at least once for the process.
        // ProjectFileIO normally triggers it as a side effect of its own
        // startup; this test has no ProjectFileIO to do that for it.
        audacity::sqlite::Initialize();
        sqlite3_open(":memory:", &m_db);
    }

    ~ScopedInMemoryDb()
    {
        sqlite3_close(m_db);
    }

    sqlite3* get() const { return m_db; }

private:
    sqlite3* m_db = nullptr;
};
}

TEST(Au3ProjectMetadata, GeneratesAndPersistsAStableProjectId)
{
    ScopedInMemoryDb db;

    const QString first = internal::chronicleStableProjectIdFromDb(db.get());
    EXPECT_FALSE(first.isEmpty());

    // Asking again against the same database returns the identifier that
    // was already written there rather than generating a second one, which
    // is what lets a project's history follow it across a rename.
    const QString second = internal::chronicleStableProjectIdFromDb(db.get());
    EXPECT_EQ(first, second);
}

TEST(Au3ProjectMetadata, TwoDatabasesGetDifferentIds)
{
    ScopedInMemoryDb dbOne;
    ScopedInMemoryDb dbTwo;

    const QString idOne = internal::chronicleStableProjectIdFromDb(dbOne.get());
    const QString idTwo = internal::chronicleStableProjectIdFromDb(dbTwo.get());

    EXPECT_FALSE(idOne.isEmpty());
    EXPECT_FALSE(idTwo.isEmpty());
    EXPECT_NE(idOne, idTwo);
}

TEST(Au3ProjectMetadata, AnUnreachableDatabaseReturnsAnEmptyIdRatherThanCrashing)
{
    EXPECT_TRUE(internal::chronicleStableProjectIdFromDb(nullptr).isEmpty());
}

TEST(Au3ProjectMetadata, AZeroProjectPointerReturnsAnEmptyIdRatherThanCrashing)
{
    EXPECT_TRUE(chronicleStableProjectId(0).isEmpty());
}

TEST(Au3ProjectMetadata, WriteAndReadBundleRoundTripsExactly)
{
    ScopedInMemoryDb db;

    const QByteArray original("a packed history bundle, standing in for real git bundle bytes");
    ASSERT_TRUE(internal::writeChronicleBundleToDb(db.get(), original, "git-bundle"));

    QString format;
    const QByteArray readBack = internal::readChronicleBundleFromDb(db.get(), &format);
    EXPECT_EQ(readBack, original);
    EXPECT_EQ(format, QString("git-bundle"));
}

TEST(Au3ProjectMetadata, WritingASecondBundleReplacesTheFirstRatherThanAppending)
{
    ScopedInMemoryDb db;

    ASSERT_TRUE(internal::writeChronicleBundleToDb(db.get(), "first bundle", "chronicle-file-store-v1"));
    ASSERT_TRUE(internal::writeChronicleBundleToDb(db.get(), "second, newer bundle", "chronicle-file-store-v1"));

    QString format;
    const QByteArray readBack = internal::readChronicleBundleFromDb(db.get(), &format);
    EXPECT_EQ(readBack, QByteArray("second, newer bundle"));
}

TEST(Au3ProjectMetadata, ReadingWithNothingEmbeddedReturnsAnEmptyArray)
{
    ScopedInMemoryDb db;

    QString format;
    EXPECT_TRUE(internal::readChronicleBundleFromDb(db.get(), &format).isEmpty());
    EXPECT_TRUE(format.isEmpty());
}

TEST(Au3ProjectMetadata, WriteAndReadRejectAnUnreachableDatabaseRatherThanCrashing)
{
    EXPECT_FALSE(internal::writeChronicleBundleToDb(nullptr, "bytes", "git-bundle"));
    EXPECT_TRUE(internal::readChronicleBundleFromDb(nullptr, nullptr).isEmpty());
    EXPECT_TRUE(writeChronicleBundle(0, "bytes", "git-bundle") == false);
    EXPECT_TRUE(readChronicleBundle(0, nullptr).isEmpty());
}

TEST(Au3ProjectMetadata, WritingAnEmptyByteArrayIsRefusedRatherThanClearingTheBundle)
{
    ScopedInMemoryDb db;

    ASSERT_TRUE(internal::writeChronicleBundleToDb(db.get(), "a real bundle", "git-bundle"));
    EXPECT_FALSE(internal::writeChronicleBundleToDb(db.get(), QByteArray(), "git-bundle"));

    QString format;
    EXPECT_EQ(internal::readChronicleBundleFromDb(db.get(), &format), QByteArray("a real bundle"));
}

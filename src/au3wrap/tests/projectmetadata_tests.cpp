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

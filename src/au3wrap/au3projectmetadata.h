/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <cstdint>

#include <QString>

struct sqlite3;

namespace au::au3 {
/*!
 * A stable identifier for the local version history, stored in a small table
 * of the project's own aup4 database rather than derived from the file path.
 * The path changes when a project is renamed or moved; this identifier does
 * not, so the history a project has recorded stays its history across a
 * rename or a move.
 *
 * The identifier is created and persisted the first time it is asked for and
 * is then read back unchanged on every later call, for the lifetime of the
 * project file. au3ProjectPtr is the raw pointer returned by
 * IAudacityProject::au3ProjectPtr(), reinterpreted internally as the au3
 * AudacityProject it actually is; this header stays free of au3 and Qt
 * widget types so callers outside au3wrap do not need to link against them.
 *
 * Returns an empty string when the project's database cannot be reached at
 * all (a genuine disk or I/O failure). A caller must treat that as "no
 * stable identifier available yet" and fall back to something else, never as
 * a reason to fail whatever it was doing.
 */
QString chronicleStableProjectId(uintptr_t au3ProjectPtr);

namespace internal {
//! The part of chronicleStableProjectId that touches only a plain sqlite3
//! handle: create the table if it is missing, read the id if one is there,
//! otherwise generate one and persist it. Split out so it can be tested
//! directly against an in-memory database, without needing a live au3
//! AudacityProject.
QString chronicleStableProjectIdFromDb(sqlite3* db);
}
}

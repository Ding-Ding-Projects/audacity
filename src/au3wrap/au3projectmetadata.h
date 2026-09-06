/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <cstdint>

#include <QByteArray>
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

/*!
 * Writes the packed local history (from ISnapshotStore::packHistory) into a
 * single row `chronicle_bundle` table in the project's own aup4 database,
 * replacing whatever was embedded there before. format names the packing
 * scheme ("git-bundle" or "chronicle-file-store-v1") so a later reader knows
 * which store to hand the bytes to.
 *
 * Returns false when the project's database cannot be reached or the write
 * itself fails. A caller must let the save that triggered this succeed
 * regardless, and only report that history embedding did not happen this
 * time; the previously embedded bundle, if any, is left in place on failure.
 */
bool writeChronicleBundle(uintptr_t au3ProjectPtr, const QByteArray& bytes, const QString& format);

/*!
 * Reads the bundle written by writeChronicleBundle, if any. formatOut
 * receives the format string it was written with. Returns an empty byte
 * array, with formatOut left untouched, when nothing has been embedded yet
 * or the project's database cannot be reached.
 */
QByteArray readChronicleBundle(uintptr_t au3ProjectPtr, QString* formatOut);

namespace internal {
//! The part of chronicleStableProjectId that touches only a plain sqlite3
//! handle: create the table if it is missing, read the id if one is there,
//! otherwise generate one and persist it. Split out so it can be tested
//! directly against an in-memory database, without needing a live au3
//! AudacityProject.
QString chronicleStableProjectIdFromDb(sqlite3* db);

//! The plain sqlite3 handle part of writeChronicleBundle and
//! readChronicleBundle, tested the same way.
bool writeChronicleBundleToDb(sqlite3* db, const QByteArray& bytes, const QString& format);
QByteArray readChronicleBundleFromDb(sqlite3* db, QString* formatOut);
}
}

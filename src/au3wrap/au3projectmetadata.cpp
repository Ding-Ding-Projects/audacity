/*
* Audacity: A Digital Audio Editor
*/
#include "au3projectmetadata.h"

#include <sqlite3.h>

#include <QDateTime>
#include <QUuid>

#include "au3-project-file-io/DBConnection.h"
#include "au3-project-file-io/ProjectFileIO.h"
#include "au3-project/Project.h"

#include "framework/global/log.h"

#include "au3types.h"

using namespace au::au3;

namespace {
const char* PROJECT_ID_KEY = "project_id";

bool execSql(sqlite3* db, const char* sql)
{
    char* errorMessage = nullptr;
    const int result = sqlite3_exec(db, sql, nullptr, nullptr, &errorMessage);
    if (result != SQLITE_OK) {
        LOGW() << "chronicle: sqlite exec failed: " << (errorMessage ? errorMessage : "unknown error");
        sqlite3_free(errorMessage);
        return false;
    }
    return true;
}

bool ensureMetaTable(sqlite3* db)
{
    return execSql(db, "CREATE TABLE IF NOT EXISTS chronicle_meta (key TEXT PRIMARY KEY, value TEXT) WITHOUT ROWID;");
}

QString readMetaValue(sqlite3* db, const char* key)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql = "SELECT value FROM chronicle_meta WHERE key = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        return QString();
    }

    QString value;
    sqlite3_bind_text(statement, 1, key, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement) == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(statement, 0);
        if (text) {
            value = QString::fromUtf8(reinterpret_cast<const char*>(text));
        }
    }
    sqlite3_finalize(statement);
    return value;
}

sqlite3* dbForProject(uintptr_t au3ProjectPtr)
{
    if (au3ProjectPtr == 0) {
        return nullptr;
    }

    Au3Project* project = reinterpret_cast<Au3Project*>(au3ProjectPtr);
    try {
        auto& projectFileIO = ProjectFileIO::Get(*project);
        return projectFileIO.GetConnection().DB();
    } catch (...) {
        // GetConnection() throws only on a genuine disk or I/O failure. The
        // caller falls back to something else rather than losing history
        // entirely over a database it cannot reach right now.
        LOGW() << "chronicle: could not reach the project's database";
        return nullptr;
    }
}

const char* CHRONICLE_BUNDLE_TABLE_SQL
    ="CREATE TABLE IF NOT EXISTS chronicle_bundle ("
     "id INTEGER PRIMARY KEY CHECK (id = 1), "
     "format TEXT, "
     "bytes BLOB, "
     "created_at INTEGER);";

bool ensureBundleTable(sqlite3* db)
{
    return execSql(db, CHRONICLE_BUNDLE_TABLE_SQL);
}

bool writeMetaValue(sqlite3* db, const char* key, const QString& value)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql = "INSERT OR REPLACE INTO chronicle_meta (key, value) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        return false;
    }

    const QByteArray utf8Value = value.toUtf8();
    sqlite3_bind_text(statement, 1, key, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, utf8Value.constData(), utf8Value.size(), SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}
}

QString au::au3::internal::chronicleStableProjectIdFromDb(sqlite3* db)
{
    if (!db || !ensureMetaTable(db)) {
        return QString();
    }

    QString id = readMetaValue(db, PROJECT_ID_KEY);
    if (!id.isEmpty()) {
        return id;
    }

    id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!writeMetaValue(db, PROJECT_ID_KEY, id)) {
        LOGW() << "chronicle: could not persist a new stable project id";
        return QString();
    }

    return id;
}

QString au::au3::chronicleStableProjectId(uintptr_t au3ProjectPtr)
{
    return internal::chronicleStableProjectIdFromDb(dbForProject(au3ProjectPtr));
}

bool au::au3::internal::writeChronicleBundleToDb(sqlite3* db, const QByteArray& bytes, const QString& format)
{
    if (!db || bytes.isEmpty() || !ensureBundleTable(db)) {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql = "INSERT OR REPLACE INTO chronicle_bundle (id, format, bytes, created_at) VALUES (1, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        return false;
    }

    const QByteArray utf8Format = format.toUtf8();
    sqlite3_bind_text(statement, 1, utf8Format.constData(), utf8Format.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(statement, 2, bytes.constData(), bytes.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement, 3, static_cast<sqlite3_int64>(QDateTime::currentSecsSinceEpoch()));

    const bool ok = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

QByteArray au::au3::internal::readChronicleBundleFromDb(sqlite3* db, QString* formatOut)
{
    if (!db || !ensureBundleTable(db)) {
        return QByteArray();
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql = "SELECT format, bytes FROM chronicle_bundle WHERE id = 1;";
    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        return QByteArray();
    }

    QByteArray bytes;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        if (formatOut) {
            const unsigned char* formatText = sqlite3_column_text(statement, 0);
            *formatOut = formatText ? QString::fromUtf8(reinterpret_cast<const char*>(formatText)) : QString();
        }
        const void* blob = sqlite3_column_blob(statement, 1);
        const int size = sqlite3_column_bytes(statement, 1);
        if (blob && size > 0) {
            bytes = QByteArray(reinterpret_cast<const char*>(blob), size);
        }
    }
    sqlite3_finalize(statement);
    return bytes;
}

bool au::au3::writeChronicleBundle(uintptr_t au3ProjectPtr, const QByteArray& bytes, const QString& format)
{
    return internal::writeChronicleBundleToDb(dbForProject(au3ProjectPtr), bytes, format);
}

QByteArray au::au3::readChronicleBundle(uintptr_t au3ProjectPtr, QString* formatOut)
{
    return internal::readChronicleBundleFromDb(dbForProject(au3ProjectPtr), formatOut);
}

/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class QFileSystemWatcher;

namespace au::experience {
//! The parsed contents of the shared School mode record. This is a user
//! experience lock, never a security boundary: the whole record can be
//! erased by deleting the file named in sharedFilePath(), and doing so is
//! the documented recovery route when the unlock credential is forgotten.
struct SchoolModeRecord
{
    bool on = false;
    //! Display name shown everywhere the mode is mentioned. Defaults to
    //! "School mode" but the shipped name must never be shown again once
    //! this has been changed.
    QString displayName = QStringLiteral("School mode");
    //! SHA-256 hex digest of the unlock PIN or password, salted with a
    //! random per-record salt. Empty means no credential has been set yet,
    //! in which case the mode cannot be turned on (there would be no way
    //! to turn it back off).
    QString credentialHashHex;
    QString credentialSaltHex;

    bool isValid() const { return !displayName.isEmpty() && displayName.size() <= 80; }
};

//! Parses and serializes the shared School mode JSON record, and verifies
//! an entered unlock credential against a stored salted hash. Every method
//! here is pure and safe to unit test without touching a real file.
class SchoolModeStore
{
public:
    struct ParseResult
    {
        bool ok = false;
        //! True when a validated pre-version record was accepted as the
        //! documented version-0 format. The next save serializes version 1.
        bool migratedFromVersion0 = false;
        QString error;
        SchoolModeRecord record;
    };

    struct SharedRecordResult
    {
        bool available = false;
        bool hasKnownRecord = false;
        QString error;
        SchoolModeRecord record;
    };

    static ParseResult parse(const QByteArray& json);
    static QByteArray serialize(const SchoolModeRecord& record);

    //! Generates a fresh random salt as lowercase hex.
    static QString newSaltHex();
    //! SHA-256 of (salt || credential), as lowercase hex.
    static QString hashCredential(const QString& credential, const QString& saltHex);
    //! Verifies a candidate credential against a stored hash and salt.
    static bool verifyCredential(const QString& candidate, const QString& saltHex, const QString& storedHashHex);

    //! Path to the shared record: "<app data parent>/shared/school-mode.json".
    //! The parent directory is one level above this application's own data
    //! directory, so every app on this machine that honours School mode
    //! reads and writes the same file.
    static QString sharedFilePath();
    static ParseResult readRecordFile(const QString& path);
    static SharedRecordResult sharedRecord();
};

//! Watches the shared file and exposes the live state to the rest of the
//! application. Applying it (hiding Cantonese, bilingual, funny levels,
//! personal vocabulary and the dim sum surprise) is done by callers reading
//! isOn() and renamedDisplayName(); this class only owns the file.
class SchoolModeService : public QObject
{
    Q_OBJECT

public:
    explicit SchoolModeService(QObject* parent = nullptr, const QString& recordPath = QString());

    bool isOn() const { return m_record.on; }
    bool isAvailable() const { return m_available; }
    bool hasKnownRecord() const { return m_hasKnownRecord; }
    QString error() const { return m_error; }
    QString displayName() const { return m_record.displayName; }
    bool hasCredential() const { return !m_record.credentialHashHex.isEmpty(); }

    void reload();

    //! Turns the mode on. Requires a credential to already be set, or one
    //! is created from newCredential if provided (first activation only).
    bool turnOn(const QString& newCredential = QString());
    //! Turns the mode off. Requires the correct credential.
    bool turnOff(const QString& credential);
    //! Renames the mode. The shipped name "School mode" is never shown
    //! again once this has been called with a different name.
    bool rename(const QString& newDisplayName);

signals:
    void stateChanged();

private:
    bool save(const SchoolModeRecord& record);
    void onFileChanged();

    SchoolModeRecord m_record;
    QString m_recordPath;
    bool m_available = true;
    bool m_hasKnownRecord = false;
    QString m_error;
    QFileSystemWatcher* m_watcher = nullptr;
};
}

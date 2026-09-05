/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

namespace au::chronicle {
//! One file inside a revision, as reported by the snapshot store.
struct RevisionFile {
    QString path;
    qint64 size = 0;
    //! "added", "modified", "deleted" or "unchanged".
    QString status;
};

//! One entry of the local, append-only version history.
struct Revision {
    //! The store identifier. A commit hash for the git store, a content hash
    //! for the fallback store.
    QString id;
    QDateTime timestamp;
    //! A human readable description of what changed, for example
    //! "Deleted track Vocals" or "Changed theme to dark".
    QString label;
    //! The action that produced the snapshot. Used for the action filter
    //! chips and for the label derivation.
    QString action;

    bool isValid() const { return !id.isEmpty(); }
};

//! The identifiers used for the action field. They are stable, so a stored
//! history stays readable across releases.
namespace actions {
inline const QString ProjectSave = QStringLiteral("project-save");
inline const QString SettingsChange = QStringLiteral("settings-change");
inline const QString PresetSave = QStringLiteral("preset-save");
inline const QString PresetDelete = QStringLiteral("preset-delete");
inline const QString Restore = QStringLiteral("restore");
inline const QString DiscardUnsaved = QStringLiteral("discard-unsaved");
inline const QString Manual = QStringLiteral("manual");
}

//! Human readable titles for the action identifiers above.
QString actionTitle(const QString& action);
}

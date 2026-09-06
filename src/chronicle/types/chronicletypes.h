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

//! The family an action belongs to, used to group the filter chips the way an
//! audio editor user thinks about their work rather than by the raw action
//! identifier. One of "edit", "clip", "track", "effect", "generate", "label",
//! "envelope", "project-settings", "save" or "restore".
QString actionFamily(const QString& action);

//! Human readable title for a family identifier, for example "edit" becomes
//! "Edit".
QString actionFamilyTitle(const QString& family);

//! Whether a revision produced by this action should be marked as a milestone
//! in the panel (a save, an export or a restore), independent of any label or
//! star the user later adds.
bool isMilestoneAction(const QString& action);
}

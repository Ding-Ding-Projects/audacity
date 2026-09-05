/*
* Audacity: A Digital Audio Editor
*/
#include "projecthistorywatcher.h"

#include <QCryptographicHash>

using namespace au::chronicle;

void ProjectHistoryWatcher::init()
{
    globalContext()->currentProjectChanged().onNotify(this, [this]() {
        onCurrentProjectChanged();
    });

    onCurrentProjectChanged();
}

void ProjectHistoryWatcher::onCurrentProjectChanged()
{
    const auto project = globalContext()->currentProject();
    if (!project) {
        versionHistory()->setCurrentProject(QString(), QString());
        return;
    }

    const QString path = QString::fromStdString(project->path().toStdString());

    // The identifier is derived from the path rather than from the name, so
    // two projects with the same name keep separate histories, and it is
    // hashed so that no part of the user's folder layout ends up in a
    // directory name.
    const QString id = QString::fromLatin1(
        QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex()).left(16);

    versionHistory()->setCurrentProject(id, path);

    m_lastNeedSave = project->needSave().val;

    project->needSaveChanged().onNotify(this, [this]() {
        const auto current = globalContext()->currentProject();
        if (!current) {
            return;
        }
        const bool needSave = current->needSave().val;
        // A save is the moment unsaved work stops being unsaved.
        if (m_lastNeedSave && !needSave) {
            versionHistory()->recordSnapshot(actions::ProjectSave, current->displayName());
        }
        m_lastNeedSave = needSave;
    });

    project->aboutCloseBegin().onNotify(this, [this]() {
        const auto current = globalContext()->currentProject();
        if (current && current->needSave().val) {
            // The entry that describes the loss is recorded before the work is
            // gone, so it is itself part of the history.
            versionHistory()->recordSnapshot(actions::DiscardUnsaved, current->displayName());
        }
    });
}

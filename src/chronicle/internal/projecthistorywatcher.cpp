/*
* Audacity: A Digital Audio Editor
*/
#include "projecthistorywatcher.h"

#include <QCryptographicHash>

#include "framework/global/translation.h"

#include "au3wrap/au3projectmetadata.h"

using namespace au::chronicle;

void ProjectHistoryWatcher::init()
{
    globalContext()->currentProjectChanged().onNotify(this, [this]() {
        onCurrentProjectChanged();
    });

    projectHistory()->historyChanged().onReceive(this, [this](trackedit::HistoryEvent event) {
        onHistoryChanged(event);
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

    // The identifier lives in the project's own aup4 database, so a project's
    // history follows it across a rename or a move rather than starting over.
    // It is only unavailable when the project's database itself cannot be
    // reached, which is rare enough that falling back to a hash of the path
    // is a reasonable second choice: a project without a stable id yet still
    // gets a history, just not one that survives being renamed.
    QString id = au::au3::chronicleStableProjectId(project->au3ProjectPtr());
    if (id.isEmpty()) {
        id = QString::fromLatin1(
            QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex()).left(16);
    }

    versionHistory()->setCurrentProject(id, path);

    // If this file was last saved somewhere else, its embedded history can be
    // ahead of whatever this machine has recorded for the same project id.
    // Absorbing is fast forward only, so an older or unrelated bundle simply
    // does nothing here.
    QString embeddedFormat;
    const QByteArray embedded = au::au3::readChronicleBundle(project->au3ProjectPtr(), &embeddedFormat);
    if (!embedded.isEmpty()) {
        versionHistory()->absorbEmbeddedHistory(embedded);
    }

    m_lastNeedSave = project->needSave().val;
    m_lastActionCount = projectHistory()->undoRedoActionCount();

    project->needSaveChanged().onNotify(this, [this]() {
        const auto current = globalContext()->currentProject();
        if (!current) {
            return;
        }
        const bool needSave = current->needSave().val;
        // A save is the moment unsaved work stops being unsaved.
        if (m_lastNeedSave && !needSave) {
            versionHistory()->recordSnapshot(actions::ProjectSave, current->displayName());
            embedHistoryIfEnabled(*current);
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

void ProjectHistoryWatcher::embedHistoryIfEnabled(const project::IAudacityProject& project)
{
    if (!versionHistory()->embedHistoryInSaveFile()) {
        return;
    }

    const QByteArray packed = versionHistory()->packHistoryForEmbedding();
    if (packed.isEmpty()) {
        // Nothing to embed yet (a brand new project with no recorded
        // revision), which is not a failure worth telling anyone about.
        return;
    }

    const bool embedded = au::au3::writeChronicleBundle(
        project.au3ProjectPtr(), packed, versionHistory()->embeddedHistoryFormat());

    if (!embedded) {
        // The save itself already succeeded by the time this runs, so this
        // is reported as its own, separate, non-blocking notice rather than
        // as a reason the save failed.
        notificationCenter()->push(
            au::experience::NotificationType::Warning,
            muse::qtrc("chronicle", "Project history was not saved"),
            muse::qtrc("chronicle", "The project itself saved correctly, but its local version history "
                                    "could not be embedded in the file this time."));
    }
}

void ProjectHistoryWatcher::onHistoryChanged(trackedit::HistoryEvent event)
{
    if (!versionHistory()->commitOnEveryAction()) {
        return;
    }

    if (!globalContext()->currentProject()) {
        return;
    }

    // RestoredState fires for undo and redo themselves. A revision recording
    // that the user moved through their own history would only clutter it,
    // and the states it moves between are already on record.
    if (event != trackedit::HistoryEvent::NewState) {
        return;
    }

    const size_t count = projectHistory()->undoRedoActionCount();
    if (count == m_lastActionCount) {
        // A continuous drag or an in-place edit consolidates into the entry
        // that is already there (UndoPushType::CONSOLIDATE), so the entry
        // count does not grow. Recording nothing here is what keeps a whole
        // drag as one revision, taken at the point it actually settles.
        return;
    }
    m_lastActionCount = count;

    const size_t index = projectHistory()->currentStateIndex();
    const QString name = projectHistory()->lastActionNameAtIdx(index).qTranslated();
    if (name.isEmpty()) {
        return;
    }

    versionHistory()->recordSnapshot(name, QString(), name);
}

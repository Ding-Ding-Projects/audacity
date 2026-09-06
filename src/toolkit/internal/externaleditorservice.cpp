#include "shared/profilepaths.h"
/*
* Audacity: A Digital Audio Editor
*/

#include "externaleditorservice.h"

#include <QStandardPaths>
#include <QProcess>
#include <QDir>
#include <QFile>

using namespace au::toolkit;

namespace {
struct KnownEditor {
    QString id;
    QString label;
    QString executableName;
    bool workspaceCapable;
};

const QList<KnownEditor>& knownEditors()
{
    static const QList<KnownEditor> editors = {
        { QStringLiteral("vscode"), QStringLiteral("Visual Studio Code"), QStringLiteral("code"), true },
        { QStringLiteral("vscode-insiders"), QStringLiteral("Visual Studio Code Insiders"), QStringLiteral(
              "code-insiders"), true },
        { QStringLiteral("vscodium"), QStringLiteral("VSCodium"), QStringLiteral("codium"), true },
        { QStringLiteral("gedit"), QStringLiteral("gedit"), QStringLiteral("gedit"), false },
        { QStringLiteral("kate"), QStringLiteral("Kate"), QStringLiteral("kate"), false },
    };
    return editors;
}
}

ExternalEditorService::ExternalEditorService(QObject* parent)
    : QObject(parent)
{
}

QString ExternalEditorService::preferredEditorId() const
{
    return m_preferredEditorId;
}

void ExternalEditorService::setPreferredEditorId(const QString& id)
{
    if (m_preferredEditorId == id) {
        return;
    }
    m_preferredEditorId = id;
    emit preferredEditorIdChanged();
}

QVariantList ExternalEditorService::detectEditors() const
{
    QVariantList result;
    bool anyFound = false;

    for (const KnownEditor& editor : knownEditors()) {
        const QString path = QStandardPaths::findExecutable(editor.executableName);
        QVariantMap entry;
        entry[QStringLiteral("id")] = editor.id;
        entry[QStringLiteral("label")] = editor.label;
        entry[QStringLiteral("executablePath")] = path;
        entry[QStringLiteral("found")] = !path.isEmpty();
        entry[QStringLiteral("workspaceCapable")] = editor.workspaceCapable;
        result << entry;
        anyFound = anyFound || !path.isEmpty();
    }

    for (const QVariant& custom : m_customEditors) {
        result << custom;
        anyFound = true;
    }

    if (!anyFound) {
        const_cast<ExternalEditorService*>(this)->noEditorFound();
    }

    // Discovery and installation are separate from permission to launch.
    // Keep installed-editor facts intact and expose why launching is unavailable.
    for (QVariant& item : result) {
        QVariantMap entry = item.toMap();
        entry[QStringLiteral("available")] = !au::profile::Paths::active() && entry.value(QStringLiteral("found")).toBool();
        entry[QStringLiteral("unavailableReason")] = au::profile::Paths::active()
            ? QStringLiteral("External editors are unavailable in an isolated verification profile.") : QString();
        item = entry;
    }
    return result;
}

void ExternalEditorService::addCustomEditor(const QString& id, const QString& label, const QString& executablePath)
{
    QVariantMap entry;
    entry[QStringLiteral("id")] = id;
    entry[QStringLiteral("label")] = label;
    entry[QStringLiteral("executablePath")] = executablePath;
    entry[QStringLiteral("found")] = QFile::exists(executablePath);
    entry[QStringLiteral("workspaceCapable")] = false;
    m_customEditors << entry;
}

namespace {
QString executableForId(const QString& editorId, const QVariantList& customEditors)
{
    for (const KnownEditor& editor : knownEditors()) {
        if (editor.id == editorId) {
            return QStandardPaths::findExecutable(editor.executableName);
        }
    }
    for (const QVariant& v : customEditors) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == editorId) {
            return m.value(QStringLiteral("executablePath")).toString();
        }
    }
    return QString();
}
}

bool ExternalEditorService::openFolder(const QString& editorId, const QString& folderPath)
{
    if (au::profile::Paths::active()) return false;
    const QString exe = executableForId(editorId, m_customEditors);
    if (exe.isEmpty()) {
        return false;
    }
    // Passing the folder as a literal argument opens it as the workspace
    // root in Visual Studio Code and its variants.
    return QProcess::startDetached(exe, { folderPath });
}

bool ExternalEditorService::openFile(const QString& editorId, const QString& filePath)
{
    if (au::profile::Paths::active()) return false;
    const QString exe = executableForId(editorId, m_customEditors);
    if (exe.isEmpty()) {
        return false;
    }
    return QProcess::startDetached(exe, { filePath });
}

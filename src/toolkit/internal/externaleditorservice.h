/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>

namespace au::toolkit {
//! Detects locally installed external code editors and opens a folder or a
//! file in the user's chosen one. Detection only looks at PATH and a small
//! set of standard installation locations; it never invokes a shell string,
//! it launches the found executable directly with literal arguments.
class ExternalEditorService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString preferredEditorId READ preferredEditorId WRITE setPreferredEditorId NOTIFY preferredEditorIdChanged)

public:
    explicit ExternalEditorService(QObject* parent = nullptr);

    QString preferredEditorId() const;
    void setPreferredEditorId(const QString& id);

    //! Each entry: { id, label, executablePath, found }.
    Q_INVOKABLE QVariantList detectEditors() const;

    Q_INVOKABLE void addCustomEditor(const QString& id, const QString& label, const QString& executablePath);

    //! Opens the given folder as the editor's workspace root when the
    //! editor supports that (Visual Studio Code and its variants do).
    Q_INVOKABLE bool openFolder(const QString& editorId, const QString& folderPath);
    Q_INVOKABLE bool openFile(const QString& editorId, const QString& filePath);

signals:
    void preferredEditorIdChanged();
    void noEditorFound();

private:
    QString m_preferredEditorId;
    QVariantList m_customEditors;
};
}

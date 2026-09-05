/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QVariantMap>

#include "async/asyncable.h"
#include "context/iuicontextresolver.h"
#include "modularity/ioc.h"

#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#include "ui/iuiactionsregister.h"

#include "muse_framework_config.h"
#ifdef MUSE_MODULE_SHORTCUTS
#include "shortcuts/ishortcutsregister.h"
#endif

namespace au::companion {
/*!
 * \brief The model behind the command palette.
 *
 * It indexes, in one flat list:
 *
 *   - every action registered with the muse UI actions register, with its
 *     title, its current shortcut, its enabled state and the module section
 *     it came from;
 *   - every preferences page and every setting inside it, from the hand
 *     written index in \c palette/settingsindex.json;
 *   - every appearance control the palette can drive directly;
 *   - every open project tab and dock panel, pushed in from QML through
 *     \c setContextRows because only the running window knows them;
 *   - every documentation article found under the repository \c docs
 *     directory, when the application can see it.
 *
 * Selecting a row either triggers an action or teleports: the palette opens
 * the owning surface, selects the page, scrolls the control into view,
 * focuses it and pulses a highlight.
 */
class CommandPaletteModel : public QAbstractListModel, public muse::async::Asyncable, public muse::actions::Actionable,
    public muse::Contextable
{
    Q_OBJECT

    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(bool useRegex READ useRegex WRITE setUseRegex NOTIFY useRegexChanged)
    Q_PROPERTY(bool filterValid READ filterValid NOTIFY filterChanged)
    Q_PROPERTY(QString filterError READ filterError NOTIFY filterChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool fullWindow READ fullWindow WRITE setFullWindow NOTIFY fullWindowChanged)

public:
    enum Roles {
        RowTypeRole = Qt::UserRole + 1, // "action" | "setting" | "appearance" | "page" | "context" | "doc"
        TitleRole,
        SubtitleRole,
        SectionRole,
        ShortcutRole,
        IconRole,
        EnabledRole,
        CheckableRole,
        CheckedRole,
        ControlTypeRole, // "" | "switch" | "slider" | "dropdown" | "color" | "text" | "number" | "radio" | "action" | "info"
        SettingKeyRole,
        SettingValueRole,
        OptionsRole,
        OptionLabelsRole,
        MinimumRole,
        MaximumRole,
        StepRole,
        ActionCodeRole,
        PageIdRole,
        TargetRole,
        PayloadRole
    };
    Q_ENUM(Roles)

    muse::ContextInject<muse::ui::IUiActionsRegister> actionsRegister { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher { this };
#ifdef MUSE_MODULE_SHORTCUTS
    muse::ContextInject<muse::shortcuts::IShortcutsRegister> shortcutsRegister { this };
#endif

    explicit CommandPaletteModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const;
    void setFilter(const QString& filter);
    bool useRegex() const;
    void setUseRegex(bool value);
    bool filterValid() const;
    QString filterError() const;

    bool fullWindow() const;
    void setFullWindow(bool value);

    //! Registers this model as the handler for the command palette action, so
    //! that the global Ctrl+Shift+F shortcut raises \c openRequested. Called
    //! once by the palette host.
    Q_INVOKABLE void registerAsPaletteHost();

    //! Builds the whole index. Safe to call again; the palette calls it every
    //! time it opens so that the action states and the open tabs are fresh.
    Q_INVOKABLE void reload();

    //! Rows only the running window knows: open project tabs and dock panels.
    //! Each entry takes \c title, \c subtitle, \c section, \c actionCode and
    //! an optional \c payload.
    Q_INVOKABLE void setContextRows(const QVariantList& rows);

    //! Triggers the row: dispatches its action, or reports the teleport the
    //! host should perform.
    Q_INVOKABLE void activate(int row);

    //! Live setting access for the inline controls. Both go through the same
    //! muse settings API the originating preferences surface uses, so a change
    //! made here is the same change made there.
    Q_INVOKABLE QVariant settingValue(const QString& key) const;
    Q_INVOKABLE void setSettingValue(const QString& key, const QVariant& value);

    //! Every setting key the model indexed. Used by the guard test.
    Q_INVOKABLE QStringList indexedSettingKeys() const;
    //! Every teleport target label the model indexed. Used by the guard test.
    Q_INVOKABLE QStringList indexedTargets() const;

    //! The directory the documentation rows were read from, or an empty string
    //! when no documentation was found.
    Q_INVOKABLE QString documentationRoot() const;

signals:
    void filterChanged();
    void useRegexChanged();
    void countChanged();
    void fullWindowChanged();

    //! Asks the host to open the preferences dialog on \a pageId and pulse a
    //! highlight on the control labelled \a target.
    void teleportToPreferences(const QString& pageId, const QString& target);
    //! Asks the host to open the documentation article at \a path.
    void teleportToDocument(const QString& path);
    //! Asks the host to raise the panel or project tab named by \a payload.
    void teleportToContext(const QVariantMap& payload);
    //! Raised after an action was dispatched, so the host can close itself.
    void activated();
    //! Raised when the command palette action is dispatched.
    void openRequested();

private:
    void onPaletteActionTriggered();

    struct Row
    {
        QString type;
        QString title;
        QString subtitle;
        QString section;
        QString shortcut;
        int icon = 0;
        bool enabled = true;
        bool checkable = false;
        bool checked = false;
        QString controlType;
        QString settingKey;
        QVariantList options;
        QVariantList optionLabels;
        double minimum = 0.0;
        double maximum = 1.0;
        double step = 1.0;
        QString actionCode;
        QString pageId;
        QString target;
        QVariantMap payload;
        QString haystack;
    };

    void buildActions();
    void buildPreferences();
    void buildAppearance();
    void buildDocumentation();
    void applyFilter();
    static QString rowHaystack(const Row& row);

    QVector<Row> m_all;
    QVector<int> m_visible;
    QVariantList m_contextRows;

    QString m_filter;
    bool m_useRegex = false;
    bool m_filterValid = true;
    QString m_filterError;
    QString m_documentationRoot;
};
}

/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QAbstractListModel>

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"

#include "inotificationcenter.h"

namespace au::experience {
//! Feeds both the toast stack and the notification centre. The "active" mode
//! lists what is on screen; the history mode lists everything, filtered by the
//! search field.
class NotificationListModel : public QAbstractListModel, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool historyMode READ historyMode WRITE setHistoryMode NOTIFY historyModeChanged FINAL)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged FINAL)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

    muse::GlobalInject<INotificationCenter> notificationCenter;

public:
    explicit NotificationListModel(QObject* parent = nullptr);

    enum Roles {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        TitleRole,
        BodyRole,
        ActionTextRole,
        ActionCodeRole,
        TimeTextRole,
        DismissedRole,
        PersistentRole
    };

    Q_INVOKABLE void load();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool historyMode() const;
    void setHistoryMode(bool value);

    QString searchText() const;
    void setSearchText(const QString& text);

    Q_INVOKABLE void dismiss(int id);
    Q_INVOKABLE void dismissAll();
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void triggerAction(int id);

    //! The rows currently shown by this model (already filtered by search),
    //! as an indented JSON array. Used for the notification centre's export
    //! action.
    Q_INVOKABLE QString exportJson() const;

signals:
    void historyModeChanged();
    void searchTextChanged();
    void countChanged();

private:
    void reload();
    bool matches(const Notification& notification) const;

    std::vector<Notification> m_rows;
    bool m_historyMode = false;
    QString m_searchText;
};
}

/*
 * Audacity: A Digital Audio Editor
 */
#include "notificationlistmodel.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>

namespace au::experience {
NotificationListModel::NotificationListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void NotificationListModel::load()
{
    reload();
    notificationCenter()->changed().onNotify(this, [this]() {
        reload();
    });
}

void NotificationListModel::reload()
{
    std::vector<Notification> source = m_historyMode ? notificationCenter()->history() : notificationCenter()->active();

    std::vector<Notification> rows;
    for (const Notification& notification : source) {
        if (matches(notification)) {
            rows.push_back(notification);
        }
    }

    beginResetModel();
    m_rows = rows;
    endResetModel();
    emit countChanged();
}

bool NotificationListModel::matches(const Notification& notification) const
{
    if (m_searchText.isEmpty()) {
        return true;
    }

    //! The search field offers the regular expression builder, so an
    //! expression that compiles is used as one and anything else is treated as
    //! plain text.
    const QRegularExpression expression(m_searchText, QRegularExpression::CaseInsensitiveOption);
    if (expression.isValid()) {
        return expression.match(notification.title).hasMatch() || expression.match(notification.body).hasMatch();
    }

    return notification.title.contains(m_searchText, Qt::CaseInsensitive)
           || notification.body.contains(m_searchText, Qt::CaseInsensitive);
}

int NotificationListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_rows.size());
}

QHash<int, QByteArray> NotificationListModel::roleNames() const
{
    return {
        { IdRole, "notificationId" },
        { TypeRole, "notificationType" },
        { TitleRole, "title" },
        { BodyRole, "body" },
        { ActionTextRole, "actionText" },
        { ActionCodeRole, "actionCode" },
        { TimeTextRole, "timeText" },
        { DismissedRole, "dismissed" },
        { PersistentRole, "persistent" }
    };
}

QVariant NotificationListModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    const Notification& notification = m_rows.at(static_cast<size_t>(index.row()));

    switch (role) {
    case IdRole:
        return notification.id;
    case TypeRole:
        return static_cast<int>(notification.type);
    case TitleRole:
        return notification.title;
    case BodyRole:
        return notification.body;
    case ActionTextRole:
        return notification.actionText;
    case ActionCodeRole:
        return notification.actionCode;
    case TimeTextRole:
        return QLocale().toString(QDateTime::fromMSecsSinceEpoch(notification.timestamp), QLocale::ShortFormat);
    case DismissedRole:
        return notification.dismissed;
    case PersistentRole:
        //! A warning or an error waits for the reader.
        return notification.type == NotificationType::Warning || notification.type == NotificationType::Error;
    default:
        break;
    }

    return QVariant();
}

bool NotificationListModel::historyMode() const
{
    return m_historyMode;
}

void NotificationListModel::setHistoryMode(bool value)
{
    if (m_historyMode == value) {
        return;
    }
    m_historyMode = value;
    emit historyModeChanged();
    reload();
}

QString NotificationListModel::searchText() const
{
    return m_searchText;
}

void NotificationListModel::setSearchText(const QString& text)
{
    if (m_searchText == text) {
        return;
    }
    m_searchText = text;
    emit searchTextChanged();
    reload();
}

void NotificationListModel::dismiss(int id)
{
    notificationCenter()->dismiss(id);
}

void NotificationListModel::dismissAll()
{
    notificationCenter()->dismissAll();
}

void NotificationListModel::clearHistory()
{
    notificationCenter()->clearHistory();
}

void NotificationListModel::triggerAction(int id)
{
    for (const Notification& notification : m_rows) {
        if (notification.id == id && !notification.actionCode.isEmpty()) {
            notificationCenter()->actionRequested().send(notification.actionCode);
            notificationCenter()->dismiss(id);
            return;
        }
    }
}

QString NotificationListModel::exportJson() const
{
    QJsonArray array;
    for (const Notification& notification : m_rows) {
        array.append(QJsonObject::fromVariantMap(notification.toMap()));
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Indented));
}
}

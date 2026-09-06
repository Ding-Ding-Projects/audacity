/*
* Audacity: A Digital Audio Editor
*/

#include "bookmarkmodel.h"

#include <algorithm>
#include <functional>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

using namespace au::toolkit;

BookmarkModel::BookmarkModel(QObject* parent)
    : QAbstractListModel(parent)
{
    load();
}

QString BookmarkModel::storePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/toolkit";
    QDir().mkpath(dir);
    return dir + "/docs-bookmarks.json";
}

void BookmarkModel::load()
{
    QFile file(storePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return;
    }

    m_bookmarks.clear();
    for (const QJsonValue& value : doc.array()) {
        const QJsonObject obj = value.toObject();
        Bookmark bookmark;
        bookmark.id = obj.value("id").toString();
        bookmark.articleId = obj.value("articleId").toString();
        bookmark.title = obj.value("title").toString();
        if (bookmark.id.isEmpty() || bookmark.articleId.isEmpty()) {
            continue;
        }
        m_bookmarks.append(bookmark);
    }
}

void BookmarkModel::save() const
{
    QJsonArray array;
    for (const Bookmark& bookmark : m_bookmarks) {
        QJsonObject obj;
        obj["id"] = bookmark.id;
        obj["articleId"] = bookmark.articleId;
        obj["title"] = bookmark.title;
        array.append(obj);
    }

    QFile file(storePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}

int BookmarkModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_bookmarks.size());
}

int BookmarkModel::rowCountProperty() const
{
    return rowCount();
}

QVariant BookmarkModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_bookmarks.size()) {
        return QVariant();
    }

    const Bookmark& bookmark = m_bookmarks.at(index.row());
    switch (role) {
    case IdRole:
        return bookmark.id;
    case ArticleIdRole:
        return bookmark.articleId;
    case TitleRole:
    case Qt::DisplayRole:
        return bookmark.title;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> BookmarkModel::roleNames() const
{
    return {
        { IdRole, "id" },
        { ArticleIdRole, "articleId" },
        { TitleRole, "title" }
    };
}

bool BookmarkModel::isBookmarked(const QString& articleId) const
{
    for (const Bookmark& bookmark : m_bookmarks) {
        if (bookmark.articleId == articleId) {
            return true;
        }
    }
    return false;
}

void BookmarkModel::add(const QString& articleId, const QString& title)
{
    if (articleId.isEmpty() || isBookmarked(articleId)) {
        return;
    }

    const int row = static_cast<int>(m_bookmarks.size());
    beginInsertRows(QModelIndex(), row, row);
    Bookmark bookmark;
    bookmark.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    bookmark.articleId = articleId;
    bookmark.title = title;
    m_bookmarks.append(bookmark);
    endInsertRows();

    save();
    emit countChanged();
}

void BookmarkModel::removeByArticleId(const QString& articleId)
{
    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks.at(i).articleId == articleId) {
            removeAt(i);
            return;
        }
    }
}

void BookmarkModel::removeAt(int row)
{
    if (row < 0 || row >= m_bookmarks.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_bookmarks.removeAt(row);
    endRemoveRows();

    save();
    emit countChanged();
}

void BookmarkModel::removeMany(const QVariantList& rows)
{
    // Descending order so earlier removals never shift a later index.
    QVector<int> indexes;
    indexes.reserve(rows.size());
    for (const QVariant& value : rows) {
        indexes.append(value.toInt());
    }
    std::sort(indexes.begin(), indexes.end(), std::greater<int>());

    for (int row : indexes) {
        removeAt(row);
    }
}

void BookmarkModel::rename(const QString& articleId, const QString& newTitle)
{
    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks.at(i).articleId == articleId) {
            m_bookmarks[i].title = newTitle;
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, { TitleRole, Qt::DisplayRole });
            save();
            return;
        }
    }
}

void BookmarkModel::toggle(const QString& articleId, const QString& title)
{
    if (isBookmarked(articleId)) {
        removeByArticleId(articleId);
    } else {
        add(articleId, title);
    }
}

QVariantList BookmarkModel::toExportRows() const
{
    QVariantList rows;
    for (const Bookmark& bookmark : m_bookmarks) {
        QVariantMap row;
        row["id"] = bookmark.id;
        row["articleId"] = bookmark.articleId;
        row["title"] = bookmark.title;
        rows.append(row);
    }
    return rows;
}

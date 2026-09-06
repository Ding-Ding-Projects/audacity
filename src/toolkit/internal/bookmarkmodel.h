/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <QVector>

namespace au::toolkit {
//! Bookmarks for the in-app documentation browser. Each bookmark names an
//! article id (see DocsIndex) plus a user editable title that starts as the
//! article's own title and can be renamed without touching the article
//! itself. Persisted as a small JSON array under the application's user
//! data directory, in a "toolkit" subdirectory, as "docs-bookmarks.json".
class BookmarkModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCountProperty NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        ArticleIdRole,
        TitleRole
    };
    Q_ENUM(Role)

    explicit BookmarkModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const;

    Q_INVOKABLE bool isBookmarked(const QString& articleId) const;
    Q_INVOKABLE void add(const QString& articleId, const QString& title);
    Q_INVOKABLE void removeByArticleId(const QString& articleId);
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void removeMany(const QVariantList& rows);
    Q_INVOKABLE void rename(const QString& articleId, const QString& newTitle);
    Q_INVOKABLE void toggle(const QString& articleId, const QString& title);

    //! Rows shaped for ExportServiceWrapper::exportRows: each entry a map
    //! with "id", "articleId" and "title".
    Q_INVOKABLE QVariantList toExportRows() const;

signals:
    void countChanged();

private:
    struct Bookmark {
        QString id;
        QString articleId;
        QString title;
    };

    QString storePath() const;
    void load();
    void save() const;

    QVector<Bookmark> m_bookmarks;
};
}

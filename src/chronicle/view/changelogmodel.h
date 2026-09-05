/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QObject>
#include <QVariantList>

#include "internal/changelogparser.h"

namespace au::chronicle {
/*!
 * The model behind the "What's new" dialog.
 *
 * It reads the release facing changelog that ships with the application,
 * groups it by released version and exposes each entry with the full commit
 * hash it describes and the address of that commit.
 */
class ChangelogModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList releases READ releases NOTIFY changed)
    Q_PROPERTY(QStringList versions READ versions NOTIFY loaded)
    Q_PROPERTY(bool available READ available NOTIFY loaded)

    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY changed)
    Q_PROPERTY(QString fromDate READ fromDate WRITE setFromDate NOTIFY changed)
    Q_PROPERTY(QString toDate READ toDate WRITE setToDate NOTIFY changed)

public:
    explicit ChangelogModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();
    //! Test seam: parse the given text instead of the shipped changelog.
    Q_INVOKABLE void loadText(const QString& markdown);

    QVariantList releases() const;
    QStringList versions() const;
    bool available() const { return !m_releases.isEmpty(); }

    QString searchText() const { return m_searchText; }
    void setSearchText(const QString& value);
    QString fromDate() const { return m_fromDate; }
    void setFromDate(const QString& value);
    QString toDate() const { return m_toDate; }
    void setToDate(const QString& value);

    Q_INVOKABLE void clearFilters();

    //! "markdown", "json" or "html". Returns true when the file was written.
    Q_INVOKABLE bool exportTo(const QString& destinationUrl, const QString& format) const;
    //! The exported text, so the format can be asserted without a file.
    Q_INVOKABLE QString exportText(const QString& format) const;

signals:
    void changed();
    void loaded();

private:
    bool passes(const ChangelogRelease& release, const ChangelogEntry& entry) const;

    QList<ChangelogRelease> m_releases;
    QString m_searchText;
    QString m_fromDate;
    QString m_toDate;
};
}

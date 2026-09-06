/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QDate>
#include <QObject>
#include <QVariantList>

#include "framework/global/modularity/ioc.h"
#include "framework/global/async/asyncable.h"
#include "actions/iactionsdispatcher.h"

#include "iversionhistoryservice.h"

namespace au::chronicle {
/*!
 * The model behind the History side panel.
 *
 * It exposes the revisions as plain lists so that the panel can filter them by
 * date range, by action and by search term without a proxy model in between.
 */
class VersionHistoryModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QVariantList revisions READ revisions NOTIFY revisionsChanged)
    Q_PROPERTY(QVariantList actionCounts READ actionCounts NOTIFY revisionsChanged)
    Q_PROPERTY(QVariantList familyCounts READ familyCounts NOTIFY revisionsChanged)
    Q_PROPERTY(QString storeKind READ storeKind NOTIFY revisionsChanged)

    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filterChanged)
    Q_PROPERTY(QString fromDate READ fromDate WRITE setFromDate NOTIFY filterChanged)
    Q_PROPERTY(QString toDate READ toDate WRITE setToDate NOTIFY filterChanged)
    Q_PROPERTY(QStringList selectedActions READ selectedActions WRITE setSelectedActions NOTIFY filterChanged)
    Q_PROPERTY(QStringList selectedFamilies READ selectedFamilies WRITE setSelectedFamilies NOTIFY filterChanged)

    Q_PROPERTY(int retentionCount READ retentionCount WRITE setRetentionCount NOTIFY retentionChanged)
    Q_PROPERTY(int retentionDays READ retentionDays WRITE setRetentionDays NOTIFY retentionChanged)

public:
    explicit VersionHistoryModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    //! The revision list after the date range, action and search filters.
    QVariantList revisions() const;
    //! One entry per recorded action, with its count, for the filter chips.
    QVariantList actionCounts() const;
    //! One entry per action family (Edit, Clip, Track, Effect, Generate,
    //! Label, Envelope, Project settings, Save, Restore), with its count.
    QVariantList familyCounts() const;
    QString storeKind() const;

    QString searchText() const { return m_searchText; }
    void setSearchText(const QString& value);
    QString fromDate() const { return m_fromDate; }
    void setFromDate(const QString& value);
    QString toDate() const { return m_toDate; }
    void setToDate(const QString& value);
    QStringList selectedActions() const { return m_selectedActions; }
    void setSelectedActions(const QStringList& value);
    QStringList selectedFamilies() const { return m_selectedFamilies; }
    void setSelectedFamilies(const QStringList& value);

    Q_INVOKABLE void clearFilters();

    //! The file list of a revision, with sizes, for the diff summary.
    Q_INVOKABLE QVariantList filesOf(const QString& revisionId) const;

    Q_INVOKABLE bool restore(const QString& revisionId);
    Q_INVOKABLE bool setLabel(const QString& revisionId, const QString& label);
    Q_INVOKABLE bool setStarred(const QString& revisionId, bool starred);
    Q_INVOKABLE bool setPinned(const QString& revisionId, bool pinned);
    Q_INVOKABLE bool exportRevision(const QString& revisionId, const QString& destinationUrl);
    //! Exports a revision into a private temporary folder and asks the
    //! application to open the exported project file, without touching the
    //! project that is currently open. Returns false when the export itself
    //! fails or the revision's project file cannot be found afterward.
    Q_INVOKABLE bool openAsNewProject(const QString& revisionId);
    Q_INVOKABLE int prune();

    //! One entry per calendar day that has a revision, oldest first, with
    //! its count, for a timeline rail grouped by day.
    Q_INVOKABLE QVariantList dayGroups() const;

    //! Repository size on disk (every project's history combined), backend
    //! name and revision count, for the storage panel. This model has no
    //! reference to the currently open project's own database, so the size
    //! of what is actually embedded in the current save file is not
    //! included here; ProjectHistoryWatcher is what writes and reads it.
    Q_INVOKABLE QVariantMap storageInfo() const;

    //! Compares two revisions by their recorded file lists: how many files
    //! were added, modified, deleted, or left unchanged between them, and
    //! the total size of each revision's files. This does not compare
    //! track/clip/sample-rate content, which this history does not capture
    //! per revision yet.
    Q_INVOKABLE QVariantMap compareRevisions(const QString& revisionIdA, const QString& revisionIdB) const;

    //! Records a snapshot on demand, used by the panel's refresh action.
    Q_INVOKABLE QString recordSnapshot(const QString& action, const QString& detail);

    int retentionCount() const;
    void setRetentionCount(int value);
    int retentionDays() const;
    void setRetentionDays(int value);

signals:
    void revisionsChanged();
    void filterChanged();
    void retentionChanged();

private:
    muse::GlobalInject<IVersionHistoryService> service;
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher { this };

    bool passes(const Revision& revision) const;

    QString m_searchText;
    QString m_fromDate;
    QString m_toDate;
    QStringList m_selectedActions;
    QStringList m_selectedFamilies;
};
}

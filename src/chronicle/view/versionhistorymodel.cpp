#include "shared/profilepaths.h"
/*
* Audacity: A Digital Audio Editor
*/
#include "versionhistorymodel.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>
#include <QUrl>
#include <QUuid>

#include "actions/actiontypes.h"

#include "log.h"

using namespace au::chronicle;

VersionHistoryModel::VersionHistoryModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void VersionHistoryModel::load()
{
    service()->revisionsChanged().onNotify(this, [this]() {
        emit revisionsChanged();
    });
    emit revisionsChanged();
    emit retentionChanged();
}

bool VersionHistoryModel::passes(const Revision& revision) const
{
    if (!m_selectedActions.isEmpty() && !m_selectedActions.contains(revision.action)) {
        return false;
    }

    if (!m_selectedFamilies.isEmpty() && !m_selectedFamilies.contains(actionFamily(revision.action))) {
        return false;
    }

    if (!m_fromDate.isEmpty()) {
        const QDate from = QDate::fromString(m_fromDate, Qt::ISODate);
        if (from.isValid() && revision.timestamp.toLocalTime().date() < from) {
            return false;
        }
    }

    if (!m_toDate.isEmpty()) {
        const QDate to = QDate::fromString(m_toDate, Qt::ISODate);
        if (to.isValid() && revision.timestamp.toLocalTime().date() > to) {
            return false;
        }
    }

    if (!m_searchText.isEmpty()) {
        const QString haystack = revision.label + QChar(u' ') + actionTitle(revision.action);
        // A search term is used as a regular expression when it is one and as
        // plain text otherwise, so a typed bracket never empties the list.
        const QRegularExpression expression(m_searchText, QRegularExpression::CaseInsensitiveOption);
        if (expression.isValid()) {
            if (!expression.match(haystack).hasMatch()) {
                return false;
            }
        } else if (!haystack.contains(m_searchText, Qt::CaseInsensitive)) {
            return false;
        }
    }

    return true;
}

QVariantList VersionHistoryModel::revisions() const
{
    QVariantList result;
    for (const Revision& revision : service()->revisions()) {
        if (!passes(revision)) {
            continue;
        }
        QVariantMap item;
        item.insert(QStringLiteral("revisionId"), revision.id);
        item.insert(QStringLiteral("shortId"), revision.id.left(10));
        item.insert(QStringLiteral("label"), revision.label);
        item.insert(QStringLiteral("action"), revision.action);
        item.insert(QStringLiteral("actionTitle"), actionTitle(revision.action));
        item.insert(QStringLiteral("actionFamily"), actionFamily(revision.action));
        item.insert(QStringLiteral("actionFamilyTitle"), actionFamilyTitle(actionFamily(revision.action)));
        item.insert(QStringLiteral("milestone"), isMilestoneAction(revision.action));
        item.insert(QStringLiteral("starred"), revision.starred);
        item.insert(QStringLiteral("pinned"), revision.pinned);
        item.insert(QStringLiteral("timestamp"),
                    revision.timestamp.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        item.insert(QStringLiteral("date"), revision.timestamp.toLocalTime().date().toString(Qt::ISODate));
        result.append(item);
    }
    return result;
}

QVariantList VersionHistoryModel::actionCounts() const
{
    QMap<QString, int> counts;
    for (const Revision& revision : service()->revisions()) {
        counts[revision.action] += 1;
    }

    QVariantList result;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QVariantMap item;
        item.insert(QStringLiteral("action"), it.key());
        item.insert(QStringLiteral("title"), actionTitle(it.key()));
        item.insert(QStringLiteral("count"), it.value());
        result.append(item);
    }
    return result;
}

QVariantList VersionHistoryModel::familyCounts() const
{
    QMap<QString, int> counts;
    for (const Revision& revision : service()->revisions()) {
        counts[actionFamily(revision.action)] += 1;
    }

    QVariantList result;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QVariantMap item;
        item.insert(QStringLiteral("family"), it.key());
        item.insert(QStringLiteral("title"), actionFamilyTitle(it.key()));
        item.insert(QStringLiteral("count"), it.value());
        result.append(item);
    }
    return result;
}

QString VersionHistoryModel::storeKind() const
{
    return service()->storeKind();
}

void VersionHistoryModel::setSearchText(const QString& value)
{
    if (m_searchText == value) {
        return;
    }
    m_searchText = value;
    emit filterChanged();
    emit revisionsChanged();
}

void VersionHistoryModel::setFromDate(const QString& value)
{
    if (m_fromDate == value) {
        return;
    }
    m_fromDate = value;
    emit filterChanged();
    emit revisionsChanged();
}

void VersionHistoryModel::setToDate(const QString& value)
{
    if (m_toDate == value) {
        return;
    }
    m_toDate = value;
    emit filterChanged();
    emit revisionsChanged();
}

void VersionHistoryModel::setSelectedActions(const QStringList& value)
{
    if (m_selectedActions == value) {
        return;
    }
    m_selectedActions = value;
    emit filterChanged();
    emit revisionsChanged();
}

void VersionHistoryModel::setSelectedFamilies(const QStringList& value)
{
    if (m_selectedFamilies == value) {
        return;
    }
    m_selectedFamilies = value;
    emit filterChanged();
    emit revisionsChanged();
}

void VersionHistoryModel::clearFilters()
{
    m_searchText.clear();
    m_fromDate.clear();
    m_toDate.clear();
    m_selectedActions.clear();
    m_selectedFamilies.clear();
    emit filterChanged();
    emit revisionsChanged();
}

QVariantList VersionHistoryModel::filesOf(const QString& revisionId) const
{
    QVariantList result;
    for (const RevisionFile& file : service()->files(revisionId)) {
        QVariantMap item;
        item.insert(QStringLiteral("path"), file.path);
        item.insert(QStringLiteral("size"), static_cast<double>(file.size));
        item.insert(QStringLiteral("status"), file.status);
        result.append(item);
    }
    return result;
}

bool VersionHistoryModel::restore(const QString& revisionId)
{
    return service()->restore(revisionId);
}

bool VersionHistoryModel::setLabel(const QString& revisionId, const QString& label)
{
    return service()->setLabel(revisionId, label);
}

bool VersionHistoryModel::setStarred(const QString& revisionId, bool starred)
{
    return service()->setStarred(revisionId, starred);
}

bool VersionHistoryModel::setPinned(const QString& revisionId, bool pinned)
{
    return service()->setPinned(revisionId, pinned);
}

bool VersionHistoryModel::exportRevision(const QString& revisionId, const QString& destinationUrl)
{
    QString path = destinationUrl;
    if (path.startsWith(QStringLiteral("file:"))) {
        path = QUrl(destinationUrl).toLocalFile();
    }
    return service()->exportRevision(revisionId, path);
}

int VersionHistoryModel::prune()
{
    return service()->prune();
}

bool VersionHistoryModel::openAsNewProject(const QString& revisionId)
{
    const QString tempDir = au::profile::Paths::temporaryPath() + QStringLiteral("/chronicle-open-")
                            + QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!service()->exportRevision(revisionId, tempDir)) {
        return false;
    }

    // exportRevision writes the project file under a "project" subfolder of
    // whatever destination it is given (see VersionHistoryService::stage),
    // under its original file name, which this does not otherwise know.
    const QDir projectDir(tempDir + QStringLiteral("/project"));
    const QStringList files = projectDir.entryList(QDir::Files);
    if (files.isEmpty()) {
        return false;
    }

    const QUrl url = QUrl::fromLocalFile(projectDir.filePath(files.first()));
    dispatcher()->dispatch("file-open", muse::actions::ActionData::make_arg1<QUrl>(url));
    return true;
}

QVariantList VersionHistoryModel::dayGroups() const
{
    QMap<QString, int> counts;
    for (const Revision& revision : service()->revisions()) {
        counts[revision.timestamp.toLocalTime().date().toString(Qt::ISODate)] += 1;
    }

    QVariantList result;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QVariantMap item;
        item.insert(QStringLiteral("date"), it.key());
        item.insert(QStringLiteral("count"), it.value());
        result.append(item);
    }
    return result;
}

QVariantMap VersionHistoryModel::storageInfo() const
{
    QVariantMap result;
    result.insert(QStringLiteral("backend"), service()->storeKind());

    // Repository size on disk is measured directly rather than tracked
    // incrementally, so it always reflects what is actually there, including
    // anything left behind by a failure this model never saw.
    qint64 totalBytes = 0;
    const QString root = service()->historyRootPath();
    QDirIterator iterator(root, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        totalBytes += QFileInfo(iterator.filePath()).size();
    }
    result.insert(QStringLiteral("repositoryBytes"), static_cast<double>(totalBytes));

    result.insert(QStringLiteral("revisionCount"), service()->revisions().size());
    return result;
}

QVariantMap VersionHistoryModel::compareRevisions(const QString& revisionIdA, const QString& revisionIdB) const
{
    const RevisionFileComparison comparison = compareRevisionFiles(
        service()->files(revisionIdA), service()->files(revisionIdB));

    QVariantMap result;
    result.insert(QStringLiteral("revisionIdA"), revisionIdA);
    result.insert(QStringLiteral("revisionIdB"), revisionIdB);
    result.insert(QStringLiteral("filesAdded"), comparison.filesAdded);
    result.insert(QStringLiteral("filesModified"), comparison.filesModified);
    result.insert(QStringLiteral("filesDeleted"), comparison.filesDeleted);
    result.insert(QStringLiteral("filesUnchanged"), comparison.filesUnchanged);
    result.insert(QStringLiteral("totalBytesA"), static_cast<double>(comparison.totalBytesA));
    result.insert(QStringLiteral("totalBytesB"), static_cast<double>(comparison.totalBytesB));
    return result;
}

QString VersionHistoryModel::recordSnapshot(const QString& action, const QString& detail)
{
    return service()->recordSnapshot(action, detail);
}

int VersionHistoryModel::retentionCount() const
{
    return service()->retentionCount();
}

void VersionHistoryModel::setRetentionCount(int value)
{
    service()->setRetentionCount(value);
    emit retentionChanged();
}

int VersionHistoryModel::retentionDays() const
{
    return service()->retentionDays();
}

void VersionHistoryModel::setRetentionDays(int value)
{
    service()->setRetentionDays(value);
    emit retentionChanged();
}

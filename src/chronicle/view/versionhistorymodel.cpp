/*
* Audacity: A Digital Audio Editor
*/
#include "versionhistorymodel.h"

#include <QMap>
#include <QRegularExpression>
#include <QUrl>

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

void VersionHistoryModel::clearFilters()
{
    m_searchText.clear();
    m_fromDate.clear();
    m_toDate.clear();
    m_selectedActions.clear();
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

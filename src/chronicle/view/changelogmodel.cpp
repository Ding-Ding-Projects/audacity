/*
* Audacity: A Digital Audio Editor
*/
#include "changelogmodel.h"

#include <QFile>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

#include "log.h"

using namespace au::chronicle;

static const QString CHANGELOG_RESOURCE = QStringLiteral(":/chronicle/CHANGELOG.md");

ChangelogModel::ChangelogModel(QObject* parent)
    : QObject(parent)
{
}

void ChangelogModel::load()
{
    QFile file(CHANGELOG_RESOURCE);
    if (!file.open(QIODevice::ReadOnly)) {
        LOGW() << "the changelog is not present in this build";
        m_releases.clear();
        emit loaded();
        emit changed();
        return;
    }

    loadText(QString::fromUtf8(file.readAll()));
}

void ChangelogModel::loadText(const QString& markdown)
{
    m_releases = ChangelogParser::parse(markdown);
    emit loaded();
    emit changed();
}

bool ChangelogModel::passes(const ChangelogRelease& release, const ChangelogEntry& entry) const
{
    if (!m_fromDate.isEmpty()) {
        const QDate from = QDate::fromString(m_fromDate, Qt::ISODate);
        if (from.isValid() && release.date.isValid() && release.date < from) {
            return false;
        }
    }
    if (!m_toDate.isEmpty()) {
        const QDate to = QDate::fromString(m_toDate, Qt::ISODate);
        if (to.isValid() && release.date.isValid() && release.date > to) {
            return false;
        }
    }

    if (m_searchText.isEmpty()) {
        return true;
    }

    const QString haystack = entry.text + QChar(u' ') + entry.group + QChar(u' ') + entry.commitSha;
    // The term is used as a regular expression when it is one and as plain
    // text otherwise, so a typed bracket never empties the list.
    const QRegularExpression expression(m_searchText, QRegularExpression::CaseInsensitiveOption);
    if (expression.isValid()) {
        return expression.match(haystack).hasMatch();
    }
    return haystack.contains(m_searchText, Qt::CaseInsensitive);
}

QVariantList ChangelogModel::releases() const
{
    QVariantList result;
    for (const ChangelogRelease& release : m_releases) {
        QVariantList entries;
        for (const ChangelogEntry& entry : release.entries) {
            if (!passes(release, entry)) {
                continue;
            }
            QVariantMap item;
            item.insert(QStringLiteral("text"), entry.text);
            item.insert(QStringLiteral("group"), entry.group);
            item.insert(QStringLiteral("commitSha"), entry.commitSha);
            item.insert(QStringLiteral("shortSha"), entry.commitSha.left(10));
            item.insert(QStringLiteral("commitUrl"), ChangelogParser::commitUrl(entry.commitSha));
            entries.append(item);
        }

        if (entries.isEmpty()) {
            continue;
        }

        QVariantMap item;
        item.insert(QStringLiteral("version"), release.version);
        item.insert(QStringLiteral("date"), release.date.isValid() ? release.date.toString(Qt::ISODate) : QString());
        item.insert(QStringLiteral("entries"), entries);
        result.append(item);
    }
    return result;
}

QStringList ChangelogModel::versions() const
{
    QStringList result;
    for (const ChangelogRelease& release : m_releases) {
        result.append(release.version);
    }
    return result;
}

void ChangelogModel::setSearchText(const QString& value)
{
    if (m_searchText == value) {
        return;
    }
    m_searchText = value;
    emit changed();
}

void ChangelogModel::setFromDate(const QString& value)
{
    if (m_fromDate == value) {
        return;
    }
    m_fromDate = value;
    emit changed();
}

void ChangelogModel::setToDate(const QString& value)
{
    if (m_toDate == value) {
        return;
    }
    m_toDate = value;
    emit changed();
}

void ChangelogModel::clearFilters()
{
    m_searchText.clear();
    m_fromDate.clear();
    m_toDate.clear();
    emit changed();
}

QString ChangelogModel::exportText(const QString& format) const
{
    const QVariantList data = releases();

    if (format == QStringLiteral("json")) {
        QJsonArray array;
        for (const QVariant& releaseValue : data) {
            const QVariantMap release = releaseValue.toMap();
            QJsonArray entries;
            for (const QVariant& entryValue : release.value(QStringLiteral("entries")).toList()) {
                const QVariantMap entry = entryValue.toMap();
                QJsonObject object;
                object.insert(QStringLiteral("text"), entry.value(QStringLiteral("text")).toString());
                object.insert(QStringLiteral("group"), entry.value(QStringLiteral("group")).toString());
                object.insert(QStringLiteral("commit"), entry.value(QStringLiteral("commitSha")).toString());
                object.insert(QStringLiteral("url"), entry.value(QStringLiteral("commitUrl")).toString());
                entries.append(object);
            }
            QJsonObject object;
            object.insert(QStringLiteral("version"), release.value(QStringLiteral("version")).toString());
            object.insert(QStringLiteral("date"), release.value(QStringLiteral("date")).toString());
            object.insert(QStringLiteral("entries"), entries);
            array.append(object);
        }
        return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }

    const bool html = format == QStringLiteral("html");
    QString text;
    if (html) {
        text += QStringLiteral("<!DOCTYPE html>\n<html lang=\"en\">\n<head><meta charset=\"utf-8\">"
                               "<title>Material Audacity changelog</title></head>\n<body>\n");
    } else {
        text += QStringLiteral("# Material Audacity changelog\n\n");
    }

    for (const QVariant& releaseValue : data) {
        const QVariantMap release = releaseValue.toMap();
        const QString version = release.value(QStringLiteral("version")).toString();
        const QString date = release.value(QStringLiteral("date")).toString();
        const QString heading = date.isEmpty() ? version : version + QStringLiteral(" - ") + date;

        if (html) {
            text += QStringLiteral("<h2>%1</h2>\n<ul>\n").arg(heading.toHtmlEscaped());
        } else {
            text += QStringLiteral("## %1\n\n").arg(heading);
        }

        for (const QVariant& entryValue : release.value(QStringLiteral("entries")).toList()) {
            const QVariantMap entry = entryValue.toMap();
            const QString body = entry.value(QStringLiteral("text")).toString();
            const QString sha = entry.value(QStringLiteral("commitSha")).toString();
            const QString url = entry.value(QStringLiteral("commitUrl")).toString();
            if (html) {
                text += QStringLiteral("<li>%1 (<a href=\"%2\"><code>%3</code></a>)</li>\n")
                        .arg(body.toHtmlEscaped(), url.toHtmlEscaped(), sha);
            } else {
                text += QStringLiteral("- %1 ([`%2`](%3))\n").arg(body, sha, url);
            }
        }

        text += html ? QStringLiteral("</ul>\n") : QStringLiteral("\n");
    }

    if (html) {
        text += QStringLiteral("</body>\n</html>\n");
    }

    return text;
}

bool ChangelogModel::exportTo(const QString& destinationUrl, const QString& format) const
{
    QString path = destinationUrl;
    if (path.startsWith(QStringLiteral("file:"))) {
        path = QUrl(destinationUrl).toLocalFile();
    }
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOGE() << "could not write the changelog export to " << path.toStdString();
        return false;
    }
    file.write(exportText(format).toUtf8());
    return true;
}

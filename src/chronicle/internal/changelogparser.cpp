/*
* Audacity: A Digital Audio Editor
*/
#include "changelogparser.h"

#include <QRegularExpression>

using namespace au::chronicle;

static const QString COMMIT_BASE_URL = QStringLiteral("https://github.com/Ding-Ding-Projects/audacity/commit/");

QString ChangelogParser::commitUrl(const QString& sha)
{
    return COMMIT_BASE_URL + sha;
}

QStringList ChangelogParser::commitShas(const QString& markdown)
{
    static const QRegularExpression shaExpression(QStringLiteral("\\b[0-9a-f]{40}\\b"));

    QStringList result;
    QRegularExpressionMatchIterator iterator = shaExpression.globalMatch(markdown);
    while (iterator.hasNext()) {
        const QString sha = iterator.next().captured(0);
        if (!result.contains(sha)) {
            result.append(sha);
        }
    }
    return result;
}

QList<ChangelogRelease> ChangelogParser::parse(const QString& markdown)
{
    static const QRegularExpression releaseHeading(
        QStringLiteral("^##\\s+(?:\\[)?([^\\]\\s]+)(?:\\])?(?:\\s+-\\s+(\\d{4}-\\d{2}-\\d{2}))?\\s*$"));
    static const QRegularExpression groupHeading(QStringLiteral("^###\\s+(.+?)\\s*$"));
    static const QRegularExpression listItem(QStringLiteral("^[-*]\\s+(.*)$"));
    static const QRegularExpression sha(QStringLiteral("\\b[0-9a-f]{40}\\b"));

    QList<ChangelogRelease> releases;
    QString group;
    bool inRelease = false;
    QString pendingText;

    const auto flush = [&]() {
        if (pendingText.isEmpty() || releases.isEmpty()) {
            pendingText.clear();
            return;
        }

        ChangelogEntry entry;
        entry.group = group;
        entry.commitSha = sha.match(pendingText).captured(0);

        // Strip the trailing commit link, which is presented separately.
        QString text = pendingText;
        text.remove(QRegularExpression(QStringLiteral("\\s*\\(\\[[^\\]]*\\]\\([^\\)]*\\)\\)\\s*$")));
        entry.text = text.trimmed();

        if (!entry.text.isEmpty()) {
            releases.last().entries.append(entry);
        }
        pendingText.clear();
    };

    const QStringList lines = markdown.split(QChar(u'\n'));
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();

        const QRegularExpressionMatch release = releaseHeading.match(line);
        if (release.hasMatch()) {
            flush();
            ChangelogRelease item;
            item.version = release.captured(1);
            if (!release.captured(2).isEmpty()) {
                item.date = QDate::fromString(release.captured(2), Qt::ISODate);
            }
            releases.append(item);
            inRelease = true;
            group.clear();
            continue;
        }

        if (!inRelease) {
            continue;
        }

        const QRegularExpressionMatch groupMatch = groupHeading.match(line);
        if (groupMatch.hasMatch()) {
            flush();
            group = groupMatch.captured(1);
            continue;
        }

        const QRegularExpressionMatch itemMatch = listItem.match(line);
        if (itemMatch.hasMatch()) {
            flush();
            pendingText = itemMatch.captured(1);
            continue;
        }

        if (line.isEmpty()) {
            flush();
            continue;
        }

        // A continuation line of the current list item.
        if (!pendingText.isEmpty()) {
            pendingText += QChar(u' ') + line;
        }
    }

    flush();
    return releases;
}

/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QDate>
#include <QList>
#include <QString>

namespace au::chronicle {
//! One line of a changelog section.
struct ChangelogEntry {
    QString text;
    //! The full forty character commit hash. Every entry carries one, so the
    //! reader can always reach the exact commit the line describes.
    QString commitSha;
    QString group;
};

//! One released version.
struct ChangelogRelease {
    QString version;
    QDate date;
    QList<ChangelogEntry> entries;
};

/*!
 * Reads the release facing changelog.
 *
 * The format is the one CHANGELOG.md at the repository root uses: a level two
 * heading per release, "## <version> - <YYYY-MM-DD>", level three headings for
 * the groups, and list items that end with a link whose target is the commit
 * the line describes.
 */
class ChangelogParser
{
public:
    static QList<ChangelogRelease> parse(const QString& markdown);

    //! Every full commit hash mentioned anywhere in the text. Used by the
    //! build time validation.
    static QStringList commitShas(const QString& markdown);

    static QString commitUrl(const QString& sha);
};
}

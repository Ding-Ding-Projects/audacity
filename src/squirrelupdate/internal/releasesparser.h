/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QStringList>

#include "squirrelupdatetypes.h"

namespace au::squirrelupdate {
//! Reads a Squirrel.Windows RELEASES feed and compares package versions.
//!
//! Everything here is pure: no network, no file system, no settings. That is
//! what makes it directly testable, which the tests under tests/ rely on.
class ReleasesParser
{
public:
    //! Parses a whole RELEASES file. Malformed lines are skipped rather than
    //! failing the whole feed, because one bad line must not hide the good
    //! entries. When errors is given, every skipped line is described there.
    static ReleaseEntryList parse(const QString& text, QStringList* errors = nullptr);

    //! Parses one line. Returns an invalid entry and fills error when the line
    //! is not a well formed release entry. Blank lines and lines starting with
    //! '#' are comments and produce an invalid entry with no error.
    static ReleaseEntry parseLine(const QString& line, QString* error = nullptr);

    //! "Audacity-4.0.0-m3001-full.nupkg" becomes "4.0.0-m3001".
    static QString versionFromFileName(const QString& fileName);

    //! Compares two SemVer 1 versions. Returns a negative number when a is
    //! older than b, zero when they are equal and a positive number when a is
    //! newer. A version without a pre-release label is newer than the same
    //! numbers with one, as SemVer requires.
    static int compareVersions(const QString& a, const QString& b);

    //! The newest entry, preferring a full package when a full and a delta
    //! describe the same version. Returns an invalid entry for an empty list.
    static ReleaseEntry newest(const ReleaseEntryList& entries);

    //! The newest entry that is strictly newer than installedVersion.
    static ReleaseEntry newestAfter(const ReleaseEntryList& entries, const QString& installedVersion);
};
}

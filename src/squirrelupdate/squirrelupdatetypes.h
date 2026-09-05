/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QList>
#include <QString>

namespace au::squirrelupdate {
//! One line of a Squirrel.Windows RELEASES feed.
//!
//! A line is "<sha1> <filename> <size>". The hash is SHA1 because that is
//! what Squirrel writes and what its updater checks. It is an integrity
//! check only: nothing in this project is code signed, so no hash here
//! establishes authenticity.
struct ReleaseEntry
{
    QString sha1;
    QString fileName;
    qint64 size = 0;

    //! Version parsed out of the file name, for example "4.0.0-m3001".
    QString version;

    //! Delta packages carry only the difference against the previous release.
    bool isDelta = false;

    bool isValid() const
    {
        return !sha1.isEmpty() && !fileName.isEmpty() && size > 0 && !version.isEmpty();
    }

    bool operator==(const ReleaseEntry& other) const
    {
        return sha1 == other.sha1 && fileName == other.fileName
               && size == other.size && version == other.version
               && isDelta == other.isDelta;
    }
};

using ReleaseEntryList = QList<ReleaseEntry>;

//! The result of a completed check, as the banner needs it.
struct AvailableUpdate
{
    bool available = false;
    QString version;
    QString fileName;
    bool isDelta = false;

    //! Absolute path of the downloaded and hash verified package.
    QString packagePath;
};
}

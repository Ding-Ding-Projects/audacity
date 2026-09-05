/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QString>

#include "squirrelupdatetypes.h"

namespace au::squirrelupdate {
//! Checks a downloaded package against the feed entry that described it.
//!
//! This is an integrity check, not an authenticity check. Nothing in this
//! project is code signed, and a matching hash proves only that the bytes on
//! disk are the bytes the feed listed.
class PackageVerifier
{
public:
    //! Uppercase hexadecimal SHA1 of the file, empty when it cannot be read.
    static QString sha1OfFile(const QString& filePath);

    //! Uppercase hexadecimal SHA256 of the file, empty when it cannot be read.
    //! Published in the release notes so a user can check a download by hand.
    static QString sha256OfFile(const QString& filePath);

    //! True when the file exists, its size matches the entry and its SHA1
    //! matches the entry. On failure, error describes what did not match.
    static bool verify(const QString& filePath, const ReleaseEntry& entry, QString* error = nullptr);
};
}

/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QString>

namespace au::squirrelupdate {
//! Reads the Squirrel.Windows install layout from a path.
//!
//! A Squirrel installation looks like this:
//!
//!   %LocalAppData%\Audacity\Update.exe
//!   %LocalAppData%\Audacity\app-4.0.0-m3001\MaterialAudacity.exe
//!   %LocalAppData%\Audacity\app-4.0.0-m3001\bin\Audacity4.exe
//!
//! The running executable therefore sits below a directory named
//! app-<version>, and the updater sits one level above that directory. The
//! functions here are pure string work so the tests can exercise them on any
//! platform.
class SquirrelInstallLayout
{
public:
    //! The version in the nearest app-<version> ancestor of executableDir, or
    //! an empty string when the path is not inside a Squirrel installation.
    static QString versionFromPath(const QString& executableDir);

    //! The directory holding Update.exe, that is the parent of the nearest
    //! app-<version> ancestor. Empty when there is no such ancestor.
    static QString rootDirFromPath(const QString& executableDir);

    //! The nearest app-<version> ancestor itself. Empty when there is none.
    static QString appDirFromPath(const QString& executableDir);
};
}

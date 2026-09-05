/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "framework/global/modularity/imoduleinterface.h"
#include "framework/global/async/notification.h"
#include "framework/global/types/ret.h"

#include "squirrelupdatetypes.h"

namespace au::squirrelupdate {
//! Checks a Squirrel.Windows RELEASES feed in the background and, when a newer
//! package is available and its hash verifies, offers a restart.
//!
//! The service is built on every platform and does nothing on any platform
//! other than Windows, where Squirrel.Windows is the only installer.
class ISquirrelUpdateService : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(au::squirrelupdate::ISquirrelUpdateService)

public:
    virtual ~ISquirrelUpdateService() = default;

    //! True only on Windows, and only when the running copy sits inside a
    //! Squirrel installation.
    virtual bool isSupported() const = 0;

    //! The version of the running installation, read from the app-<version>
    //! directory name. Empty when the copy is not a Squirrel installation.
    virtual QString installedVersion() const = 0;

    //! Starts one background check. Does nothing when unsupported, disabled or
    //! already checking.
    virtual void checkForUpdate() = 0;

    virtual bool isChecking() const = 0;

    //! The verified update the banner should offer, if any.
    virtual AvailableUpdate availableUpdate() const = 0;
    virtual muse::async::Notification availableUpdateChanged() const = 0;

    //! Hides the offer until the next check finds something newer again.
    virtual void dismiss() = 0;

    //! Applies the update with Update.exe and restarts. Returns a failing Ret
    //! when there is nothing to apply or the updater could not be run.
    virtual muse::Ret restartToUpdate() = 0;

    //! The last error, for the log and for the banner's supporting text.
    virtual QString lastError() const = 0;
};
}

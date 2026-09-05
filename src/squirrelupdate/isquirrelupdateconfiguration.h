/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QString>

#include "framework/global/modularity/imoduleinterface.h"
#include "framework/global/async/notification.h"

namespace au::squirrelupdate {
//! The three settings the Squirrel feed checker reads, persisted through the
//! muse settings store and edited in the Update preferences page.
class ISquirrelUpdateConfiguration : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(au::squirrelupdate::ISquirrelUpdateConfiguration)

public:
    virtual ~ISquirrelUpdateConfiguration() = default;

    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool value) = 0;

    virtual QString feedUrl() const = 0;
    virtual void setFeedUrl(const QString& url) = 0;
    virtual QString defaultFeedUrl() const = 0;

    //! How long to wait between background checks, in hours.
    virtual int checkIntervalHours() const = 0;
    virtual void setCheckIntervalHours(int hours) = 0;

    virtual muse::async::Notification changed() const = 0;
};
}

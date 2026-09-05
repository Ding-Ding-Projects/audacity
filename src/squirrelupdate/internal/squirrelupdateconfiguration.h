/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "framework/global/async/asyncable.h"

#include "isquirrelupdateconfiguration.h"

namespace au::squirrelupdate {
class SquirrelUpdateConfiguration : public ISquirrelUpdateConfiguration, public muse::async::Asyncable
{
public:
    void init();

    bool isEnabled() const override;
    void setEnabled(bool value) override;

    QString feedUrl() const override;
    void setFeedUrl(const QString& url) override;
    QString defaultFeedUrl() const override;

    int checkIntervalHours() const override;
    void setCheckIntervalHours(int hours) override;

    muse::async::Notification changed() const override;

private:
    muse::async::Notification m_changed;
};
}

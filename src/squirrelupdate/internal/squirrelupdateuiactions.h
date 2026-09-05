/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "framework/ui/iuiactionsmodule.h"
#include "framework/global/async/asyncable.h"

namespace au::squirrelupdate {
//! Registers the single action the Squirrel update checker exposes to menus
//! and to shortcuts: "check-squirrel-update", shown as Help > Check for
//! updates. The action is always enabled; whether anything actually happens
//! depends on the platform and the installation, and the preferences page and
//! the ready to restart banner say so honestly.
class SquirrelUpdateUiActions : public muse::ui::IUiActionsModule, public muse::async::Asyncable
{
public:
    const muse::ui::UiActionList& actionsList() const override;

    bool actionEnabled(const muse::ui::UiAction& action) const override;
    muse::async::Channel<muse::actions::ActionCodeList> actionEnabledChanged() const override;

    bool actionChecked(const muse::ui::UiAction& action) const override;
    muse::async::Channel<muse::actions::ActionCodeList> actionCheckedChanged() const override;

private:
    muse::async::Channel<muse::actions::ActionCodeList> m_actionEnabledChanged;
    muse::async::Channel<muse::actions::ActionCodeList> m_actionCheckedChanged;
};
}

/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include "async/asyncable.h"
#include "ui/iuiactionsmodule.h"

namespace au::companion {
//! Registers the actions the companion surfaces own. Today that is the single
//! command palette action, which the global Ctrl+Shift+F shortcut triggers.
class CompanionUiActions : public muse::ui::IUiActionsModule, public muse::async::Asyncable
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

/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "framework/global/modularity/ioc.h"
#include "framework/global/async/asyncable.h"
#include "framework/ui/iuiactionsmodule.h"

namespace au::chronicle {
//! The actions the chronicle module adds to the application.
class ChronicleUiActions : public muse::ui::IUiActionsModule, public muse::Contextable, public muse::async::Asyncable
{
public:
    explicit ChronicleUiActions(const muse::modularity::ContextPtr& ctx)
        : muse::Contextable(ctx) {}

    const muse::ui::UiActionList& actionsList() const override;

    bool actionEnabled(const muse::ui::UiAction& act) const override;
    muse::async::Channel<muse::actions::ActionCodeList> actionEnabledChanged() const override;

    bool actionChecked(const muse::ui::UiAction& act) const override;
    muse::async::Channel<muse::actions::ActionCodeList> actionCheckedChanged() const override;

private:
    muse::async::Channel<muse::actions::ActionCodeList> m_actionEnabledChanged;
    muse::async::Channel<muse::actions::ActionCodeList> m_actionCheckedChanged;
};
}

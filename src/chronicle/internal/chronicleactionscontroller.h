/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "framework/global/modularity/ioc.h"
#include "framework/global/async/asyncable.h"
#include "framework/actions/iactionsdispatcher.h"
#include "framework/actions/actionable.h"
#include "framework/interactive/iinteractive.h"

namespace au::chronicle {
//! Handles the actions the chronicle module contributes: opening the
//! "What's new" dialog from the Help menu.
class ChronicleActionsController : public muse::actions::Actionable, public muse::async::Asyncable, public muse::Contextable
{
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher { this };
    muse::ContextInject<muse::IInteractive> interactive { this };

public:
    explicit ChronicleActionsController(const muse::modularity::ContextPtr& ctx)
        : muse::Contextable(ctx) {}

    void init();

private:
    void openChangelog();
};
}

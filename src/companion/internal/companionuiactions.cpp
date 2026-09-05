/*
* Audacity: A Digital Audio Editor
*/

#include "companionuiactions.h"

#include "context/shortcutcontext.h"
#include "context/uicontext.h"
#include "types/translatablestring.h"

using namespace au::companion;
using namespace muse;
using namespace muse::ui;

static const UiActionList COMPANION_ACTIONS = {
    UiAction("companion-command-palette",
             au::context::UiCtxAny,
             au::context::CTX_ANY,
             //: Action title: shown as a menu item or a button label; keep it short
             TranslatableString("action", "Command palette"),
             //: Action description: shown as a tooltip; can be a full sentence
             TranslatableString("action_description",
                                "Search every command, preference, panel and documentation article")
             )
};

const UiActionList& CompanionUiActions::actionsList() const
{
    return COMPANION_ACTIONS;
}

bool CompanionUiActions::actionEnabled(const UiAction&) const
{
    return true;
}

muse::async::Channel<muse::actions::ActionCodeList> CompanionUiActions::actionEnabledChanged() const
{
    return m_actionEnabledChanged;
}

bool CompanionUiActions::actionChecked(const UiAction&) const
{
    return false;
}

muse::async::Channel<muse::actions::ActionCodeList> CompanionUiActions::actionCheckedChanged() const
{
    return m_actionCheckedChanged;
}

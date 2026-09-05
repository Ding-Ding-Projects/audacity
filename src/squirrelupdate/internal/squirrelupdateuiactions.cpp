/*
* Audacity: A Digital Audio Editor
*/
#include "squirrelupdateuiactions.h"

#include "context/shortcutcontext.h"
#include "context/uicontext.h"
#include "types/translatablestring.h"

using namespace au::squirrelupdate;
using namespace muse;
using namespace muse::ui;

static const UiActionList SQUIRREL_UPDATE_ACTIONS = {
    UiAction("check-squirrel-update",
             au::context::UiCtxAny,
             au::context::CTX_ANY,
             //: Action title: shown as a menu item; keep it short
             TranslatableString("action", "Check for updates"),
             //: Action description: shown as a tooltip; can be a full sentence
             TranslatableString("action_description",
                                "Checks the unsigned Squirrel.Windows release feed for a newer version")
             )
};

const UiActionList& SquirrelUpdateUiActions::actionsList() const
{
    return SQUIRREL_UPDATE_ACTIONS;
}

bool SquirrelUpdateUiActions::actionEnabled(const UiAction&) const
{
    return true;
}

muse::async::Channel<muse::actions::ActionCodeList> SquirrelUpdateUiActions::actionEnabledChanged() const
{
    return m_actionEnabledChanged;
}

bool SquirrelUpdateUiActions::actionChecked(const UiAction&) const
{
    return false;
}

muse::async::Channel<muse::actions::ActionCodeList> SquirrelUpdateUiActions::actionCheckedChanged() const
{
    return m_actionCheckedChanged;
}

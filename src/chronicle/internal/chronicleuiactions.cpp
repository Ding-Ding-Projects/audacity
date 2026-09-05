/*
* Audacity: A Digital Audio Editor
*/
#include "chronicleuiactions.h"

#include "context/uicontext.h"
#include "context/shortcutcontext.h"
#include "types/translatablestring.h"

using namespace au::chronicle;
using namespace muse;
using namespace muse::ui;

static const UiActionList CHRONICLE_ACTIONS = {
    UiAction("whats-new",
             au::context::UiCtxAny,
             au::context::CTX_ANY,
             //: Action title: shown as a menu item or a button label; keep it short
             TranslatableString("action", "What's new…"),
             //: Action description: shown as a tooltip; can be a full sentence
             TranslatableString("action_description", "Show the changes in this and earlier releases")
             ),
};

const UiActionList& ChronicleUiActions::actionsList() const
{
    return CHRONICLE_ACTIONS;
}

bool ChronicleUiActions::actionEnabled(const UiAction&) const
{
    return true;
}

muse::async::Channel<muse::actions::ActionCodeList> ChronicleUiActions::actionEnabledChanged() const
{
    return m_actionEnabledChanged;
}

bool ChronicleUiActions::actionChecked(const UiAction&) const
{
    return false;
}

muse::async::Channel<muse::actions::ActionCodeList> ChronicleUiActions::actionCheckedChanged() const
{
    return m_actionCheckedChanged;
}

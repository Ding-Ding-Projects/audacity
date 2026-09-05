/*
* Audacity: A Digital Audio Editor
*/

#include "companionmodule.h"

#include <QQmlEngine>
#include <QtQml>

#include "ui/iuiactionsregister.h"

#include "internal/companionuiactions.h"
#include "palette/commandpalettemodel.h"
#include "regex/regexengine.h"

using namespace au::companion;
using namespace muse::modularity;

static const std::string mname("companion");

static void companion_init_qrc()
{
    Q_INIT_RESOURCE(au_companion);
}

std::string CompanionModule::moduleName() const
{
    return mname;
}

void CompanionModule::registerResources()
{
    companion_init_qrc();
}

void CompanionModule::registerUiTypes()
{
    qmlRegisterType<CommandPaletteModel>("Audacity.Companion", 1, 0, "CommandPaletteModel");
    qmlRegisterType<RegexEngine>("Audacity.Companion", 1, 0, "RegexEngine");
}

IContextSetup* CompanionModule::newContext(const ContextPtr& ctx) const
{
    return new CompanionContext(ctx);
}

void CompanionContext::registerExports()
{
    m_uiActions = std::make_shared<CompanionUiActions>();
}

void CompanionContext::onInit(const muse::IApplication::RunMode& mode)
{
    if (mode != muse::IApplication::RunMode::GuiApp) {
        return;
    }

    auto actionsRegister = ioc()->resolve<muse::ui::IUiActionsRegister>(mname);
    if (actionsRegister) {
        actionsRegister->reg(m_uiActions);
    }
}

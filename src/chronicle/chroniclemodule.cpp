/*
* Audacity: A Digital Audio Editor
*/
#include "chroniclemodule.h"

#include <QtQml>

#include "framework/ui/iuiactionsregister.h"
#include "framework/interactive/iinteractiveuriregister.h"

#include "internal/versionhistoryservice.h"
#include "internal/chronicleactionscontroller.h"
#include "internal/chronicleuiactions.h"
#include "internal/projecthistorywatcher.h"

#include "view/versionhistorymodel.h"
#include "view/changelogmodel.h"
#include "view/tabstripmodel.h"
#include "view/chronicledebughooks.h"

#include "log.h"

using namespace au::chronicle;
using namespace muse::modularity;
using namespace muse::ui;
using namespace muse::interactive;

static const std::string mname("chronicle");

static void chronicle_init_qrc()
{
    Q_INIT_RESOURCE(au_chronicle);
}

std::string ChronicleModule::moduleName() const
{
    return "au::chronicle";
}

void ChronicleModule::registerResources()
{
    chronicle_init_qrc();
}

void ChronicleModule::registerExports()
{
    m_versionHistoryService = std::make_shared<VersionHistoryService>();
    globalIoc()->registerExport<IVersionHistoryService>(mname, m_versionHistoryService);
}

void ChronicleModule::resolveImports()
{
    auto ir = globalIoc()->resolve<IInteractiveUriRegister>(mname);
    if (ir) {
        ir->registerQmlUri(muse::Uri("audacity://chronicle/changelog"),
                           "Audacity/Chronicle/ChangelogDialog.qml");
    }
}

void ChronicleModule::registerUiTypes()
{
    qmlRegisterType<VersionHistoryModel>("Audacity.Chronicle", 1, 0, "VersionHistoryModel");
    qmlRegisterType<ChangelogModel>("Audacity.Chronicle", 1, 0, "ChangelogModel");
    qmlRegisterType<TabStripModel>("Audacity.Chronicle", 1, 0, "TabStripModel");
    qmlRegisterSingletonType<ChronicleDebugHooks>("Audacity.Chronicle", 1, 0, "ChronicleDebugHooks",
                                                  [](QQmlEngine*, QJSEngine*) -> QObject* {
        return new ChronicleDebugHooks();
    });
}

void ChronicleModule::onInit(const muse::IApplication::RunMode& mode)
{
    if (mode == muse::IApplication::RunMode::AudioPluginRegistration) {
        return;
    }

    m_versionHistoryService->init();
    // Applying the retention settings at start up keeps the history from
    // growing without bound in a long lived installation.
    m_versionHistoryService->prune();
}

muse::modularity::IContextSetup* ChronicleModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new ChronicleContext(ctx);
}

// =====================================================
// ChronicleContext
// =====================================================

void ChronicleContext::registerExports()
{
    m_actionsController = std::make_shared<ChronicleActionsController>(iocContext());
    m_uiActions = std::make_shared<ChronicleUiActions>(iocContext());
    m_projectWatcher = std::make_shared<ProjectHistoryWatcher>(iocContext());
}

void ChronicleContext::onInit(const muse::IApplication::RunMode& mode)
{
    if (mode == muse::IApplication::RunMode::AudioPluginRegistration) {
        return;
    }

    m_actionsController->init();
    m_projectWatcher->init();

    auto ar = ioc()->resolve<IUiActionsRegister>(mname);
    if (ar) {
        ar->reg(m_uiActions);
    }
}

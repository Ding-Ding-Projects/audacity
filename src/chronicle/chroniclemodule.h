/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <memory>

#include "modularity/imodulesetup.h"

namespace au::chronicle {
class VersionHistoryService;
class ChronicleActionsController;
class ChronicleUiActions;
class ProjectHistoryWatcher;

/*!
 * The chronicle module: the local version history, the changelog viewer and
 * the browser style tab strip.
 */
class ChronicleModule : public muse::modularity::IModuleSetup
{
public:
    std::string moduleName() const override;
    void registerResources() override;
    void registerExports() override;
    void resolveImports() override;
    void registerUiTypes() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

    muse::modularity::IContextSetup* newContext(const muse::modularity::ContextPtr& ctx) const override;

private:
    std::shared_ptr<VersionHistoryService> m_versionHistoryService;
};

class ChronicleContext : public muse::modularity::IContextSetup
{
public:
    explicit ChronicleContext(const muse::modularity::ContextPtr& ctx)
        : muse::modularity::IContextSetup(ctx) {}

    void registerExports() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

private:
    std::shared_ptr<ChronicleActionsController> m_actionsController;
    std::shared_ptr<ChronicleUiActions> m_uiActions;
    std::shared_ptr<ProjectHistoryWatcher> m_projectWatcher;
};
}

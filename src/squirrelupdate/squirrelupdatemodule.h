/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <memory>

#include "framework/global/modularity/imodulesetup.h"

namespace au::squirrelupdate {
class SquirrelUpdateConfiguration;
class SquirrelUpdateService;
class SquirrelUpdateUiActions;

//! Registers the Squirrel.Windows update checker.
//!
//! The module is built on every platform. The service does nothing on any
//! platform other than Windows, where Squirrel.Windows is the only installer,
//! so the module can be linked everywhere without pulling in Windows only
//! behaviour anywhere else.
class SquirrelUpdateModule : public muse::modularity::IModuleSetup
{
public:
    SquirrelUpdateModule();
    ~SquirrelUpdateModule() override;

    std::string moduleName() const override;
    void registerExports() override;
    void registerUiTypes() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

    muse::modularity::IContextSetup* newContext(const muse::modularity::ContextPtr& ctx) const override;

private:
    std::shared_ptr<SquirrelUpdateConfiguration> m_configuration;
    std::shared_ptr<SquirrelUpdateService> m_service;
};

//! Registers the "check-squirrel-update" action so the Help menu and the
//! shortcut table can reach it. Lives in the module context, the same place
//! the command palette action is registered from in the companion module.
class SquirrelUpdateContext : public muse::modularity::IContextSetup
{
public:
    SquirrelUpdateContext(const muse::modularity::ContextPtr& ctx)
        : muse::modularity::IContextSetup(ctx) {}

    void registerExports() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

private:
    std::shared_ptr<SquirrelUpdateUiActions> m_uiActions;
};
}

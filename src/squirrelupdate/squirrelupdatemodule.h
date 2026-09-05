/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <memory>

#include "framework/global/modularity/imodulesetup.h"

namespace au::squirrelupdate {
class SquirrelUpdateConfiguration;
class SquirrelUpdateService;

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

private:
    std::shared_ptr<SquirrelUpdateConfiguration> m_configuration;
    std::shared_ptr<SquirrelUpdateService> m_service;
};
}

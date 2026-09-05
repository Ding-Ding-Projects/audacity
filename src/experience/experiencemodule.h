/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <memory>

#include "framework/global/modularity/imodulesetup.h"

namespace au::experience {
class ExperienceConfiguration;
class ExperienceService;
class NotificationCenter;
class SettingsScheduler;

class ExperienceModule : public muse::modularity::IModuleSetup
{
public:
    ExperienceModule();
    ~ExperienceModule() override;

    std::string moduleName() const override;
    void registerExports() override;
    void registerUiTypes() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

private:
    std::shared_ptr<ExperienceConfiguration> m_configuration;
    std::shared_ptr<ExperienceService> m_service;
    std::shared_ptr<NotificationCenter> m_notificationCenter;
    std::shared_ptr<SettingsScheduler> m_scheduler;
};
}

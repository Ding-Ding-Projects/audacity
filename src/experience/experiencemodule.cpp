/*
 * Audacity: A Digital Audio Editor
 */
#include "experiencemodule.h"

#include "framework/global/modularity/ioc.h"

#include "internal/experienceconfiguration.h"
#include "internal/experienceservice.h"
#include "internal/messagestyler.h"
#include "internal/notificationcenter.h"
#include "internal/settingsscheduler.h"

namespace au::experience {
static const std::string mname("experience");

ExperienceModule::ExperienceModule() = default;
ExperienceModule::~ExperienceModule() = default;

std::string ExperienceModule::moduleName() const
{
    return mname;
}

void ExperienceModule::registerExports()
{
    m_configuration = std::make_shared<ExperienceConfiguration>();
    m_service = std::make_shared<ExperienceService>();
    m_notificationCenter = std::make_shared<NotificationCenter>();
    m_scheduler = std::make_shared<SettingsScheduler>();

    globalIoc()->registerExport<IExperienceConfiguration>(mname, m_configuration);
    globalIoc()->registerExport<IMessageStyler>(mname, std::make_shared<MessageStyler>());
    globalIoc()->registerExport<INotificationCenter>(mname, m_notificationCenter);
    globalIoc()->registerExport<IExperienceService>(mname, m_service);
}

void ExperienceModule::registerUiTypes()
{
}

void ExperienceModule::onInit(const muse::IApplication::RunMode& mode)
{
    if (mode != muse::IApplication::RunMode::GuiApp) {
        return;
    }

    m_configuration->init();
    m_service->init();
    m_scheduler->init();
}
}

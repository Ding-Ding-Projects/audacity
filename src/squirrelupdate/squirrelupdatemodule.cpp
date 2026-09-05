/*
* Audacity: A Digital Audio Editor
*/
#include "squirrelupdatemodule.h"

#include "framework/global/modularity/ioc.h"

#include "internal/squirrelupdateconfiguration.h"
#include "internal/squirrelupdateservice.h"

namespace au::squirrelupdate {
static const std::string mname("squirrelupdate");

SquirrelUpdateModule::SquirrelUpdateModule() = default;
SquirrelUpdateModule::~SquirrelUpdateModule() = default;

std::string SquirrelUpdateModule::moduleName() const
{
    return mname;
}

void SquirrelUpdateModule::registerExports()
{
    m_configuration = std::make_shared<SquirrelUpdateConfiguration>();
    m_service = std::make_shared<SquirrelUpdateService>();

    muse::modularity::globalIoc()->registerExport<ISquirrelUpdateConfiguration>(mname, m_configuration);
    muse::modularity::globalIoc()->registerExport<ISquirrelUpdateService>(mname, m_service);
}

void SquirrelUpdateModule::registerUiTypes()
{
}

void SquirrelUpdateModule::onInit(const muse::IApplication::RunMode& mode)
{
    m_configuration->init();

    if (mode != muse::IApplication::RunMode::GuiApp) {
        return;
    }

    m_service->init();
}
}

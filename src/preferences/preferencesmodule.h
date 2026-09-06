#pragma once

#include "modularity/imodulesetup.h"

#include "framework/global/iapplication.h"

namespace au::preferences {
class PreferencesModule : public muse::modularity::IModuleSetup
{
    muse::GlobalInject<muse::IApplication> m_application;

public:
    std::string moduleName() const override;

    void registerUiTypes() override;
    void resolveImports() override;

    //! Debug-only deterministic capture hook. When AU_OPEN_PREFERENCES is
    //! set to "<pageId>" or "<pageId>#<sectionObjectName>", opens the
    //! Preferences dialog on that page once the window is up, and, when a
    //! section name is given, scrolls that section into view. There is no
    //! setting anywhere that turns this on; it only exists for a script
    //! that already controls the process environment.
    void onStartApp() override;
};
}

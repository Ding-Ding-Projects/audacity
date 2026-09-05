/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include "modularity/imodulesetup.h"

namespace au::toolkit {
/*!
 * \brief The toolkit module: local model management, universal export,
 * bulk selection, external editor handoff, the in-app documentation
 * browser and reusable failure recovery presentation.
 *
 * The module registers its QML types into the \c Audacity.Toolkit import
 * and ships the bundled documentation articles read by the documentation
 * browser.
 */
class ToolkitModule : public muse::modularity::IModuleSetup
{
public:
    std::string moduleName() const override;
    void registerResources() override;
    void registerUiTypes() override;
};
}

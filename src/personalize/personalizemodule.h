/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include "modularity/imodulesetup.h"

namespace au::personalize {
/*!
 * \brief The personalize module.
 *
 * Owns everything a person can use to make the running application their
 * own: the per element appearance editor, the display name setting, the toy
 * locks and their unlock wizard, the joke support desk that recovers a
 * locked out user, and the local, offline authenticator used to protect a
 * lock with a one time code.
 *
 * None of this is a security boundary. Every surface it ships says so
 * plainly wherever a person might otherwise mistake it for one.
 */
class PersonalizeModule : public muse::modularity::IModuleSetup
{
public:
    std::string moduleName() const override;
    void registerResources() override;
    void registerUiTypes() override;
    void onInit(const muse::IApplication::RunMode& mode) override;
};
}

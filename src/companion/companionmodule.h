/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <memory>

#include "modularity/imodulesetup.h"

namespace au::companion {
class CompanionUiActions;

/*!
 * \brief The companion module: the command palette and the regular expression
 * builder workbench.
 *
 * The module owns no scene of its own. It registers the QML types both
 * surfaces are built from into the \c Audacity.Companion import, registers the
 * command palette action so that the global Ctrl+Shift+F shortcut reaches it,
 * and ships the hand written settings index the palette reads.
 */
class CompanionModule : public muse::modularity::IModuleSetup
{
public:
    std::string moduleName() const override;
    void registerResources() override;
    void registerUiTypes() override;

    muse::modularity::IContextSetup* newContext(const muse::modularity::ContextPtr& ctx) const override;
};

//! The UI actions register lives in the module context rather than globally,
//! so the command palette action is registered from here.
class CompanionContext : public muse::modularity::IContextSetup
{
public:
    CompanionContext(const muse::modularity::ContextPtr& ctx)
        : muse::modularity::IContextSetup(ctx) {}

    void registerExports() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

private:
    std::shared_ptr<CompanionUiActions> m_uiActions;
};
}

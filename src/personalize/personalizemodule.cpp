/*
* Audacity: A Digital Audio Editor
*/

#include "personalizemodule.h"

#include <QQmlEngine>
#include <QtQml>

#include "internal/appearanceoverrides.h"
#include "internal/authenticatormodel.h"
#include "internal/brandingmodel.h"
#include "internal/displaynamesettings.h"
#include "internal/lockregistry.h"
#include "internal/mutationhistory.h"
#include "internal/qrcodemodel.h"
#include "internal/supporttickets.h"
#include "framework/global/iglobalconfiguration.h"
#include <QFile>

using namespace au::personalize;
using namespace muse::modularity;

static const std::string mname("personalize");

static void personalize_init_qrc()
{
    Q_INIT_RESOURCE(au_personalize);
}

std::string PersonalizeModule::moduleName() const
{
    return mname;
}

void PersonalizeModule::registerResources()
{
    personalize_init_qrc();
}

void PersonalizeModule::registerUiTypes()
{
    auto brandingCreator = [](QQmlEngine*, QJSEngine*) -> QObject* {
        static muse::GlobalInject<muse::IGlobalConfiguration> configuration;
        QFile shipped(":/images/AudacityLogo.png");
        shipped.open(QIODevice::ReadOnly);
        return new BrandingModel(configuration()->userAppDataPath().toQString() + "/personalize", shipped.readAll());
    };
    qmlRegisterSingletonType<BrandingModel>("Audacity.Personalize", 1, 0, "BrandingModel", brandingCreator);
    // The overrides store is registered under both the Audacity.Personalize
    // module (where the editor popover lives) and the Audacity.M3 module
    // (where every Material 3 component resolves its own overrides), so the
    // same store backs both without the uicomponents module having to import
    // or link against personalize. The two registrations share one instance,
    // owned by C++ rather than by either QML engine slot, so it is never
    // destroyed twice.
    auto appearanceOverridesCreator = [](QQmlEngine*, QJSEngine*) -> QObject* {
        static AppearanceOverrides* instance = []() {
            auto* overrides = new AppearanceOverrides();
            QQmlEngine::setObjectOwnership(overrides, QQmlEngine::CppOwnership);
            return overrides;
        }();
        return instance;
    };
    qmlRegisterSingletonType<AppearanceOverrides>("Audacity.Personalize", 1, 0, "AppearanceOverrides",
                                                  appearanceOverridesCreator);
    qmlRegisterSingletonType<AppearanceOverrides>("Audacity.M3", 1, 0, "AppearanceOverrides",
                                                  appearanceOverridesCreator);
    qmlRegisterSingletonType<DisplayNameSettings>("Audacity.Personalize", 1, 0, "DisplayNameSettings",
                                                  [](QQmlEngine*, QJSEngine*) -> QObject* {
        return new DisplayNameSettings();
    });
    qmlRegisterSingletonType<LockRegistry>("Audacity.Personalize", 1, 0, "LockRegistry",
                                           [](QQmlEngine*, QJSEngine*) -> QObject* {
        return new LockRegistry();
    });
    qmlRegisterSingletonType<MutationHistory>("Audacity.Personalize", 1, 0, "MutationHistory",
                                              [](QQmlEngine*, QJSEngine*) -> QObject* {
        return new MutationHistory();
    });

    qmlRegisterType<QrCodeModel>("Audacity.Personalize", 1, 0, "QrCodeModel");
    qmlRegisterType<AuthenticatorModel>("Audacity.Personalize", 1, 0, "AuthenticatorModel");
    qmlRegisterType<SupportTickets>("Audacity.Personalize", 1, 0, "SupportTickets");
}

void PersonalizeModule::onInit(const muse::IApplication::RunMode& mode)
{
    Q_UNUSED(mode)
}

/*
* Audacity: A Digital Audio Editor
*/

#include "personalizemodule.h"

#include <QQmlEngine>
#include <QtQml>

#include "internal/appearanceoverrides.h"
#include "internal/authenticatormodel.h"
#include "internal/displaynamesettings.h"
#include "internal/lockregistry.h"
#include "internal/mutationhistory.h"
#include "internal/qrcodemodel.h"
#include "internal/supporttickets.h"

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
    qmlRegisterSingletonType<AppearanceOverrides>("Audacity.Personalize", 1, 0, "AppearanceOverrides",
                                                  [](QQmlEngine*, QJSEngine*) -> QObject* {
        return new AppearanceOverrides();
    });
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

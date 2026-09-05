/*
* Audacity: A Digital Audio Editor
*/

#include <QFontDatabase>
#include <QQmlEngine>
#include <QtQml>

#include "iapplication.h"

#include "uicomponentsmodule.h"

#include "components/timecodemodeselector.h"
#include "components/timecodemodel.h"
#include "components/bpmmodel.h"
#include "components/frequencymodel.h"
#include "components/tablesortfilterproxymodel.h"
#include "components/m3themeprovider.h"

#include "settings.h"

#include "log.h"

using namespace au::uicomponents;

static void uicomponents_init_qrc()
{
    Q_INIT_RESOURCE(au_uicomponents);
}

std::string UiComponentsModule::moduleName() const
{
    return "au::uicomponents";
}

void UiComponentsModule::registerResources()
{
    uicomponents_init_qrc();
}

void UiComponentsModule::registerUiTypes()
{
    // Material 3 token engine. Reached from QML as the singleton M3 in the
    // module Audacity.M3. The grouped token objects are owned by the provider
    // and are never constructed from QML.
    qmlRegisterUncreatableType<M3ColorTokens>("Audacity.M3", 1, 0, "M3ColorTokens",
                                              "M3ColorTokens is owned by the M3 singleton");
    qmlRegisterUncreatableType<M3TypographyTokens>("Audacity.M3", 1, 0, "M3TypographyTokens",
                                                   "M3TypographyTokens is owned by the M3 singleton");
    qmlRegisterUncreatableType<M3ShapeTokens>("Audacity.M3", 1, 0, "M3ShapeTokens",
                                              "M3ShapeTokens is owned by the M3 singleton");
    qmlRegisterUncreatableType<M3MotionTokens>("Audacity.M3", 1, 0, "M3MotionTokens",
                                               "M3MotionTokens is owned by the M3 singleton");
    qmlRegisterUncreatableType<M3StateLayerTokens>("Audacity.M3", 1, 0, "M3StateLayerTokens",
                                                   "M3StateLayerTokens is owned by the M3 singleton");
    qmlRegisterUncreatableType<M3ElevationTokens>("Audacity.M3", 1, 0, "M3ElevationTokens",
                                                  "M3ElevationTokens is owned by the M3 singleton");
    qmlRegisterUncreatableType<M3DensityTokens>("Audacity.M3", 1, 0, "M3DensityTokens",
                                                "M3DensityTokens is owned by the M3 singleton");

    qmlRegisterSingletonType<M3ThemeProvider>("Audacity.M3", 1, 0, "M3",
                                              [](QQmlEngine*, QJSEngine*) -> QObject* {
        M3ThemeProvider* provider = new M3ThemeProvider();
        provider->init();
        return provider;
    });

    qmlRegisterUncreatableType<TimecodeModeSelector>("Audacity.UiComponents", 1, 0, "TimecodeModeSelector",
                                                     "TimecodeModeSelector is a simple enum");
    qmlRegisterType<TimecodeModel>("Audacity.UiComponents", 1, 0, "TimecodeModel");
    qmlRegisterType<BPMModel>("Audacity.UiComponents", 1, 0, "BPMModel");
    qmlRegisterType<FrequencyModel>("Audacity.UiComponents", 1, 0, "FrequencyModel");
    qmlRegisterType<TableSortFilterProxyModel>("Audacity.UiComponents", 1, 0, "TableSortFilterProxyModel");
}

void UiComponentsModule::onInit(const muse::IApplication::RunMode& mode)
{
    UNUSED(mode);

    /*
     * Make Roboto Flex the default user interface font.
     *
     * The framework picks the platform font in UiConfiguration::init. Rather
     * than change the framework, Audacity replaces the default value here,
     * after the framework has started. A family the user chose themselves is
     * stored as a user value and still wins over this default, so nobody's
     * preference is overwritten.
     */
    static const muse::Settings::Key UI_FONT_FAMILY_KEY("ui", "ui/theme/fontFamily");
    static const QString M3_FONT_FAMILY("Roboto Flex");

    if (QFontDatabase::families().contains(M3_FONT_FAMILY)) {
        muse::settings()->setDefaultValue(UI_FONT_FAMILY_KEY,
                                          muse::Val(M3_FONT_FAMILY.toStdString()));
    } else {
        LOGW() << "Roboto Flex is not available, keeping the platform user interface font";
    }
}

void UiComponentsModule::onDeinit()
{
}

#include "preferencesmodule.h"

#include <QtQml>

#include <QTimer>

#include "framework/global/modularity/ioc.h"

#include "framework/interactive/iinteractive.h"
#include "framework/interactive/iinteractiveuriregister.h"
#include "framework/global/types/uri.h"
#include "framework/global/types/val.h"

#include "importexport/import/types/importtypes.h"

#include "types/preferencestypes.h"

using namespace au::preferences;

std::string PreferencesModule::moduleName() const
{
    return "preferences";
}

void PreferencesModule::registerUiTypes()
{
    qmlRegisterUncreatableType<au::importexport::TempoDetectionPref>(
        "Audacity.Preferences", 1, 0, "TempoDetection", "Not creatable from QML");
    qmlRegisterUncreatableType<au::preferences::SaveBehaviorPref>(
        "Audacity.Preferences", 1, 0, "SaveBehavior", "Not creatable from QML");
}

void PreferencesModule::resolveImports()
{
    auto ir = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(moduleName());
    if (ir) {
        ir->registerQmlUri(muse::Uri("audacity://preferences"), "Audacity.Preferences", "PreferencesDialog");
    }
}

void PreferencesModule::onStartApp()
{
    if (!qEnvironmentVariableIsSet("AU_OPEN_PREFERENCES")) {
        return;
    }

    const QString spec = qEnvironmentVariable("AU_OPEN_PREFERENCES");
    const int hashIndex = spec.indexOf(QLatin1Char('#'));
    const QString pageId = hashIndex >= 0 ? spec.left(hashIndex) : spec;
    const QString sectionObjectName = hashIndex >= 0 ? spec.mid(hashIndex + 1) : QString();

    if (pageId.isEmpty()) {
        return;
    }

    // The context (and its per-context IInteractive) is created as the app
    // finishes starting up, some time after this function runs; resolving
    // it now would find nothing. A short one-shot delay gives the single
    // window this debug-only hook targets time to exist, and the actual
    // resolution happens once that delay has elapsed.
    const std::string thisModuleName = moduleName();
    // The module object itself lives for the whole process, so capturing
    // "this" here is safe even though the timer fires well after this
    // function has returned.
    QTimer::singleShot(2000, [this, thisModuleName, pageId, sectionObjectName]() {
        std::shared_ptr<muse::IApplication> app = m_application.get();
        if (!app) {
            return;
        }
        const std::vector<muse::modularity::ContextPtr> contexts = app->contexts();
        if (contexts.empty()) {
            return;
        }

        // IInteractive is a per-context interface (one per open project
        // window), so it cannot be injected globally on the module the way
        // IInteractiveUriRegister above is. Resolve it against the first
        // available context, which for this debug-only hook is always the
        // single window the app opens on startup.
        std::shared_ptr<muse::IInteractive> interactive
            = muse::modularity::ioc(contexts.front())->resolve<muse::IInteractive>(thisModuleName);
        if (!interactive) {
            return;
        }

        muse::UriQuery preferencesUri("audacity://preferences");
        preferencesUri.addParam("currentPageId", muse::Val(pageId.toStdString()));
        if (!sectionObjectName.isEmpty()) {
            preferencesUri.addParam("highlightObjectName", muse::Val(sectionObjectName.toStdString()));
        }
        interactive->open(preferencesUri);
    });
}

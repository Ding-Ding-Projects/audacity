/*
* Audacity: A Digital Audio Editor
*/

#include "toolkitmodule.h"

#include <QQmlEngine>
#include <QtQml>

#include "internal/ollamaclient.h"
#include "internal/bulkselectionmodel.h"
#include "internal/pullcartmodel.h"
#include "internal/docsindex.h"
#include "internal/externaleditorservice.h"
#include "internal/hardwarefitservice.h"
#include "internal/exportservicewrapper.h"
#include "internal/bookmarkmodel.h"

using namespace au::toolkit;
using namespace muse::modularity;

static const std::string mname("toolkit");

static void toolkit_init_qrc()
{
    Q_INIT_RESOURCE(au_toolkit);
}

std::string ToolkitModule::moduleName() const
{
    return mname;
}

void ToolkitModule::registerResources()
{
    toolkit_init_qrc();
}

void ToolkitModule::registerUiTypes()
{
    qmlRegisterType<OllamaClient>("Audacity.Toolkit", 1, 0, "OllamaClient");
    qmlRegisterType<BulkSelectionModel>("Audacity.Toolkit", 1, 0, "BulkSelectionModel");
    qmlRegisterType<PullCartModel>("Audacity.Toolkit", 1, 0, "PullCartModel");
    qmlRegisterType<DocsIndex>("Audacity.Toolkit", 1, 0, "DocsIndex");
    qmlRegisterType<ExternalEditorService>("Audacity.Toolkit", 1, 0, "ExternalEditorService");
    qmlRegisterType<HardwareFitService>("Audacity.Toolkit", 1, 0, "HardwareFitService");
    qmlRegisterType<ExportServiceWrapper>("Audacity.Toolkit", 1, 0, "ExportServiceWrapper");
    qmlRegisterType<BookmarkModel>("Audacity.Toolkit", 1, 0, "BookmarkModel");
}

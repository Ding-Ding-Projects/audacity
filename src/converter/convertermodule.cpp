#include "convertermodule.h"
#include "view/converterpresentationmodel.h"
#include <QtQml>
#include "framework/interactive/iinteractiveuriregister.h"
void au::converter::ConverterModule::resolveImports()
{
    if (auto reg = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(moduleName()))
        reg->registerQmlUri(muse::Uri("audacity://converter"), "Audacity/Converter/ConverterPage.qml");
}
void au::converter::ConverterModule::registerUiTypes()
{
    qmlRegisterType<ConverterPresentationModel>("Audacity.Converter", 1, 0, "ConverterPresentationModel");
}

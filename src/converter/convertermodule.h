#pragma once
#include "modularity/imodulesetup.h"
namespace au::converter {
class ConverterModule final : public muse::modularity::IModuleSetup {
public:
    std::string moduleName() const override { return "au::converter"; }
    void resolveImports() override;
    void registerUiTypes() override;
};
}

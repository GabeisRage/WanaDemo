#pragma once

#include "Modules/ModuleManager.h"

class WANAWORKSCORE_API FWanaWorksCoreModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

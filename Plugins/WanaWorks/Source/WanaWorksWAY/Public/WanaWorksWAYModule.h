#pragma once

#include "Modules/ModuleManager.h"

class WANAWORKSWAY_API FWanaWorksWAYModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

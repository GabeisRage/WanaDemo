#pragma once

#include "Modules/ModuleManager.h"

class WANAWORKSWIT_API FWanaWorksWITModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

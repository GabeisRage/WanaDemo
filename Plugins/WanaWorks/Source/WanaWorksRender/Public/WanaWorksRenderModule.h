#pragma once

#include "Modules/ModuleManager.h"

class WANAWORKSRENDER_API FWanaWorksRenderModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

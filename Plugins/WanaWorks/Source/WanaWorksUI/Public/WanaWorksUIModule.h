#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;

class WANAWORKSUI_API FWanaWorksUIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedRef<SDockTab> SpawnWanaWorksTab(const FSpawnTabArgs& Args);

    static const FName WanaWorksTabName;
};

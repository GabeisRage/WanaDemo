#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

WANAWORKSCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogWanaWorks, Log, All);

class WANAWORKSCORE_API FWanaLogger
{
public:
    static void Log(const FString& Message);
    static void Warning(const FString& Message);
    static void Error(const FString& Message);
    static void Verbose(const FString& Message);
};

class WANAWORKSCORE_API FWanaWorksCoreModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

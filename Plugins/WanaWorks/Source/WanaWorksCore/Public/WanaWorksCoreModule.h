#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

WANAWORKSCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogWanaWorks, Log, All);

class WANAWORKSCORE_API FWanaWorksCoreModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

struct WANAWORKSCORE_API FWanaVersionInfo
{
    FString PluginVersion;
    FString BuildDate;

    FWanaVersionInfo()
        : PluginVersion(TEXT("0.1.0"))
        , BuildDate(TEXT(__DATE__))
    {
    }
};

class WANAWORKSCORE_API FWanaLogger
{
public:
    static void Log(const FString& Message);
    static void Warning(const FString& Message);
    static void Error(const FString& Message);
    static void Verbose(const FString& Message);
};

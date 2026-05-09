#include "WanaWorksCoreModule.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogWanaWorks);

IMPLEMENT_MODULE(FWanaWorksCoreModule, WanaWorksCore)

void FWanaWorksCoreModule::StartupModule()
{
    FWanaLogger::Log(TEXT("WanaWorksCore module started."));
}

void FWanaWorksCoreModule::ShutdownModule()
{
    FWanaLogger::Log(TEXT("WanaWorksCore module shutdown."));
}

void FWanaLogger::Log(const FString& Message)
{
    UE_LOG(LogWanaWorks, Log, TEXT("%s"), *Message);
}

void FWanaLogger::Warning(const FString& Message)
{
    UE_LOG(LogWanaWorks, Warning, TEXT("%s"), *Message);
}

void FWanaLogger::Error(const FString& Message)
{
    UE_LOG(LogWanaWorks, Error, TEXT("%s"), *Message);
}

void FWanaLogger::Verbose(const FString& Message)
{
    UE_LOG(LogWanaWorks, Verbose, TEXT("%s"), *Message);
}

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WanaSettings.generated.h"

UCLASS(config=Editor, defaultconfig, meta=(DisplayName="WanaWorks"))
class WANAWORKSCORE_API UWanaSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UWanaSettings();

    UPROPERTY(config, EditAnywhere, Category="Logging")
    bool bEnableVerboseLogging = false;

    UPROPERTY(config, EditAnywhere, Category="Sandbox")
    bool bEnableSandboxMode = true;

    UPROPERTY(config, EditAnywhere, Category="AI")
    FString OpenAIApiKey = TEXT("");

    virtual FName GetCategoryName() const override { return FName("WanaWorks"); }
};

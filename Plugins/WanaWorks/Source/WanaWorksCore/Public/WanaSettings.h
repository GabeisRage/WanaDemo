#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WanaSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Wana Works"))
class WANAWORKSCORE_API UWanaSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UWanaSettings();

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Wana Works|Logging")
    bool bEnableVerboseLogging;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Wana Works|Sandbox")
    bool bEnableSandboxMode;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Wana Works|API")
    FString OpenAIApiKey;

    virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
};

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

    /** Not saved. config properties on this class are written to Config/DefaultEditor.ini,
        which is shared and can be committed. Read WANA_OPENAI_API_KEY or OPENAI_API_KEY
        through GetOpenAIApiKey(). The memory module does not read this field. */
    UPROPERTY(Transient)
    FString OpenAIApiKey = TEXT("");

    static FString GetOpenAIApiKey();

    virtual FName GetCategoryName() const override { return FName("WanaWorks"); }
};

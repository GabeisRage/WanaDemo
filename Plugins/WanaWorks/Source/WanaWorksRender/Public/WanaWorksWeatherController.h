#pragma once

#include "CoreMinimal.h"

class UWorld;

enum class EWanaWeatherPreset : uint8
{
    Clear,
    Overcast,
    Storm
};

struct WANAWORKSRENDER_API FWanaWeatherApplyResult
{
    bool bSucceeded = false;
    FString PresetName;
    FString ErrorMessage;
    TArray<FString> UpdatedSystems;
};

class WANAWORKSRENDER_API FWanaWorksWeatherController
{
public:
    static FWanaWeatherApplyResult ApplyPreset(UWorld* World, EWanaWeatherPreset Preset);
};

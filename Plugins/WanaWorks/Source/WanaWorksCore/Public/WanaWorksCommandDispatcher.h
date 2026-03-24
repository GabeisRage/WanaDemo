#pragma once

#include "CoreMinimal.h"

enum class EWanaCommandAction : uint8
{
    None,
    WeatherClear,
    WeatherOvercast,
    WeatherStorm,
    SpawnCube,
    ListSelection,
    ClassifySelected,
    AddMemoryTest,
    AddPreferenceTest,
    ShowMemory,
    ShowPreferences,
    EnsureRelationshipProfile,
    ShowRelationshipProfile,
    ApplyRelationshipState
};

struct WANAWORKSCORE_API FWanaCommandRequest
{
    FString CommandText;
};

struct WANAWORKSCORE_API FWanaCommandResponse
{
    bool bSucceeded = false;
    bool bClearLog = false;
    EWanaCommandAction Action = EWanaCommandAction::None;
    FString ActionArgument;
    FString StatusMessage;
    TArray<FString> OutputLines;
};

class WANAWORKSCORE_API FWanaCommandDispatcher
{
public:
    static FWanaCommandResponse Dispatch(const FWanaCommandRequest& Request);
};

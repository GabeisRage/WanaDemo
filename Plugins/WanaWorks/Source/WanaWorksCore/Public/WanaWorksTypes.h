#pragma once

#include "CoreMinimal.h"
#include "WanaWorksTypes.generated.h"

UENUM(BlueprintType)
enum class EWanaBehaviorDomain : uint8
{
    Environment,
    Character,
    Prop,
    Vehicle,
    Unknown
};

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaPredictionProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works")
    FName ClassificationTag = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works")
    EWanaBehaviorDomain Domain = EWanaBehaviorDomain::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works")
    float Confidence = 0.0f;
};

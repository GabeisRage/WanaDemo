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

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaMovementReadiness
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bCanAttemptMovement = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bHasUsableMovementCapability = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bHasMovementContext = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bTargetReachableHint = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bSupportsDirectActorMove = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bSupportsLocomotionPulse = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bAnimationDrivenLocomotionDetected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    float DistanceToTarget = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    FString Status = TEXT("Movement blocked");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    FString Detail = TEXT("No readiness data available.");
};

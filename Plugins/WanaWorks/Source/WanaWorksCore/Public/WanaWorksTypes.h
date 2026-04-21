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

UENUM(BlueprintType)
enum class EWanaMovementReadinessLevel : uint8
{
    Allowed UMETA(DisplayName = "Movement Allowed"),
    Limited UMETA(DisplayName = "Movement Limited"),
    Blocked UMETA(DisplayName = "Movement Blocked"),
    Unclear UMETA(DisplayName = "Movement Unclear")
};

UENUM(BlueprintType)
enum class EWanaPhysicalState : uint8
{
    Stable UMETA(DisplayName = "Stable"),
    Alert UMETA(DisplayName = "Alert"),
    Staggered UMETA(DisplayName = "Staggered"),
    OffBalance UMETA(DisplayName = "Off Balance"),
    Bracing UMETA(DisplayName = "Bracing"),
    Recovering UMETA(DisplayName = "Recovering"),
    Panicked UMETA(DisplayName = "Panicked")
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
    EWanaMovementReadinessLevel ReadinessLevel = EWanaMovementReadinessLevel::Blocked;

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
    bool bObstaclePressureDetected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bMovementSpaceRestricted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bLowConfidenceContext = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    bool bPathToTargetObstructed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    float DistanceToTarget = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    float ObstaclePressure = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    FString Status = TEXT("Movement blocked");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WIT")
    FString Detail = TEXT("No readiness data available.");
};

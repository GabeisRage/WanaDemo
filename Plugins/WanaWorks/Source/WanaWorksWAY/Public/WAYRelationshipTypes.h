#pragma once

#include "CoreMinimal.h"
#include "WAYRelationshipTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EWAYRelationshipState : uint8
{
    Acquaintance UMETA(DisplayName = "Acquaintance"),
    Friend UMETA(DisplayName = "Friend"),
    Partner UMETA(DisplayName = "Partner"),
    Enemy UMETA(DisplayName = "Enemy"),
    Neutral UMETA(DisplayName = "Neutral")
};

UENUM(BlueprintType)
enum class EWAYReactionState : uint8
{
    Hostile UMETA(DisplayName = "Hostile"),
    Cooperative UMETA(DisplayName = "Cooperative"),
    Protective UMETA(DisplayName = "Protective"),
    Cautious UMETA(DisplayName = "Cautious"),
    Observational UMETA(DisplayName = "Observational")
};

UENUM(BlueprintType)
enum class EWAYBehaviorPreset : uint8
{
    None UMETA(DisplayName = "None"),
    FollowTarget UMETA(DisplayName = "Follow Target"),
    GuardTarget UMETA(DisplayName = "Guard Target"),
    ObserveTarget UMETA(DisplayName = "Observe Target"),
    ApproachHostile UMETA(DisplayName = "Approach Hostile")
};

UENUM(BlueprintType)
enum class EWAYBehaviorExecutionMode : uint8
{
    Unknown UMETA(DisplayName = "Unknown"),
    FacingOnly UMETA(DisplayName = "Facing Only"),
    MovementAllowed UMETA(DisplayName = "Movement Allowed"),
    FallbackActive UMETA(DisplayName = "Fallback Active")
};

USTRUCT(BlueprintType)
struct WANAWORKSWAY_API FWAYRelationshipSeed
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    EWAYRelationshipState RelationshipState = EWAYRelationshipState::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Trust = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Fear = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Respect = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Attachment = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Hostility = 0.0f;
};

USTRUCT(BlueprintType)
struct WANAWORKSWAY_API FWAYRelationshipProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    AActor* TargetActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    EWAYRelationshipState RelationshipState = EWAYRelationshipState::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Trust = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Fear = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Respect = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Attachment = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Hostility = 0.0f;
};

USTRUCT(BlueprintType)
struct WANAWORKSWAY_API FWAYTargetEvaluation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    AActor* TargetActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    FWAYRelationshipProfile RelationshipProfile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    EWAYReactionState ReactionState = EWAYReactionState::Observational;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    EWAYBehaviorPreset RecommendedBehavior = EWAYBehaviorPreset::None;
};

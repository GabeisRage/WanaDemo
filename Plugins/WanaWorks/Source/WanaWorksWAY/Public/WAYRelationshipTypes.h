#pragma once

#include "CoreMinimal.h"
#include "WanaWorksTypes.h"
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

UENUM(BlueprintType)
enum class EWAYAnimationHookApplicationStatus : uint8
{
    NotAvailable UMETA(DisplayName = "Not Available"),
    Limited UMETA(DisplayName = "Limited"),
    Active UMETA(DisplayName = "Active")
};

UENUM(BlueprintType)
enum class EWAYAutomaticAnimationIntegrationStatus : uint8
{
    Ready UMETA(DisplayName = "Ready"),
    Applied UMETA(DisplayName = "Applied"),
    Limited UMETA(DisplayName = "Limited"),
    NotSupported UMETA(DisplayName = "Not Supported")
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

USTRUCT(BlueprintType)
struct WANAWORKSWAY_API FWAYAnimationHookState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    bool bHookStateValid = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    bool bFacingHookRequested = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    bool bTurnToTargetRequested = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    bool bLocomotionSafeExecutionHint = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    bool bMovementLimitedFallbackHint = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    bool bOutwardGuardHintRequested = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    bool bPhysicalReactionStateAvailable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    AActor* TargetActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    FVector TargetWorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    float TargetDistance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    EWAYReactionState ReactionState = EWAYReactionState::Observational;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    EWAYRelationshipState RelationshipState = EWAYRelationshipState::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    EWAYBehaviorPreset RecommendedBehavior = EWAYBehaviorPreset::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    EWAYBehaviorExecutionMode ExecutionMode = EWAYBehaviorExecutionMode::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    EWAYAnimationHookApplicationStatus ApplicationStatus = EWAYAnimationHookApplicationStatus::NotAvailable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    EWanaPhysicalState PhysicalState = EWanaPhysicalState::Stable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    float PhysicalStabilityScore = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    float PhysicalRecoveryProgress = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    float PhysicalInstabilityAlpha = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    FVector PhysicalImpactDirection = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    float PhysicalImpactStrength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    FString BehaviorIntent = TEXT("No behavior intent");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    FString VisibleBehaviorLabel = TEXT("None");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    FString IdentityRoleHint = TEXT("Neutral");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    FString PostureHint = TEXT("Neutral observe posture");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    FString PostureCategory = TEXT("Observe");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    FString FallbackHint = TEXT("Stable observe stance");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Animation")
    FString Detail = TEXT("Animation hook state is not initialized.");
};

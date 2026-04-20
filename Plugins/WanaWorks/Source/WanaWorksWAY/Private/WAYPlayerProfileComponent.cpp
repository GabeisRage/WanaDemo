#include "WAYPlayerProfileComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "WanaWorksEnvironmentReadiness.h"
#include "WanaIdentityComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogWanaWorksWAY, Log, All);

namespace
{
constexpr float BasicReactionMoveDistance = 120.0f;
constexpr float BasicReactionProtectiveRange = 260.0f;
constexpr float StarterFollowRange = 180.0f;
constexpr float BasicReactionMoveInputScale = 0.9f;
constexpr float BasicReactionMovePulseDuration = 0.45f;
constexpr float BasicReactionProtectiveMovePulseDuration = 0.30f;
constexpr float BasicReactionFollowMovePulseDuration = 0.60f;
constexpr float BasicReactionFollowMoveInputScale = 0.78f;
constexpr float BasicReactionRetreatMoveInputScale = 0.82f;

float ClampRelationshipValue(float Value)
{
    return FMath::Clamp(Value, 0.0f, 1.0f);
}

FString GetReactionStateLogLabel(EWAYReactionState ReactionState)
{
    const UEnum* ReactionEnum = StaticEnum<EWAYReactionState>();
    return ReactionEnum
        ? ReactionEnum->GetDisplayNameTextByValue(static_cast<int64>(ReactionState)).ToString()
        : TEXT("Observational");
}

FString GetBehaviorPresetLogLabel(EWAYBehaviorPreset BehaviorPreset)
{
    const UEnum* BehaviorEnum = StaticEnum<EWAYBehaviorPreset>();
    return BehaviorEnum
        ? BehaviorEnum->GetDisplayNameTextByValue(static_cast<int64>(BehaviorPreset)).ToString()
        : TEXT("None");
}

FString GetAnimationHookApplicationStatusLogLabel(EWAYAnimationHookApplicationStatus ApplicationStatus)
{
    const UEnum* ApplicationStatusEnum = StaticEnum<EWAYAnimationHookApplicationStatus>();
    return ApplicationStatusEnum
        ? ApplicationStatusEnum->GetDisplayNameTextByValue(static_cast<int64>(ApplicationStatus)).ToString()
        : TEXT("Not Available");
}

FString GetVisibleBehaviorLabel(EWAYBehaviorPreset BehaviorPreset, EWAYReactionState ReactionState)
{
    if (BehaviorPreset != EWAYBehaviorPreset::None)
    {
        return GetBehaviorPresetLogLabel(BehaviorPreset);
    }

    return FString::Printf(TEXT("%s Response"), *GetReactionStateLogLabel(ReactionState));
}

EWAYReactionState ResolveReactionStateForBehaviorPreset(EWAYBehaviorPreset BehaviorPreset)
{
    switch (BehaviorPreset)
    {
    case EWAYBehaviorPreset::ApproachHostile:
        return EWAYReactionState::Hostile;

    case EWAYBehaviorPreset::FollowTarget:
        return EWAYReactionState::Cooperative;

    case EWAYBehaviorPreset::GuardTarget:
        return EWAYReactionState::Protective;

    case EWAYBehaviorPreset::ObserveTarget:
        return EWAYReactionState::Observational;

    case EWAYBehaviorPreset::None:
    default:
        return EWAYReactionState::Observational;
    }
}

void InspectAnimationHookAvailability(const AActor* Actor, bool& bOutHasSkeletalMeshComponent, bool& bOutHasAnimBlueprint)
{
    bOutHasSkeletalMeshComponent = false;
    bOutHasAnimBlueprint = false;

    if (!Actor)
    {
        return;
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
    bOutHasSkeletalMeshComponent = SkeletalMeshComponents.Num() > 0;

    for (const USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (!SkeletalMeshComponent)
        {
            continue;
        }

        if (const_cast<USkeletalMeshComponent*>(SkeletalMeshComponent)->GetAnimClass() != nullptr
            || SkeletalMeshComponent->GetAnimInstance() != nullptr
            || SkeletalMeshComponent->GetAnimationMode() == EAnimationMode::AnimationBlueprint)
        {
            bOutHasAnimBlueprint = true;
            break;
        }
    }
}

void FaceActorDirection(AActor* Actor, const FVector& Direction)
{
    if (!Actor || Direction.IsNearlyZero())
    {
        return;
    }

    FRotator DesiredRotation = Direction.Rotation();
    DesiredRotation.Pitch = 0.0f;
    DesiredRotation.Roll = 0.0f;
    Actor->SetActorRotation(DesiredRotation);
}

bool TryMoveActor(AActor* Actor, const FVector& Offset)
{
    if (!Actor || Offset.IsNearlyZero())
    {
        return false;
    }

    USceneComponent* RootComponent = Actor->GetRootComponent();

    if (!RootComponent || RootComponent->Mobility != EComponentMobility::Movable || RootComponent->IsSimulatingPhysics())
    {
        return false;
    }

    FHitResult SweepHit;
    return Actor->SetActorLocation(Actor->GetActorLocation() + Offset, true, &SweepHit, ETeleportType::None);
}

float GetFallbackMoveDistance(EWAYReactionState ReactionState, float DistanceToTarget)
{
    switch (ReactionState)
    {
    case EWAYReactionState::Hostile:
        return FMath::Clamp(DistanceToTarget * 0.30f, 90.0f, BasicReactionMoveDistance * 1.15f);

    case EWAYReactionState::Protective:
        return FMath::Clamp(DistanceToTarget - BasicReactionProtectiveRange, 40.0f, BasicReactionMoveDistance * 0.75f);

    case EWAYReactionState::Cautious:
        return FMath::Clamp(BasicReactionMoveDistance * 0.60f, 55.0f, 90.0f);

    default:
        return BasicReactionMoveDistance;
    }
}

float GetFollowFallbackMoveDistance(float DistanceToTarget)
{
    return FMath::Clamp(DistanceToTarget - StarterFollowRange, 55.0f, BasicReactionMoveDistance * 0.90f);
}

FString DescribeBasicReactionBehavior(AActor* ObserverActor, AActor* TargetActor, EWAYReactionState ReactionState)
{
    if (!ObserverActor || !TargetActor)
    {
        return TEXT("Skipped basic response because the observer or target was missing.");
    }

    FVector DirectionToTarget = TargetActor->GetActorLocation() - ObserverActor->GetActorLocation();
    DirectionToTarget.Z = 0.0f;

    if (DirectionToTarget.IsNearlyZero())
    {
        DirectionToTarget = ObserverActor->GetActorForwardVector();
        DirectionToTarget.Z = 0.0f;
    }

    const FVector SafeDirection = DirectionToTarget.GetSafeNormal();

    switch (ReactionState)
    {
    case EWAYReactionState::Hostile:
        return TEXT("Locked attention on the target and attempted a deliberate hostile advance.");

    case EWAYReactionState::Cooperative:
        return TEXT("Held a cooperative follow-ready stance and kept attention on the target.");

    case EWAYReactionState::Protective:
        return DirectionToTarget.SizeSquared() > FMath::Square(BasicReactionProtectiveRange)
            ? TEXT("Moved to establish a clearer guard position near the target.")
            : TEXT("Held a readable guard stance near the target and faced outward protectively.");

    case EWAYReactionState::Cautious:
        return TEXT("Kept attention on the target and prepared a cautious retreat.");

    case EWAYReactionState::Observational:
    default:
        return TEXT("Held an attentive observation stance on the target.");
    }
}

void ApplyReactionFacing(AActor* ObserverActor, const FVector& SafeDirection, EWAYReactionState ReactionState, float DistanceToTarget)
{
    if (!ObserverActor)
    {
        return;
    }

    switch (ReactionState)
    {
    case EWAYReactionState::Hostile:
    case EWAYReactionState::Cooperative:
    case EWAYReactionState::Cautious:
    case EWAYReactionState::Observational:
        FaceActorDirection(ObserverActor, SafeDirection);
        break;

    case EWAYReactionState::Protective:
        if (DistanceToTarget > BasicReactionProtectiveRange)
        {
            FaceActorDirection(ObserverActor, SafeDirection);
        }
        else
        {
            FaceActorDirection(ObserverActor, -SafeDirection);
        }
        break;

    default:
        break;
    }
}

bool IsMovementDrivenReaction(EWAYReactionState ReactionState)
{
    return ReactionState == EWAYReactionState::Hostile
        || ReactionState == EWAYReactionState::Protective
        || ReactionState == EWAYReactionState::Cautious;
}
}

UWAYPlayerProfileComponent::UWAYPlayerProfileComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.SetTickFunctionEnable(false);
}

void UWAYPlayerProfileComponent::BeginPlay()
{
    Super::BeginPlay();

    OnReactionChanged.RemoveDynamic(this, &UWAYPlayerProfileComponent::HandleReactionChanged);
    OnReactionChanged.AddDynamic(this, &UWAYPlayerProfileComponent::HandleReactionChanged);
    UpdateAnimationHookState(
        nullptr,
        EWAYReactionState::Observational,
        EWAYBehaviorPreset::None,
        EWAYBehaviorExecutionMode::Unknown,
        false,
        false,
        TEXT("Animation hook layer is ready, but no target is currently driving it."));
}

void UWAYPlayerProfileComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* ObserverActor = GetOwner();
    AActor* TargetActor = ActiveBasicReactionTarget.Get();
    APawn* ObserverPawn = Cast<APawn>(ObserverActor);

    if (!ObserverPawn || !TargetActor || ActiveBasicReactionTimeRemaining <= 0.0f)
    {
        StopBasicReactionMovement();
        return;
    }

    FVector DirectionToTarget = TargetActor->GetActorLocation() - ObserverActor->GetActorLocation();
    DirectionToTarget.Z = 0.0f;

    if (DirectionToTarget.IsNearlyZero())
    {
        DirectionToTarget = ObserverActor->GetActorForwardVector();
        DirectionToTarget.Z = 0.0f;
    }

    const FVector SafeDirection = DirectionToTarget.GetSafeNormal();
    FVector MovementDirection = FVector::ZeroVector;

    const FWanaMovementReadiness MovementReadiness = FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(ObserverActor, TargetActor);

    if (!MovementReadiness.bCanAttemptMovement || !MovementReadiness.bHasUsableMovementCapability || !MovementReadiness.bHasMovementContext)
    {
        UE_LOG(
            LogWanaWorksWAY,
            Log,
            TEXT("Stopped WanaAI reaction movement. Observer=%s Target=%s Reason=%s"),
            *ObserverActor->GetActorNameOrLabel(),
            *TargetActor->GetActorNameOrLabel(),
            *MovementReadiness.Detail);
        StopBasicReactionMovement();
        return;
    }

    switch (ActiveBasicReactionState)
    {
    case EWAYReactionState::Hostile:
        FaceActorDirection(ObserverActor, SafeDirection);
        MovementDirection = SafeDirection;
        break;

    case EWAYReactionState::Cooperative:
        FaceActorDirection(ObserverActor, SafeDirection);

        if (DirectionToTarget.SizeSquared() > FMath::Square(StarterFollowRange))
        {
            MovementDirection = SafeDirection;
        }
        else
        {
            StopBasicReactionMovement();
            return;
        }
        break;

    case EWAYReactionState::Protective:
        if (DirectionToTarget.SizeSquared() > FMath::Square(BasicReactionProtectiveRange))
        {
            FaceActorDirection(ObserverActor, SafeDirection);
            MovementDirection = SafeDirection;
        }
        else
        {
            FaceActorDirection(ObserverActor, -SafeDirection);
        }
        break;

    case EWAYReactionState::Cautious:
        FaceActorDirection(ObserverActor, SafeDirection);
        MovementDirection = -SafeDirection;
        break;

    case EWAYReactionState::Observational:
    default:
        StopBasicReactionMovement();
        return;
    }

    if (!MovementDirection.IsNearlyZero())
    {
        ObserverPawn->AddMovementInput(MovementDirection, ActiveBasicReactionMoveInputScale, true);
    }

    ActiveBasicReactionTimeRemaining -= DeltaTime;

    if (ActiveBasicReactionTimeRemaining <= 0.0f)
    {
        StopBasicReactionMovement();
    }
}

void UWAYPlayerProfileComponent::RecordPreferenceSignal(FName SignalName, float Weight)
{
    FWAYPreferenceSignal Signal;
    Signal.SignalName = SignalName;
    Signal.Weight = Weight;
    Signals.Add(Signal);
}

bool UWAYPlayerProfileComponent::GetRelationshipProfileForTarget(AActor* TargetActor, FWAYRelationshipProfile& OutProfile) const
{
    const int32 RelationshipProfileIndex = FindRelationshipProfileIndex(TargetActor);

    if (RelationshipProfileIndex == INDEX_NONE)
    {
        OutProfile = FWAYRelationshipProfile();
        OutProfile.TargetActor = TargetActor;
        return false;
    }

    OutProfile = RelationshipProfiles[RelationshipProfileIndex];
    return true;
}

FWAYRelationshipProfile UWAYPlayerProfileComponent::EnsureRelationshipProfileForTarget(AActor* TargetActor)
{
    if (!TargetActor)
    {
        return FWAYRelationshipProfile();
    }

    int32 RelationshipProfileIndex = FindRelationshipProfileIndex(TargetActor);

    if (RelationshipProfileIndex == INDEX_NONE)
    {
        RelationshipProfileIndex = RelationshipProfiles.Add(CreateRelationshipProfile(TargetActor));
    }

    return RelationshipProfiles[RelationshipProfileIndex];
}

FWAYRelationshipProfile UWAYPlayerProfileComponent::EnsureRelationshipProfileForObservedTarget(AActor* TargetActor)
{
    return EnsureRelationshipProfileForTarget(TargetActor);
}

FWAYTargetEvaluation UWAYPlayerProfileComponent::EvaluateTarget(AActor* TargetActor)
{
    FWAYTargetEvaluation Evaluation;
    Evaluation.TargetActor = TargetActor;

    if (!TargetActor)
    {
        Evaluation.RelationshipProfile.TargetActor = nullptr;
        Evaluation.ReactionState = ResolveReactionForRelationshipState(EWAYRelationshipState::Neutral);
        Evaluation.RecommendedBehavior = EWAYBehaviorPreset::None;
        UpdateAnimationHookState(
            nullptr,
            Evaluation.ReactionState,
            Evaluation.RecommendedBehavior,
            EWAYBehaviorExecutionMode::Unknown,
            false,
            false,
            TEXT("No target is currently driving the WanaWorks animation hooks."));
        return Evaluation;
    }

    Evaluation.RelationshipProfile = EnsureRelationshipProfileForObservedTarget(TargetActor);
    Evaluation.ReactionState = ResolveReactionForRelationshipState(Evaluation.RelationshipProfile.RelationshipState);
    Evaluation.RecommendedBehavior = ResolveRecommendedBehaviorForReactionState(Evaluation.ReactionState);
    const FWanaMovementReadiness MovementReadiness = FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(GetOwner(), TargetActor);
    const EWAYBehaviorExecutionMode ExecutionMode = ResolveBehaviorExecutionModeForMovementReadiness(MovementReadiness);
    CachedBehaviorExecutionModes.Add(TargetActor, ExecutionMode);
    UpdateAnimationHookState(
        TargetActor,
        Evaluation.ReactionState,
        Evaluation.RecommendedBehavior,
        ExecutionMode,
        true,
        true,
        TEXT("Evaluation updated the current animation hook state for this target."));

    const EWAYReactionState* PreviousReaction = CachedReactionStates.Find(TargetActor);
    const EWAYBehaviorPreset* PreviousBehavior = CachedRecommendedBehaviors.Find(TargetActor);

    if (!PreviousReaction || *PreviousReaction != Evaluation.ReactionState)
    {
        CachedReactionStates.Add(TargetActor, Evaluation.ReactionState);
        OnReactionChanged.Broadcast(GetOwner(), TargetActor, Evaluation.RelationshipProfile.RelationshipState, Evaluation.ReactionState);
    }

    if (!PreviousBehavior || *PreviousBehavior != Evaluation.RecommendedBehavior)
    {
        CachedRecommendedBehaviors.Add(TargetActor, Evaluation.RecommendedBehavior);
        OnRecommendedBehaviorChanged.Broadcast(GetOwner(), TargetActor, Evaluation.ReactionState, Evaluation.RecommendedBehavior);

        if (AActor* ObserverActor = GetOwner())
        {
            UE_LOG(
                LogWanaWorksWAY,
                Log,
                TEXT("Updated WanaAI recommended behavior. Observer=%s Target=%s Reaction=%s Behavior=%s"),
                *ObserverActor->GetActorNameOrLabel(),
                *TargetActor->GetActorNameOrLabel(),
                *GetReactionStateLogLabel(Evaluation.ReactionState),
                *GetBehaviorPresetLogLabel(Evaluation.RecommendedBehavior));
        }
    }

    return Evaluation;
}

EWAYReactionState UWAYPlayerProfileComponent::EvaluateAndGetReaction(AActor* TargetActor)
{
    return EvaluateTarget(TargetActor).ReactionState;
}

EWAYReactionState UWAYPlayerProfileComponent::EvaluateAndReact(AActor* TargetActor)
{
    return EvaluateTarget(TargetActor).ReactionState;
}

EWAYBehaviorPreset UWAYPlayerProfileComponent::EvaluateTargetAndGetRecommendedBehavior(AActor* TargetActor)
{
    return EvaluateTarget(TargetActor).RecommendedBehavior;
}

bool UWAYPlayerProfileComponent::ApplyRecommendedBehaviorHook(AActor* TargetActor)
{
    return ApplyBehaviorPresetHook(TargetActor, GetRecommendedBehaviorForTarget(TargetActor));
}

bool UWAYPlayerProfileComponent::ApplyBehaviorPresetHook(AActor* TargetActor, EWAYBehaviorPreset BehaviorPreset)
{
    if (!TargetActor)
    {
        return false;
    }

    AActor* ObserverActor = GetOwner();

    if (!ObserverActor)
    {
        return false;
    }

    FVector DirectionToTarget = TargetActor->GetActorLocation() - ObserverActor->GetActorLocation();
    DirectionToTarget.Z = 0.0f;

    if (DirectionToTarget.IsNearlyZero())
    {
        DirectionToTarget = ObserverActor->GetActorForwardVector();
        DirectionToTarget.Z = 0.0f;
    }

    const FVector SafeDirection = DirectionToTarget.GetSafeNormal();
    const FWanaMovementReadiness MovementReadiness = FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(ObserverActor, TargetActor);
    const float DistanceToTarget = DirectionToTarget.Size();
    FString HookDescription;
    bool bApplied = true;
    EWAYBehaviorExecutionMode ExecutionMode = ResolveBehaviorExecutionModeForMovementReadiness(MovementReadiness);

    switch (BehaviorPreset)
    {
    case EWAYBehaviorPreset::ApproachHostile:
        ApplyReactionFacing(ObserverActor, SafeDirection, EWAYReactionState::Hostile, DistanceToTarget);
        HookDescription = TEXT("Starter hook locked attention on the hostile target and attempted a deliberate approach.");
        TryApplyMovementReaction(TargetActor, EWAYReactionState::Hostile, SafeDirection, HookDescription, &ExecutionMode);
        break;

    case EWAYBehaviorPreset::GuardTarget:
        ApplyReactionFacing(ObserverActor, SafeDirection, EWAYReactionState::Protective, DistanceToTarget);
        HookDescription = TEXT("Starter hook entered a readable guard stance around the target.");
        TryApplyMovementReaction(TargetActor, EWAYReactionState::Protective, SafeDirection, HookDescription, &ExecutionMode);
        break;

    case EWAYBehaviorPreset::ObserveTarget:
        StopBasicReactionMovement();
        ApplyReactionFacing(ObserverActor, SafeDirection, EWAYReactionState::Observational, DistanceToTarget);
        HookDescription = TEXT("Starter hook held an attentive observation stance and kept focus on the target.");
        ExecutionMode = EWAYBehaviorExecutionMode::FacingOnly;
        break;

    case EWAYBehaviorPreset::FollowTarget:
    {
        StopBasicReactionMovement();
        ApplyReactionFacing(ObserverActor, SafeDirection, EWAYReactionState::Cooperative, DistanceToTarget);

        if (DistanceToTarget <= StarterFollowRange)
        {
            HookDescription = TEXT("Starter hook held a follow-ready stance near the target and maintained clear attention.");
            ExecutionMode = EWAYBehaviorExecutionMode::FacingOnly;
            break;
        }

        if (!MovementReadiness.bCanAttemptMovement)
        {
            HookDescription = FString::Printf(
                TEXT("Starter hook held a follow-ready stance, but follow movement was blocked. %s Falling back to facing-only behavior."),
                *MovementReadiness.Detail);
            ExecutionMode = EWAYBehaviorExecutionMode::FacingOnly;
            break;
        }

        FString MovementCompatibilityDetail;

        if (StartBasicReactionMovement(TargetActor, EWAYReactionState::Cooperative, MovementCompatibilityDetail, BasicReactionFollowMoveInputScale, BasicReactionFollowMovePulseDuration))
        {
            HookDescription = FString::Printf(
                TEXT("Starter hook requested a deliberate follow step toward the target. Movement-compatible execution was used. %s"),
                *MovementReadiness.Detail);
            ExecutionMode = EWAYBehaviorExecutionMode::MovementAllowed;

            if (!MovementCompatibilityDetail.IsEmpty())
            {
                HookDescription += TEXT(" ");
                HookDescription += MovementCompatibilityDetail.TrimStartAndEnd();
            }

            break;
        }

        if (MovementReadiness.bSupportsDirectActorMove && TryMoveActor(ObserverActor, SafeDirection * GetFollowFallbackMoveDistance(DistanceToTarget)))
        {
            HookDescription = FString::Printf(
                TEXT("Starter hook stepped toward the target with a safe actor fallback. Fallback movement was used. %s"),
                *MovementReadiness.Detail);
            ExecutionMode = EWAYBehaviorExecutionMode::FallbackActive;
            break;
        }

        HookDescription = FString::Printf(
            TEXT("Starter hook held a follow-ready stance, but no locomotion-compatible follow path was available. %s"),
            *MovementReadiness.Detail);
        ExecutionMode = EWAYBehaviorExecutionMode::FacingOnly;
        break;
    }

    case EWAYBehaviorPreset::None:
    default:
        StopBasicReactionMovement();
        HookDescription = TEXT("No starter behavior hook was applied for the current target.");
        bApplied = false;
        ExecutionMode = ResolveBehaviorExecutionModeForMovementReadiness(MovementReadiness);
        break;
    }

    CachedBehaviorExecutionModes.Add(TargetActor, ExecutionMode);
    CacheBehaviorExecutionResult(
        TargetActor,
        GetVisibleBehaviorLabel(BehaviorPreset, ResolveReactionStateForBehaviorPreset(BehaviorPreset)),
        HookDescription,
        ExecutionMode);
    UpdateAnimationHookState(
        TargetActor,
        BehaviorPreset == EWAYBehaviorPreset::None ? GetReactionForTarget(TargetActor) : ResolveReactionStateForBehaviorPreset(BehaviorPreset),
        BehaviorPreset,
        ExecutionMode,
        BehaviorPreset != EWAYBehaviorPreset::None,
        BehaviorPreset != EWAYBehaviorPreset::None,
        HookDescription);

    if (bApplied && BehaviorPreset != EWAYBehaviorPreset::None)
    {
        CachedLastAppliedBehaviorHooks.Add(TargetActor, BehaviorPreset);
    }

    UE_LOG(
        LogWanaWorksWAY,
        Log,
        TEXT("Applied WanaAI starter behavior hook. Observer=%s Target=%s Behavior=%s Applied=%s Detail=%s"),
        *ObserverActor->GetActorNameOrLabel(),
        *TargetActor->GetActorNameOrLabel(),
        *GetBehaviorPresetLogLabel(BehaviorPreset),
        bApplied ? TEXT("Yes") : TEXT("No"),
        *HookDescription);

    return bApplied;
}

bool UWAYPlayerProfileComponent::ApplyBasicReactionBehavior(AActor* TargetActor)
{
    if (!TargetActor)
    {
        return false;
    }

    AActor* ObserverActor = GetOwner();

    if (!ObserverActor)
    {
        return false;
    }

    const EWAYReactionState ReactionState = GetReactionForTarget(TargetActor);
    FVector DirectionToTarget = TargetActor->GetActorLocation() - ObserverActor->GetActorLocation();
    DirectionToTarget.Z = 0.0f;

    if (DirectionToTarget.IsNearlyZero())
    {
        DirectionToTarget = ObserverActor->GetActorForwardVector();
        DirectionToTarget.Z = 0.0f;
    }

    const FVector SafeDirection = DirectionToTarget.GetSafeNormal();
    const float DistanceToTarget = DirectionToTarget.Size();

    ApplyReactionFacing(ObserverActor, SafeDirection, ReactionState, DistanceToTarget);

    FString BehaviorDescription = DescribeBasicReactionBehavior(ObserverActor, TargetActor, ReactionState);
    EWAYBehaviorExecutionMode ExecutionMode = EWAYBehaviorExecutionMode::FacingOnly;
    TryApplyMovementReaction(TargetActor, ReactionState, SafeDirection, BehaviorDescription, &ExecutionMode);
    CachedBehaviorExecutionModes.Add(TargetActor, ExecutionMode);
    CacheBehaviorExecutionResult(
        TargetActor,
        GetVisibleBehaviorLabel(ResolveRecommendedBehaviorForReactionState(ReactionState), ReactionState),
        BehaviorDescription,
        ExecutionMode);
    UpdateAnimationHookState(
        TargetActor,
        ReactionState,
        ResolveRecommendedBehaviorForReactionState(ReactionState),
        ExecutionMode,
        true,
        true,
        BehaviorDescription);

    UE_LOG(
        LogWanaWorksWAY,
        Log,
        TEXT("Applied WanaAI basic reaction behavior. Observer=%s Target=%s Reaction=%s Behavior=%s"),
        *ObserverActor->GetActorNameOrLabel(),
        *TargetActor->GetActorNameOrLabel(),
        *GetReactionStateLogLabel(ReactionState),
        *BehaviorDescription);

    return true;
}

void UWAYPlayerProfileComponent::SetRelationshipStateForTarget(AActor* TargetActor, EWAYRelationshipState NewRelationshipState)
{
    if (!TargetActor)
    {
        return;
    }

    int32 RelationshipProfileIndex = FindRelationshipProfileIndex(TargetActor);

    if (RelationshipProfileIndex == INDEX_NONE)
    {
        RelationshipProfileIndex = RelationshipProfiles.Add(CreateRelationshipProfile(TargetActor));
    }

    FWAYRelationshipProfile& Profile = RelationshipProfiles[RelationshipProfileIndex];
    const EWAYRelationshipState PreviousState = Profile.RelationshipState;

    if (PreviousState == NewRelationshipState)
    {
        return;
    }

    Profile.RelationshipState = NewRelationshipState;
    OnRelationshipStateChanged.Broadcast(TargetActor, PreviousState, NewRelationshipState);
}

EWAYReactionState UWAYPlayerProfileComponent::GetReactionForTarget(AActor* TargetActor) const
{
    FWAYRelationshipProfile Profile;

    if (GetRelationshipProfileForTarget(TargetActor, Profile))
    {
        return ResolveReactionForRelationshipState(Profile.RelationshipState);
    }

    return ResolveReactionForRelationshipState(EWAYRelationshipState::Neutral);
}

EWAYBehaviorPreset UWAYPlayerProfileComponent::GetRecommendedBehaviorForTarget(AActor* TargetActor) const
{
    if (!TargetActor)
    {
        return EWAYBehaviorPreset::None;
    }

    return ResolveRecommendedBehaviorForReactionState(GetReactionForTarget(TargetActor));
}

bool UWAYPlayerProfileComponent::HasStarterBehaviorHookForTarget(AActor* TargetActor) const
{
    return TargetActor != nullptr && GetRecommendedBehaviorForTarget(TargetActor) != EWAYBehaviorPreset::None;
}

EWAYBehaviorPreset UWAYPlayerProfileComponent::GetLastAppliedBehaviorHookForTarget(AActor* TargetActor) const
{
    if (const EWAYBehaviorPreset* CachedHook = CachedLastAppliedBehaviorHooks.Find(TargetActor))
    {
        return *CachedHook;
    }

    return EWAYBehaviorPreset::None;
}

EWAYBehaviorExecutionMode UWAYPlayerProfileComponent::GetBehaviorExecutionModeForTarget(AActor* TargetActor) const
{
    if (!TargetActor)
    {
        return EWAYBehaviorExecutionMode::Unknown;
    }

    if (const EWAYBehaviorExecutionMode* CachedExecutionMode = CachedBehaviorExecutionModes.Find(TargetActor))
    {
        return *CachedExecutionMode;
    }

    return ResolveBehaviorExecutionModeForMovementReadiness(FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(GetOwner(), TargetActor));
}

FString UWAYPlayerProfileComponent::GetVisibleBehaviorLabelForTarget(AActor* TargetActor) const
{
    if (!TargetActor)
    {
        return FString();
    }

    if (const FString* CachedLabel = CachedVisibleBehaviorLabels.Find(TargetActor))
    {
        return *CachedLabel;
    }

    return FString();
}

FString UWAYPlayerProfileComponent::GetBehaviorExecutionDetailForTarget(AActor* TargetActor) const
{
    if (!TargetActor)
    {
        return FString();
    }

    if (const FString* CachedDetail = CachedBehaviorExecutionDetails.Find(TargetActor))
    {
        return *CachedDetail;
    }

    return FString();
}

EWAYReactionState UWAYPlayerProfileComponent::ResolveReactionForRelationshipState(EWAYRelationshipState RelationshipState)
{
    switch (RelationshipState)
    {
    case EWAYRelationshipState::Enemy:
        return EWAYReactionState::Hostile;

    case EWAYRelationshipState::Friend:
        return EWAYReactionState::Cooperative;

    case EWAYRelationshipState::Partner:
        return EWAYReactionState::Protective;

    case EWAYRelationshipState::Acquaintance:
        return EWAYReactionState::Cautious;

    case EWAYRelationshipState::Neutral:
    default:
        return EWAYReactionState::Observational;
    }
}

EWAYBehaviorPreset UWAYPlayerProfileComponent::ResolveRecommendedBehaviorForReactionState(EWAYReactionState ReactionState)
{
    switch (ReactionState)
    {
    case EWAYReactionState::Hostile:
        return EWAYBehaviorPreset::ApproachHostile;

    case EWAYReactionState::Cooperative:
        return EWAYBehaviorPreset::FollowTarget;

    case EWAYReactionState::Protective:
        return EWAYBehaviorPreset::GuardTarget;

    case EWAYReactionState::Cautious:
    case EWAYReactionState::Observational:
        return EWAYBehaviorPreset::ObserveTarget;

    default:
        return EWAYBehaviorPreset::None;
    }
}

EWAYBehaviorExecutionMode UWAYPlayerProfileComponent::ResolveBehaviorExecutionModeForMovementReadiness(const FWanaMovementReadiness& MovementReadiness)
{
    if (!MovementReadiness.bCanAttemptMovement)
    {
        return EWAYBehaviorExecutionMode::FacingOnly;
    }

    if (MovementReadiness.bSupportsLocomotionPulse)
    {
        return EWAYBehaviorExecutionMode::MovementAllowed;
    }

    if (MovementReadiness.bSupportsDirectActorMove)
    {
        return EWAYBehaviorExecutionMode::FallbackActive;
    }

    return EWAYBehaviorExecutionMode::FacingOnly;
}

void UWAYPlayerProfileComponent::HandleReactionChanged(AActor* ObserverActor, AActor* TargetActor, EWAYRelationshipState RelationshipState, EWAYReactionState ReactionState)
{
    (void)RelationshipState;

    if (!bAutoApplyBasicReactionBehavior || ObserverActor != GetOwner() || !TargetActor)
    {
        return;
    }

    FString BehaviorDescription = DescribeBasicReactionBehavior(ObserverActor, TargetActor, ReactionState);
    FVector DirectionToTarget = TargetActor->GetActorLocation() - ObserverActor->GetActorLocation();
    DirectionToTarget.Z = 0.0f;

    if (DirectionToTarget.IsNearlyZero())
    {
        DirectionToTarget = ObserverActor->GetActorForwardVector();
        DirectionToTarget.Z = 0.0f;
    }

    const FVector SafeDirection = DirectionToTarget.GetSafeNormal();
    const float DistanceToTarget = DirectionToTarget.Size();

    ApplyReactionFacing(ObserverActor, SafeDirection, ReactionState, DistanceToTarget);
    EWAYBehaviorExecutionMode ExecutionMode = EWAYBehaviorExecutionMode::FacingOnly;
    TryApplyMovementReaction(TargetActor, ReactionState, SafeDirection, BehaviorDescription, &ExecutionMode);
    CachedBehaviorExecutionModes.Add(TargetActor, ExecutionMode);
    CacheBehaviorExecutionResult(
        TargetActor,
        GetVisibleBehaviorLabel(ResolveRecommendedBehaviorForReactionState(ReactionState), ReactionState),
        BehaviorDescription,
        ExecutionMode);
    UpdateAnimationHookState(
        TargetActor,
        ReactionState,
        ResolveRecommendedBehaviorForReactionState(ReactionState),
        ExecutionMode,
        true,
        true,
        BehaviorDescription);

    UE_LOG(
        LogWanaWorksWAY,
        Log,
        TEXT("Applied WanaAI automatic reaction behavior. Observer=%s Target=%s Reaction=%s Behavior=%s"),
        *ObserverActor->GetActorNameOrLabel(),
        *TargetActor->GetActorNameOrLabel(),
        *GetReactionStateLogLabel(ReactionState),
        *BehaviorDescription);
}

void UWAYPlayerProfileComponent::UpdateAnimationHookState(
    AActor* TargetActor,
    EWAYReactionState ReactionState,
    EWAYBehaviorPreset RecommendedBehavior,
    EWAYBehaviorExecutionMode ExecutionMode,
    bool bFacingHookRequested,
    bool bTurnToTargetRequested,
    const FString& Detail)
{
    bool bHasSkeletalMeshComponent = false;
    bool bHasAnimBlueprint = false;
    InspectAnimationHookAvailability(GetOwner(), bHasSkeletalMeshComponent, bHasAnimBlueprint);

    const FWanaMovementReadiness MovementReadiness = TargetActor
        ? FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(GetOwner(), TargetActor)
        : FWanaMovementReadiness();

    CurrentAnimationHookState = FWAYAnimationHookState();
    CurrentAnimationHookState.bHookStateValid = TargetActor != nullptr;
    CurrentAnimationHookState.TargetActor = TargetActor;
    CurrentAnimationHookState.TargetWorldLocation = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
    CurrentAnimationHookState.TargetDistance = TargetActor ? MovementReadiness.DistanceToTarget : 0.0f;
    CurrentAnimationHookState.bFacingHookRequested = TargetActor != nullptr && bFacingHookRequested;
    CurrentAnimationHookState.bTurnToTargetRequested = TargetActor != nullptr && bTurnToTargetRequested && bHasAnimBlueprint;
    CurrentAnimationHookState.bLocomotionSafeExecutionHint = ExecutionMode == EWAYBehaviorExecutionMode::MovementAllowed;
    CurrentAnimationHookState.ReactionState = ReactionState;
    CurrentAnimationHookState.RecommendedBehavior = RecommendedBehavior;
    CurrentAnimationHookState.ExecutionMode = ExecutionMode;

    if (!bHasSkeletalMeshComponent)
    {
        CurrentAnimationHookState.ApplicationStatus = EWAYAnimationHookApplicationStatus::NotAvailable;
        CurrentAnimationHookState.Detail = TEXT("Animation hook application is not available because the subject has no skeletal animation stack.");
    }
    else if (!bHasAnimBlueprint)
    {
        CurrentAnimationHookState.ApplicationStatus = EWAYAnimationHookApplicationStatus::Limited;
        CurrentAnimationHookState.Detail = TEXT("Animation hook application is limited because a skeletal mesh exists but no Animation Blueprint is currently assigned.");
    }
    else if (!TargetActor)
    {
        CurrentAnimationHookState.ApplicationStatus = EWAYAnimationHookApplicationStatus::Limited;
        CurrentAnimationHookState.Detail = TEXT("Animation hook application is ready, but no current target is driving it yet.");
    }
    else
    {
        CurrentAnimationHookState.ApplicationStatus = EWAYAnimationHookApplicationStatus::Active;

        if (ExecutionMode == EWAYBehaviorExecutionMode::MovementAllowed)
        {
            CurrentAnimationHookState.Detail = TEXT("Animation hook application is active with locomotion-safe execution hints.");
        }
        else if (ExecutionMode == EWAYBehaviorExecutionMode::FallbackActive)
        {
            CurrentAnimationHookState.Detail = TEXT("Animation hook application is active with fallback movement hints because locomotion-compatible execution is limited.");
        }
        else
        {
            CurrentAnimationHookState.Detail = TEXT("Animation hook application is active in facing and turn-to-target mode.");
        }
    }

    const FString TrimmedDetail = Detail.TrimStartAndEnd();

    if (!TrimmedDetail.IsEmpty())
    {
        CurrentAnimationHookState.Detail += TEXT(" ");
        CurrentAnimationHookState.Detail += TrimmedDetail;
    }

    if (AActor* ObserverActor = GetOwner())
    {
        UE_LOG(
            LogWanaWorksWAY,
            Verbose,
            TEXT("Updated WanaAI animation hook state. Observer=%s Target=%s Status=%s Reaction=%s Behavior=%s Detail=%s"),
            *ObserverActor->GetActorNameOrLabel(),
            TargetActor ? *TargetActor->GetActorNameOrLabel() : TEXT("(none)"),
            *GetAnimationHookApplicationStatusLogLabel(CurrentAnimationHookState.ApplicationStatus),
            *GetReactionStateLogLabel(CurrentAnimationHookState.ReactionState),
            *GetBehaviorPresetLogLabel(CurrentAnimationHookState.RecommendedBehavior),
            *CurrentAnimationHookState.Detail);
    }
}

void UWAYPlayerProfileComponent::CacheBehaviorExecutionResult(AActor* TargetActor, const FString& VisibleBehaviorLabel, const FString& Detail, EWAYBehaviorExecutionMode ExecutionMode)
{
    if (!TargetActor)
    {
        return;
    }

    CachedVisibleBehaviorLabels.Add(TargetActor, VisibleBehaviorLabel.IsEmpty() ? TEXT("None") : VisibleBehaviorLabel);
    CachedBehaviorExecutionDetails.Add(TargetActor, Detail.TrimStartAndEnd());
    CachedBehaviorExecutionModes.Add(TargetActor, ExecutionMode);
}

bool UWAYPlayerProfileComponent::TryApplyMovementReaction(AActor* TargetActor, EWAYReactionState ReactionState, const FVector& SafeDirection, FString& OutBehaviorDescription, EWAYBehaviorExecutionMode* OutExecutionMode)
{
    if (OutExecutionMode)
    {
        *OutExecutionMode = EWAYBehaviorExecutionMode::FacingOnly;
    }

    if (!IsMovementDrivenReaction(ReactionState))
    {
        StopBasicReactionMovement();
        return false;
    }

    AActor* ObserverActor = GetOwner();
    const FWanaMovementReadiness MovementReadiness = FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(ObserverActor, TargetActor);

    if (ReactionState == EWAYReactionState::Protective && MovementReadiness.DistanceToTarget <= BasicReactionProtectiveRange)
    {
        StopBasicReactionMovement();
        OutBehaviorDescription += TEXT(" Movement held because the observer is already within a clear protective range, so the guard stance stays readable.");
        return false;
    }

    if (!MovementReadiness.bCanAttemptMovement)
    {
        StopBasicReactionMovement();
        OutBehaviorDescription += FString::Printf(TEXT(" Movement blocked. %s Falling back to a stronger facing and hold response."), *MovementReadiness.Detail);
        return false;
    }

    const float MoveInputScale = ReactionState == EWAYReactionState::Cautious
        ? BasicReactionRetreatMoveInputScale
        : BasicReactionMoveInputScale;

    if (StartBasicReactionMovement(TargetActor, ReactionState, OutBehaviorDescription, MoveInputScale))
    {
        if (OutExecutionMode)
        {
            *OutExecutionMode = EWAYBehaviorExecutionMode::MovementAllowed;
        }

        OutBehaviorDescription += FString::Printf(TEXT(" Movement-compatible execution was used. %s"), *MovementReadiness.Detail);
        return true;
    }

    if (MovementReadiness.bSupportsDirectActorMove && ObserverActor)
    {
        const FVector FallbackOffset = ReactionState == EWAYReactionState::Hostile
            ? SafeDirection * GetFallbackMoveDistance(ReactionState, MovementReadiness.DistanceToTarget)
            : ReactionState == EWAYReactionState::Cautious
                ? -SafeDirection * GetFallbackMoveDistance(ReactionState, MovementReadiness.DistanceToTarget)
                : SafeDirection * GetFallbackMoveDistance(ReactionState, MovementReadiness.DistanceToTarget);

        if (TryMoveActor(ObserverActor, FallbackOffset))
        {
            if (OutExecutionMode)
            {
                *OutExecutionMode = EWAYBehaviorExecutionMode::FallbackActive;
            }

            OutBehaviorDescription = ReactionState == EWAYReactionState::Hostile
                ? TEXT("Locked attention on the target and advanced with a deliberate safe actor fallback.")
                : ReactionState == EWAYReactionState::Cautious
                    ? TEXT("Held attention on the target and backed away with a readable safe actor fallback.")
                    : TEXT("Closed distance with a safe actor fallback, then settled into a clearer guard-ready stance.");
            OutBehaviorDescription += FString::Printf(TEXT(" Fallback movement was used. %s"), *MovementReadiness.Detail);
            return true;
        }
    }

    StopBasicReactionMovement();
    OutBehaviorDescription += TEXT(" Movement was requested, but no locomotion-compatible path could be applied, so the actor stayed in a stronger facing-only response.");
    return false;
}

bool UWAYPlayerProfileComponent::StartBasicReactionMovement(AActor* TargetActor, EWAYReactionState ReactionState, FString& OutBehaviorDescription, float MoveInputScale, float MovePulseDurationOverride)
{
    APawn* ObserverPawn = Cast<APawn>(GetOwner());

    if (!ObserverPawn || !TargetActor)
    {
        return false;
    }

    if (ReactionState != EWAYReactionState::Hostile
        && ReactionState != EWAYReactionState::Cooperative
        && ReactionState != EWAYReactionState::Protective
        && ReactionState != EWAYReactionState::Cautious)
    {
        StopBasicReactionMovement();
        return false;
    }

    const FWanaMovementReadiness MovementReadiness = FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(GetOwner(), TargetActor);

    if (!MovementReadiness.bSupportsLocomotionPulse)
    {
        OutBehaviorDescription += FString::Printf(
            TEXT(" Locomotion-compatible starter movement is not available. %s"),
            *MovementReadiness.Detail);
        StopBasicReactionMovement();
        return false;
    }

    UPawnMovementComponent* MovementComponent = ObserverPawn->GetMovementComponent();

    if (!MovementComponent)
    {
        OutBehaviorDescription += TEXT(" Movement input was unavailable because the observer has no pawn movement component.");
        return false;
    }

    if (ACharacter* ObserverCharacter = Cast<ACharacter>(ObserverPawn))
    {
        if (UCharacterMovementComponent* CharacterMovement = ObserverCharacter->GetCharacterMovement())
        {
            if (!ObserverCharacter->GetController() && !CharacterMovement->bRunPhysicsWithNoController)
            {
                bPreviousRunPhysicsWithNoController = CharacterMovement->bRunPhysicsWithNoController;
                CharacterMovement->bRunPhysicsWithNoController = true;
                bRestoresRunPhysicsWithNoController = true;
                OutBehaviorDescription += TEXT(" Enabled no-controller movement temporarily for compatibility.");
            }
        }
    }

    ActiveBasicReactionTarget = TargetActor;
    ActiveBasicReactionState = ReactionState;
    ActiveBasicReactionMoveInputScale = FMath::Clamp(MoveInputScale, 0.2f, 1.25f);
    ActiveBasicReactionTimeRemaining = MovePulseDurationOverride > 0.0f
        ? MovePulseDurationOverride
        : (ReactionState == EWAYReactionState::Protective
            ? BasicReactionProtectiveMovePulseDuration
            : ReactionState == EWAYReactionState::Cooperative
                ? BasicReactionFollowMovePulseDuration
                : BasicReactionMovePulseDuration);
    SetComponentTickEnabled(true);
    return true;
}

void UWAYPlayerProfileComponent::StopBasicReactionMovement()
{
    if (bRestoresRunPhysicsWithNoController)
    {
        if (ACharacter* ObserverCharacter = Cast<ACharacter>(GetOwner()))
        {
            if (UCharacterMovementComponent* CharacterMovement = ObserverCharacter->GetCharacterMovement())
            {
                CharacterMovement->bRunPhysicsWithNoController = bPreviousRunPhysicsWithNoController;
            }
        }
    }

    bRestoresRunPhysicsWithNoController = false;
    bPreviousRunPhysicsWithNoController = false;
    ActiveBasicReactionTarget.Reset();
    ActiveBasicReactionState = EWAYReactionState::Observational;
    ActiveBasicReactionTimeRemaining = 0.0f;
    ActiveBasicReactionMoveInputScale = BasicReactionMoveInputScale;
    SetComponentTickEnabled(false);
}

int32 UWAYPlayerProfileComponent::FindRelationshipProfileIndex(AActor* TargetActor) const
{
    return RelationshipProfiles.IndexOfByPredicate([TargetActor](const FWAYRelationshipProfile& Profile)
    {
        return Profile.TargetActor == TargetActor;
    });
}

FWAYRelationshipSeed UWAYPlayerProfileComponent::MakeRelationshipSeedForTarget(AActor* TargetActor) const
{
    if (TargetActor)
    {
        if (const UWanaIdentityComponent* IdentityComponent = TargetActor->FindComponentByClass<UWanaIdentityComponent>())
        {
            return IdentityComponent->DefaultRelationshipSeed;
        }
    }

    return FWAYRelationshipSeed();
}

FWAYRelationshipProfile UWAYPlayerProfileComponent::CreateRelationshipProfile(AActor* TargetActor) const
{
    const FWAYRelationshipSeed Seed = MakeRelationshipSeedForTarget(TargetActor);

    FWAYRelationshipProfile Profile;
    Profile.TargetActor = TargetActor;
    Profile.RelationshipState = Seed.RelationshipState;
    Profile.Trust = ClampRelationshipValue(Seed.Trust);
    Profile.Fear = ClampRelationshipValue(Seed.Fear);
    Profile.Respect = ClampRelationshipValue(Seed.Respect);
    Profile.Attachment = ClampRelationshipValue(Seed.Attachment);
    Profile.Hostility = ClampRelationshipValue(Seed.Hostility);
    return Profile;
}

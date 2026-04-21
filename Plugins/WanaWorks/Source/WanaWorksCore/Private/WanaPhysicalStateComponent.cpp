#include "WanaPhysicalStateComponent.h"

#include "GameFramework/Actor.h"

UWanaPhysicalStateComponent::UWanaPhysicalStateComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.SetTickFunctionEnable(false);
}

void UWanaPhysicalStateComponent::BeginPlay()
{
    Super::BeginPlay();
    UpdateDerivedFlags();
}

void UWanaPhysicalStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bNeedsRecovery)
    {
        SetComponentTickEnabled(false);
        return;
    }

    const float SafeRecoveryDuration = FMath::Max(RecoveryDuration, 0.1f);
    const float RecoveryStep = DeltaTime / SafeRecoveryDuration;
    const float RecoveryTargetStability = bBracing ? 0.75f : 0.9f;

    RecoveryProgress = FMath::Clamp(RecoveryProgress + RecoveryStep, 0.0f, 1.0f);
    StabilityScore = FMath::Clamp(StabilityScore + (RecoveryStep * 0.65f), 0.0f, RecoveryTargetStability);
    FearInfluence = FMath::Clamp(FearInfluence - (RecoveryStep * 0.35f), 0.0f, 1.0f);

    UpdateDerivedFlags();

    if (RecoveryProgress >= 1.0f - KINDA_SMALL_NUMBER)
    {
        FinishRecovery();
    }
}

void UWanaPhysicalStateComponent::SetPhysicalState(EWanaPhysicalState NewPhysicalState)
{
    if (NewPhysicalState == EWanaPhysicalState::Recovering)
    {
        StartRecovery();
        return;
    }

    const EWanaPhysicalState PreviousState = PhysicalState;
    const bool bWasRecovering = bNeedsRecovery;

    PhysicalState = NewPhysicalState;
    bNeedsRecovery = false;

    switch (NewPhysicalState)
    {
    case EWanaPhysicalState::Stable:
        StabilityScore = FMath::Max(StabilityScore, 0.9f);
        RecoveryProgress = 1.0f;
        bBracing = false;
        FearInfluence = FMath::Clamp(FearInfluence, 0.0f, 0.2f);
        break;

    case EWanaPhysicalState::Alert:
        StabilityScore = FMath::Max(StabilityScore, 0.75f);
        RecoveryProgress = 1.0f;
        bBracing = false;
        FearInfluence = FMath::Max(FearInfluence, 0.25f);
        break;

    case EWanaPhysicalState::Staggered:
        StabilityScore = FMath::Min(StabilityScore, 0.45f);
        RecoveryProgress = FMath::Min(RecoveryProgress, 0.35f);
        bBracing = false;
        FearInfluence = FMath::Max(FearInfluence, 0.3f);
        break;

    case EWanaPhysicalState::OffBalance:
        StabilityScore = FMath::Min(StabilityScore, 0.3f);
        RecoveryProgress = FMath::Min(RecoveryProgress, 0.2f);
        bBracing = false;
        FearInfluence = FMath::Max(FearInfluence, 0.4f);
        break;

    case EWanaPhysicalState::Bracing:
        StabilityScore = FMath::Max(StabilityScore, 0.65f);
        RecoveryProgress = FMath::Max(RecoveryProgress, 0.75f);
        bBracing = true;
        FearInfluence = FMath::Max(FearInfluence, 0.15f);
        break;

    case EWanaPhysicalState::Panicked:
        StabilityScore = FMath::Min(StabilityScore, 0.2f);
        RecoveryProgress = FMath::Min(RecoveryProgress, 0.1f);
        bBracing = false;
        FearInfluence = FMath::Max(FearInfluence, 0.85f);
        break;

    default:
        break;
    }

    UpdateDerivedFlags();
    SetComponentTickEnabled(false);
    BroadcastPhysicalStateChanged(PreviousState, PhysicalState);

    if (bWasRecovering)
    {
        OnRecoveryFinished.Broadcast();
    }
}

void UWanaPhysicalStateComponent::EnterAlertState(float AlertFearInfluence)
{
    const EWanaPhysicalState PreviousState = PhysicalState;

    if (!bNeedsRecovery)
    {
        PhysicalState = EWanaPhysicalState::Alert;
    }

    StabilityScore = FMath::Max(StabilityScore, 0.75f);
    RecoveryProgress = 1.0f;
    bBracing = false;
    FearInfluence = FMath::Max(FMath::Clamp(AlertFearInfluence, 0.0f, 1.0f), FearInfluence);

    UpdateDerivedFlags();
    BroadcastPhysicalStateChanged(PreviousState, PhysicalState);
}

void UWanaPhysicalStateComponent::SetBracing(bool bInBracing, float StabilityLift)
{
    const EWanaPhysicalState PreviousState = PhysicalState;

    bBracing = bInBracing;

    if (bBracing)
    {
        StabilityScore = FMath::Clamp(FMath::Max(StabilityScore, 0.6f + FMath::Clamp(StabilityLift, 0.0f, 0.3f)), 0.0f, 1.0f);
        FearInfluence = FMath::Max(FearInfluence, 0.15f);

        if (!bNeedsRecovery)
        {
            PhysicalState = EWanaPhysicalState::Bracing;
        }
    }
    else if (!bNeedsRecovery && PhysicalState == EWanaPhysicalState::Bracing)
    {
        PhysicalState = FearInfluence > 0.25f ? EWanaPhysicalState::Alert : EWanaPhysicalState::Stable;
    }

    UpdateDerivedFlags();
    BroadcastPhysicalStateChanged(PreviousState, PhysicalState);
}

void UWanaPhysicalStateComponent::StartRecovery(float RecoveryDurationOverride)
{
    if (RecoveryDurationOverride > 0.0f)
    {
        RecoveryDuration = FMath::Max(RecoveryDurationOverride, 0.1f);
    }

    const EWanaPhysicalState PreviousState = PhysicalState;
    const bool bWasRecovering = bNeedsRecovery;

    PhysicalState = EWanaPhysicalState::Recovering;
    bNeedsRecovery = true;
    RecoveryProgress = 0.0f;
    StabilityScore = FMath::Clamp(FMath::Min(StabilityScore, bBracing ? 0.55f : 0.4f), 0.0f, 1.0f);
    FearInfluence = FMath::Max(FearInfluence, 0.2f);

    UpdateDerivedFlags();
    SetComponentTickEnabled(true);
    BroadcastPhysicalStateChanged(PreviousState, PhysicalState);

    if (!bWasRecovering)
    {
        OnRecoveryStarted.Broadcast();
    }
}

void UWanaPhysicalStateComponent::FinishRecovery()
{
    const EWanaPhysicalState PreviousState = PhysicalState;
    const bool bWasRecovering = bNeedsRecovery;

    bNeedsRecovery = false;
    RecoveryProgress = 1.0f;
    StabilityScore = FMath::Clamp(FMath::Max(StabilityScore, bBracing ? 0.75f : 0.9f), 0.0f, 1.0f);
    FearInfluence = FMath::Clamp(FearInfluence, 0.0f, bBracing ? 0.25f : 0.1f);
    PhysicalState = bBracing ? EWanaPhysicalState::Bracing : EWanaPhysicalState::Stable;

    UpdateDerivedFlags();
    SetComponentTickEnabled(false);
    BroadcastPhysicalStateChanged(PreviousState, PhysicalState);

    if (bWasRecovering)
    {
        OnRecoveryFinished.Broadcast();
    }
}

void UWanaPhysicalStateComponent::ApplyImpactHint(FVector ImpactDirection, float ImpactStrength, bool bAutoStartRecovery)
{
    AActor* OwnerActor = GetOwner();
    const FVector FallbackDirection = OwnerActor ? OwnerActor->GetActorForwardVector() : FVector::ForwardVector;

    LastImpactDirection = ImpactDirection.IsNearlyZero() ? FallbackDirection.GetSafeNormal() : ImpactDirection.GetSafeNormal();
    LastImpactStrength = FMath::Max(ImpactStrength, 0.0f);

    const float StabilityPenalty = LastImpactStrength * (bBracing ? 0.12f : 0.22f);
    StabilityScore = FMath::Clamp(StabilityScore - StabilityPenalty, 0.0f, 1.0f);
    FearInfluence = FMath::Clamp(FearInfluence + (LastImpactStrength * 0.25f), 0.0f, 1.0f);

    if (bAutoStartRecovery && (LastImpactStrength >= 0.55f || StabilityScore <= 0.35f))
    {
        StartRecovery();
        return;
    }

    if (LastImpactStrength >= 0.15f)
    {
        EnterAlertState(FMath::Max(FearInfluence, 0.25f));
        return;
    }

    UpdateDerivedFlags();
}

void UWanaPhysicalStateComponent::SetFearInfluence(float NewFearInfluence)
{
    FearInfluence = FMath::Clamp(NewFearInfluence, 0.0f, 1.0f);
    UpdateDerivedFlags();
}

void UWanaPhysicalStateComponent::RefreshPhysicalReadiness()
{
    UpdateDerivedFlags();
}

void UWanaPhysicalStateComponent::BroadcastPhysicalStateChanged(EWanaPhysicalState PreviousState, EWanaPhysicalState NewState)
{
    if (PreviousState != NewState)
    {
        OnPhysicalStateChanged.Broadcast(PreviousState, NewState);
    }
}

void UWanaPhysicalStateComponent::UpdateDerivedFlags()
{
    StabilityScore = FMath::Clamp(StabilityScore, 0.0f, 1.0f);
    RecoveryProgress = FMath::Clamp(RecoveryProgress, 0.0f, 1.0f);
    FearInfluence = FMath::Clamp(FearInfluence, 0.0f, 1.0f);

    const float EffectiveStability = FMath::Clamp(StabilityScore - (FearInfluence * 0.15f), 0.0f, 1.0f);
    bCanCommitToMovement = !bNeedsRecovery
        && !bBracing
        && PhysicalState != EWanaPhysicalState::Panicked
        && EffectiveStability >= MovementCommitThreshold;
    bCanCommitToAttack = !bNeedsRecovery
        && PhysicalState != EWanaPhysicalState::Panicked
        && EffectiveStability >= AttackCommitThreshold
        && FearInfluence < 0.8f;
}

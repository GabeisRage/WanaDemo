#include "WanaPhysicalStateComponent.h"

#include "GameFramework/Actor.h"

namespace
{
constexpr float LightImpactThreshold = 0.18f;
constexpr float MediumImpactThreshold = 0.42f;
constexpr float StrongImpactThreshold = 0.72f;
constexpr float SevereImpactThreshold = 0.95f;
}

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

    const float SafeRecoveryDuration = FMath::Max(ActiveRecoveryDuration, 0.1f);
    const float RecoveryStep = DeltaTime / SafeRecoveryDuration;
    const float RecoveryDifficulty = FMath::Clamp(ActiveRecoverySeverity, 0.0f, 1.0f);
    const float StabilityGainScale = bBracing
        ? FMath::Lerp(0.95f, 0.55f, RecoveryDifficulty)
        : FMath::Lerp(0.8f, 0.4f, RecoveryDifficulty);
    const float FearDecayScale = bBracing
        ? FMath::Lerp(0.55f, 0.3f, RecoveryDifficulty)
        : FMath::Lerp(0.4f, 0.2f, RecoveryDifficulty);
    const float ImpactDecayScale = bBracing
        ? FMath::Lerp(0.35f, 0.65f, RecoveryDifficulty)
        : FMath::Lerp(0.25f, 0.55f, RecoveryDifficulty);

    RecoveryProgress = FMath::Clamp(RecoveryProgress + RecoveryStep, 0.0f, 1.0f);
    StabilityScore = FMath::Clamp(StabilityScore + (RecoveryStep * StabilityGainScale), 0.0f, RecoveryTargetStability);
    FearInfluence = FMath::Clamp(FearInfluence - (RecoveryStep * FearDecayScale), 0.0f, 1.0f);
    LastImpactStrength = FMath::Clamp(LastImpactStrength - (RecoveryStep * ImpactDecayScale), 0.0f, 1.0f);

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
        InstabilityAlpha = 0.0f;
        bBracing = false;
        FearInfluence = FMath::Clamp(FearInfluence, 0.0f, 0.2f);
        ActiveRecoverySeverity = 0.0f;
        RecoveryTargetStability = 0.9f;
        break;

    case EWanaPhysicalState::Alert:
        StabilityScore = FMath::Max(StabilityScore, 0.75f);
        RecoveryProgress = 1.0f;
        InstabilityAlpha = FMath::Max(InstabilityAlpha, 0.2f);
        bBracing = false;
        FearInfluence = FMath::Max(FearInfluence, 0.25f);
        ActiveRecoverySeverity = 0.15f;
        RecoveryTargetStability = 0.8f;
        break;

    case EWanaPhysicalState::Staggered:
        StabilityScore = FMath::Min(StabilityScore, 0.45f);
        RecoveryProgress = FMath::Min(RecoveryProgress, 0.35f);
        InstabilityAlpha = FMath::Max(InstabilityAlpha, 0.6f);
        bBracing = false;
        FearInfluence = FMath::Max(FearInfluence, 0.3f);
        ActiveRecoverySeverity = 0.45f;
        RecoveryTargetStability = 0.72f;
        break;

    case EWanaPhysicalState::OffBalance:
        StabilityScore = FMath::Min(StabilityScore, 0.3f);
        RecoveryProgress = FMath::Min(RecoveryProgress, 0.2f);
        InstabilityAlpha = FMath::Max(InstabilityAlpha, 0.8f);
        bBracing = false;
        FearInfluence = FMath::Max(FearInfluence, 0.4f);
        ActiveRecoverySeverity = 0.7f;
        RecoveryTargetStability = 0.68f;
        break;

    case EWanaPhysicalState::Bracing:
        StabilityScore = FMath::Max(StabilityScore, 0.65f);
        RecoveryProgress = FMath::Max(RecoveryProgress, 0.75f);
        InstabilityAlpha = FMath::Clamp(InstabilityAlpha, 0.0f, 0.35f);
        bBracing = true;
        FearInfluence = FMath::Max(FearInfluence, 0.15f);
        ActiveRecoverySeverity = 0.2f;
        RecoveryTargetStability = 0.78f;
        break;

    case EWanaPhysicalState::Panicked:
        StabilityScore = FMath::Min(StabilityScore, 0.2f);
        RecoveryProgress = FMath::Min(RecoveryProgress, 0.1f);
        InstabilityAlpha = FMath::Max(InstabilityAlpha, 1.0f);
        bBracing = false;
        FearInfluence = FMath::Max(FearInfluence, 0.85f);
        ActiveRecoverySeverity = 1.0f;
        RecoveryTargetStability = 0.62f;
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
    InstabilityAlpha = FMath::Max(InstabilityAlpha, 0.2f);
    bBracing = false;
    FearInfluence = FMath::Max(FMath::Clamp(AlertFearInfluence, 0.0f, 1.0f), FearInfluence);
    ActiveRecoverySeverity = FMath::Max(ActiveRecoverySeverity, 0.15f);
    RecoveryTargetStability = FMath::Max(RecoveryTargetStability, 0.8f);

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
        InstabilityAlpha = FMath::Clamp(InstabilityAlpha - 0.15f, 0.0f, 1.0f);
        FearInfluence = FMath::Max(FearInfluence, 0.15f);
        RecoveryTargetStability = FMath::Max(RecoveryTargetStability, 0.78f);

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
        ActiveRecoveryDuration = FMath::Max(RecoveryDurationOverride, 0.1f);
    }
    else
    {
        ActiveRecoveryDuration = FMath::Max(RecoveryDuration, 0.1f);
    }

    const EWanaPhysicalState PreviousState = PhysicalState;
    const bool bWasRecovering = bNeedsRecovery;
    const bool bPreserveDisruptedState = PhysicalState == EWanaPhysicalState::Staggered
        || PhysicalState == EWanaPhysicalState::OffBalance
        || PhysicalState == EWanaPhysicalState::Panicked;

    PhysicalState = bPreserveDisruptedState ? PhysicalState : EWanaPhysicalState::Recovering;
    bNeedsRecovery = true;
    RecoveryProgress = 0.0f;
    StabilityScore = FMath::Clamp(FMath::Min(StabilityScore, bBracing ? 0.58f : 0.38f), 0.0f, 1.0f);
    InstabilityAlpha = FMath::Clamp(FMath::Max(InstabilityAlpha, 1.0f - StabilityScore), 0.0f, 1.0f);
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
    StabilityScore = FMath::Clamp(FMath::Max(StabilityScore, bBracing ? 0.78f : 0.9f), 0.0f, 1.0f);
    InstabilityAlpha = FMath::Clamp(FMath::Min(InstabilityAlpha, bBracing ? 0.2f : 0.1f), 0.0f, 1.0f);
    LastImpactStrength = FMath::Min(LastImpactStrength, 0.15f);
    FearInfluence = FMath::Clamp(FearInfluence, 0.0f, bBracing ? 0.25f : 0.1f);
    PhysicalState = bBracing
        ? EWanaPhysicalState::Bracing
        : (FearInfluence > 0.25f ? EWanaPhysicalState::Alert : EWanaPhysicalState::Stable);
    ActiveRecoverySeverity = 0.0f;
    ActiveRecoveryDuration = FMath::Max(RecoveryDuration, 0.1f);
    RecoveryTargetStability = bBracing ? 0.78f : 0.9f;

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
    const FVector OwnerForward = OwnerActor ? OwnerActor->GetActorForwardVector().GetSafeNormal() : FVector::ForwardVector;
    const FVector OwnerRight = OwnerActor ? OwnerActor->GetActorRightVector().GetSafeNormal() : FVector::RightVector;
    const float PreviousStability = StabilityScore;
    const bool bWasAlreadyUnstable = bNeedsRecovery || PreviousStability <= 0.45f || InstabilityAlpha >= 0.55f;

    LastImpactDirection = ImpactDirection.IsNearlyZero() ? FallbackDirection.GetSafeNormal() : ImpactDirection.GetSafeNormal();
    LastImpactStrength = FMath::Max(ImpactStrength, 0.0f);
    const float FacingImpactAlignment = FMath::Clamp(-FVector::DotProduct(LastImpactDirection, OwnerForward), 0.0f, 1.0f);
    const float RearImpactExposure = FMath::Clamp(FVector::DotProduct(LastImpactDirection, OwnerForward), 0.0f, 1.0f);
    const float LateralImpactBias = FMath::Abs(FVector::DotProduct(LastImpactDirection, OwnerRight));
    const float DirectionalDisruption = (RearImpactExposure * 0.18f) + (LateralImpactBias * 0.12f);
    const float BracingMitigation = bBracing ? FMath::Lerp(0.05f, 0.2f, FacingImpactAlignment) : 0.0f;
    const float EffectiveImpactStrength = FMath::Clamp(LastImpactStrength + DirectionalDisruption - BracingMitigation, 0.0f, 1.15f);
    const float StabilityPenalty = EffectiveImpactStrength * (bBracing ? 0.18f : 0.3f);

    LastImpactStrength = EffectiveImpactStrength;
    StabilityScore = FMath::Clamp(PreviousStability - StabilityPenalty, 0.0f, 1.0f);
    FearInfluence = FMath::Clamp(FearInfluence + (EffectiveImpactStrength * (bWasAlreadyUnstable ? 0.32f : 0.22f)), 0.0f, 1.0f);
    InstabilityAlpha = FMath::Clamp(FMath::Max(InstabilityAlpha, 1.0f - StabilityScore), 0.0f, 1.0f);

    EWanaPhysicalState NextState = PhysicalState;

    if (EffectiveImpactStrength < LightImpactThreshold)
    {
        NextState = bBracing ? EWanaPhysicalState::Bracing : EWanaPhysicalState::Alert;
    }
    else if (EffectiveImpactStrength < MediumImpactThreshold)
    {
        NextState = bBracing ? EWanaPhysicalState::Bracing : EWanaPhysicalState::Alert;
    }
    else if (EffectiveImpactStrength < StrongImpactThreshold)
    {
        NextState = EWanaPhysicalState::Staggered;
    }
    else if (EffectiveImpactStrength < SevereImpactThreshold)
    {
        NextState = EWanaPhysicalState::OffBalance;
    }
    else
    {
        NextState = bWasAlreadyUnstable ? EWanaPhysicalState::Panicked : EWanaPhysicalState::OffBalance;
    }

    const float RecoveryNeedStrength = FMath::Clamp(
        EffectiveImpactStrength
        + ((1.0f - PreviousStability) * 0.5f)
        + (bWasAlreadyUnstable ? 0.2f : 0.0f)
        - (bBracing ? 0.12f : 0.0f),
        0.0f,
        1.0f);

    ActiveRecoverySeverity = RecoveryNeedStrength;
    ActiveRecoveryDuration = FMath::Clamp(
        RecoveryDuration * FMath::Lerp(0.75f, 1.85f, RecoveryNeedStrength) * (bBracing ? 0.8f : 1.0f),
        0.2f,
        4.0f);
    RecoveryTargetStability = bBracing
        ? FMath::Lerp(0.72f, 0.82f, 1.0f - RecoveryNeedStrength)
        : FMath::Lerp(0.62f, 0.88f, 1.0f - RecoveryNeedStrength);

    const EWanaPhysicalState PreviousState = PhysicalState;

    switch (NextState)
    {
    case EWanaPhysicalState::Bracing:
        PhysicalState = EWanaPhysicalState::Bracing;
        RecoveryProgress = 1.0f;
        InstabilityAlpha = FMath::Clamp(FMath::Max(InstabilityAlpha, 0.15f), 0.0f, 1.0f);
        break;

    case EWanaPhysicalState::Alert:
        PhysicalState = EWanaPhysicalState::Alert;
        RecoveryProgress = 1.0f;
        InstabilityAlpha = FMath::Clamp(FMath::Max(InstabilityAlpha, 0.2f), 0.0f, 1.0f);
        bBracing = false;
        break;

    case EWanaPhysicalState::Staggered:
        PhysicalState = EWanaPhysicalState::Staggered;
        RecoveryProgress = FMath::Min(RecoveryProgress, 0.3f);
        InstabilityAlpha = FMath::Clamp(FMath::Max(InstabilityAlpha, 0.65f), 0.0f, 1.0f);
        bBracing = false;
        break;

    case EWanaPhysicalState::OffBalance:
        PhysicalState = EWanaPhysicalState::OffBalance;
        RecoveryProgress = FMath::Min(RecoveryProgress, 0.15f);
        InstabilityAlpha = FMath::Clamp(FMath::Max(InstabilityAlpha, 0.82f), 0.0f, 1.0f);
        bBracing = false;
        break;

    case EWanaPhysicalState::Panicked:
        PhysicalState = EWanaPhysicalState::Panicked;
        RecoveryProgress = FMath::Min(RecoveryProgress, 0.05f);
        InstabilityAlpha = 1.0f;
        bBracing = false;
        break;

    default:
        break;
    }

    UpdateDerivedFlags();
    BroadcastPhysicalStateChanged(PreviousState, PhysicalState);

    const bool bShouldRecover = bAutoStartRecovery && (RecoveryNeedStrength >= 0.3f || PhysicalState == EWanaPhysicalState::Staggered || PhysicalState == EWanaPhysicalState::OffBalance || PhysicalState == EWanaPhysicalState::Panicked);

    if (bShouldRecover)
    {
        StartRecovery(ActiveRecoveryDuration);
    }
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
    const float RecoveryDisruption = bNeedsRecovery ? (1.0f - RecoveryProgress) * 0.45f : 0.0f;
    const float ImpactDisruption = LastImpactStrength * (bNeedsRecovery ? (0.45f + ((1.0f - RecoveryProgress) * 0.25f)) : 0.2f);
    InstabilityAlpha = FMath::Clamp(FMath::Max(FMath::Max(1.0f - EffectiveStability, RecoveryDisruption), ImpactDisruption), 0.0f, 1.0f);
    bCanCommitToMovement = !bNeedsRecovery
        && !bBracing
        && PhysicalState != EWanaPhysicalState::Panicked
        && EffectiveStability >= MovementCommitThreshold;
    bCanCommitToAttack = !bNeedsRecovery
        && PhysicalState != EWanaPhysicalState::Panicked
        && EffectiveStability >= AttackCommitThreshold
        && FearInfluence < 0.8f;
}

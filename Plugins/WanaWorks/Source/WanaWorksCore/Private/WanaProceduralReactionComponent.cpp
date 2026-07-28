#include "WanaProceduralReactionComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "WanaPhysicalStateComponent.h"

namespace
{
constexpr float ReactionSettleThresholdDegrees = 0.05f;
constexpr float ReactionSettleThresholdUnits = 0.05f;
}

UWanaProceduralReactionComponent::UWanaProceduralReactionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    // Match UWanaPhysicalStateComponent: tick in editor worlds so the reaction is
    // visible when Test drives impacts on editor subjects, not only in PIE.
    bTickInEditor = true;
}

void UWanaProceduralReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!PhysicalState.IsValid() || !TargetMesh.IsValid())
    {
        ResolveBindings();
        if (!PhysicalState.IsValid() || !TargetMesh.IsValid())
        {
            return;
        }
    }

    UWanaPhysicalStateComponent* State = PhysicalState.Get();
    USkeletalMeshComponent* Mesh = TargetMesh.Get();
    AActor* Owner = GetOwner();
    if (!State || !Mesh || !Owner)
    {
        return;
    }

    const float InstabilityAlpha = FMath::Clamp(State->InstabilityAlpha, 0.0f, 1.0f);
    const float LeanScale = InstabilityAlpha * ComputeStateLeanScale(State->PhysicalState, State->RecoveryProgress);

    FRotator TargetOffset = FRotator::ZeroRotator;
    float TargetDip = 0.0f;

    if (LeanScale > KINDA_SMALL_NUMBER)
    {
        const FVector WorldImpact = State->LastImpactDirection.GetSafeNormal();
        const FVector LocalImpact = Owner->GetActorTransform().InverseTransformVectorNoScale(WorldImpact);

        // Lean in the direction the impact pushed the body.
        TargetOffset.Pitch = FMath::Clamp(LocalImpact.X, -1.0f, 1.0f) * MaxLeanAngleDegrees * LeanScale;
        TargetOffset.Roll = FMath::Clamp(LocalImpact.Y, -1.0f, 1.0f) * MaxLeanAngleDegrees * LeanScale;
        TargetDip = -MaxInstabilityDip * LeanScale;

        const bool bSwayingState =
            State->PhysicalState == EWanaPhysicalState::Staggered ||
            State->PhysicalState == EWanaPhysicalState::OffBalance ||
            State->PhysicalState == EWanaPhysicalState::Panicked;
        if (bSwayingState)
        {
            SwayTime += DeltaTime * SwayFrequency;
            TargetOffset.Roll += FMath::Sin(SwayTime) * StaggerSwayDegrees * InstabilityAlpha;
            TargetOffset.Pitch += FMath::Sin(SwayTime * 0.63f) * StaggerSwayDegrees * 0.5f * InstabilityAlpha;
        }
    }
    else
    {
        SwayTime = 0.0f;
    }

    const float TargetMagnitude = FMath::Abs(TargetOffset.Pitch) + FMath::Abs(TargetOffset.Roll) + FMath::Abs(TargetDip);
    const float CurrentMagnitude = FMath::Abs(CurrentReactionOffset.Pitch) + FMath::Abs(CurrentReactionOffset.Roll) + FMath::Abs(CurrentDip);
    const float InterpSpeed = TargetMagnitude > CurrentMagnitude ? ReactionInterpSpeed : RecoveryInterpSpeed;
    CurrentReactionOffset = FMath::RInterpTo(CurrentReactionOffset, TargetOffset, DeltaTime, InterpSpeed);
    CurrentDip = FMath::FInterpTo(CurrentDip, TargetDip, DeltaTime, InterpSpeed);

    const bool bSettled =
        FMath::Abs(CurrentReactionOffset.Pitch) < ReactionSettleThresholdDegrees &&
        FMath::Abs(CurrentReactionOffset.Roll) < ReactionSettleThresholdDegrees &&
        FMath::Abs(CurrentReactionOffset.Yaw) < ReactionSettleThresholdDegrees &&
        FMath::Abs(CurrentDip) < ReactionSettleThresholdUnits &&
        LeanScale <= KINDA_SMALL_NUMBER;

    if (bSettled)
    {
        if (bOffsetApplied)
        {
            RestoreBaseTransform();
        }
        // While idle, keep the captured base fresh so user edits to the mesh
        // transform are respected instead of being snapped back to a stale pose.
        CaptureBaseTransform();
        bReactionActive = false;
        return;
    }

    if (!bBaseTransformCaptured)
    {
        CaptureBaseTransform();
    }

    Mesh->SetRelativeRotation(FRotator(
        BaseRelativeRotation.Pitch + CurrentReactionOffset.Pitch,
        BaseRelativeRotation.Yaw + CurrentReactionOffset.Yaw,
        BaseRelativeRotation.Roll + CurrentReactionOffset.Roll));
    Mesh->SetRelativeLocation(BaseRelativeLocation + FVector(0.0f, 0.0f, CurrentDip));
    bOffsetApplied = true;
    bReactionActive = true;
}

void UWanaProceduralReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RestoreBaseTransform();
    Super::EndPlay(EndPlayReason);
}

void UWanaProceduralReactionComponent::OnUnregister()
{
    RestoreBaseTransform();
    Super::OnUnregister();
}

void UWanaProceduralReactionComponent::RefreshReactionBindings()
{
    PhysicalState.Reset();
    TargetMesh.Reset();
    bBaseTransformCaptured = false;
    ResolveBindings();
}

void UWanaProceduralReactionComponent::TriggerTestImpact(FVector ImpactDirection, float ImpactStrength)
{
    if (!PhysicalState.IsValid())
    {
        ResolveBindings();
    }

    if (UWanaPhysicalStateComponent* State = PhysicalState.Get())
    {
        State->ApplyImpactHint(ImpactDirection, ImpactStrength, true);
        SetComponentTickEnabled(true);
    }
}

void UWanaProceduralReactionComponent::ResolveBindings()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    if (!PhysicalState.IsValid())
    {
        PhysicalState = Owner->FindComponentByClass<UWanaPhysicalStateComponent>();
    }

    if (!TargetMesh.IsValid())
    {
        TargetMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
        bBaseTransformCaptured = false;
    }
}

void UWanaProceduralReactionComponent::CaptureBaseTransform()
{
    if (USkeletalMeshComponent* Mesh = TargetMesh.Get())
    {
        BaseRelativeRotation = Mesh->GetRelativeRotation();
        BaseRelativeLocation = Mesh->GetRelativeLocation();
        bBaseTransformCaptured = true;
    }
}

void UWanaProceduralReactionComponent::RestoreBaseTransform()
{
    if (!bOffsetApplied || !bBaseTransformCaptured)
    {
        bOffsetApplied = false;
        return;
    }

    if (USkeletalMeshComponent* Mesh = TargetMesh.Get())
    {
        Mesh->SetRelativeRotation(BaseRelativeRotation);
        Mesh->SetRelativeLocation(BaseRelativeLocation);
    }

    CurrentReactionOffset = FRotator::ZeroRotator;
    CurrentDip = 0.0f;
    bOffsetApplied = false;
    bReactionActive = false;
}

float UWanaProceduralReactionComponent::ComputeStateLeanScale(EWanaPhysicalState State, float RecoveryProgress) const
{
    switch (State)
    {
    case EWanaPhysicalState::Stable:
        return 0.0f;
    case EWanaPhysicalState::Alert:
        return 0.2f;
    case EWanaPhysicalState::Bracing:
        return 0.35f;
    case EWanaPhysicalState::Recovering:
        // Settle out as recovery completes so the arc reads visually.
        return FMath::Lerp(0.6f, 0.0f, FMath::Clamp(RecoveryProgress, 0.0f, 1.0f));
    case EWanaPhysicalState::Panicked:
        return 0.85f;
    case EWanaPhysicalState::Staggered:
    case EWanaPhysicalState::OffBalance:
    default:
        return 1.0f;
    }
}

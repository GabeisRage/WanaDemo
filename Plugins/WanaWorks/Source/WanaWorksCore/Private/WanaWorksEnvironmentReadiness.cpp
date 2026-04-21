#include "WanaWorksEnvironmentReadiness.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
constexpr float LongRangeMovementHintDistance = 3000.0f;
constexpr float MinObstacleProbeDistance = 120.0f;
constexpr float MaxObstacleProbeDistance = 240.0f;
constexpr float MediumObstaclePressureThreshold = 0.34f;
constexpr float HighObstaclePressureThreshold = 0.75f;

FString GetMovementReadinessStatusLabel(EWanaMovementReadinessLevel ReadinessLevel)
{
    switch (ReadinessLevel)
    {
    case EWanaMovementReadinessLevel::Allowed:
        return TEXT("Movement allowed");

    case EWanaMovementReadinessLevel::Limited:
        return TEXT("Movement limited");

    case EWanaMovementReadinessLevel::Unclear:
        return TEXT("Movement unclear");

    case EWanaMovementReadinessLevel::Blocked:
    default:
        return TEXT("Movement blocked");
    }
}

void SetMovementReadinessState(
    FWanaMovementReadiness& Readiness,
    EWanaMovementReadinessLevel ReadinessLevel,
    bool bCanAttemptMovement,
    const FString& Detail)
{
    Readiness.ReadinessLevel = ReadinessLevel;
    Readiness.bCanAttemptMovement = bCanAttemptMovement;
    Readiness.Status = GetMovementReadinessStatusLabel(ReadinessLevel);
    Readiness.Detail = Detail;
}

void EvaluateEnvironmentSignals(AActor* ObserverActor, AActor* TargetActor, const FVector& SafeDirection, FWanaMovementReadiness& Readiness)
{
    UWorld* World = ObserverActor ? ObserverActor->GetWorld() : nullptr;

    if (!World || !ObserverActor)
    {
        return;
    }

    FVector BoundsOrigin = ObserverActor->GetActorLocation();
    FVector BoundsExtent(40.0f, 40.0f, 80.0f);
    ObserverActor->GetActorBounds(true, BoundsOrigin, BoundsExtent, false);

    const double HorizontalExtent = FMath::Max(FMath::Max(BoundsExtent.X, BoundsExtent.Y), 30.0);
    const float ProbeDistance = FMath::Clamp(static_cast<float>(HorizontalExtent * 2.5), MinObstacleProbeDistance, MaxObstacleProbeDistance);
    FVector ProbeOrigin = BoundsOrigin;
    ProbeOrigin.Z = BoundsOrigin.Z + FMath::Clamp(BoundsExtent.Z * 0.35f, 20.0f, 80.0f);

    FVector ProbeForward = SafeDirection;

    if (ProbeForward.IsNearlyZero())
    {
        ProbeForward = ObserverActor->GetActorForwardVector().GetSafeNormal2D();
    }

    FVector ProbeRight = FVector::CrossProduct(FVector::UpVector, ProbeForward).GetSafeNormal();

    if (ProbeRight.IsNearlyZero())
    {
        ProbeRight = ObserverActor->GetActorRightVector().GetSafeNormal2D();
    }

    TArray<FVector> ProbeDirections;
    ProbeDirections.Reserve(4);
    ProbeDirections.Add(ProbeForward);
    ProbeDirections.Add(-ProbeForward);
    ProbeDirections.Add(ProbeRight);
    ProbeDirections.Add(-ProbeRight);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WanaWorksMovementReadiness), false);
    QueryParams.AddIgnoredActor(ObserverActor);

    if (TargetActor)
    {
        QueryParams.AddIgnoredActor(TargetActor);
    }

    int32 BlockedProbeCount = 0;
    int32 ValidProbeCount = 0;

    for (const FVector& ProbeDirection : ProbeDirections)
    {
        const FVector SafeProbeDirection = ProbeDirection.GetSafeNormal();

        if (SafeProbeDirection.IsNearlyZero())
        {
            continue;
        }

        ++ValidProbeCount;

        FHitResult ProbeHit;
        const FVector ProbeEnd = ProbeOrigin + (SafeProbeDirection * ProbeDistance);
        const bool bProbeBlocked = World->LineTraceSingleByChannel(ProbeHit, ProbeOrigin, ProbeEnd, ECC_Visibility, QueryParams);

        if (bProbeBlocked && ProbeHit.GetActor())
        {
            ++BlockedProbeCount;
        }
    }

    Readiness.ObstaclePressure = ValidProbeCount > 0
        ? static_cast<float>(BlockedProbeCount) / static_cast<float>(ValidProbeCount)
        : 0.0f;

    const float PathProbeDistance = FMath::Clamp(Readiness.DistanceToTarget, 0.0f, ProbeDistance * 2.0f);

    if (PathProbeDistance > 60.0f && !ProbeForward.IsNearlyZero())
    {
        FHitResult PathHit;
        const FVector PathEnd = ProbeOrigin + (ProbeForward * PathProbeDistance);
        Readiness.bPathToTargetObstructed = World->LineTraceSingleByChannel(PathHit, ProbeOrigin, PathEnd, ECC_Visibility, QueryParams) && PathHit.GetActor() != nullptr;
    }

    Readiness.bObstaclePressureDetected =
        Readiness.ObstaclePressure >= MediumObstaclePressureThreshold
        || Readiness.bPathToTargetObstructed;

    Readiness.bMovementSpaceRestricted =
        Readiness.ObstaclePressure >= HighObstaclePressureThreshold
        || (Readiness.bPathToTargetObstructed && Readiness.DistanceToTarget <= ProbeDistance * 1.35f);
}
}

FWanaMovementReadiness FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(AActor* ObserverActor, AActor* TargetActor)
{
    FWanaMovementReadiness Readiness;

    if (!ObserverActor || !TargetActor)
    {
        SetMovementReadinessState(Readiness, EWanaMovementReadinessLevel::Blocked, false, TEXT("Movement blocked because the observer or target is missing."));
        return Readiness;
    }

    if (ObserverActor == TargetActor)
    {
        SetMovementReadinessState(Readiness, EWanaMovementReadinessLevel::Blocked, false, TEXT("Movement blocked because self-target reactions stay in facing-only debug mode."));
        return Readiness;
    }

    if (ObserverActor->GetWorld() != TargetActor->GetWorld())
    {
        SetMovementReadinessState(Readiness, EWanaMovementReadinessLevel::Blocked, false, TEXT("Movement blocked because the observer and target are not in the same world."));
        return Readiness;
    }

    FVector DirectionToTarget = TargetActor->GetActorLocation() - ObserverActor->GetActorLocation();
    DirectionToTarget.Z = 0.0f;
    Readiness.DistanceToTarget = DirectionToTarget.Size();
    const FVector SafeDirection = DirectionToTarget.GetSafeNormal();
    const bool bLongRangeTarget = Readiness.DistanceToTarget > LongRangeMovementHintDistance;
    EvaluateEnvironmentSignals(ObserverActor, TargetActor, SafeDirection, Readiness);
    Readiness.bTargetReachableHint = !bLongRangeTarget && !Readiness.bMovementSpaceRestricted;

    if (const APawn* ObserverPawn = Cast<APawn>(ObserverActor))
    {
        const UPawnMovementComponent* MovementComponent = ObserverPawn->GetMovementComponent();
        const bool bHasMovementComponent = MovementComponent != nullptr;
        const bool bMovementComponentActive = bHasMovementComponent && MovementComponent->IsActive();
        const bool bHasController = ObserverPawn->GetController() != nullptr;
        bool bHasAnimBlueprint = false;
        bool bCanRunWithoutController = false;

        if (const ACharacter* ObserverCharacter = Cast<ACharacter>(ObserverPawn))
        {
            if (const USkeletalMeshComponent* MeshComponent = ObserverCharacter->GetMesh())
            {
                bHasAnimBlueprint =
                    MeshComponent->GetAnimationMode() == EAnimationMode::AnimationBlueprint
                    && MeshComponent->GetAnimInstance() != nullptr;
            }

            if (const UCharacterMovementComponent* CharacterMovement = ObserverCharacter->GetCharacterMovement())
            {
                bCanRunWithoutController = CharacterMovement->bRunPhysicsWithNoController;
            }
        }

        Readiness.bAnimationDrivenLocomotionDetected = bHasAnimBlueprint;
        Readiness.bHasUsableMovementCapability = bHasMovementComponent;
        const bool bHasStrongMovementContext = bHasController || bCanRunWithoutController;
        const bool bHasWeakMovementContext = !bHasStrongMovementContext && !bHasAnimBlueprint && bMovementComponentActive;
        Readiness.bSupportsLocomotionPulse = bHasStrongMovementContext || bHasWeakMovementContext;
        Readiness.bHasMovementContext = Readiness.bSupportsLocomotionPulse;
        Readiness.bLowConfidenceContext = bHasWeakMovementContext || (bLongRangeTarget && !bHasStrongMovementContext);

        if (!bHasMovementComponent)
        {
            SetMovementReadinessState(Readiness, EWanaMovementReadinessLevel::Blocked, false, TEXT("Movement blocked because the observer has no usable movement component."));
        }
        else if (bHasAnimBlueprint)
        {
            if (!bHasStrongMovementContext)
            {
                SetMovementReadinessState(
                    Readiness,
                    EWanaMovementReadinessLevel::Blocked,
                    false,
                    TEXT("Movement blocked because an animation-driven locomotion stack was detected without a controller-safe movement context. Falling back to facing-only behavior avoids conflicting with the current Anim BP."));
            }
            else if (Readiness.bMovementSpaceRestricted)
            {
                SetMovementReadinessState(
                    Readiness,
                    EWanaMovementReadinessLevel::Blocked,
                    false,
                    TEXT("Movement blocked because nearby space is too tight or obstructed for a safe locomotion-driven reaction."));
            }
            else if (Readiness.bObstaclePressureDetected || bLongRangeTarget)
            {
                SetMovementReadinessState(
                    Readiness,
                    EWanaMovementReadinessLevel::Limited,
                    true,
                    TEXT("Movement is limited because the current environment is partially obstructed or long-range. Use only short, cautious movement or a visible fallback."));
            }
            else
            {
                SetMovementReadinessState(
                    Readiness,
                    EWanaMovementReadinessLevel::Allowed,
                    true,
                    TEXT("A controller-safe animation-driven locomotion context is available, so WanaWorks can use a short animation-friendly reaction pulse."));
            }
        }
        else if (!Readiness.bHasMovementContext)
        {
            SetMovementReadinessState(
                Readiness,
                EWanaMovementReadinessLevel::Blocked,
                false,
                TEXT("Movement blocked because no movement context was detected for locomotion-safe reactions."));
        }
        else if (Readiness.bMovementSpaceRestricted)
        {
            SetMovementReadinessState(
                Readiness,
                EWanaMovementReadinessLevel::Blocked,
                false,
                TEXT("Movement blocked because nearby obstacle pressure or boundary restriction leaves too little room for a safe reaction pulse."));
        }
        else if (bHasWeakMovementContext)
        {
            SetMovementReadinessState(
                Readiness,
                EWanaMovementReadinessLevel::Unclear,
                true,
                TEXT("Movement is unclear because only a lightweight local movement context was detected. Prefer cautious fallback behavior unless the current stack proves it can move safely."));
        }
        else if (Readiness.bObstaclePressureDetected || bLongRangeTarget)
        {
            SetMovementReadinessState(
                Readiness,
                EWanaMovementReadinessLevel::Limited,
                true,
                TEXT("Movement is limited because the environment is partly obstructed or the target is far away. Keep reactions short and context-aware."));
        }
        else
        {
            SetMovementReadinessState(
                Readiness,
                EWanaMovementReadinessLevel::Allowed,
                true,
                bHasController
                    ? TEXT("A controller-backed locomotion stack is available for a safe reaction pulse.")
                    : TEXT("No controller was detected, but no-controller character movement is available for a safe local pulse."));
        }

        if (Readiness.bObstaclePressureDetected && !Readiness.bMovementSpaceRestricted)
        {
            Readiness.Detail += TEXT(" Nearby obstacle pressure was detected, so movement should stay deliberate.");
        }

        if (Readiness.bLowConfidenceContext && Readiness.ReadinessLevel != EWanaMovementReadinessLevel::Blocked)
        {
            Readiness.Detail += TEXT(" Confidence is lower than normal for this movement context.");
        }

        return Readiness;
    }

    if (const USceneComponent* RootComponent = ObserverActor->GetRootComponent())
    {
        if (RootComponent->Mobility == EComponentMobility::Movable && !RootComponent->IsSimulatingPhysics())
        {
            Readiness.bHasUsableMovementCapability = true;
            Readiness.bHasMovementContext = true;
            Readiness.bSupportsDirectActorMove = true;
            Readiness.bLowConfidenceContext = true;

            if (Readiness.bMovementSpaceRestricted)
            {
                SetMovementReadinessState(
                    Readiness,
                    EWanaMovementReadinessLevel::Blocked,
                    false,
                    TEXT("Movement blocked because the actor is movable, but nearby space is too restricted for a safe direct movement fallback."));
            }
            else if (Readiness.bObstaclePressureDetected || bLongRangeTarget || Readiness.bPathToTargetObstructed)
            {
                SetMovementReadinessState(
                    Readiness,
                    EWanaMovementReadinessLevel::Unclear,
                    true,
                    TEXT("Movement is unclear because only a direct actor move fallback is available and the current space is partly obstructed or low-confidence."));
            }
            else
            {
                SetMovementReadinessState(
                    Readiness,
                    EWanaMovementReadinessLevel::Limited,
                    true,
                    TEXT("No pawn locomotion stack was detected, so only a safe direct actor move fallback is available."));
            }

            return Readiness;
        }
    }

    SetMovementReadinessState(Readiness, EWanaMovementReadinessLevel::Blocked, false, TEXT("Movement blocked because the observer is not currently movable."));
    return Readiness;
}

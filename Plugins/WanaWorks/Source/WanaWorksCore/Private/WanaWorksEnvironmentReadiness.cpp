#include "WanaWorksEnvironmentReadiness.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

namespace
{
constexpr float LongRangeMovementHintDistance = 3000.0f;
}

FWanaMovementReadiness FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(AActor* ObserverActor, AActor* TargetActor)
{
    FWanaMovementReadiness Readiness;

    if (!ObserverActor || !TargetActor)
    {
        Readiness.Detail = TEXT("Movement blocked because the observer or target is missing.");
        return Readiness;
    }

    if (ObserverActor == TargetActor)
    {
        Readiness.Detail = TEXT("Movement blocked because self-target reactions stay in facing-only debug mode.");
        return Readiness;
    }

    if (ObserverActor->GetWorld() != TargetActor->GetWorld())
    {
        Readiness.Detail = TEXT("Movement blocked because the observer and target are not in the same world.");
        return Readiness;
    }

    FVector DirectionToTarget = TargetActor->GetActorLocation() - ObserverActor->GetActorLocation();
    DirectionToTarget.Z = 0.0f;
    Readiness.DistanceToTarget = DirectionToTarget.Size();
    Readiness.bTargetReachableHint = true;

    const bool bLongRangeTarget = Readiness.DistanceToTarget > LongRangeMovementHintDistance;

    if (const APawn* ObserverPawn = Cast<APawn>(ObserverActor))
    {
        const UPawnMovementComponent* MovementComponent = ObserverPawn->GetMovementComponent();
        const bool bHasMovementComponent = MovementComponent != nullptr;
        const bool bMovementComponentActive = bHasMovementComponent && MovementComponent->IsActive();
        const bool bHasController = ObserverPawn->GetController() != nullptr;

        bool bCanRunWithoutController = false;

        if (const ACharacter* ObserverCharacter = Cast<ACharacter>(ObserverPawn))
        {
            if (const UCharacterMovementComponent* CharacterMovement = ObserverCharacter->GetCharacterMovement())
            {
                bCanRunWithoutController = CharacterMovement->bRunPhysicsWithNoController;
            }
        }

        Readiness.bHasUsableMovementCapability = bHasMovementComponent;
        Readiness.bHasMovementContext = bHasController || bMovementComponentActive || bCanRunWithoutController;
        Readiness.bTargetReachableHint = !bLongRangeTarget;

        if (Readiness.bHasUsableMovementCapability && Readiness.bHasMovementContext)
        {
            Readiness.bCanAttemptMovement = true;
            Readiness.Status = TEXT("Movement allowed");

            if (bHasController)
            {
                Readiness.Detail = TEXT("A pawn movement context is available for a safe reaction pulse.");
            }
            else if (bCanRunWithoutController)
            {
                Readiness.Detail = TEXT("No controller was detected, but no-controller character movement is available.");
            }
            else
            {
                Readiness.Detail = TEXT("No controller was detected, but a local movement component is active for a compatibility-safe pulse.");
            }

            if (bLongRangeTarget)
            {
                Readiness.Detail += TEXT(" The target is long-range, so only a short local reaction pulse should be attempted.");
            }

            return Readiness;
        }

        if (!bHasMovementComponent)
        {
            Readiness.Detail = TEXT("Movement blocked because the observer has no usable movement component.");
        }
        else
        {
            Readiness.Detail = TEXT("Movement blocked because no navigation or movement context was detected for locomotion-safe reactions.");
        }

        return Readiness;
    }

    if (const USceneComponent* RootComponent = ObserverActor->GetRootComponent())
    {
        if (RootComponent->Mobility == EComponentMobility::Movable && !RootComponent->IsSimulatingPhysics())
        {
            Readiness.bCanAttemptMovement = true;
            Readiness.bHasUsableMovementCapability = true;
            Readiness.bHasMovementContext = true;
            Readiness.bTargetReachableHint = !bLongRangeTarget;
            Readiness.bSupportsDirectActorMove = true;
            Readiness.Status = TEXT("Movement allowed");
            Readiness.Detail = TEXT("No pawn locomotion stack was detected, so only a safe direct actor move fallback is available.");

            if (bLongRangeTarget)
            {
                Readiness.Detail += TEXT(" The target is long-range, so the fallback should stay small and local.");
            }

            return Readiness;
        }
    }

    Readiness.Detail = TEXT("Movement blocked because the observer is not currently movable.");
    return Readiness;
}

#include "WAYPlayerProfileComponent.h"

#include "Components/SceneComponent.h"
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
constexpr float BasicReactionMoveInputScale = 0.9f;
constexpr float BasicReactionMovePulseDuration = 0.45f;
constexpr float BasicReactionProtectiveMovePulseDuration = 0.30f;

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
        return TEXT("Turned toward the target and requested a short forward movement pulse.");

    case EWAYReactionState::Cooperative:
        return TEXT("Turned toward the target cooperatively.");

    case EWAYReactionState::Protective:
        return DirectionToTarget.SizeSquared() > FMath::Square(BasicReactionProtectiveRange)
            ? TEXT("Moved closer to the target and faced outward protectively.")
            : TEXT("Held position near the target and faced outward protectively.");

    case EWAYReactionState::Cautious:
        return TEXT("Turned toward the target and requested a short retreat movement pulse.");

    case EWAYReactionState::Observational:
    default:
        return TEXT("Turned toward the target and observed.");
    }
}

void ApplyReactionFacing(AActor* ObserverActor, const FVector& SafeDirection, EWAYReactionState ReactionState)
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
        FaceActorDirection(ObserverActor, -SafeDirection);
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

    case EWAYReactionState::Protective:
        FaceActorDirection(ObserverActor, -SafeDirection);

        if (DirectionToTarget.SizeSquared() > FMath::Square(BasicReactionProtectiveRange))
        {
            MovementDirection = SafeDirection;
        }
        break;

    case EWAYReactionState::Cautious:
        FaceActorDirection(ObserverActor, SafeDirection);
        MovementDirection = -SafeDirection;
        break;

    case EWAYReactionState::Cooperative:
    case EWAYReactionState::Observational:
    default:
        StopBasicReactionMovement();
        return;
    }

    if (!MovementDirection.IsNearlyZero())
    {
        ObserverPawn->AddMovementInput(MovementDirection, BasicReactionMoveInputScale, true);
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
        return Evaluation;
    }

    Evaluation.RelationshipProfile = EnsureRelationshipProfileForObservedTarget(TargetActor);
    Evaluation.ReactionState = ResolveReactionForRelationshipState(Evaluation.RelationshipProfile.RelationshipState);

    const EWAYReactionState* PreviousReaction = CachedReactionStates.Find(TargetActor);

    if (!PreviousReaction || *PreviousReaction != Evaluation.ReactionState)
    {
        CachedReactionStates.Add(TargetActor, Evaluation.ReactionState);
        OnReactionChanged.Broadcast(GetOwner(), TargetActor, Evaluation.RelationshipProfile.RelationshipState, Evaluation.ReactionState);
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

    ApplyReactionFacing(ObserverActor, SafeDirection, ReactionState);

    FString BehaviorDescription = DescribeBasicReactionBehavior(ObserverActor, TargetActor, ReactionState);
    TryApplyMovementReaction(TargetActor, ReactionState, SafeDirection, BehaviorDescription);

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

    ApplyReactionFacing(ObserverActor, SafeDirection, ReactionState);
    TryApplyMovementReaction(TargetActor, ReactionState, SafeDirection, BehaviorDescription);

    UE_LOG(
        LogWanaWorksWAY,
        Log,
        TEXT("Applied WanaAI automatic reaction behavior. Observer=%s Target=%s Reaction=%s Behavior=%s"),
        *ObserverActor->GetActorNameOrLabel(),
        *TargetActor->GetActorNameOrLabel(),
        *GetReactionStateLogLabel(ReactionState),
        *BehaviorDescription);
}

bool UWAYPlayerProfileComponent::TryApplyMovementReaction(AActor* TargetActor, EWAYReactionState ReactionState, const FVector& SafeDirection, FString& OutBehaviorDescription)
{
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
        OutBehaviorDescription += TEXT(" Movement held because the observer is already within protective range.");
        return false;
    }

    if (!MovementReadiness.bCanAttemptMovement)
    {
        StopBasicReactionMovement();
        OutBehaviorDescription += FString::Printf(TEXT(" Movement blocked. %s Falling back to facing behavior."), *MovementReadiness.Detail);
        return false;
    }

    if (StartBasicReactionMovement(TargetActor, ReactionState, OutBehaviorDescription))
    {
        OutBehaviorDescription += FString::Printf(TEXT(" Movement allowed. %s"), *MovementReadiness.Detail);
        return true;
    }

    if (MovementReadiness.bSupportsDirectActorMove && ObserverActor)
    {
        const FVector FallbackOffset = ReactionState == EWAYReactionState::Hostile
            ? SafeDirection * BasicReactionMoveDistance
            : ReactionState == EWAYReactionState::Cautious
                ? -SafeDirection * (BasicReactionMoveDistance * 0.6f)
                : SafeDirection * (BasicReactionMoveDistance * 0.35f);

        if (TryMoveActor(ObserverActor, FallbackOffset))
        {
            OutBehaviorDescription = ReactionState == EWAYReactionState::Hostile
                ? TEXT("Turned toward the target and advanced slightly with a safe actor move fallback.")
                : ReactionState == EWAYReactionState::Cautious
                    ? TEXT("Turned toward the target and backed away slightly with a safe actor move fallback.")
                    : TEXT("Moved a little closer to the target with a safe actor move fallback and faced outward protectively.");
            OutBehaviorDescription += FString::Printf(TEXT(" Movement allowed. %s"), *MovementReadiness.Detail);
            return true;
        }
    }

    StopBasicReactionMovement();
    OutBehaviorDescription += TEXT(" Movement was allowed, but no safe locomotion path could be applied, so the actor stayed in a facing-only response.");
    return false;
}

bool UWAYPlayerProfileComponent::StartBasicReactionMovement(AActor* TargetActor, EWAYReactionState ReactionState, FString& OutBehaviorDescription)
{
    APawn* ObserverPawn = Cast<APawn>(GetOwner());

    if (!ObserverPawn || !TargetActor)
    {
        return false;
    }

    if (ReactionState != EWAYReactionState::Hostile
        && ReactionState != EWAYReactionState::Protective
        && ReactionState != EWAYReactionState::Cautious)
    {
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
    ActiveBasicReactionTimeRemaining = ReactionState == EWAYReactionState::Protective
        ? BasicReactionProtectiveMovePulseDuration
        : BasicReactionMovePulseDuration;
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

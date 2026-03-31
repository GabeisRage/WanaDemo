#include "WAYPlayerProfileComponent.h"

#include "Components/SceneComponent.h"
#include "WanaIdentityComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogWanaWorksWAY, Log, All);

namespace
{
constexpr float BasicReactionMoveDistance = 120.0f;
constexpr float BasicReactionProtectiveRange = 260.0f;

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

FString ApplyBasicReactionBehaviorInternal(AActor* ObserverActor, AActor* TargetActor, EWAYReactionState ReactionState)
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
    {
        FaceActorDirection(ObserverActor, SafeDirection);
        const bool bMoved = TryMoveActor(ObserverActor, SafeDirection * BasicReactionMoveDistance);
        return bMoved
            ? TEXT("Turned toward the target and advanced slightly.")
            : TEXT("Turned toward the target.");
    }

    case EWAYReactionState::Cooperative:
        FaceActorDirection(ObserverActor, SafeDirection);
        return TEXT("Turned toward the target cooperatively.");

    case EWAYReactionState::Protective:
    {
        bool bMovedCloser = false;

        if (DirectionToTarget.SizeSquared() > FMath::Square(BasicReactionProtectiveRange))
        {
            bMovedCloser = TryMoveActor(ObserverActor, SafeDirection * (BasicReactionMoveDistance * 0.75f));
        }

        FaceActorDirection(ObserverActor, -SafeDirection);
        return bMovedCloser
            ? TEXT("Moved closer to the target and faced outward protectively.")
            : TEXT("Held position near the target and faced outward protectively.");
    }

    case EWAYReactionState::Cautious:
    {
        FaceActorDirection(ObserverActor, SafeDirection);
        const bool bRetreated = TryMoveActor(ObserverActor, -SafeDirection * (BasicReactionMoveDistance * 0.6f));
        return bRetreated
            ? TEXT("Turned toward the target and backed away slightly.")
            : TEXT("Turned toward the target cautiously.");
    }

    case EWAYReactionState::Observational:
    default:
        FaceActorDirection(ObserverActor, SafeDirection);
        return TEXT("Turned toward the target and observed.");
    }
}
}

UWAYPlayerProfileComponent::UWAYPlayerProfileComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWAYPlayerProfileComponent::BeginPlay()
{
    Super::BeginPlay();

    OnReactionChanged.RemoveDynamic(this, &UWAYPlayerProfileComponent::HandleReactionChanged);
    OnReactionChanged.AddDynamic(this, &UWAYPlayerProfileComponent::HandleReactionChanged);
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
    const FString BehaviorDescription = ApplyBasicReactionBehaviorInternal(ObserverActor, TargetActor, ReactionState);

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

    const FString BehaviorDescription = ApplyBasicReactionBehaviorInternal(ObserverActor, TargetActor, ReactionState);

    UE_LOG(
        LogWanaWorksWAY,
        Log,
        TEXT("Applied WanaAI automatic reaction behavior. Observer=%s Target=%s Reaction=%s Behavior=%s"),
        *ObserverActor->GetActorNameOrLabel(),
        *TargetActor->GetActorNameOrLabel(),
        *GetReactionStateLogLabel(ReactionState),
        *BehaviorDescription);
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

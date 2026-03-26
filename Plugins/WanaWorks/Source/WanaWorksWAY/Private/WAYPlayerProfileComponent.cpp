#include "WAYPlayerProfileComponent.h"

#include "WanaIdentityComponent.h"

namespace
{
float ClampRelationshipValue(float Value)
{
    return FMath::Clamp(Value, 0.0f, 1.0f);
}
}

UWAYPlayerProfileComponent::UWAYPlayerProfileComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
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

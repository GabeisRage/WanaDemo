#pragma once

#include "Components/ActorComponent.h"
#include "WAYRelationshipTypes.h"
#include "WAYPlayerProfileComponent.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct WANAWORKSWAY_API FWAYPreferenceSignal
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    FName SignalName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Weight = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FWAYRelationshipStateChangedSignature, AActor*, TargetActor, EWAYRelationshipState, PreviousState, EWAYRelationshipState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FWAYReactionChangedSignature, AActor*, ObserverActor, AActor*, TargetActor, EWAYRelationshipState, RelationshipState, EWAYReactionState, ReactionState);

UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSWAY_API UWAYPlayerProfileComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWAYPlayerProfileComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY")
    void RecordPreferenceSignal(FName SignalName, float Weight);

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAY")
    const TArray<FWAYPreferenceSignal>& GetSignals() const { return Signals; }

    // Relationship profiles are owned by this component's actor and keyed by target actor.
    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY", meta = (DisplayName = "Get Relationship Profile For Target", Keywords = "WAY relationship observer target profile"))
    bool GetRelationshipProfileForTarget(AActor* TargetActor, FWAYRelationshipProfile& OutProfile) const;

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY")
    FWAYRelationshipProfile EnsureRelationshipProfileForTarget(AActor* TargetActor);

    // Convenience wrapper for observation flows. The owning actor remains the observer.
    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY")
    FWAYRelationshipProfile EnsureRelationshipProfileForObservedTarget(AActor* TargetActor);

    // Evaluates a target from this component owner's perspective and returns the current relationship and reaction.
    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY", meta = (DisplayName = "Evaluate Target", Keywords = "WAY reaction evaluate observer target AI"))
    FWAYTargetEvaluation EvaluateTarget(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY", meta = (DisplayName = "Evaluate And Get Reaction", Keywords = "WAY reaction evaluate observer target AI"))
    EWAYReactionState EvaluateAndGetReaction(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY", meta = (DisplayName = "Evaluate And React", Keywords = "WAY reaction evaluate react observer target AI"))
    EWAYReactionState EvaluateAndReact(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Apply Basic Reaction Behavior", Keywords = "WAY reaction behavior visual response observer target AI"))
    bool ApplyBasicReactionBehavior(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY")
    void SetRelationshipStateForTarget(AActor* TargetActor, EWAYRelationshipState NewRelationshipState);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY", meta = (DisplayName = "Get Reaction For Target", Keywords = "WAY reaction observer target AI"))
    EWAYReactionState GetReactionForTarget(AActor* TargetActor) const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAY")
    const TArray<FWAYRelationshipProfile>& GetRelationshipProfiles() const { return RelationshipProfiles; }

    UPROPERTY(BlueprintAssignable, Category = "Wana Works|WAY")
    FWAYRelationshipStateChangedSignature OnRelationshipStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Wana Works|WAY")
    FWAYReactionChangedSignature OnReactionChanged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Behavior")
    bool bAutoApplyBasicReactionBehavior = false;

    static EWAYReactionState ResolveReactionForRelationshipState(EWAYRelationshipState RelationshipState);

private:
    UFUNCTION()
    void HandleReactionChanged(AActor* ObserverActor, AActor* TargetActor, EWAYRelationshipState RelationshipState, EWAYReactionState ReactionState);

    bool TryApplyMovementReaction(AActor* TargetActor, EWAYReactionState ReactionState, const FVector& SafeDirection, FString& OutBehaviorDescription);
    bool StartBasicReactionMovement(AActor* TargetActor, EWAYReactionState ReactionState, FString& OutBehaviorDescription);
    void StopBasicReactionMovement();
    int32 FindRelationshipProfileIndex(AActor* TargetActor) const;
    FWAYRelationshipSeed MakeRelationshipSeedForTarget(AActor* TargetActor) const;
    FWAYRelationshipProfile CreateRelationshipProfile(AActor* TargetActor) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|WAY", meta = (AllowPrivateAccess = "true"))
    TArray<FWAYPreferenceSignal> Signals;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|WAY", meta = (AllowPrivateAccess = "true"))
    TArray<FWAYRelationshipProfile> RelationshipProfiles;

    TMap<AActor*, EWAYReactionState> CachedReactionStates;
    TWeakObjectPtr<AActor> ActiveBasicReactionTarget;
    EWAYReactionState ActiveBasicReactionState = EWAYReactionState::Observational;
    float ActiveBasicReactionTimeRemaining = 0.0f;
    bool bRestoresRunPhysicsWithNoController = false;
    bool bPreviousRunPhysicsWithNoController = false;
};

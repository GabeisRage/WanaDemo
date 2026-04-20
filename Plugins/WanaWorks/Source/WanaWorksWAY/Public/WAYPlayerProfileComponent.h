#pragma once

#include "Components/ActorComponent.h"
#include "WanaWorksTypes.h"
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FWAYRecommendedBehaviorChangedSignature, AActor*, ObserverActor, AActor*, TargetActor, EWAYReactionState, ReactionState, EWAYBehaviorPreset, RecommendedBehavior);

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

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Evaluate Target And Get Recommended Behavior", Keywords = "WAY behavior preset evaluate observer target AI"))
    EWAYBehaviorPreset EvaluateTargetAndGetRecommendedBehavior(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Apply Recommended Behavior Hook", Keywords = "WAY behavior preset hook recommended observer target AI"))
    bool ApplyRecommendedBehaviorHook(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Apply Behavior Preset Hook", Keywords = "WAY behavior preset hook observer target AI"))
    bool ApplyBehaviorPresetHook(AActor* TargetActor, EWAYBehaviorPreset BehaviorPreset);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Apply Basic Reaction Behavior", Keywords = "WAY reaction behavior visual response observer target AI"))
    bool ApplyBasicReactionBehavior(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY")
    void SetRelationshipStateForTarget(AActor* TargetActor, EWAYRelationshipState NewRelationshipState);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY", meta = (DisplayName = "Get Reaction For Target", Keywords = "WAY reaction observer target AI"))
    EWAYReactionState GetReactionForTarget(AActor* TargetActor) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Get Recommended Behavior For Target", Keywords = "WAY behavior preset observer target AI"))
    EWAYBehaviorPreset GetRecommendedBehaviorForTarget(AActor* TargetActor) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Has Starter Behavior Hook For Target", Keywords = "WAY behavior preset starter hook observer target AI"))
    bool HasStarterBehaviorHookForTarget(AActor* TargetActor) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Get Last Applied Behavior Hook For Target", Keywords = "WAY behavior preset last applied hook observer target AI"))
    EWAYBehaviorPreset GetLastAppliedBehaviorHookForTarget(AActor* TargetActor) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Get Behavior Execution Mode For Target", Keywords = "WAY behavior execution mode observer target AI"))
    EWAYBehaviorExecutionMode GetBehaviorExecutionModeForTarget(AActor* TargetActor) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Get Visible Behavior Label For Target", Keywords = "WAY visible behavior label observer target AI"))
    FString GetVisibleBehaviorLabelForTarget(AActor* TargetActor) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY|Behavior", meta = (DisplayName = "Get Behavior Execution Detail For Target", Keywords = "WAY behavior execution detail observer target AI"))
    FString GetBehaviorExecutionDetailForTarget(AActor* TargetActor) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY|Animation", meta = (DisplayName = "Get Current Animation Hook State", Keywords = "WAY animation hook state anim blueprint"))
    FWAYAnimationHookState GetCurrentAnimationHookState() const { return CurrentAnimationHookState; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY|Animation", meta = (DisplayName = "Get Animation Hook Application Status", Keywords = "WAY animation hook application anim blueprint"))
    EWAYAnimationHookApplicationStatus GetAnimationHookApplicationStatus() const { return CurrentAnimationHookState.ApplicationStatus; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wana Works|WAY|Animation", meta = (DisplayName = "Has Active Animation Hook State", Keywords = "WAY animation hook active anim blueprint"))
    bool HasActiveAnimationHookState() const { return CurrentAnimationHookState.ApplicationStatus == EWAYAnimationHookApplicationStatus::Active; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAY")
    const TArray<FWAYRelationshipProfile>& GetRelationshipProfiles() const { return RelationshipProfiles; }

    UPROPERTY(BlueprintAssignable, Category = "Wana Works|WAY")
    FWAYRelationshipStateChangedSignature OnRelationshipStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Wana Works|WAY")
    FWAYReactionChangedSignature OnReactionChanged;

    UPROPERTY(BlueprintAssignable, Category = "Wana Works|WAY|Behavior")
    FWAYRecommendedBehaviorChangedSignature OnRecommendedBehaviorChanged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY|Behavior")
    bool bAutoApplyBasicReactionBehavior = false;

    static EWAYReactionState ResolveReactionForRelationshipState(EWAYRelationshipState RelationshipState);
    static EWAYBehaviorPreset ResolveRecommendedBehaviorForReactionState(EWAYReactionState ReactionState);
    static EWAYBehaviorExecutionMode ResolveBehaviorExecutionModeForMovementReadiness(const FWanaMovementReadiness& MovementReadiness);

private:
    UFUNCTION()
    void HandleReactionChanged(AActor* ObserverActor, AActor* TargetActor, EWAYRelationshipState RelationshipState, EWAYReactionState ReactionState);

    void UpdateAnimationHookState(
        AActor* TargetActor,
        EWAYReactionState ReactionState,
        EWAYBehaviorPreset RecommendedBehavior,
        EWAYBehaviorExecutionMode ExecutionMode,
        bool bFacingHookRequested,
        bool bTurnToTargetRequested,
        const FString& Detail);
    void CacheBehaviorExecutionResult(AActor* TargetActor, const FString& VisibleBehaviorLabel, const FString& Detail, EWAYBehaviorExecutionMode ExecutionMode);
    bool TryApplyMovementReaction(AActor* TargetActor, EWAYReactionState ReactionState, const FVector& SafeDirection, FString& OutBehaviorDescription, EWAYBehaviorExecutionMode* OutExecutionMode = nullptr);
    bool StartBasicReactionMovement(AActor* TargetActor, EWAYReactionState ReactionState, FString& OutBehaviorDescription, float MoveInputScale = 0.9f, float MovePulseDurationOverride = 0.0f);
    void StopBasicReactionMovement();
    int32 FindRelationshipProfileIndex(AActor* TargetActor) const;
    FWAYRelationshipSeed MakeRelationshipSeedForTarget(AActor* TargetActor) const;
    FWAYRelationshipProfile CreateRelationshipProfile(AActor* TargetActor) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|WAY", meta = (AllowPrivateAccess = "true"))
    TArray<FWAYPreferenceSignal> Signals;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|WAY", meta = (AllowPrivateAccess = "true"))
    TArray<FWAYRelationshipProfile> RelationshipProfiles;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WAY|Animation", meta = (AllowPrivateAccess = "true"))
    FWAYAnimationHookState CurrentAnimationHookState;

    TMap<AActor*, EWAYReactionState> CachedReactionStates;
    TMap<AActor*, EWAYBehaviorPreset> CachedRecommendedBehaviors;
    TMap<AActor*, EWAYBehaviorPreset> CachedLastAppliedBehaviorHooks;
    TMap<AActor*, EWAYBehaviorExecutionMode> CachedBehaviorExecutionModes;
    TMap<AActor*, FString> CachedVisibleBehaviorLabels;
    TMap<AActor*, FString> CachedBehaviorExecutionDetails;
    TWeakObjectPtr<AActor> ActiveBasicReactionTarget;
    EWAYReactionState ActiveBasicReactionState = EWAYReactionState::Observational;
    float ActiveBasicReactionTimeRemaining = 0.0f;
    float ActiveBasicReactionMoveInputScale = 0.9f;
    bool bRestoresRunPhysicsWithNoController = false;
    bool bPreviousRunPhysicsWithNoController = false;
};

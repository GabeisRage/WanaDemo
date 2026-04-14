#pragma once

#include "CoreMinimal.h"
#include "WanaWorksCommandDispatcher.h"
#include "WanaWorksTypes.h"
#include "WAYRelationshipTypes.h"

class AActor;
class UAnimBlueprint;
class UObject;

struct FWanaSelectedActorIdentitySnapshot
{
    bool bHasSelectedActor = false;
    bool bHasIdentityComponent = false;
    TWeakObjectPtr<AActor> SelectedActor;
    FString SelectedActorLabel;
    FString FactionTag;
    FWAYRelationshipSeed DefaultRelationshipSeed;
    FString ReputationTagsSummary;
};

struct FWanaSelectedCharacterEnhancementSnapshot
{
    bool bHasSelectedActor = false;
    bool bIsProjectAsset = false;
    bool bHasIdentityComponent = false;
    bool bHasWAIComponent = false;
    bool bHasWAYComponent = false;
    bool bIsPawnActor = false;
    bool bHasAIControllerClass = false;
    bool bAutoPossessAIEnabled = false;
    bool bHasSkeletalMeshComponent = false;
    bool bHasAnimBlueprint = false;
    TWeakObjectPtr<AActor> SelectedActor;
    FString SelectedActorLabel;
    FString SubjectSourceLabel;
    FString SubjectAssetPath;
    FString ActorTypeLabel;
    FString LinkedAnimationBlueprintLabel;
    FString LinkedAIControllerLabel;
    FString AnimationCompatibilitySummary;
    FString AIReadinessSummary;
};

struct FWanaSelectedRelationshipContextSnapshot
{
    bool bHasObserverActor = false;
    bool bHasTargetActor = false;
    bool bTargetFallsBackToObserver = false;
    bool bObserverHasRelationshipComponent = false;
    TWeakObjectPtr<AActor> ObserverActor;
    TWeakObjectPtr<AActor> TargetActor;
    FString ObserverActorLabel;
    FString TargetActorLabel;
};

struct FWanaEnvironmentReadinessSnapshot
{
    bool bHasObserverActor = false;
    bool bHasTargetActor = false;
    bool bTargetFallsBackToObserver = false;
    TWeakObjectPtr<AActor> ObserverActor;
    TWeakObjectPtr<AActor> TargetActor;
    FString PairSourceLabel;
    FString ObserverActorLabel;
    FString TargetActorLabel;
    FWanaMovementReadiness MovementReadiness;
};

struct FWanaBehaviorResultsSnapshot
{
    bool bHasObserverActor = false;
    bool bHasTargetActor = false;
    bool bHasWAYComponent = false;
    bool bHasRelationshipProfile = false;
    bool bStarterHookAvailable = false;
    bool bTargetFallsBackToObserver = false;
    TWeakObjectPtr<AActor> ObserverActor;
    TWeakObjectPtr<AActor> TargetActor;
    FString PairSourceLabel;
    FString ObserverActorLabel;
    FString TargetActorLabel;
    EWAYRelationshipState RelationshipState = EWAYRelationshipState::Neutral;
    EWAYReactionState ReactionState = EWAYReactionState::Observational;
    EWAYBehaviorPreset RecommendedBehavior = EWAYBehaviorPreset::None;
    EWAYBehaviorPreset LastAppliedHook = EWAYBehaviorPreset::None;
    EWAYBehaviorExecutionMode ExecutionMode = EWAYBehaviorExecutionMode::Unknown;
};

namespace WanaWorksUIEditorActions
{
    bool GetSelectedActorIdentitySnapshot(FWanaSelectedActorIdentitySnapshot& OutSnapshot);
    bool GetSelectedCharacterEnhancementSnapshot(FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot);
    bool GetCharacterEnhancementSnapshotForActor(const AActor* Actor, FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot);
    bool GetCharacterEnhancementSnapshotForSubjectObject(const UObject* SubjectObject, FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot);
    bool GetSelectedRelationshipContextSnapshot(FWanaSelectedRelationshipContextSnapshot& OutSnapshot);
    bool GetEnvironmentReadinessSnapshotForActorPair(const AActor* ObserverActor, const AActor* TargetActor, bool bTargetFallsBackToObserver, const FString& PairSourceLabel, FWanaEnvironmentReadinessSnapshot& OutSnapshot);
    bool GetBehaviorResultsSnapshotForActorPair(const AActor* ObserverActor, const AActor* TargetActor, bool bTargetFallsBackToObserver, const FString& PairSourceLabel, FWanaBehaviorResultsSnapshot& OutSnapshot);
    FWanaCommandResponse ExecuteWeatherPresetCommand(const FString& PresetName);
    FWanaCommandResponse ExecuteSpawnCubeCommand();
    FWanaCommandResponse ExecuteListSelectionCommand();
    FWanaCommandResponse ExecuteClassifySelectedCommand();
    FWanaCommandResponse ExecuteAddMemoryTestCommand();
    FWanaCommandResponse ExecuteAddPreferenceTestCommand();
    FWanaCommandResponse ExecuteShowMemoryCommand();
    FWanaCommandResponse ExecuteShowPreferencesCommand();
    FWanaCommandResponse ExecuteEnsureRelationshipProfileCommand();
    FWanaCommandResponse ExecuteShowRelationshipProfileCommand();
    FWanaCommandResponse ExecuteApplyRelationshipStateCommand(const FString& RelationshipStateText);
    FWanaCommandResponse ExecuteEnsureIdentityComponentCommand();
    FWanaCommandResponse ExecuteApplyIdentityCommand(const FString& FactionTagText, EWAYRelationshipState DefaultRelationshipState);
    FWanaCommandResponse ExecuteApplyCharacterEnhancementCommand(const FString& PresetLabel);
    FWanaCommandResponse ExecuteCreateSandboxDuplicateCommand();
    FWanaCommandResponse ExecuteCreateSandboxSubjectFromAssetCommand(const UObject* SubjectObject, const UAnimBlueprint* AnimationBlueprintOverride = nullptr);
    FWanaCommandResponse ExecuteCreateFinalizedBuildSubjectFromAssetCommand(const UObject* SubjectObject, const UAnimBlueprint* AnimationBlueprintOverride = nullptr);
    FWanaCommandResponse ExecutePrepareSelectedActorForAITestCommand();
    FWanaCommandResponse ExecuteEvaluateLiveTargetCommand();
    FWanaCommandResponse ExecuteEvaluateActorPairCommand(AActor* ObserverActor, AActor* TargetActor, bool bTargetFallsBackToObserver = false);
    FWanaCommandResponse ExecuteScanEnvironmentReadinessCommand(AActor* ObserverActor, AActor* TargetActor, const FString& PairSourceLabel, bool bTargetFallsBackToObserver = false);
    FWanaCommandResponse ExecuteFocusActorCommand(AActor* Actor, const FString& RoleLabel);
    FWanaCommandResponse ExecuteApplyStarterAndTestTargetCommand(const FString& PresetLabel);
}

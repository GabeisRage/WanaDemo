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
    bool bHasPhysicalStateComponent = false;
    bool bIsPawnActor = false;
    bool bHasAIControllerClass = false;
    bool bAutoPossessAIEnabled = false;
    bool bHasPlayerControllerSignal = false;
    bool bHasLivePlayerController = false;
    bool bAutoPossessPlayerEnabled = false;
    bool bHasPawnMovementComponent = false;
    bool bHasCameraComponent = false;
    bool bHasSpringArmComponent = false;
    bool bHasSkeletalMeshComponent = false;
    bool bHasAnimBlueprint = false;
    bool bHasAnimationBlueprintAsset = false;
    bool bHasAnimationBlueprintGeneratedClass = false;
    bool bHasAnimationParentInstanceClass = false;
    TWeakObjectPtr<AActor> SelectedActor;
    FString SelectedActorLabel;
    FString SubjectSourceLabel;
    FString SubjectAssetPath;
    FString ActorTypeLabel;
    FString SkeletalMeshLabel;
    FString SkeletonLabel;
    FString LinkedAnimationBlueprintLabel;
    FString AnimationMeshComponentLabel;
    FString AnimationAssignedClassLabel;
    FString AnimationAssignedClassPath;
    FString AnimationBlueprintAssetLabel;
    FString AnimationBlueprintAssetPath;
    FString AnimationGeneratedClassLabel;
    FString AnimationParentInstanceClassLabel;
    FString AnimationInstanceClassLabel;
    FString LinkedAIControllerLabel;
    FString LinkedPlayerControllerLabel;
    FString AutoPossessPlayerLabel;
    FString PlayableControlSummary;
    FString CharacterControlModeLabel;
    FString CompatibleSkeletonSummary;
    FString AnimationCompatibilitySummary;
    FString AIReadinessSummary;
    bool bHasAutomaticAnimationIntegrationComponent = false;
    bool bHasAnimationInstance = false;
    bool bAnimationAutoAttachSucceeded = false;
    bool bAnimationAutoWireSucceeded = false;
    bool bAnimationHookStateReadable = false;
    int32 AnimationSupportedAutoWireFieldCount = 0;
    int32 AnimationLastAppliedAutoWireFieldCount = 0;
    EWAYAutomaticAnimationIntegrationStatus AnimationAutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::NotSupported;
    FString AnimationIntegrationTargetLabel;
    FString AnimationAutomaticIntegrationDetail;
    bool bAnimationFacingHookRequested = false;
    bool bAnimationTurnToTargetRequested = false;
    bool bAnimationLocomotionHintSafe = false;
    bool bAnimationMovementLimitedFallbackHint = false;
    bool bAnimationOutwardGuardHintRequested = false;
    bool bAnimationPhysicalReactionStateAvailable = false;
    EWAYAnimationHookApplicationStatus AnimationHookApplicationStatus = EWAYAnimationHookApplicationStatus::NotAvailable;
    EWAYReactionState AnimationReactionState = EWAYReactionState::Observational;
    EWAYBehaviorPreset AnimationRecommendedBehavior = EWAYBehaviorPreset::None;
    EWAYBehaviorExecutionMode AnimationExecutionMode = EWAYBehaviorExecutionMode::Unknown;
    EWAYRelationshipState AnimationRelationshipState = EWAYRelationshipState::Neutral;
    EWanaPhysicalState AnimationPhysicalState = EWanaPhysicalState::Stable;
    float AnimationPhysicalStabilityScore = 1.0f;
    float AnimationPhysicalRecoveryProgress = 1.0f;
    float AnimationPhysicalInstabilityAlpha = 0.0f;
    FVector AnimationPhysicalImpactDirection = FVector::ZeroVector;
    float AnimationPhysicalImpactStrength = 0.0f;
    FString AnimationBehaviorIntent;
    FString AnimationVisibleBehaviorLabel;
    FString AnimationIdentityRoleHint;
    FString AnimationPostureHint;
    FString AnimationPostureCategory;
    FString AnimationFallbackHint;
    FString AnimationHookDetail;
    EWanaPhysicalState PhysicalState = EWanaPhysicalState::Stable;
    float PhysicalStabilityScore = 1.0f;
    float PhysicalRecoveryProgress = 1.0f;
    float PhysicalInstabilityAlpha = 0.0f;
    FVector PhysicalLastImpactDirection = FVector::ZeroVector;
    float PhysicalLastImpactStrength = 0.0f;
    bool bPhysicalBracing = false;
    bool bPhysicalCanCommitToMovement = true;
    bool bPhysicalCanCommitToAttack = true;
    bool bPhysicalNeedsRecovery = false;
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
    FString VisibleBehaviorLabel;
    FString BehaviorExecutionDetail;
    FWanaMovementReadiness MovementReadiness;
    bool bAnimationFacingHookRequested = false;
    bool bAnimationTurnToTargetRequested = false;
    bool bAnimationLocomotionHintSafe = false;
    bool bAnimationMovementLimitedFallbackHint = false;
    bool bAnimationOutwardGuardHintRequested = false;
    bool bAnimationPhysicalReactionStateAvailable = false;
    EWAYAnimationHookApplicationStatus AnimationHookApplicationStatus = EWAYAnimationHookApplicationStatus::NotAvailable;
    EWAYReactionState AnimationReactionState = EWAYReactionState::Observational;
    EWAYBehaviorPreset AnimationRecommendedBehavior = EWAYBehaviorPreset::None;
    EWAYBehaviorExecutionMode AnimationExecutionMode = EWAYBehaviorExecutionMode::Unknown;
    EWAYRelationshipState AnimationRelationshipState = EWAYRelationshipState::Neutral;
    EWanaPhysicalState AnimationPhysicalState = EWanaPhysicalState::Stable;
    float AnimationPhysicalStabilityScore = 1.0f;
    float AnimationPhysicalRecoveryProgress = 1.0f;
    float AnimationPhysicalInstabilityAlpha = 0.0f;
    FVector AnimationPhysicalImpactDirection = FVector::ZeroVector;
    float AnimationPhysicalImpactStrength = 0.0f;
    FString AnimationBehaviorIntent;
    FString AnimationVisibleBehaviorLabel;
    FString AnimationIdentityRoleHint;
    FString AnimationPostureHint;
    FString AnimationPostureCategory;
    FString AnimationFallbackHint;
    FString AnimationHookDetail;
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
    FWanaCommandResponse ExecuteCreateFinalizedBuildAssetFromActorCommand(AActor* SourceActor);
    FWanaCommandResponse ExecutePrepareSelectedActorForAITestCommand();
    FWanaCommandResponse ExecuteEvaluateLiveTargetCommand();
    FWanaCommandResponse ExecuteEvaluateActorPairCommand(AActor* ObserverActor, AActor* TargetActor, bool bTargetFallsBackToObserver = false);
    FWanaCommandResponse ExecuteScanEnvironmentReadinessCommand(AActor* ObserverActor, AActor* TargetActor, const FString& PairSourceLabel, bool bTargetFallsBackToObserver = false);
    FWanaCommandResponse ExecuteFocusActorCommand(AActor* Actor, const FString& RoleLabel);
    FWanaCommandResponse ExecuteApplyStarterAndTestTargetCommand(const FString& PresetLabel);
}

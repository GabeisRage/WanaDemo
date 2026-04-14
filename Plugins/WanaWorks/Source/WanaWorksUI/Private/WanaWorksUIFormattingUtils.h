#pragma once

#include "CoreMinimal.h"
#include "WanaWorksUIEditorActions.h"
#include "WAYRelationshipTypes.h"

struct FWanaCommandResponse;

namespace WanaWorksUIFormattingUtils
{
    FString StripStatusPrefix(const FString& StatusMessage);
    FString GetFeedbackToneLabel(const FString& StatusMessage, bool bSucceeded);
    const TCHAR* GetCharacterEnhancementPresetLabel(const FString& PresetLabel);
    const TCHAR* GetCharacterEnhancementWorkflowLabel(const FString& WorkflowLabel);
    FString GetCharacterEnhancementResultsWorkflowLabel(const FString& WorkflowLabel);
    FString GetEnhancementComponentResultLabel(bool bHasComponent, bool bWasAdded);
    FString GetAnimationReadinessResultLabel(bool bHasSkeletalMeshComponent, bool bHasAnimBlueprint);
    bool IsAIReadyForLightweightTesting(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetAIControllerPresenceLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetAnimationBlueprintStatusLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetAnimationIntegrationStatusLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetAnimationHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetFacingHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetTurnToTargetHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetLocomotionHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetReactionAnimationHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetAnimationIntegrationNotes(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    FString GetCompatibilityStatusLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    const TCHAR* GetRelationshipStateLabel(EWAYRelationshipState RelationshipState);
    bool TryParseRelationshipStateLabel(const FString& Label, EWAYRelationshipState& OutRelationshipState);
    FString GetMovementCapabilitySummaryLabel(const FWanaEnvironmentReadinessSnapshot& Snapshot);
    FString GetNavigationContextSummaryLabel(const FWanaEnvironmentReadinessSnapshot& Snapshot);
    FString GetReachabilitySummaryLabel(const FWanaEnvironmentReadinessSnapshot& Snapshot);
    FString GetFallbackModeSummaryLabel(const FWanaEnvironmentReadinessSnapshot& Snapshot);
    FString FormatResponseOutputLine(const FString& Line);
    void AppendLinesWithPrefix(FWanaCommandResponse& Response, const FWanaCommandResponse& SourceResponse, const TCHAR* Prefix);
    FString GetReactionStateSummaryLabel(EWAYReactionState ReactionState);
    FString GetBehaviorPresetSummaryLabel(EWAYBehaviorPreset BehaviorPreset);
    FString GetBehaviorExecutionModeSummaryLabel(EWAYBehaviorExecutionMode ExecutionMode);
}

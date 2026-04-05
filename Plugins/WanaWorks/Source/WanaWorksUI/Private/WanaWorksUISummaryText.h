#pragma once

#include "CoreMinimal.h"
#include "WanaWorksUIEditorActions.h"

namespace WanaWorksUISummaryText
{
    FString BuildSubjectSetupSummaryText(
        const FWanaSelectedCharacterEnhancementSnapshot* Snapshot,
        const FString& WorkflowLabel,
        const FString& PresetLabel);

    FString BuildSubjectStackSummaryText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot);

    FString BuildSavedSubjectProgressSummaryText(
        bool bInitialized,
        const FString& SubjectLabel,
        const FString& SubjectTypeLabel,
        const FString& WorkflowLabel,
        const FString& PresetLabel,
        const FString& AIControllerResult,
        const FString& AnimationResult,
        const FString& IdentityResult,
        const FString& WAIResult,
        const FString& WAYResult,
        const FString& AIReadyResult);

    FString BuildCharacterEnhancementSummaryText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot);
    FString BuildCharacterEnhancementChainText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot);
    FString BuildCharacterEnhancementWorkflowText(const FString& WorkflowLabel);

    FString BuildEnhancementResultsText(
        bool bInitialized,
        const FString& SubjectLabel,
        const FString& IdentityResult,
        const FString& WAYResult,
        const FString& WAIResult,
        const FString& AIReadyResult,
        const FString& AnimationResult,
        const FString& WorkflowUsed,
        bool bSandboxCopyCreated,
        bool bOriginalPreserved);

    FString BuildBehaviorResultsText(const FWanaBehaviorResultsSnapshot& Snapshot);
    FString BuildWITEnvironmentReadinessText(const FWanaEnvironmentReadinessSnapshot& Snapshot);

    FString BuildTestSandboxSummaryText(
        const FString& CurrentSelectionLabel,
        const FString& ObserverLabel,
        const FString& TargetLabel,
        bool bHasObserver,
        bool bHasTarget,
        bool bReadyToEvaluate,
        bool bSelfTargetDebugPair);
}

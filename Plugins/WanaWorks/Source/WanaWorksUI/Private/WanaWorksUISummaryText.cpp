#include "WanaWorksUISummaryText.h"

#include "WanaWorksUIFormattingUtils.h"

namespace WanaWorksUISummaryText
{
FString BuildSubjectSetupSummaryText(
    const FWanaSelectedCharacterEnhancementSnapshot* Snapshot,
    const FString& WorkflowLabel,
    const FString& PresetLabel)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("Selected Subject: (none)\nDetected Type: Unknown / Unsupported\nCurrent Workflow: Use Original Character\nStarter Preset: Identity Only\nSafe To Enhance: Select a subject in the editor first.");
    }

    return FString::Printf(
        TEXT("Selected Subject: %s\nDetected Type: %s\nCurrent Workflow: %s\nStarter Preset: %s\nSafe To Enhance: %s"),
        *Snapshot->SelectedActorLabel,
        *Snapshot->ActorTypeLabel,
        WanaWorksUIFormattingUtils::GetCharacterEnhancementWorkflowLabel(WorkflowLabel),
        WanaWorksUIFormattingUtils::GetCharacterEnhancementPresetLabel(PresetLabel),
        *WanaWorksUIFormattingUtils::GetCompatibilityStatusLabel(*Snapshot));
}

FString BuildSubjectStackSummaryText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("AI Controller: Missing\nAnimation Blueprint: Unknown\nIdentity: Missing\nWAI: Missing\nWAY: Missing\nAI-Ready: No");
    }

    return FString::Printf(
        TEXT("AI Controller: %s\nAnimation Blueprint: %s\nIdentity: %s\nWAI: %s\nWAY: %s\nAI-Ready: %s"),
        *WanaWorksUIFormattingUtils::GetAIControllerPresenceLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetAnimationBlueprintStatusLabel(*Snapshot),
        Snapshot->bHasIdentityComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot->bHasWAIComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot->bHasWAYComponent ? TEXT("Present") : TEXT("Missing"),
        WanaWorksUIFormattingUtils::IsAIReadyForLightweightTesting(*Snapshot) ? TEXT("Yes") : TEXT("No"));
}

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
    const FString& AIReadyResult)
{
    if (!bInitialized)
    {
        return TEXT("Saved Subject: Not saved yet\nSaved Workflow: Use Save Progress when you want to keep this setup snapshot handy for the current Wana Works session.");
    }

    return FString::Printf(
        TEXT("Saved Subject: %s\nDetected Type: %s\nWorkflow: %s\nStarter Preset: %s\nAI Controller: %s\nAnimation Blueprint: %s\nIdentity: %s\nWAI: %s\nWAY: %s\nAI-Ready: %s"),
        SubjectLabel.IsEmpty() ? TEXT("(none)") : *SubjectLabel,
        SubjectTypeLabel.IsEmpty() ? TEXT("Unknown / Unsupported") : *SubjectTypeLabel,
        WorkflowLabel.IsEmpty() ? TEXT("Use Original Character") : *WorkflowLabel,
        PresetLabel.IsEmpty() ? TEXT("Identity Only") : *PresetLabel,
        AIControllerResult.IsEmpty() ? TEXT("Missing") : *AIControllerResult,
        AnimationResult.IsEmpty() ? TEXT("Unknown") : *AnimationResult,
        IdentityResult.IsEmpty() ? TEXT("Missing") : *IdentityResult,
        WAIResult.IsEmpty() ? TEXT("Missing") : *WAIResult,
        WAYResult.IsEmpty() ? TEXT("Missing") : *WAYResult,
        AIReadyResult.IsEmpty() ? TEXT("No") : *AIReadyResult);
}

FString BuildCharacterEnhancementSummaryText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("Selected Actor Name: (none)\nActor Type: (none)\nIdentity: Missing\nWAY: Missing\nWAI: Missing\nAnimation: Missing\nAI Ready: Warning\nCompatibility: Warning");
    }

    return FString::Printf(
        TEXT("Selected Actor Name: %s\nActor Type: %s\nIdentity: %s\nWAY: %s\nWAI: %s\nAnimation: %s\nAI Ready: %s\nCompatibility: %s"),
        *Snapshot->SelectedActorLabel,
        *Snapshot->ActorTypeLabel,
        Snapshot->bHasIdentityComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot->bHasWAYComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot->bHasWAIComponent ? TEXT("Present") : TEXT("Missing"),
        *Snapshot->AnimationCompatibilitySummary,
        *Snapshot->AIReadinessSummary,
        *WanaWorksUIFormattingUtils::GetCompatibilityStatusLabel(*Snapshot));
}

FString BuildCharacterEnhancementChainText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot)
{
    const TCHAR* BehaviorGraphStatus = Snapshot && Snapshot->bHasSelectedActor && Snapshot->bHasWAIComponent ? TEXT("READY") : TEXT("ACTIVE");
    const TCHAR* CombatLogicStatus = Snapshot && Snapshot->bHasSelectedActor && Snapshot->bHasWAYComponent ? TEXT("READY") : TEXT("ACTIVE");
    const TCHAR* AnimationHooksStatus = !Snapshot || !Snapshot->bHasSelectedActor
        ? TEXT("ACTIVE")
        : (Snapshot->bHasAnimBlueprint ? TEXT("READY") : (Snapshot->bHasSkeletalMeshComponent ? TEXT("WARNING") : TEXT("ACTIVE")));

    return FString::Printf(
        TEXT("%s  Behavior Graph Ready\n%s  Combat Logic Synced\n%s  Animation Hooks Attached"),
        BehaviorGraphStatus,
        CombatLogicStatus,
        AnimationHooksStatus);
}

FString BuildCharacterEnhancementWorkflowText(const FString& WorkflowLabel)
{
    if (WorkflowLabel == TEXT("Create Sandbox Duplicate"))
    {
        return TEXT("Safest option for testing. Creates a clearly named WanaWorks sandbox copy, keeps the original actor untouched, and selects the duplicate for continued setup.");
    }

    if (WorkflowLabel == TEXT("Convert to AI-Ready Test Subject"))
    {
        return TEXT("Keeps the current actor, applies WanaAI, and adds lightweight AI-ready pawn prep without replacing Character BP or Anim BP assets.");
    }

    return TEXT("Enhances the currently selected actor in place. WanaWorks adds to the existing setup instead of replacing Character BP or Animation Blueprint logic.");
}

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
    bool bOriginalPreserved)
{
    if (!bInitialized)
    {
        return TEXT("Subject: (none)\nIdentity: Missing\nWAY: Missing\nWAI: Missing\nAI-Ready: No\nAnimation Readiness: Missing\nWorkflow Used: Not run yet\nSandbox Copy: Not Used\nOriginal Preserved: Yes");
    }

    return FString::Printf(
        TEXT("Subject: %s\nIdentity: %s\nWAY: %s\nWAI: %s\nAI-Ready: %s\nAnimation Readiness: %s\nWorkflow Used: %s\nSandbox Copy: %s\nOriginal Preserved: %s"),
        SubjectLabel.IsEmpty() ? TEXT("(none)") : *SubjectLabel,
        *IdentityResult,
        *WAYResult,
        *WAIResult,
        *AIReadyResult,
        *AnimationResult,
        WorkflowUsed.IsEmpty() ? TEXT("Not run yet") : *WorkflowUsed,
        bSandboxCopyCreated ? TEXT("Created") : TEXT("Not Used"),
        bOriginalPreserved ? TEXT("Yes") : TEXT("No"));
}

FString BuildBehaviorResultsText(const FWanaBehaviorResultsSnapshot& Snapshot)
{
    const FString RelationshipStateLabel = Snapshot.bHasWAYComponent
        ? (Snapshot.bHasRelationshipProfile
            ? FString(WanaWorksUIFormattingUtils::GetRelationshipStateLabel(Snapshot.RelationshipState))
            : TEXT("(not evaluated yet)"))
        : TEXT("(observer missing WAY)");
    const FString ReactionStateLabel = Snapshot.bHasWAYComponent
        ? WanaWorksUIFormattingUtils::GetReactionStateSummaryLabel(Snapshot.ReactionState)
        : TEXT("(not available)");
    const FString RecommendedBehaviorLabel = Snapshot.bHasWAYComponent
        ? WanaWorksUIFormattingUtils::GetBehaviorPresetSummaryLabel(Snapshot.RecommendedBehavior)
        : TEXT("(not available)");
    const FString LastAppliedHookLabel = Snapshot.bHasWAYComponent
        ? WanaWorksUIFormattingUtils::GetBehaviorPresetSummaryLabel(Snapshot.LastAppliedHook)
        : TEXT("None");
    const FString ExecutionModeLabel = Snapshot.bHasWAYComponent
        ? WanaWorksUIFormattingUtils::GetBehaviorExecutionModeSummaryLabel(Snapshot.ExecutionMode)
        : TEXT("Unknown");

    return FString::Printf(
        TEXT("Observer: %s\nTarget: %s%s\nRelationship State: %s\nReaction State: %s\nRecommended Behavior: %s\nStarter Hook Available: %s\nLast Applied Hook: %s\nExecution Mode: %s"),
        Snapshot.bHasObserverActor ? *Snapshot.ObserverActorLabel : TEXT("(not assigned)"),
        Snapshot.bHasTargetActor ? *Snapshot.TargetActorLabel : TEXT("(not assigned)"),
        Snapshot.bTargetFallsBackToObserver ? TEXT(" (observer fallback)") : TEXT(""),
        *RelationshipStateLabel,
        *ReactionStateLabel,
        *RecommendedBehaviorLabel,
        Snapshot.bStarterHookAvailable ? TEXT("Yes") : TEXT("No"),
        *LastAppliedHookLabel,
        *ExecutionModeLabel);
}

FString BuildWITEnvironmentReadinessText(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    return FString::Printf(
        TEXT("Pair Source: %s\nObserver: %s\nTarget: %s\nMovement Capability: %s\nNavigation Context: %s\nReachability: %s\nFallback Mode: %s\nReadiness Detail: %s"),
        Snapshot.PairSourceLabel.IsEmpty() ? TEXT("No active pair") : *Snapshot.PairSourceLabel,
        Snapshot.bHasObserverActor ? *Snapshot.ObserverActorLabel : TEXT("(not assigned)"),
        Snapshot.bHasTargetActor ? *Snapshot.TargetActorLabel : TEXT("(not assigned)"),
        *WanaWorksUIFormattingUtils::GetMovementCapabilitySummaryLabel(Snapshot),
        *WanaWorksUIFormattingUtils::GetNavigationContextSummaryLabel(Snapshot),
        *WanaWorksUIFormattingUtils::GetReachabilitySummaryLabel(Snapshot),
        *WanaWorksUIFormattingUtils::GetFallbackModeSummaryLabel(Snapshot),
        Snapshot.MovementReadiness.Detail.IsEmpty() ? TEXT("No readiness details available.") : *Snapshot.MovementReadiness.Detail);
}

FString BuildTestSandboxSummaryText(
    const FString& CurrentSelectionLabel,
    const FString& ObserverLabel,
    const FString& TargetLabel,
    bool bHasObserver,
    bool bHasTarget,
    bool bReadyToEvaluate,
    bool bSelfTargetDebugPair)
{
    const FString EvaluationMode = !bReadyToEvaluate
        ? TEXT("Assignment incomplete")
        : (bSelfTargetDebugPair ? TEXT("Self-target debug") : TEXT("Explicit observer and target"));
    const FString NextStep = !bHasObserver
        ? TEXT("Use Selected as Observer to assign the evaluating actor.")
        : (!bHasTarget
            ? TEXT("Use Selected as Target to assign the actor being evaluated.")
            : (bSelfTargetDebugPair
                ? TEXT("Self-target debug mode is active. Assign a different target for normal observer-target testing.")
                : TEXT("Ready to evaluate the explicit sandbox pair.")));

    return FString::Printf(
        TEXT("Current Selection: %s\nSandbox Observer: %s\nSandbox Target: %s\nEvaluation Mode: %s\nReady To Evaluate: %s\nNext Step: %s"),
        *CurrentSelectionLabel,
        *ObserverLabel,
        *TargetLabel,
        *EvaluationMode,
        bReadyToEvaluate ? TEXT("Yes") : TEXT("No"),
        *NextStep);
}
}

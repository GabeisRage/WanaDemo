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
        return TEXT("Current Subject: (none)\nSubject Source: WanaWorks picker or editor selection fallback\nDetected Type: Unknown / Unsupported\nCurrent Workflow: Use Original Character\nStarter Preset: Identity Only\nSafe To Enhance: Choose a Character Pawn or AI Pawn in WanaWorks. Editor selection still works as a fallback shortcut.");
    }

    return FString::Printf(
        TEXT("Current Subject: %s\nSubject Source: %s\nDetected Type: %s\nCurrent Workflow: %s\nStarter Preset: %s\nSafe To Enhance: %s"),
        *Snapshot->SelectedActorLabel,
        Snapshot->SubjectSourceLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot->SubjectSourceLabel,
        *Snapshot->ActorTypeLabel,
        WanaWorksUIFormattingUtils::GetCharacterEnhancementWorkflowLabel(WorkflowLabel),
        WanaWorksUIFormattingUtils::GetCharacterEnhancementPresetLabel(PresetLabel),
        *WanaWorksUIFormattingUtils::GetCompatibilityStatusLabel(*Snapshot));
}

FString BuildSubjectStackSummaryText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("AI Controller: Missing\nLinked AI Controller: (not linked)\nAnimation Blueprint: Unknown\nLinked Animation Blueprint: (not linked)\nIdentity: Missing\nWAI: Missing\nWAY: Missing\nAI-Ready: No\nCompatibility Notes: Pick a project asset to inspect the stack before you create a sandbox subject.");
    }

    return FString::Printf(
        TEXT("AI Controller: %s\nLinked AI Controller: %s\nAnimation Blueprint: %s\nLinked Animation Blueprint: %s\nIdentity: %s\nWAI: %s\nWAY: %s\nAI-Ready: %s\nCompatibility Notes: %s"),
        *WanaWorksUIFormattingUtils::GetAIControllerPresenceLabel(*Snapshot),
        Snapshot->LinkedAIControllerLabel.IsEmpty() ? TEXT("(not linked)") : *Snapshot->LinkedAIControllerLabel,
        *WanaWorksUIFormattingUtils::GetAnimationBlueprintStatusLabel(*Snapshot),
        Snapshot->LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("(not linked)") : *Snapshot->LinkedAnimationBlueprintLabel,
        Snapshot->bHasIdentityComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot->bHasWAIComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot->bHasWAYComponent ? TEXT("Present") : TEXT("Missing"),
        WanaWorksUIFormattingUtils::IsAIReadyForLightweightTesting(*Snapshot) ? TEXT("Yes") : TEXT("No"),
        *Snapshot->AIReadinessSummary);
}

FString BuildAnimationIntegrationSummaryText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("Animation Blueprint: Unknown\nIntegration Status: Unknown\nExisting Anim BP Preserved: Yes\nHook Application: Not Available\nHook Readiness: Missing\nFacing Hook: Missing\nTurn-To-Target Hook: Missing\nLocomotion Hook: Missing\nReaction Animation Hook: Missing\nNotes: Choose a subject in WanaWorks to inspect safe animation integration.");
    }

    return FString::Printf(
        TEXT("Animation Blueprint: %s\nIntegration Status: %s\nExisting Anim BP Preserved: Yes\nHook Application: %s\nHook Readiness: %s\nFacing Hook: %s\nTurn-To-Target Hook: %s\nLocomotion Hook: %s\nReaction Animation Hook: %s\nNotes: %s"),
        *WanaWorksUIFormattingUtils::GetAnimationBlueprintStatusLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetAnimationIntegrationStatusLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetAnimationHookApplicationStatusLabel(Snapshot->AnimationHookApplicationStatus),
        *WanaWorksUIFormattingUtils::GetAnimationHookReadinessLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetFacingHookReadinessLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetTurnToTargetHookReadinessLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetLocomotionHookReadinessLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetReactionAnimationHookReadinessLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetAnimationIntegrationNotes(*Snapshot));
}

FString BuildAnimationHookUsageText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("Hook Source: WAYPlayerProfileComponent -> Get Current Animation Hook State\nCurrent Hook Application: Not Available\nCore Fields: bFacingHookRequested, bTurnToTargetRequested, ReactionState, RecommendedBehavior, bLocomotionSafeExecutionHint\nTypical Use: Use facing and turn flags for turn-to-target logic. Use ReactionState for reaction-driven animation branching. Use RecommendedBehavior for higher-level animation intent. Use the locomotion-safe hint to avoid forcing movement-driven animation logic.\nAnim BP Example: Get Owning Actor -> Get Component By Class (WAYPlayerProfileComponent) -> Get Current Animation Hook State\nNotes: Pick a Character Pawn or AI Pawn in WanaWorks to see subject-specific hook guidance.");
    }

    const FString HookApplicationLabel = WanaWorksUIFormattingUtils::GetAnimationHookApplicationStatusLabel(Snapshot->AnimationHookApplicationStatus);
    const FString FacingFieldSummary = Snapshot->bAnimationFacingHookRequested
        ? TEXT("true right now. Good for active face-target or aim-offset style logic.")
        : TEXT("false right now. Keep facing logic relaxed unless your Anim BP already has another turn driver.");
    const FString TurnFieldSummary = Snapshot->bAnimationTurnToTargetRequested
        ? TEXT("true right now. Good for turn-in-place or turn-to-target branching.")
        : TEXT("false right now. A hard turn request is not currently being pushed.");
    const FString ReactionFieldSummary = Snapshot->AnimationHookApplicationStatus == EWAYAnimationHookApplicationStatus::NotAvailable
        ? TEXT("Available once a live WAY-driven subject is present. Use it for reaction-driven animation branching.")
        : TEXT("Available from the current hook state. Use it for reaction-driven animation branching.");
    const FString BehaviorFieldSummary = Snapshot->AnimationHookApplicationStatus == EWAYAnimationHookApplicationStatus::Active
        ? TEXT("Available from the current hook state. Use it as higher-level animation intent.")
        : TEXT("Available when the subject has a live WAY hook state. Use it as higher-level animation intent.");
    const FString LocomotionHintSummary = Snapshot->bAnimationLocomotionHintSafe
        ? TEXT("true right now. Movement-aware animation logic can stay lightweight and compatibility-friendly.")
        : TEXT("false right now. Prefer facing, turn, or hold-state logic instead of forcing movement-driven animation behavior.");
    const FString Notes = Snapshot->AnimationHookDetail.IsEmpty()
        ? TEXT("WanaWorks preserves the current Animation Blueprint and only exposes safe hook state on top.")
        : Snapshot->AnimationHookDetail;

    return FString::Printf(
        TEXT("Hook Source: WAYPlayerProfileComponent -> Get Current Animation Hook State\nCurrent Hook Application: %s\nCurrent Subject: %s\nDetected Animation Blueprint: %s\nCore Fields:\n- bFacingHookRequested: %s\n- bTurnToTargetRequested: %s\n- ReactionState: %s\n- RecommendedBehavior: %s\n- bLocomotionSafeExecutionHint: %s\nTypical Use: Use facing and turn flags for turn-to-target logic. Use ReactionState for reaction-driven animation branching. Use RecommendedBehavior for higher-level animation intent. Use the locomotion-safe hint to avoid forcing movement-driven animation logic.\nAnim BP Example: Get Owning Actor -> Get Component By Class (WAYPlayerProfileComponent) -> Get Current Animation Hook State\nNotes: %s"),
        *HookApplicationLabel,
        *Snapshot->SelectedActorLabel,
        Snapshot->LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("(not linked)") : *Snapshot->LinkedAnimationBlueprintLabel,
        *FacingFieldSummary,
        *TurnFieldSummary,
        *ReactionFieldSummary,
        *BehaviorFieldSummary,
        *LocomotionHintSummary,
        *Notes);
}

FString BuildWorkflowPresetSummaryText(
    const FString& PresetLabel,
    bool bIsCustomPreset,
    bool bHasSavedCustomPreset,
    const FString& SavedTimestamp,
    const FString& SourceLabel,
    const FString& WorkflowLabel,
    const FString& EnhancementPresetLabel,
    const FString& IdentitySeedLabel,
    const FString& RelationshipLabel,
    const FString& RecommendedBehaviorLabel,
    const FString& CoverageLabel,
    const FString& ApplyNotes,
    const FString& PersistenceNotes)
{
    if (bIsCustomPreset && !bHasSavedCustomPreset)
    {
        return TEXT("Preset: Saved Custom Preset\nType: Custom Saved\nSaved At: Not saved yet\nWorkflow Path: (not captured)\nStarter Preset: (not captured)\nIdentity Seed: Neutral\nRelationship Default: Neutral\nRecommended Behavior: None\nCoverage: Save Current as Preset to capture the current workflow.\nNotes: The custom preset slot is empty right now.");
    }

    return FString::Printf(
        TEXT("Preset: %s\nType: %s\nSaved At: %s\nSource Subject: %s\nWorkflow Path: %s\nStarter Preset: %s\nIdentity Seed: %s\nRelationship Default: %s\nRecommended Behavior: %s\nCoverage: %s\nNotes: %s\nPersistence: %s"),
        PresetLabel.IsEmpty() ? TEXT("Full WanaAI Starter") : *PresetLabel,
        bIsCustomPreset ? TEXT("Custom Saved") : TEXT("Built-In Starter"),
        SavedTimestamp.IsEmpty() ? TEXT("Built-in") : *SavedTimestamp,
        SourceLabel.IsEmpty() ? TEXT("(built-in preset)") : *SourceLabel,
        WorkflowLabel.IsEmpty() ? TEXT("Use Original Character") : *WorkflowLabel,
        EnhancementPresetLabel.IsEmpty() ? TEXT("Identity Only") : *EnhancementPresetLabel,
        IdentitySeedLabel.IsEmpty() ? TEXT("Neutral") : *IdentitySeedLabel,
        RelationshipLabel.IsEmpty() ? TEXT("Neutral") : *RelationshipLabel,
        RecommendedBehaviorLabel.IsEmpty() ? TEXT("None") : *RecommendedBehaviorLabel,
        CoverageLabel.IsEmpty() ? TEXT("Identity") : *CoverageLabel,
        ApplyNotes.IsEmpty() ? TEXT("No notes available.") : *ApplyNotes,
        PersistenceNotes.IsEmpty() ? TEXT("Built into the current workflow.") : *PersistenceNotes);
}

FString BuildSavedSubjectProgressSummaryText(
    bool bInitialized,
    const FString& SavedTimestamp,
    const FString& SubjectPath,
    const FString& SubjectLabel,
    const FString& SubjectTypeLabel,
    const FString& WorkflowLabel,
    const FString& PresetLabel,
    const FString& AIControllerResult,
    const FString& AnimationResult,
    const FString& IdentityResult,
    const FString& WAIResult,
    const FString& WAYResult,
    const FString& AIReadyResult,
    const FString& ObserverLabel,
    const FString& TargetLabel)
{
    if (!bInitialized)
    {
        return TEXT("Saved Subject: Not saved yet\nSaved Workflow: Use Save Progress to keep this setup snapshot in Wana Works for later steps and future editor sessions.");
    }

    return FString::Printf(
        TEXT("Saved At: %s\nSaved Subject: %s\nSubject Path: %s\nDetected Type: %s\nWorkflow: %s\nStarter Preset: %s\nSaved Observer: %s\nSaved Target: %s\nAI Controller: %s\nAnimation Blueprint: %s\nIdentity: %s\nWAI: %s\nWAY: %s\nAI-Ready: %s\nPersistence: Stored in editor settings for this project. Use Restore Last Saved State to bring back workflow choices and saved actor assignments."),
        SavedTimestamp.IsEmpty() ? TEXT("Unknown") : *SavedTimestamp,
        SubjectLabel.IsEmpty() ? TEXT("(none)") : *SubjectLabel,
        SubjectPath.IsEmpty() ? TEXT("(not stored)") : *SubjectPath,
        SubjectTypeLabel.IsEmpty() ? TEXT("Unknown / Unsupported") : *SubjectTypeLabel,
        WorkflowLabel.IsEmpty() ? TEXT("Use Original Character") : *WorkflowLabel,
        PresetLabel.IsEmpty() ? TEXT("Identity Only") : *PresetLabel,
        ObserverLabel.IsEmpty() ? TEXT("(none saved)") : *ObserverLabel,
        TargetLabel.IsEmpty() ? TEXT("(none saved)") : *TargetLabel,
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
        return TEXT("Current Subject: (none)\nSubject Source: WanaWorks picker or editor selection fallback\nActor Type: (none)\nIdentity: Missing\nWAY: Missing\nWAI: Missing\nAnimation: Missing\nAI Ready: Warning\nCompatibility: Warning");
    }

    return FString::Printf(
        TEXT("Current Subject: %s\nSubject Source: %s\nActor Type: %s\nIdentity: %s\nWAY: %s\nWAI: %s\nAnimation: %s\nAI Ready: %s\nCompatibility: %s"),
        *Snapshot->SelectedActorLabel,
        Snapshot->SubjectSourceLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot->SubjectSourceLabel,
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
    const FString AnimationHookApplicationLabel = WanaWorksUIFormattingUtils::GetAnimationHookApplicationStatusLabel(Snapshot.AnimationHookApplicationStatus);
    const FString FacingHookLabel = WanaWorksUIFormattingUtils::GetAnimationHookRequestSummaryLabel(Snapshot.bAnimationFacingHookRequested);
    const FString TurnToTargetHookLabel = WanaWorksUIFormattingUtils::GetAnimationHookRequestSummaryLabel(Snapshot.bAnimationTurnToTargetRequested);

    return FString::Printf(
        TEXT("Observer: %s\nTarget: %s%s\nRelationship State: %s\nReaction State: %s\nRecommended Behavior: %s\nStarter Hook Available: %s\nLast Applied Hook: %s\nExecution Mode: %s\nAnimation Hook Application: %s\nFacing Hook: %s\nTurn-To-Target Hook: %s"),
        Snapshot.bHasObserverActor ? *Snapshot.ObserverActorLabel : TEXT("(not assigned)"),
        Snapshot.bHasTargetActor ? *Snapshot.TargetActorLabel : TEXT("(not assigned)"),
        Snapshot.bTargetFallsBackToObserver ? TEXT(" (observer fallback)") : TEXT(""),
        *RelationshipStateLabel,
        *ReactionStateLabel,
        *RecommendedBehaviorLabel,
        Snapshot.bStarterHookAvailable ? TEXT("Yes") : TEXT("No"),
        *LastAppliedHookLabel,
        *ExecutionModeLabel,
        *AnimationHookApplicationLabel,
        *FacingHookLabel,
        *TurnToTargetHookLabel);
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

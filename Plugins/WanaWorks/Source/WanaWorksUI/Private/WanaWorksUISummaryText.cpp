#include "WanaWorksUISummaryText.h"

#include "GameFramework/Actor.h"
#include "WanaWorksUIFormattingUtils.h"

namespace WanaWorksUISummaryText
{
namespace
{
FString GetImpactSourceDirectionLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (Snapshot.PhysicalLastImpactStrength <= KINDA_SMALL_NUMBER || Snapshot.PhysicalLastImpactDirection.IsNearlyZero())
    {
        return TEXT("No active impact");
    }

    const FVector ImpactDirection = Snapshot.PhysicalLastImpactDirection.GetSafeNormal();
    const AActor* SubjectActor = Snapshot.SelectedActor.Get();

    if (SubjectActor)
    {
        const float RightDot = FVector::DotProduct(ImpactDirection, SubjectActor->GetActorRightVector().GetSafeNormal());
        const float ForwardDot = FVector::DotProduct(ImpactDirection, SubjectActor->GetActorForwardVector().GetSafeNormal());

        if (FMath::Abs(RightDot) >= FMath::Abs(ForwardDot))
        {
            return RightDot >= 0.0f ? TEXT("From Left") : TEXT("From Right");
        }

        return ForwardDot >= 0.0f ? TEXT("From Behind") : TEXT("From Front");
    }

    return TEXT("Directional impact recorded");
}

FString GetImpactEffectSummaryText(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (Snapshot.PhysicalLastImpactStrength <= KINDA_SMALL_NUMBER)
    {
        return TEXT("No active impact effect");
    }

    const FString StrengthLabel = Snapshot.PhysicalLastImpactStrength < 0.18f
        ? TEXT("Light")
        : (Snapshot.PhysicalLastImpactStrength < 0.42f
            ? TEXT("Medium")
            : (Snapshot.PhysicalLastImpactStrength < 0.72f
                ? TEXT("Strong")
                : TEXT("Severe")));

    FString StateLabel;

    switch (Snapshot.PhysicalState)
    {
    case EWanaPhysicalState::Bracing:
        StateLabel = TEXT("brace response");
        break;
    case EWanaPhysicalState::Alert:
        StateLabel = TEXT("alerted impact response");
        break;
    case EWanaPhysicalState::Staggered:
        StateLabel = TEXT("directional stagger");
        break;
    case EWanaPhysicalState::OffBalance:
        StateLabel = TEXT("off-balance disruption");
        break;
    case EWanaPhysicalState::Panicked:
        StateLabel = TEXT("severe destabilization");
        break;
    case EWanaPhysicalState::Recovering:
        StateLabel = TEXT("recovery posture");
        break;
    case EWanaPhysicalState::Stable:
    default:
        StateLabel = TEXT("light physical disturbance");
        break;
    }

    return Snapshot.bPhysicalNeedsRecovery
        ? FString::Printf(TEXT("%s %s with active recovery"), *StrengthLabel, *StateLabel)
        : FString::Printf(TEXT("%s %s"), *StrengthLabel, *StateLabel);
}

FString GetAnimationHookConsumptionReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("Not Supported");
    }

    if (!Snapshot.bHasWAYComponent || !Snapshot.bAnimationHookStateReadable)
    {
        return TEXT("Needs Enhance");
    }

    if (!Snapshot.bHasAnimBlueprint || !Snapshot.bHasAnimationInstance)
    {
        return TEXT("Limited");
    }

    if (Snapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Applied
        || Snapshot.AnimationHookApplicationStatus == EWAYAnimationHookApplicationStatus::Active)
    {
        return TEXT("Applied");
    }

    if (Snapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Ready)
    {
        return TEXT("Ready");
    }

    return TEXT("Limited");
}

FString GetAnimationHookConsumptionDetailText(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    return FString::Printf(
        TEXT("Skeletal mesh %s. Anim BP %s. Animation instance %s. Hook provider %s. Hook state %s. Physical provider %s. Auto-wire %s."),
        Snapshot.bHasSkeletalMeshComponent ? TEXT("Ready") : TEXT("Not Supported"),
        Snapshot.bHasAnimBlueprint ? TEXT("Ready") : TEXT("Limited"),
        Snapshot.bHasAnimationInstance ? TEXT("Ready") : TEXT("Limited"),
        Snapshot.bHasWAYComponent ? TEXT("Ready") : TEXT("Needs Enhance"),
        Snapshot.bAnimationHookStateReadable ? TEXT("Ready") : TEXT("Needs Enhance"),
        Snapshot.bHasPhysicalStateComponent ? TEXT("Ready") : TEXT("Limited"),
        *WanaWorksUIFormattingUtils::GetAutomaticAnimationWireSummaryLabel(Snapshot));
}

FString GetAnimBPAdapterReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("Not Supported");
    }

    if (!Snapshot.bHasAnimBlueprint)
    {
        return TEXT("Limited");
    }

    if (!Snapshot.bHasWAYComponent || !Snapshot.bAnimationHookStateReadable)
    {
        return TEXT("Needs Enhance");
    }

    if (!Snapshot.bHasAnimationInstance || !Snapshot.bHasPhysicalStateComponent)
    {
        return TEXT("Partially Ready");
    }

    return TEXT("Ready");
}

FString GetAdapterStrategySummary(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("Not Supported");
    }

    if (!Snapshot.bHasAnimBlueprint)
    {
        return TEXT("Needs Enhance");
    }

    if (!Snapshot.bHasWAYComponent || !Snapshot.bAnimationHookStateReadable)
    {
        return TEXT("Needs Enhance");
    }

    return TEXT("Generated Adapter");
}

FString GetAnimationPreviewBodyLanguageLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    TArray<FString> StateLabels;

    if (!Snapshot.AnimationPostureCategory.IsEmpty())
    {
        if (Snapshot.AnimationPostureCategory.Equals(TEXT("Guard"), ESearchCase::IgnoreCase))
        {
            StateLabels.Add(TEXT("Protective"));
        }
        else if (Snapshot.AnimationPostureCategory.Equals(TEXT("Follow"), ESearchCase::IgnoreCase))
        {
            StateLabels.Add(TEXT("Cooperative"));
        }
        else if (Snapshot.AnimationPostureCategory.Equals(TEXT("Hostile"), ESearchCase::IgnoreCase))
        {
            StateLabels.Add(TEXT("Tense"));
        }
        else if (Snapshot.AnimationPostureCategory.Equals(TEXT("Observe"), ESearchCase::IgnoreCase))
        {
            StateLabels.Add(TEXT("Observant"));
        }
        else
        {
            StateLabels.Add(Snapshot.AnimationPostureCategory);
        }
    }
    else if (!Snapshot.AnimationPostureHint.IsEmpty())
    {
        StateLabels.Add(Snapshot.AnimationPostureHint);
    }
    else
    {
        StateLabels.Add(TEXT("Observant"));
    }

    if (Snapshot.bPhysicalBracing || Snapshot.AnimationPhysicalState == EWanaPhysicalState::Bracing)
    {
        StateLabels.AddUnique(TEXT("Braced"));
    }

    if (Snapshot.bPhysicalNeedsRecovery || Snapshot.PhysicalState == EWanaPhysicalState::Recovering || Snapshot.AnimationPhysicalState == EWanaPhysicalState::Recovering)
    {
        StateLabels.AddUnique(TEXT("Recovering"));
    }

    if (Snapshot.PhysicalState == EWanaPhysicalState::Staggered
        || Snapshot.PhysicalState == EWanaPhysicalState::OffBalance
        || Snapshot.PhysicalState == EWanaPhysicalState::Panicked
        || Snapshot.AnimationPhysicalState == EWanaPhysicalState::Staggered
        || Snapshot.AnimationPhysicalState == EWanaPhysicalState::OffBalance
        || Snapshot.AnimationPhysicalState == EWanaPhysicalState::Panicked)
    {
        StateLabels.AddUnique(TEXT("Disrupted"));
    }

    if (Snapshot.bAnimationMovementLimitedFallbackHint)
    {
        StateLabels.AddUnique(TEXT("Movement Limited"));
    }

    if (!Snapshot.bAnimationLocomotionHintSafe && Snapshot.bAnimationFacingHookRequested)
    {
        StateLabels.AddUnique(TEXT("Facing Fallback"));
    }

    return FString::Join(StateLabels, TEXT(", "));
}

FString GetAnimationPreviewImpactLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bAnimationPhysicalReactionStateAvailable || Snapshot.AnimationPhysicalImpactStrength <= KINDA_SMALL_NUMBER)
    {
        return TEXT("No active animation impact direction");
    }

    return FString::Printf(
        TEXT("%s strength %.2f"),
        *Snapshot.AnimationPhysicalImpactDirection.GetSafeNormal().ToCompactString(),
        Snapshot.AnimationPhysicalImpactStrength);
}

FString BuildAnimationPreviewConsumptionText(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    const FString ReactionLabel = WanaWorksUIFormattingUtils::GetReactionStateSummaryLabel(Snapshot.AnimationReactionState);
    const FString BehaviorLabel = Snapshot.AnimationBehaviorIntent.IsEmpty()
        ? WanaWorksUIFormattingUtils::GetBehaviorPresetSummaryLabel(Snapshot.AnimationRecommendedBehavior)
        : Snapshot.AnimationBehaviorIntent;
    const FString PostureLabel = Snapshot.AnimationPostureHint.IsEmpty()
        ? TEXT("neutral / observant")
        : Snapshot.AnimationPostureHint;
    const FString MovementLabel = Snapshot.bAnimationLocomotionHintSafe
        ? TEXT("locomotion-safe")
        : (Snapshot.bAnimationMovementLimitedFallbackHint ? TEXT("movement limited") : TEXT("facing/hold preferred"));

    return FString::Printf(
        TEXT("Body Language: %s\nPosture Hint: %s\nBehavior Intent: %s\nReaction State: %s\nFacing Hook: %s\nTurn-To-Target Hook: %s\nLocomotion Hint: %s\nMovement Fallback: %s\nInstability: %.2f\nRecovery: %d%%\nImpact Direction: %s\nHook Consumption: %s - %s"),
        *GetAnimationPreviewBodyLanguageLabel(Snapshot),
        *PostureLabel,
        *BehaviorLabel,
        *ReactionLabel,
        *WanaWorksUIFormattingUtils::GetAnimationHookRequestSummaryLabel(Snapshot.bAnimationFacingHookRequested),
        *WanaWorksUIFormattingUtils::GetAnimationHookRequestSummaryLabel(Snapshot.bAnimationTurnToTargetRequested),
        *MovementLabel,
        Snapshot.bAnimationMovementLimitedFallbackHint ? (Snapshot.AnimationFallbackHint.IsEmpty() ? TEXT("safe hold") : *Snapshot.AnimationFallbackHint) : TEXT("Idle"),
        Snapshot.AnimationPhysicalInstabilityAlpha,
        FMath::RoundToInt(Snapshot.AnimationPhysicalRecoveryProgress * 100.0f),
        *GetAnimationPreviewImpactLabel(Snapshot),
        *GetAnimationHookConsumptionReadinessLabel(Snapshot),
        *GetAnimationHookConsumptionDetailText(Snapshot));
}
}

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
        return TEXT("Character Blueprint: Unknown\nSkeletal Mesh: Unknown\nSkeleton: Unknown\nAnimation Blueprint: Unknown\nPlayer Controller: Unknown\nAI Controller: Unknown\nControl Mode: Unknown\nIdentity: Missing\nCompatibility Notes: Pick a Character Blueprint, Pawn, or AI-ready character asset to inspect the shared body/control stack.");
    }

    return FString::Printf(
        TEXT("Character Blueprint: %s\nSkeletal Mesh: %s\nSkeleton: %s\nAnimation Blueprint: %s\nLinked Animation Blueprint: %s\nPlayer Controller: %s\nAuto Possess Player: %s\nAI Controller: %s\nLinked AI Controller: %s\nControl Mode: %s\nIdentity: %s\nWAI: %s\nWAY: %s\nAI-Ready: %s\nCompatibility Notes: %s"),
        *Snapshot->SelectedActorLabel,
        Snapshot->SkeletalMeshLabel.IsEmpty() ? (Snapshot->bHasSkeletalMeshComponent ? TEXT("(mesh component detected)") : TEXT("Missing")) : *Snapshot->SkeletalMeshLabel,
        Snapshot->SkeletonLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot->SkeletonLabel,
        *WanaWorksUIFormattingUtils::GetAnimationBlueprintStatusLabel(*Snapshot),
        Snapshot->LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("(not linked)") : *Snapshot->LinkedAnimationBlueprintLabel,
        Snapshot->LinkedPlayerControllerLabel.IsEmpty() ? TEXT("(not detected)") : *Snapshot->LinkedPlayerControllerLabel,
        Snapshot->AutoPossessPlayerLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot->AutoPossessPlayerLabel,
        *WanaWorksUIFormattingUtils::GetAIControllerPresenceLabel(*Snapshot),
        Snapshot->LinkedAIControllerLabel.IsEmpty() ? TEXT("(not linked)") : *Snapshot->LinkedAIControllerLabel,
        Snapshot->CharacterControlModeLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot->CharacterControlModeLabel,
        Snapshot->bHasIdentityComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot->bHasWAIComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot->bHasWAYComponent ? TEXT("Present") : TEXT("Missing"),
        WanaWorksUIFormattingUtils::IsAIReadyForLightweightTesting(*Snapshot) ? TEXT("Yes") : TEXT("No"),
        Snapshot->CompatibleSkeletonSummary.IsEmpty() ? *Snapshot->AIReadinessSummary : *Snapshot->CompatibleSkeletonSummary);
}

FString BuildSandboxPreviewSummaryText(
    const FWanaSelectedCharacterEnhancementSnapshot* Snapshot,
    const FString& PreviewModeLabel,
    const FString& PreviewAssetLabel,
    bool bHasLivePreviewSubject)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("Stage Status: Waiting for subject\nStage Mode: Picker-ready workspace\nAnimation: (not available)\nWanaAnimation: Not Supported - no selected subject\nNext Step: Choose an AI Pawn or Character Pawn to wake the studio stage.");
    }

    const FString StageSubjectLabel = FString::Printf(
        TEXT("%s (%s)"),
        *Snapshot->SelectedActorLabel,
        Snapshot->ActorTypeLabel.IsEmpty() ? TEXT("Subject") : *Snapshot->ActorTypeLabel);
    const FString NextStep = bHasLivePreviewSubject
        ? TEXT("Live working subject is active. Use Focus Subject for full in-world inspection while the studio stage stays synced.")
        : TEXT("Showing the picked asset until a live working subject becomes active through enhancement or testing.");

    return FString::Printf(
        TEXT("Stage Subject: %s\nStage Mode: %s\nAnimation: %s\n%s\nNext Step: %s"),
        *StageSubjectLabel,
        PreviewModeLabel.IsEmpty() ? TEXT("Subject preview") : *PreviewModeLabel,
        Snapshot->LinkedAnimationBlueprintLabel.IsEmpty()
            ? (PreviewAssetLabel.IsEmpty() ? TEXT("(not linked)") : *PreviewAssetLabel)
            : *Snapshot->LinkedAnimationBlueprintLabel,
        *BuildAnimationPreviewConsumptionText(*Snapshot),
        *NextStep);
}

FString BuildAnimationIntegrationSummaryText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("Animation Blueprint: Unknown\nAssigned Anim Class: Unknown\nAnim BP Generated Class: Unknown\nParent Anim Instance Class: Unknown\nIntegration Status: Unknown\nExisting Anim BP Preserved: Yes\nAutomatic Integration: Not Supported\nAuto-Attach: Not Supported\nAuto-Wire: Not Supported\nAuto-Wire Fields: 0 supported, 0 applied\nIntegration Target: (not prepared)\nAnim BP Wiring Readiness: Not Supported\nGenerated Adapter Status: Not Supported\nAdapter Output: (not prepared)\nDirect Graph Edit Safety: Unsafe by policy\nRecommended Strategy: Needs Enhance\nHook Provider: Needs Enhance\nHook State Readable: Needs Enhance\nSkeletal Mesh: Not Supported\nSkeleton: Unknown\nCompatible Skeleton: Unknown\nAnimation Instance: Limited\nHook Consumption Readiness: Not Supported\nPhysical Provider: Limited\nHook Application: Not Available\nHook Readiness: Missing\nFacing Hook: Missing\nTurn-To-Target Hook: Missing\nLocomotion Hook: Missing\nReaction Animation Hook: Missing\nPosture Hint: Missing\nBody Language Category: Missing\nMovement-Limited Hook: Missing\nPhysical Reaction Hook: Missing\nNotes: Choose a subject in WanaWorks to inspect safe animation integration.");
    }

    return FString::Printf(
        TEXT("Animation Blueprint: %s\nAssigned Anim Class: %s\nAnim BP Asset: %s\nAnim BP Generated Class: %s\nParent Anim Instance Class: %s\nIntegration Status: %s\nExisting Anim BP Preserved: Yes\nAutomatic Integration: %s\nAuto-Attach: %s\nAuto-Wire: %s\nAuto-Wire Fields: %d supported, %d applied\nIntegration Target: %s\nAnim BP Wiring Readiness: %s\nGenerated Adapter Status: %s\nAdapter Output: %s\nDirect Graph Edit Safety: %s\nRecommended Strategy: %s\nHook Provider: %s\nHook State Readable: %s\nSkeletal Mesh: %s\nSkeleton: %s\nCompatible Skeleton: %s\nAnimation Instance: %s\nHook Consumption Readiness: %s\nPhysical Provider: %s\nHook Application: %s\nHook Readiness: %s\nFacing Hook: %s\nTurn-To-Target Hook: %s\nLocomotion Hook: %s\nReaction Animation Hook: %s\nPosture Hint: %s\nBody Language Category: %s\nMovement-Limited Hook: %s\nPhysical Reaction Hook: %s\nNotes: %s"),
        *WanaWorksUIFormattingUtils::GetAnimationBlueprintStatusLabel(*Snapshot),
        Snapshot->AnimationAssignedClassLabel.IsEmpty() ? TEXT("(not assigned)") : *Snapshot->AnimationAssignedClassLabel,
        Snapshot->AnimationBlueprintAssetLabel.IsEmpty() ? TEXT("(not detected)") : *Snapshot->AnimationBlueprintAssetLabel,
        Snapshot->AnimationGeneratedClassLabel.IsEmpty() ? TEXT("(not detected)") : *Snapshot->AnimationGeneratedClassLabel,
        Snapshot->AnimationParentInstanceClassLabel.IsEmpty() ? TEXT("(not detected)") : *Snapshot->AnimationParentInstanceClassLabel,
        *WanaWorksUIFormattingUtils::GetAnimationIntegrationStatusLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetAutomaticAnimationIntegrationStatusLabel(Snapshot->AnimationAutomaticIntegrationStatus),
        *WanaWorksUIFormattingUtils::GetAutomaticAnimationAttachSummaryLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetAutomaticAnimationWireSummaryLabel(*Snapshot),
        Snapshot->AnimationSupportedAutoWireFieldCount,
        Snapshot->AnimationLastAppliedAutoWireFieldCount,
        Snapshot->AnimationIntegrationTargetLabel.IsEmpty() ? TEXT("(not prepared)") : *Snapshot->AnimationIntegrationTargetLabel,
        *GetAnimBPAdapterReadinessLabel(*Snapshot),
        Snapshot->bHasAnimBlueprint ? TEXT("Adapter Recommended") : (Snapshot->bHasSkeletalMeshComponent ? TEXT("Limited") : TEXT("Not Supported")),
        Snapshot->bHasAnimBlueprint ? TEXT("/Game/WanaWorks/Adapters/(saved by Build)") : TEXT("(not prepared)"),
        Snapshot->bHasAnimBlueprint ? TEXT("Limited until shared-stack risk is evaluated") : TEXT("Limited"),
        *GetAdapterStrategySummary(*Snapshot),
        Snapshot->bHasWAYComponent ? TEXT("Ready") : TEXT("Needs Enhance"),
        Snapshot->bAnimationHookStateReadable ? TEXT("Ready") : TEXT("Needs Enhance"),
        Snapshot->SkeletalMeshLabel.IsEmpty() ? (Snapshot->bHasSkeletalMeshComponent ? TEXT("Ready") : TEXT("Not Supported")) : *Snapshot->SkeletalMeshLabel,
        Snapshot->SkeletonLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot->SkeletonLabel,
        Snapshot->CompatibleSkeletonSummary.IsEmpty() ? TEXT("Unknown") : *Snapshot->CompatibleSkeletonSummary,
        Snapshot->bHasAnimationInstance ? TEXT("Ready") : TEXT("Limited"),
        *GetAnimationHookConsumptionReadinessLabel(*Snapshot),
        Snapshot->bHasPhysicalStateComponent ? TEXT("Ready") : TEXT("Limited"),
        *WanaWorksUIFormattingUtils::GetAnimationHookApplicationStatusLabel(Snapshot->AnimationHookApplicationStatus),
        *WanaWorksUIFormattingUtils::GetAnimationHookReadinessLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetFacingHookReadinessLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetTurnToTargetHookReadinessLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetLocomotionHookReadinessLabel(*Snapshot),
        *WanaWorksUIFormattingUtils::GetReactionAnimationHookReadinessLabel(*Snapshot),
        Snapshot->AnimationPostureHint.IsEmpty() ? TEXT("(not driven yet)") : *Snapshot->AnimationPostureHint,
        Snapshot->AnimationPostureCategory.IsEmpty() ? TEXT("(not driven yet)") : *Snapshot->AnimationPostureCategory,
        *WanaWorksUIFormattingUtils::GetAnimationHookRequestSummaryLabel(Snapshot->bAnimationMovementLimitedFallbackHint),
        Snapshot->bAnimationPhysicalReactionStateAvailable ? TEXT("Readable") : TEXT("Limited"),
        *WanaWorksUIFormattingUtils::GetAnimationIntegrationNotes(*Snapshot));
}

FString BuildAnimationHookUsageText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("Hook Source: WAYPlayerProfileComponent -> Get Current Animation Hook State\nCurrent Hook Application: Not Available\nAutomatic Pass: WanaWorks can auto-attach a supported working-subject animation bridge when a subject is prepared through the workspace or finalized workflow.\nCore Fields: bFacingHookRequested, bTurnToTargetRequested, ReactionState, RecommendedBehavior, bLocomotionSafeExecutionHint, bMovementLimitedFallbackHint, BehaviorIntent, VisibleBehaviorLabel, IdentityRoleHint, RelationshipState, PostureHint, PostureCategory, FallbackHint, PhysicalInstabilityAlpha, PhysicalRecoveryProgress, PhysicalImpactDirection\nBlueprint Accessors: Get Current Animation Hook State, Get Animation Posture Hint, Get Animation Reaction State, Get Animation Recommended Behavior, Get Animation Visible Behavior Label, Is Animation Locomotion Safe Execution Hint Active, Is Animation Movement Limited Fallback Hint Active, Get Animation Physical Instability Alpha, Get Animation Physical Recovery Progress, Get Animation Physical Impact Direction\nTypical Use: Use facing and turn flags for turn-to-target logic. Use posture/category/fallback strings for readable body-language state. Use ReactionState and RecommendedBehavior for higher-level animation intent. Use the locomotion-safe and movement-limited hints to avoid forcing movement-driven animation logic.\nPhysical State Bridge: Read UWanaPhysicalStateComponent for PhysicalState, StabilityScore, RecoveryProgress, LastImpactDirection, LastImpactStrength, bBracing, bNeedsRecovery, bCanCommitToMovement, bCanCommitToAttack, and InstabilityAlpha.\nAnim BP Example: Get Owning Actor -> Get Component By Class (WAYPlayerProfileComponent) -> Get Current Animation Hook State\nPhysical Example: Get Owning Actor -> Get Component By Class (UWanaPhysicalStateComponent) -> Read LastImpactDirection and InstabilityAlpha for directional stagger or recovery pose blending.\nNotes: Pick a Character Pawn or AI Pawn in WanaWorks to see subject-specific hook guidance.");
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
    const FString MovementLimitedHintSummary = Snapshot->bAnimationMovementLimitedFallbackHint
        ? TEXT("true right now. Prefer hold, facing, turn-in-place, or fallback stance animation over locomotion pressure.")
        : TEXT("false right now. No movement-limited fallback is being requested by the current hook state.");
    const FString BodyLanguageSummary = Snapshot->AnimationPostureHint.IsEmpty()
        ? TEXT("No posture hint has been driven yet.")
        : FString::Printf(
            TEXT("%s posture in the %s category; behavior intent is %s with %s fallback."),
            *Snapshot->AnimationPostureHint,
            Snapshot->AnimationPostureCategory.IsEmpty() ? TEXT("Observe") : *Snapshot->AnimationPostureCategory,
            Snapshot->AnimationBehaviorIntent.IsEmpty() ? TEXT("neutral reaction readiness") : *Snapshot->AnimationBehaviorIntent,
            Snapshot->AnimationFallbackHint.IsEmpty() ? TEXT("stable observe stance") : *Snapshot->AnimationFallbackHint);
    const FString RelationshipInfluenceSummary = FString::Printf(
        TEXT("%s relationship with %s identity role hint."),
        WanaWorksUIFormattingUtils::GetRelationshipStateLabel(Snapshot->AnimationRelationshipState),
        Snapshot->AnimationIdentityRoleHint.IsEmpty() ? TEXT("neutral") : *Snapshot->AnimationIdentityRoleHint);
    const FString PhysicalReactionHookSummary = Snapshot->bAnimationPhysicalReactionStateAvailable
        ? FString::Printf(
            TEXT("Physical reaction hook is readable: %s, stability %.2f, recovery %d%%, instability %.2f, impact %.2f."),
            *WanaWorksUIFormattingUtils::GetPhysicalStateSummaryLabel(*Snapshot),
            Snapshot->AnimationPhysicalStabilityScore,
            FMath::RoundToInt(Snapshot->AnimationPhysicalRecoveryProgress * 100.0f),
            Snapshot->AnimationPhysicalInstabilityAlpha,
            Snapshot->AnimationPhysicalImpactStrength)
        : TEXT("Physical reaction hook is limited because no readable physical-state component is available.");
    const FString PhysicalBridgeSummary = Snapshot->bHasPhysicalStateComponent
        ? FString::Printf(
            TEXT("PhysicalState is %s right now. Use InstabilityAlpha %.2f for stagger or recovery blending. Use LastImpactDirection plus LastImpactStrength %.2f for directional lean or hit-reaction branching. Use bBracing and bNeedsRecovery to gate brace and recovery postures."),
            *WanaWorksUIFormattingUtils::GetPhysicalStateSummaryLabel(*Snapshot),
            Snapshot->PhysicalInstabilityAlpha,
            Snapshot->PhysicalLastImpactStrength)
        : TEXT("Attach UWanaPhysicalStateComponent if you want existing Anim BPs to read directional impact, stagger, recovery need, and commitment reduction without replacing the Anim BP.");
    const FString Notes = !Snapshot->AnimationAutomaticIntegrationDetail.IsEmpty()
        ? Snapshot->AnimationAutomaticIntegrationDetail
        : (Snapshot->AnimationHookDetail.IsEmpty()
            ? TEXT("WanaWorks preserves the current Animation Blueprint and layers safe hook state plus workspace-only auto integration on top when supported.")
            : Snapshot->AnimationHookDetail);

    return FString::Printf(
        TEXT("Hook Source: WAYPlayerProfileComponent -> Get Current Animation Hook State\nCurrent Hook Application: %s\nAutomatic Pass: %s\nHook Consumption Readiness: %s\nCurrent Subject: %s\nDetected Animation Blueprint: %s\nCore Fields:\n- bFacingHookRequested: %s\n- bTurnToTargetRequested: %s\n- ReactionState: %s\n- RecommendedBehavior: %s\n- VisibleBehaviorLabel: %s\n- bLocomotionSafeExecutionHint: %s\n- bMovementLimitedFallbackHint: %s\n- Body Language: %s\n- Identity / Relationship Influence: %s\n- Physical Reaction: %s\nBlueprint Accessors: Get Animation Posture Hint, Get Animation Reaction State, Get Animation Recommended Behavior, Get Animation Visible Behavior Label, Is Animation Locomotion Safe Execution Hint Active, Is Animation Movement Limited Fallback Hint Active, Get Animation Physical Instability Alpha, Get Animation Physical Recovery Progress, Get Animation Physical Impact Direction\nTypical Use: Use facing and turn flags for turn-to-target logic. Use posture/category/fallback strings for readable body-language state. Use ReactionState and RecommendedBehavior for higher-level animation intent. Use the locomotion-safe and movement-limited hints to avoid forcing movement-driven animation logic.\nPhysical State Bridge: %s\nAnim BP Example: Get Owning Actor -> Get Component By Class (WAYPlayerProfileComponent) -> Get Current Animation Hook State\nPhysical Example: Get Owning Actor -> Get Component By Class (UWanaPhysicalStateComponent) -> Read PhysicalState, InstabilityAlpha, LastImpactDirection, and LastImpactStrength\nNotes: %s"),
        *HookApplicationLabel,
        *WanaWorksUIFormattingUtils::GetAutomaticAnimationIntegrationStatusLabel(Snapshot->AnimationAutomaticIntegrationStatus),
        *GetAnimationHookConsumptionReadinessLabel(*Snapshot),
        *Snapshot->SelectedActorLabel,
        Snapshot->LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("(not linked)") : *Snapshot->LinkedAnimationBlueprintLabel,
        *FacingFieldSummary,
        *TurnFieldSummary,
        *ReactionFieldSummary,
        *BehaviorFieldSummary,
        Snapshot->AnimationVisibleBehaviorLabel.IsEmpty() ? TEXT("(not driven yet)") : *Snapshot->AnimationVisibleBehaviorLabel,
        *LocomotionHintSummary,
        *MovementLimitedHintSummary,
        *BodyLanguageSummary,
        *RelationshipInfluenceSummary,
        *PhysicalReactionHookSummary,
        *PhysicalBridgeSummary,
        *Notes);
}

FString BuildPhysicalStateSummaryText(const FWanaSelectedCharacterEnhancementSnapshot* Snapshot)
{
    if (!Snapshot || !Snapshot->bHasSelectedActor)
    {
        return TEXT("Physical State: Not Available\nStability Score: --\nRecovery Progress: --\nInstability Alpha: --\nCurrent Impact: --\nImpact Effect: --\nWanaAnimation Body Language: Unknown\nNotes: Choose a Character Pawn or AI Pawn in WanaWorks to inspect the readable body-state layer.");
    }

    if (!Snapshot->bHasPhysicalStateComponent)
    {
        return TEXT("Physical State: Not Available\nStability Score: --\nRecovery Progress: --\nInstability Alpha: --\nCurrent Impact: --\nImpact Effect: --\nWanaAnimation Body Language: Limited until physical state is attached\nNotes: Attach UWanaPhysicalStateComponent to this subject to expose a safe, readable body-state layer without replacing the current Character BP, Anim BP, or AI logic.");
    }

    const FString Notes = Snapshot->bPhysicalNeedsRecovery
        ? TEXT("Recovery is active. Other systems can read this and stay conservative until the subject settles.")
        : (Snapshot->bPhysicalBracing
            ? TEXT("Bracing is active. Other systems can use this as a defensive or hold-posture hint without forcing locomotion.")
            : TEXT("Body-state layer is readable and stable. Other systems can consume it safely as lightweight physical awareness."));
    const FString DisruptionLabel = Snapshot->PhysicalState == EWanaPhysicalState::Staggered
        ? TEXT("Yes - staggered")
        : (Snapshot->PhysicalState == EWanaPhysicalState::OffBalance
            ? TEXT("Yes - off balance")
            : (Snapshot->PhysicalState == EWanaPhysicalState::Panicked
                ? TEXT("Yes - highly disrupted")
                : TEXT("No")));
    const FString AnimationBodyLanguageLabel = Snapshot->AnimationPostureHint.IsEmpty()
        ? TEXT("Not driven yet")
        : FString::Printf(
            TEXT("%s posture; physical reaction hook %s."),
            *Snapshot->AnimationPostureHint,
            Snapshot->bAnimationPhysicalReactionStateAvailable ? TEXT("readable") : TEXT("limited"));

    return FString::Printf(
        TEXT("Physical State: %s\nStability Score: %.2f\nRecovery Progress: %d%%\nInstability Alpha: %.2f\nCurrent Impact: %s (%.2f)\nImpact Effect: %s\nDisrupted Posture: %s\nMovement Commitment: %s\nAttack Commitment: %s\nWanaAnimation Body Language: %s\nNotes: %s"),
        *WanaWorksUIFormattingUtils::GetPhysicalStateSummaryLabel(*Snapshot),
        Snapshot->PhysicalStabilityScore,
        FMath::RoundToInt(Snapshot->PhysicalRecoveryProgress * 100.0f),
        Snapshot->PhysicalInstabilityAlpha,
        *GetImpactSourceDirectionLabel(*Snapshot),
        Snapshot->PhysicalLastImpactStrength,
        *GetImpactEffectSummaryText(*Snapshot),
        *DisruptionLabel,
        Snapshot->bPhysicalCanCommitToMovement ? TEXT("Ready") : TEXT("Hold"),
        Snapshot->bPhysicalCanCommitToAttack ? TEXT("Ready") : TEXT("Hold"),
        *AnimationBodyLanguageLabel,
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
        return TEXT("Current Subject: (none)\nSubject Source: WanaWorks picker or editor selection fallback\nActor Type: (none)\nControl Mode: Unknown\nPlayer Control: Unknown\nIdentity: Missing\nWAY: Missing\nWAI: Missing\nAnimation: Missing\nAI Ready: Warning\nCompatibility: Warning");
    }

    return FString::Printf(
        TEXT("Current Subject: %s\nSubject Source: %s\nActor Type: %s\nControl Mode: %s\nPlayer Control: %s\nIdentity: %s\nWAY: %s\nWAI: %s\nAnimation: %s\nAI Ready: %s\nCompatibility: %s"),
        *Snapshot->SelectedActorLabel,
        Snapshot->SubjectSourceLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot->SubjectSourceLabel,
        *Snapshot->ActorTypeLabel,
        Snapshot->CharacterControlModeLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot->CharacterControlModeLabel,
        Snapshot->PlayableControlSummary.IsEmpty() ? TEXT("Unknown") : *Snapshot->PlayableControlSummary,
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
        : ((Snapshot->AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Applied
            || Snapshot->AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Ready)
            ? TEXT("READY")
            : (Snapshot->AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Limited
                ? TEXT("WARNING")
                : (Snapshot->bHasAnimBlueprint ? TEXT("READY") : (Snapshot->bHasSkeletalMeshComponent ? TEXT("WARNING") : TEXT("ACTIVE")))));

    return FString::Printf(
        TEXT("%s  Behavior Graph Ready\n%s  Combat Logic Synced\n%s  Animation Hooks Attached"),
        BehaviorGraphStatus,
        CombatLogicStatus,
        AnimationHooksStatus);
}

FString BuildCharacterEnhancementWorkflowText(const FString& WorkflowLabel)
{
    if (WorkflowLabel == TEXT("Create Sandbox Duplicate") || WorkflowLabel == TEXT("Create Working Copy"))
    {
        return TEXT("Safest option for testing. Creates a clearly named WanaWorks working copy, keeps the original actor untouched, and selects the duplicate for continued setup.");
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
        return TEXT("Subject: (none)\nIdentity: Missing\nWAY: Missing\nWAI: Missing\nAI-Ready: No\nAnimation Readiness: Missing\nWorkflow Used: Not run yet\nWorking Copy: Not Used\nOriginal Preserved: Yes");
    }

    return FString::Printf(
        TEXT("Subject: %s\nIdentity: %s\nWAY: %s\nWAI: %s\nAI-Ready: %s\nAnimation Readiness: %s\nWorkflow Used: %s\nWorking Copy: %s\nOriginal Preserved: %s"),
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
    const FString VisibleBehaviorLabel = Snapshot.VisibleBehaviorLabel.IsEmpty()
        ? TEXT("(not applied yet)")
        : Snapshot.VisibleBehaviorLabel;
    const FString ResultNotesLabel = Snapshot.BehaviorExecutionDetail.IsEmpty()
        ? TEXT("No visible behavior has been applied to this target yet.")
        : Snapshot.BehaviorExecutionDetail;
    const FString MovementConfidenceLabel = (Snapshot.bHasObserverActor && Snapshot.bHasTargetActor)
        ? WanaWorksUIFormattingUtils::GetMovementReadinessStatusSummaryLabel(Snapshot.MovementReadiness)
        : TEXT("Unknown");
    const FString EnvironmentShapingLabel = (Snapshot.bHasObserverActor && Snapshot.bHasTargetActor)
        ? WanaWorksUIFormattingUtils::GetEnvironmentShapingSummaryLabel(Snapshot.MovementReadiness)
        : TEXT("No active observer-target pair is shaping visible behavior yet.");
    const FString AnimationHookApplicationLabel = WanaWorksUIFormattingUtils::GetAnimationHookApplicationStatusLabel(Snapshot.AnimationHookApplicationStatus);
    const FString FacingHookLabel = WanaWorksUIFormattingUtils::GetAnimationHookRequestSummaryLabel(Snapshot.bAnimationFacingHookRequested);
    const FString TurnToTargetHookLabel = WanaWorksUIFormattingUtils::GetAnimationHookRequestSummaryLabel(Snapshot.bAnimationTurnToTargetRequested);
    const FString AnimationBehaviorIntentLabel = Snapshot.AnimationBehaviorIntent.IsEmpty()
        ? TEXT("(not driven yet)")
        : Snapshot.AnimationBehaviorIntent;
    const FString AnimationPostureHintLabel = Snapshot.AnimationPostureHint.IsEmpty()
        ? TEXT("(not driven yet)")
        : Snapshot.AnimationPostureHint;
    const FString AnimationBodyLanguageLabel = Snapshot.AnimationPostureCategory.IsEmpty()
        ? TEXT("Observant")
        : Snapshot.AnimationPostureCategory;
    const FString AnimationReactionStateLabel = WanaWorksUIFormattingUtils::GetReactionStateSummaryLabel(Snapshot.AnimationReactionState);
    const FString AnimationFallbackHintLabel = Snapshot.AnimationFallbackHint.IsEmpty()
        ? TEXT("(not driven yet)")
        : Snapshot.AnimationFallbackHint;
    const FString LocomotionHookLabel = Snapshot.bAnimationLocomotionHintSafe
        ? TEXT("Locomotion Safe")
        : (Snapshot.bAnimationMovementLimitedFallbackHint ? TEXT("Movement Limited") : TEXT("Facing / Hold Preferred"));
    const FString MovementLimitedHookLabel = WanaWorksUIFormattingUtils::GetAnimationHookRequestSummaryLabel(Snapshot.bAnimationMovementLimitedFallbackHint);
    const FString OutwardGuardHookLabel = WanaWorksUIFormattingUtils::GetAnimationHookRequestSummaryLabel(Snapshot.bAnimationOutwardGuardHintRequested);
    const FString PhysicalReactionHookLabel = Snapshot.bAnimationPhysicalReactionStateAvailable
        ? FString::Printf(TEXT("Readable (instability %.2f, recovery %d%%)"), Snapshot.AnimationPhysicalInstabilityAlpha, FMath::RoundToInt(Snapshot.AnimationPhysicalRecoveryProgress * 100.0f))
        : TEXT("Limited");
    const FString AnimationImpactDirectionLabel = Snapshot.bAnimationPhysicalReactionStateAvailable && Snapshot.AnimationPhysicalImpactStrength > KINDA_SMALL_NUMBER
        ? Snapshot.AnimationPhysicalImpactDirection.GetSafeNormal().ToCompactString()
        : TEXT("No active impact direction");

    return FString::Printf(
        TEXT("Observer: %s\nTarget: %s%s\nRelationship State: %s\nReaction State: %s\nRecommended Behavior: %s\nStarter Hook Available: %s\nVisible Behavior: %s\nLast Applied Hook: %s\nExecution Mode: %s\nMovement Confidence: %s\nEnvironment Shaping: %s\nResult Notes: %s\nAnimation Hook Application: %s\nAnimation Reaction State: %s\nBody Language State: %s\nFacing Hook: %s\nTurn-To-Target Hook: %s\nLocomotion Hook: %s\nAnimation Behavior Intent: %s\nAnimation Posture Hint: %s\nAnimation Fallback Hint: %s\nMovement-Limited Hook: %s\nOutward Guard Hook: %s\nPhysical Reaction Hook: %s\nImpact Direction: %s"),
        Snapshot.bHasObserverActor ? *Snapshot.ObserverActorLabel : TEXT("(not assigned)"),
        Snapshot.bHasTargetActor ? *Snapshot.TargetActorLabel : TEXT("(not assigned)"),
        Snapshot.bTargetFallsBackToObserver ? TEXT(" (observer fallback)") : TEXT(""),
        *RelationshipStateLabel,
        *ReactionStateLabel,
        *RecommendedBehaviorLabel,
        Snapshot.bStarterHookAvailable ? TEXT("Yes") : TEXT("No"),
        *VisibleBehaviorLabel,
        *LastAppliedHookLabel,
        *ExecutionModeLabel,
        *MovementConfidenceLabel,
        *EnvironmentShapingLabel,
        *ResultNotesLabel,
        *AnimationHookApplicationLabel,
        *AnimationReactionStateLabel,
        *AnimationBodyLanguageLabel,
        *FacingHookLabel,
        *TurnToTargetHookLabel,
        *LocomotionHookLabel,
        *AnimationBehaviorIntentLabel,
        *AnimationPostureHintLabel,
        *AnimationFallbackHintLabel,
        *MovementLimitedHookLabel,
        *OutwardGuardHookLabel,
        *PhysicalReactionHookLabel,
        *AnimationImpactDirectionLabel);
}

FString BuildWITEnvironmentReadinessText(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    const FString MovementConfidenceLabel = (Snapshot.bHasObserverActor && Snapshot.bHasTargetActor)
        ? WanaWorksUIFormattingUtils::GetMovementReadinessStatusSummaryLabel(Snapshot.MovementReadiness)
        : TEXT("Unknown");
    const FString ObstaclePressureLabel = (Snapshot.bHasObserverActor && Snapshot.bHasTargetActor)
        ? WanaWorksUIFormattingUtils::GetObstaclePressureSummaryLabel(Snapshot.MovementReadiness)
        : TEXT("Unknown");
    const FString MovementSpaceLabel = (Snapshot.bHasObserverActor && Snapshot.bHasTargetActor)
        ? WanaWorksUIFormattingUtils::GetMovementSpaceSummaryLabel(Snapshot.MovementReadiness)
        : TEXT("Unknown");

    return FString::Printf(
        TEXT("Pair Source: %s\nObserver: %s\nTarget: %s\nMovement Confidence: %s\nMovement Capability: %s\nNavigation Context: %s\nReachability: %s\nObstacle Pressure: %s\nMovement Space: %s\nFallback Mode: %s\nReadiness Detail: %s"),
        Snapshot.PairSourceLabel.IsEmpty() ? TEXT("No active pair") : *Snapshot.PairSourceLabel,
        Snapshot.bHasObserverActor ? *Snapshot.ObserverActorLabel : TEXT("(not assigned)"),
        Snapshot.bHasTargetActor ? *Snapshot.TargetActorLabel : TEXT("(not assigned)"),
        *MovementConfidenceLabel,
        *WanaWorksUIFormattingUtils::GetMovementCapabilitySummaryLabel(Snapshot),
        *WanaWorksUIFormattingUtils::GetNavigationContextSummaryLabel(Snapshot),
        *WanaWorksUIFormattingUtils::GetReachabilitySummaryLabel(Snapshot),
        *ObstaclePressureLabel,
        *MovementSpaceLabel,
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
                : TEXT("Ready to evaluate the explicit working pair.")));

    return FString::Printf(
        TEXT("Current Selection: %s\nWorking Copy Observer: %s\nWorking Copy Target: %s\nEvaluation Mode: %s\nReady To Evaluate: %s\nNext Step: %s"),
        *CurrentSelectionLabel,
        *ObserverLabel,
        *TargetLabel,
        *EvaluationMode,
        bReadyToEvaluate ? TEXT("Yes") : TEXT("No"),
        *NextStep);
}
}

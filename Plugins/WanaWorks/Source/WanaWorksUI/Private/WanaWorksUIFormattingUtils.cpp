#include "WanaWorksUIFormattingUtils.h"

#include "UObject/UnrealType.h"

namespace WanaWorksUIFormattingUtils
{
FString StripStatusPrefix(const FString& StatusMessage)
{
    FString TrimmedMessage = StatusMessage.TrimStartAndEnd();

    if (TrimmedMessage.StartsWith(TEXT("Status:"), ESearchCase::IgnoreCase))
    {
        TrimmedMessage = TrimmedMessage.RightChop(7).TrimStartAndEnd();
    }

    return TrimmedMessage;
}

FString GetFeedbackToneLabel(const FString& StatusMessage, bool bSucceeded)
{
    const FString LowerStatus = StatusMessage.TrimStartAndEnd().ToLower();

    if (!bSucceeded ||
        LowerStatus.Contains(TEXT("warning")) ||
        LowerStatus.Contains(TEXT("no actors")) ||
        LowerStatus.Contains(TEXT("missing")) ||
        LowerStatus.Contains(TEXT("could not")) ||
        LowerStatus.Contains(TEXT("failed")) ||
        LowerStatus.Contains(TEXT("invalid")))
    {
        return TEXT("WARNING");
    }

    if (LowerStatus.Contains(TEXT("already")) ||
        LowerStatus.Contains(TEXT("preserved")) ||
        LowerStatus.Contains(TEXT("safe")))
    {
        return TEXT("SAFE");
    }

    if (LowerStatus.Contains(TEXT("live")) || LowerStatus.Contains(TEXT("evaluat")))
    {
        return TEXT("LIVE");
    }

    if (LowerStatus.Contains(TEXT("ready")) ||
        LowerStatus.Contains(TEXT("applied")) ||
        LowerStatus.Contains(TEXT("ensured")) ||
        LowerStatus.Contains(TEXT("complete")) ||
        LowerStatus.Contains(TEXT("initialized")))
    {
        return TEXT("READY");
    }

    return TEXT("ACTIVE");
}

const TCHAR* GetCharacterEnhancementPresetLabel(const FString& PresetLabel)
{
    return PresetLabel.IsEmpty() ? TEXT("Identity Only") : *PresetLabel;
}

const TCHAR* GetCharacterEnhancementWorkflowLabel(const FString& WorkflowLabel)
{
    if (WorkflowLabel.Equals(TEXT("Create Sandbox Duplicate")))
    {
        return TEXT("Create Sandbox Duplicate");
    }

    if (WorkflowLabel.Equals(TEXT("Convert to AI-Ready Test Subject")))
    {
        return TEXT("Convert to AI-Ready Test Subject");
    }

    return TEXT("Use Original Character");
}

FString GetCharacterEnhancementResultsWorkflowLabel(const FString& WorkflowLabel)
{
    if (WorkflowLabel.Equals(TEXT("Create Sandbox Duplicate")))
    {
        return TEXT("Sandbox Duplicate");
    }

    if (WorkflowLabel.Equals(TEXT("Convert to AI-Ready Test Subject")))
    {
        return TEXT("AI-Ready Test Subject");
    }

    if (WorkflowLabel.Equals(TEXT("Use Original Character")))
    {
        return TEXT("Original");
    }

    return WorkflowLabel.IsEmpty() ? TEXT("Not run yet") : WorkflowLabel;
}

FString GetEnhancementComponentResultLabel(bool bHasComponent, bool bWasAdded)
{
    if (bWasAdded)
    {
        return TEXT("Added");
    }

    return bHasComponent ? TEXT("Present") : TEXT("Missing");
}

FString GetAnimationReadinessResultLabel(bool bHasSkeletalMeshComponent, bool bHasAnimBlueprint)
{
    if (!bHasSkeletalMeshComponent)
    {
        return TEXT("Missing");
    }

    return bHasAnimBlueprint ? TEXT("Detected") : TEXT("Warning");
}

bool IsAIReadyForLightweightTesting(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    return Snapshot.bIsPawnActor && Snapshot.bAutoPossessAIEnabled && Snapshot.bHasAIControllerClass;
}

FString GetAIControllerPresenceLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    return Snapshot.bHasAIControllerClass ? TEXT("Present") : TEXT("Missing");
}

FString GetAnimationBlueprintStatusLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("Unknown");
    }

    return Snapshot.bHasAnimBlueprint ? TEXT("Detected") : TEXT("Missing");
}

FString GetAnimationIntegrationStatusLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("Unknown");
    }

    if (Snapshot.AnimationHookApplicationStatus == EWAYAnimationHookApplicationStatus::Active)
    {
        return TEXT("Compatible");
    }

    return Snapshot.bHasAnimBlueprint ? TEXT("Compatible") : TEXT("Limited");
}

FString GetAnimationHookApplicationStatusLabel(EWAYAnimationHookApplicationStatus ApplicationStatus)
{
    const UEnum* ApplicationStatusEnum = StaticEnum<EWAYAnimationHookApplicationStatus>();
    return ApplicationStatusEnum
        ? ApplicationStatusEnum->GetDisplayNameTextByValue(static_cast<int64>(ApplicationStatus)).ToString()
        : TEXT("Not Available");
}

FString GetAnimationHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (Snapshot.AnimationHookApplicationStatus == EWAYAnimationHookApplicationStatus::Active)
    {
        return TEXT("Active");
    }

    if (!Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("Missing");
    }

    return Snapshot.bHasAnimBlueprint ? TEXT("Available") : TEXT("Limited");
}

FString GetFacingHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSelectedActor)
    {
        return TEXT("Missing");
    }

    if (Snapshot.AnimationHookApplicationStatus == EWAYAnimationHookApplicationStatus::Active && Snapshot.bAnimationFacingHookRequested)
    {
        return TEXT("Active");
    }

    return Snapshot.bHasSkeletalMeshComponent ? TEXT("Available") : TEXT("Limited");
}

FString GetTurnToTargetHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSelectedActor || !Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("Missing");
    }

    if (Snapshot.AnimationHookApplicationStatus == EWAYAnimationHookApplicationStatus::Active && Snapshot.bAnimationTurnToTargetRequested)
    {
        return TEXT("Active");
    }

    return Snapshot.bHasAnimBlueprint ? TEXT("Available") : TEXT("Limited");
}

FString GetLocomotionHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSelectedActor || !Snapshot.bIsPawnActor)
    {
        return TEXT("Missing");
    }

    if (IsAIReadyForLightweightTesting(Snapshot) && Snapshot.bHasAnimBlueprint)
    {
        return Snapshot.bAnimationLocomotionHintSafe ? TEXT("Compatible") : TEXT("Limited");
    }

    return TEXT("Limited");
}

FString GetReactionAnimationHookReadinessLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSelectedActor || !Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("Missing");
    }

    if (Snapshot.bHasAnimBlueprint && (Snapshot.bHasWAYComponent || Snapshot.bHasWAIComponent))
    {
        return TEXT("Ready");
    }

    return TEXT("Limited");
}

FString GetAnimationIntegrationNotes(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSelectedActor)
    {
        return TEXT("Select a character subject to inspect how WanaWorks can layer onto the current animation setup.");
    }

    if (!Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("No skeletal animation stack was detected yet, so WanaWorks keeps animation integration in a safe visibility-only state.");
    }

    if (Snapshot.bHasAnimBlueprint)
    {
        if (!Snapshot.AnimationHookDetail.IsEmpty())
        {
            return Snapshot.AnimationHookDetail;
        }

        return TEXT("WanaWorks can enhance facing, reaction flow, and locomotion-aware readiness around the current Animation Blueprint without replacing the asset.");
    }

    return TEXT("A skeletal mesh is present, but no Animation Blueprint was detected. WanaWorks preserves the current setup and reports only limited safe hook readiness in this phase.");
}

FString GetAnimationHookRequestSummaryLabel(bool bRequested)
{
    return bRequested ? TEXT("Requested") : TEXT("Idle");
}

FString GetCompatibilityStatusLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    return Snapshot.bHasSkeletalMeshComponent && !Snapshot.bHasAnimBlueprint ? TEXT("Warning") : TEXT("Safe");
}

const TCHAR* GetRelationshipStateLabel(EWAYRelationshipState RelationshipState)
{
    switch (RelationshipState)
    {
    case EWAYRelationshipState::Acquaintance:
        return TEXT("Acquaintance");
    case EWAYRelationshipState::Friend:
        return TEXT("Friend");
    case EWAYRelationshipState::Partner:
        return TEXT("Partner");
    case EWAYRelationshipState::Enemy:
        return TEXT("Enemy");
    case EWAYRelationshipState::Neutral:
    default:
        return TEXT("Neutral");
    }
}

bool TryParseRelationshipStateLabel(const FString& Label, EWAYRelationshipState& OutRelationshipState)
{
    const FString NormalizedLabel = Label.TrimStartAndEnd().ToLower();

    if (NormalizedLabel == TEXT("acquaintance"))
    {
        OutRelationshipState = EWAYRelationshipState::Acquaintance;
        return true;
    }

    if (NormalizedLabel == TEXT("friend"))
    {
        OutRelationshipState = EWAYRelationshipState::Friend;
        return true;
    }

    if (NormalizedLabel == TEXT("partner"))
    {
        OutRelationshipState = EWAYRelationshipState::Partner;
        return true;
    }

    if (NormalizedLabel == TEXT("enemy"))
    {
        OutRelationshipState = EWAYRelationshipState::Enemy;
        return true;
    }

    if (NormalizedLabel == TEXT("neutral"))
    {
        OutRelationshipState = EWAYRelationshipState::Neutral;
        return true;
    }

    return false;
}

FString GetMovementCapabilitySummaryLabel(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    if (!Snapshot.bHasObserverActor)
    {
        return TEXT("Unknown");
    }

    return Snapshot.MovementReadiness.bHasUsableMovementCapability ? TEXT("Yes") : TEXT("No");
}

FString GetNavigationContextSummaryLabel(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    if (!Snapshot.bHasObserverActor)
    {
        return TEXT("Unknown");
    }

    return Snapshot.MovementReadiness.bHasMovementContext ? TEXT("Detected") : TEXT("Missing");
}

FString GetReachabilitySummaryLabel(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    if (!Snapshot.bHasObserverActor || !Snapshot.bHasTargetActor)
    {
        return TEXT("Unknown");
    }

    return Snapshot.MovementReadiness.bTargetReachableHint ? TEXT("Likely") : TEXT("Blocked");
}

FString GetFallbackModeSummaryLabel(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    if (!Snapshot.bHasObserverActor || !Snapshot.bHasTargetActor)
    {
        return TEXT("Facing Only");
    }

    return Snapshot.MovementReadiness.bCanAttemptMovement ? TEXT("Movement Allowed") : TEXT("Facing Only");
}

FString FormatResponseOutputLine(const FString& Line)
{
    const FString TrimmedLine = Line.TrimStartAndEnd();

    if (TrimmedLine.IsEmpty())
    {
        return FString();
    }

    auto IndentLine = [&TrimmedLine]()
    {
        return FString::Printf(TEXT("  %s"), *TrimmedLine);
    };

    auto FormatNote = [&TrimmedLine](const TCHAR* Prefix)
    {
        return FString::Printf(TEXT("  - %s"), *TrimmedLine.RightChop(FCString::Strlen(Prefix)).TrimStartAndEnd());
    };

    if (TrimmedLine.StartsWith(TEXT("Guided Workflow:")))
    {
        return FString::Printf(TEXT("[FLOW] %s"), *TrimmedLine.RightChop(16).TrimStartAndEnd());
    }

    if (TrimmedLine.StartsWith(TEXT("Compatibility Notes:")))
    {
        return FormatNote(TEXT("Compatibility Notes:"));
    }

    if (TrimmedLine.StartsWith(TEXT("Readiness Notes:")))
    {
        return FormatNote(TEXT("Readiness Notes:"));
    }

    return IndentLine();
}

void AppendLinesWithPrefix(FWanaCommandResponse& Response, const FWanaCommandResponse& SourceResponse, const TCHAR* Prefix)
{
    for (const FString& OutputLine : SourceResponse.OutputLines)
    {
        if (OutputLine.StartsWith(Prefix))
        {
            Response.OutputLines.Add(OutputLine);
        }
    }
}

FString GetReactionStateSummaryLabel(EWAYReactionState ReactionState)
{
    const UEnum* ReactionEnum = StaticEnum<EWAYReactionState>();
    return ReactionEnum
        ? ReactionEnum->GetDisplayNameTextByValue(static_cast<int64>(ReactionState)).ToString()
        : TEXT("Observational");
}

FString GetBehaviorPresetSummaryLabel(EWAYBehaviorPreset BehaviorPreset)
{
    const UEnum* BehaviorEnum = StaticEnum<EWAYBehaviorPreset>();
    return BehaviorEnum
        ? BehaviorEnum->GetDisplayNameTextByValue(static_cast<int64>(BehaviorPreset)).ToString()
        : TEXT("None");
}

FString GetBehaviorExecutionModeSummaryLabel(EWAYBehaviorExecutionMode ExecutionMode)
{
    const UEnum* ExecutionModeEnum = StaticEnum<EWAYBehaviorExecutionMode>();
    return ExecutionModeEnum
        ? ExecutionModeEnum->GetDisplayNameTextByValue(static_cast<int64>(ExecutionMode)).ToString()
        : TEXT("Unknown");
}
}

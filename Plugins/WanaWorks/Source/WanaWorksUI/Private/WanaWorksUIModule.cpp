#include "WanaWorksUIModule.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "Selection.h"
#include "Widgets/InvalidateWidgetReason.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "WanaWorksCommandDispatcher.h"
#include "WanaWorksUIEditorActions.h"
#include "WanaWorksUIFormattingUtils.h"
#include "WanaWorksUISummaryText.h"
#include "WanaWorksUITabBuilder.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FWanaWorksUIModule"

namespace UIFmt = WanaWorksUIFormattingUtils;
namespace UISummary = WanaWorksUISummaryText;

const FName FWanaWorksUIModule::WanaWorksTabName(TEXT("WanaWorksTab"));

IMPLEMENT_MODULE(FWanaWorksUIModule, WanaWorksUI)

void FWanaWorksUIModule::StartupModule()
{
    StatusMessage = TEXT("Status: Wana Works Initialized");
    CommandText.Reset();
    LogOutput = TEXT("Wana Works Initialized");
    IdentityFactionTagText.Reset();
    SelectedEnhancementPresetLabel = TEXT("Identity Only");
    SelectedEnhancementWorkflowLabel = TEXT("Use Original Character");
    LastEnhancementResultsActor.Reset();
    LastEnhancementResultsActorLabel = TEXT("(none)");
    LastEnhancementWorkflowUsed = TEXT("Not run yet");
    LastEnhancementIdentityResult = TEXT("Missing");
    LastEnhancementWAYResult = TEXT("Missing");
    LastEnhancementWAIResult = TEXT("Missing");
    LastEnhancementAIReadyResult = TEXT("No");
    LastEnhancementAnimationResult = TEXT("Missing");
    bLastEnhancementSandboxCopyCreated = false;
    bLastEnhancementOriginalPreserved = true;
    SelectedIdentitySeedState = EWAYRelationshipState::Neutral;
    SelectedRelationshipState = EWAYRelationshipState::Neutral;
    SandboxObserverActor.Reset();
    SandboxTargetActor.Reset();
    EnhancementPresetOptions =
    {
        MakeShared<FString>(TEXT("Identity Only")),
        MakeShared<FString>(TEXT("Relationship Starter")),
        MakeShared<FString>(TEXT("Full WanaAI Starter"))
    };
    EnhancementWorkflowOptions =
    {
        MakeShared<FString>(TEXT("Use Original Character")),
        MakeShared<FString>(TEXT("Create Sandbox Duplicate")),
        MakeShared<FString>(TEXT("Convert to AI-Ready Test Subject"))
    };
    RelationshipStateOptions =
    {
        MakeShared<FString>(TEXT("Acquaintance")),
        MakeShared<FString>(TEXT("Friend")),
        MakeShared<FString>(TEXT("Partner")),
        MakeShared<FString>(TEXT("Enemy")),
        MakeShared<FString>(TEXT("Neutral"))
    };

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        WanaWorksTabName,
        FOnSpawnTab::CreateRaw(this, &FWanaWorksUIModule::SpawnWanaWorksTab))
        .SetDisplayName(LOCTEXT("WanaWorksTabTitle", "Wana Works"))
        .SetTooltipText(LOCTEXT("WanaWorksTabTooltip", "Open the Wana Works tab."))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
        .SetMenuType(ETabSpawnerMenuType::Enabled);

    SelectionChangedHandle = USelection::SelectionChangedEvent.AddRaw(this, &FWanaWorksUIModule::HandleEditorSelectionChanged);
    SelectObjectHandle = USelection::SelectObjectEvent.AddRaw(this, &FWanaWorksUIModule::HandleEditorSelectionChanged);

    FGlobalTabmanager::Get()->TryInvokeTab(WanaWorksTabName);
}

void FWanaWorksUIModule::ShutdownModule()
{
    if (SelectionChangedHandle.IsValid())
    {
        USelection::SelectionChangedEvent.Remove(SelectionChangedHandle);
        SelectionChangedHandle.Reset();
    }

    if (SelectObjectHandle.IsValid())
    {
        USelection::SelectObjectEvent.Remove(SelectObjectHandle);
        SelectObjectHandle.Reset();
    }

    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WanaWorksTabName);
    }
}

void FWanaWorksUIModule::RefreshReactiveUI(bool bForceRefresh)
{
    RefreshIdentityEditorState(bForceRefresh);

    if (const TSharedPtr<SDockTab> PinnedTab = WanaWorksTab.Pin())
    {
        PinnedTab->Invalidate(EInvalidateWidgetReason::Layout);
    }
}

void FWanaWorksUIModule::HandleEditorSelectionChanged(UObject* NewSelection)
{
    (void)NewSelection;
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::HandleCommandTextChanged(const FText& NewText)
{
    CommandText = NewText.ToString();
}

void FWanaWorksUIModule::HandleIdentityFactionTagTextChanged(const FText& NewText)
{
    IdentityFactionTagText = NewText.ToString();
}

void FWanaWorksUIModule::HandleIdentitySeedStateOptionSelected(TSharedPtr<FString> SelectedOption)
{
    if (!SelectedOption.IsValid())
    {
        return;
    }

    EWAYRelationshipState ParsedRelationshipState = EWAYRelationshipState::Neutral;

    if (UIFmt::TryParseRelationshipStateLabel(*SelectedOption, ParsedRelationshipState))
    {
        SelectedIdentitySeedState = ParsedRelationshipState;
    }
}

void FWanaWorksUIModule::HandleEnhancementPresetOptionSelected(TSharedPtr<FString> SelectedOption)
{
    if (SelectedOption.IsValid())
    {
        SelectedEnhancementPresetLabel = *SelectedOption;
    }
}

void FWanaWorksUIModule::HandleEnhancementWorkflowOptionSelected(TSharedPtr<FString> SelectedOption)
{
    if (SelectedOption.IsValid())
    {
        SelectedEnhancementWorkflowLabel = *SelectedOption;
    }
}

void FWanaWorksUIModule::HandleRelationshipStateOptionSelected(TSharedPtr<FString> SelectedOption)
{
    if (!SelectedOption.IsValid())
    {
        return;
    }

    EWAYRelationshipState ParsedRelationshipState = EWAYRelationshipState::Neutral;

    if (UIFmt::TryParseRelationshipStateLabel(*SelectedOption, ParsedRelationshipState))
    {
        SelectedRelationshipState = ParsedRelationshipState;
    }
}

void FWanaWorksUIModule::UpdateEnhancementResultsState(
    const FWanaSelectedCharacterEnhancementSnapshot* BeforeSnapshot,
    const FWanaSelectedCharacterEnhancementSnapshot* AfterSnapshot,
    const FString& WorkflowUsed,
    bool bUpdateWorkflowMetadata,
    bool bSandboxCopyCreated,
    bool bOriginalPreserved)
{
    const FWanaSelectedCharacterEnhancementSnapshot* EffectiveSnapshot =
        (AfterSnapshot && AfterSnapshot->bHasSelectedActor) ? AfterSnapshot :
        ((BeforeSnapshot && BeforeSnapshot->bHasSelectedActor) ? BeforeSnapshot : nullptr);

    if (!EffectiveSnapshot)
    {
        return;
    }

    const bool bSameSubject = bEnhancementResultsInitialized && LastEnhancementResultsActor.Get() == EffectiveSnapshot->SelectedActor.Get();
    const bool bIdentityAdded = EffectiveSnapshot->bHasIdentityComponent && BeforeSnapshot && BeforeSnapshot->bHasSelectedActor && !BeforeSnapshot->bHasIdentityComponent;
    const bool bWAYAdded = EffectiveSnapshot->bHasWAYComponent && BeforeSnapshot && BeforeSnapshot->bHasSelectedActor && !BeforeSnapshot->bHasWAYComponent;
    const bool bWAIAdded = EffectiveSnapshot->bHasWAIComponent && BeforeSnapshot && BeforeSnapshot->bHasSelectedActor && !BeforeSnapshot->bHasWAIComponent;

    bEnhancementResultsInitialized = true;
    LastEnhancementResultsActor = EffectiveSnapshot->SelectedActor;
    LastEnhancementResultsActorLabel = EffectiveSnapshot->SelectedActorLabel;
    LastEnhancementIdentityResult = UIFmt::GetEnhancementComponentResultLabel(EffectiveSnapshot->bHasIdentityComponent, bIdentityAdded);
    LastEnhancementWAYResult = UIFmt::GetEnhancementComponentResultLabel(EffectiveSnapshot->bHasWAYComponent, bWAYAdded);
    LastEnhancementWAIResult = UIFmt::GetEnhancementComponentResultLabel(EffectiveSnapshot->bHasWAIComponent, bWAIAdded);
    LastEnhancementAIReadyResult = UIFmt::IsAIReadyForLightweightTesting(*EffectiveSnapshot) ? TEXT("Yes") : TEXT("No");
    LastEnhancementAnimationResult = UIFmt::GetAnimationReadinessResultLabel(EffectiveSnapshot->bHasSkeletalMeshComponent, EffectiveSnapshot->bHasAnimBlueprint);

    if (bUpdateWorkflowMetadata)
    {
        LastEnhancementWorkflowUsed = UIFmt::GetCharacterEnhancementResultsWorkflowLabel(WorkflowUsed);
        bLastEnhancementSandboxCopyCreated = bSandboxCopyCreated;
        bLastEnhancementOriginalPreserved = bOriginalPreserved;
    }
    else if (!bSameSubject)
    {
        LastEnhancementWorkflowUsed = TEXT("Not run yet");
        bLastEnhancementSandboxCopyCreated = false;
        bLastEnhancementOriginalPreserved = true;
    }
}

void FWanaWorksUIModule::UpdateSavedSubjectProgressState(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSelectedActor)
    {
        return;
    }

    bSavedSubjectProgressInitialized = true;
    LastSavedSubjectLabel = Snapshot.SelectedActorLabel;
    LastSavedSubjectTypeLabel = Snapshot.ActorTypeLabel;
    LastSavedSubjectWorkflowLabel = UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel);
    LastSavedSubjectPresetLabel = UIFmt::GetCharacterEnhancementPresetLabel(SelectedEnhancementPresetLabel);
    LastSavedSubjectAIControllerResult = UIFmt::GetAIControllerPresenceLabel(Snapshot);
    LastSavedSubjectAnimationResult = UIFmt::GetAnimationBlueprintStatusLabel(Snapshot);
    LastSavedSubjectIdentityResult = Snapshot.bHasIdentityComponent ? TEXT("Present") : TEXT("Missing");
    LastSavedSubjectWAIResult = Snapshot.bHasWAIComponent ? TEXT("Present") : TEXT("Missing");
    LastSavedSubjectWAYResult = Snapshot.bHasWAYComponent ? TEXT("Present") : TEXT("Missing");
    LastSavedSubjectAIReadyResult = UIFmt::IsAIReadyForLightweightTesting(Snapshot) ? TEXT("Yes") : TEXT("No");
}

void FWanaWorksUIModule::ApplyResponse(const FWanaCommandResponse& Response, const FString& EchoedCommand)
{
    if (Response.bClearLog)
    {
        LogOutput.Reset();
    }
    else if (!EchoedCommand.IsEmpty())
    {
        if (!LogOutput.IsEmpty())
        {
            AppendLogLine(TEXT(""));
        }

        AppendLogLine(FString::Printf(TEXT("> %s"), *EchoedCommand));
    }
    else if (!LogOutput.IsEmpty())
    {
        AppendLogLine(TEXT(""));
    }

    StatusMessage = Response.StatusMessage;

    if (!StatusMessage.IsEmpty())
    {
        AppendLogLine(FString::Printf(
            TEXT("[%s] %s"),
            *UIFmt::GetFeedbackToneLabel(StatusMessage, Response.bSucceeded),
            *UIFmt::StripStatusPrefix(StatusMessage)));
    }

    for (const FString& OutputLine : Response.OutputLines)
    {
        const FString FormattedLine = UIFmt::FormatResponseOutputLine(OutputLine);

        if (!FormattedLine.IsEmpty())
        {
            AppendLogLine(FormattedLine);
        }
    }
}

void FWanaWorksUIModule::RefreshIdentityEditorState(bool bForceRefresh)
{
    FWanaSelectedActorIdentitySnapshot Snapshot;
    WanaWorksUIEditorActions::GetSelectedActorIdentitySnapshot(Snapshot);

    if (!bForceRefresh && bIdentityStateInitialized && CachedIdentityActor.Get() == Snapshot.SelectedActor.Get())
    {
        return;
    }

    bIdentityStateInitialized = true;
    CachedIdentityActor = Snapshot.SelectedActor;
    IdentityFactionTagText = Snapshot.FactionTag;
    SelectedIdentitySeedState = Snapshot.DefaultRelationshipSeed.RelationshipState;
}

void FWanaWorksUIModule::ExecuteCommandText(const FString& InCommandText)
{
    CommandText = InCommandText;
    RunCommand();
}

void FWanaWorksUIModule::EnsureIdentityComponent()
{
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteEnsureIdentityComponentCommand();
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ApplyIdentity()
{
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteApplyIdentityCommand(IdentityFactionTagText, SelectedIdentitySeedState);
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::CreateAIReadyController()
{
    FWanaSelectedCharacterEnhancementSnapshot BeforeSnapshot;
    const bool bHasBeforeSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(BeforeSnapshot) && BeforeSnapshot.bHasSelectedActor;
    FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecutePrepareSelectedActorForAITestCommand();

    if (bHasBeforeSnapshot)
    {
        Response.OutputLines.Insert(FString::Printf(TEXT("Selected Subject: %s"), *BeforeSnapshot.SelectedActorLabel), 0);
        Response.OutputLines.Insert(TEXT("Workflow Action: Create AI-Ready Controller"), 1);
    }

    if (Response.bSucceeded && bHasBeforeSnapshot)
    {
        FWanaSelectedCharacterEnhancementSnapshot AfterSnapshot;
        const bool bHasAfterSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(AfterSnapshot) && AfterSnapshot.bHasSelectedActor;
        const FWanaSelectedCharacterEnhancementSnapshot& EffectiveSnapshot = bHasAfterSnapshot ? AfterSnapshot : BeforeSnapshot;
        const FString AIControllerPathLabel = !EffectiveSnapshot.bIsPawnActor
            ? TEXT("Not applicable")
            : (EffectiveSnapshot.bHasAIControllerClass
                ? TEXT("Present")
                : (EffectiveSnapshot.bAutoPossessAIEnabled ? TEXT("Prepared for later assignment") : TEXT("Missing")));

        Response.OutputLines.Add(FString::Printf(TEXT("Detected Type: %s"), *EffectiveSnapshot.ActorTypeLabel));
        Response.OutputLines.Add(FString::Printf(TEXT("AI Controller Path: %s"), *AIControllerPathLabel));
        Response.OutputLines.Add(TEXT("Compatibility Notes: WanaWorks keeps the existing subject setup and only prepares a lightweight AI-ready path."));
        UpdateEnhancementResultsState(
            bHasBeforeSnapshot ? &BeforeSnapshot : nullptr,
            bHasAfterSnapshot ? &AfterSnapshot : (bHasBeforeSnapshot ? &BeforeSnapshot : nullptr),
            FString(),
            false,
            false,
            true);
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ConvertSelectedSubjectToAIReady()
{
    FWanaSelectedCharacterEnhancementSnapshot BeforeSnapshot;
    const bool bHasBeforeSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(BeforeSnapshot) && BeforeSnapshot.bHasSelectedActor;
    FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecutePrepareSelectedActorForAITestCommand();

    if (bHasBeforeSnapshot)
    {
        Response.OutputLines.Insert(FString::Printf(TEXT("Selected Subject: %s"), *BeforeSnapshot.SelectedActorLabel), 0);
        Response.OutputLines.Insert(TEXT("Workflow Action: Convert to AI-Ready Subject"), 1);
    }

    if (Response.bSucceeded && bHasBeforeSnapshot)
    {
        FWanaSelectedCharacterEnhancementSnapshot AfterSnapshot;
        const bool bHasAfterSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(AfterSnapshot) && AfterSnapshot.bHasSelectedActor;
        const FWanaSelectedCharacterEnhancementSnapshot& EffectiveSnapshot = bHasAfterSnapshot ? AfterSnapshot : BeforeSnapshot;
        const FString ConvertedSubjectReadyLabel = !EffectiveSnapshot.bIsPawnActor
            ? TEXT("Not applicable")
            : (UIFmt::IsAIReadyForLightweightTesting(EffectiveSnapshot) ? TEXT("Yes") : TEXT("No"));

        Response.OutputLines.Add(FString::Printf(TEXT("Detected Type: %s"), *EffectiveSnapshot.ActorTypeLabel));
        Response.OutputLines.Add(FString::Printf(TEXT("Converted Subject Ready: %s"), *ConvertedSubjectReadyLabel));
        Response.OutputLines.Add(TEXT("Compatibility Notes: The original Character BP, AI Controller path, and Animation Blueprint setup were preserved."));
        UpdateEnhancementResultsState(
            bHasBeforeSnapshot ? &BeforeSnapshot : nullptr,
            bHasAfterSnapshot ? &AfterSnapshot : (bHasBeforeSnapshot ? &BeforeSnapshot : nullptr),
            FString(),
            false,
            false,
            true);
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::SaveSubjectProgress()
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;

    if (!WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) || !Snapshot.bHasSelectedActor)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No subject selected.");
        Response.OutputLines.Add(TEXT("Select a subject before saving WanaWorks progress."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    UpdateSavedSubjectProgressState(Snapshot);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Saved subject setup progress.");
    Response.OutputLines.Add(FString::Printf(TEXT("Saved Subject: %s"), *Snapshot.SelectedActorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Detected Type: %s"), *Snapshot.ActorTypeLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Workflow Preference: %s"), UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel)));
    Response.OutputLines.Add(FString::Printf(TEXT("Starter Preset: %s"), UIFmt::GetCharacterEnhancementPresetLabel(SelectedEnhancementPresetLabel)));
    Response.OutputLines.Add(FString::Printf(TEXT("AI Controller: %s"), *LastSavedSubjectAIControllerResult));
    Response.OutputLines.Add(FString::Printf(TEXT("Animation Blueprint: %s"), *LastSavedSubjectAnimationResult));
    Response.OutputLines.Add(FString::Printf(TEXT("Identity: %s"), *LastSavedSubjectIdentityResult));
    Response.OutputLines.Add(FString::Printf(TEXT("WAI: %s"), *LastSavedSubjectWAIResult));
    Response.OutputLines.Add(FString::Printf(TEXT("WAY: %s"), *LastSavedSubjectWAYResult));
    Response.OutputLines.Add(FString::Printf(TEXT("AI-Ready: %s"), *LastSavedSubjectAIReadyResult));
    Response.OutputLines.Add(TEXT("Readiness Notes: This snapshot is kept in the current Wana Works session so your setup progress is easy to pick back up."));
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ApplyCharacterEnhancement()
{
    FWanaSelectedCharacterEnhancementSnapshot SourceSnapshot;

    if (!WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(SourceSnapshot) || !SourceSnapshot.bHasSelectedActor)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No actors selected.");
        Response.OutputLines.Add(TEXT("Select a character or pawn before applying WanaAI enhancement."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    const FString WorkflowLabel = UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel);
    FWanaCommandResponse WorkflowResponse;
    FWanaCommandResponse EnhancementResponse;
    FWanaCommandResponse AIReadyResponse;
    const bool bUseSandboxDuplicate = WorkflowLabel == TEXT("Create Sandbox Duplicate");
    const bool bUseAIReadyPrep = WorkflowLabel == TEXT("Convert to AI-Ready Test Subject");

    if (bUseSandboxDuplicate)
    {
        WorkflowResponse = WanaWorksUIEditorActions::ExecuteCreateSandboxDuplicateCommand();

        if (!WorkflowResponse.bSucceeded)
        {
            ApplyResponse(WorkflowResponse);
            RefreshReactiveUI(true);
            return;
        }
    }

    EnhancementResponse = WanaWorksUIEditorActions::ExecuteApplyCharacterEnhancementCommand(SelectedEnhancementPresetLabel);

    if (!EnhancementResponse.bSucceeded)
    {
        ApplyResponse(EnhancementResponse);
        RefreshReactiveUI(true);
        return;
    }

    if (bUseAIReadyPrep)
    {
        AIReadyResponse = WanaWorksUIEditorActions::ExecutePrepareSelectedActorForAITestCommand();

        if (!AIReadyResponse.bSucceeded)
        {
            ApplyResponse(AIReadyResponse);
            RefreshReactiveUI(true);
            return;
        }
    }

    FWanaSelectedCharacterEnhancementSnapshot ActiveSnapshot;
    const bool bHasActiveSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(ActiveSnapshot) && ActiveSnapshot.bHasSelectedActor;

    if (bHasActiveSnapshot)
    {
        SandboxObserverActor = ActiveSnapshot.SelectedActor;
    }

    const bool bAIReadyNotApplicable = bUseAIReadyPrep && AIReadyResponse.StatusMessage.Contains(TEXT("not applicable"), ESearchCase::IgnoreCase);
    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = bUseSandboxDuplicate
        ? TEXT("Status: Prepared sandbox duplicate for WanaAI testing.")
        : (bUseAIReadyPrep
            ? (bAIReadyNotApplicable
                ? TEXT("Status: Applied WanaAI enhancement with AI-ready notes.")
                : TEXT("Status: Prepared AI-ready WanaAI test subject."))
            : TEXT("Status: Applied WanaAI enhancement to the original actor."));
    Response.OutputLines.Add(FString::Printf(TEXT("Selected Actor: %s"), *SourceSnapshot.SelectedActorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Workflow Path: %s"), *WorkflowLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Original Or Duplicate: %s"), bUseSandboxDuplicate ? TEXT("Sandbox duplicate") : TEXT("Original actor")));
    Response.OutputLines.Add(FString::Printf(TEXT("Active Subject: %s"), bHasActiveSnapshot ? *ActiveSnapshot.SelectedActorLabel : *SourceSnapshot.SelectedActorLabel));

    if (bHasActiveSnapshot)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Identity: %s"), ActiveSnapshot.bHasIdentityComponent ? TEXT("Present") : TEXT("Missing")));
        Response.OutputLines.Add(FString::Printf(TEXT("WAY: %s"), ActiveSnapshot.bHasWAYComponent ? TEXT("Present") : TEXT("Missing")));
        Response.OutputLines.Add(FString::Printf(TEXT("WAI: %s"), ActiveSnapshot.bHasWAIComponent ? TEXT("Present") : TEXT("Missing")));
        Response.OutputLines.Add(FString::Printf(TEXT("Anim BP / Animation: %s"), *ActiveSnapshot.AnimationCompatibilitySummary));
        Response.OutputLines.Add(FString::Printf(TEXT("AI Readiness: %s"), *ActiveSnapshot.AIReadinessSummary));
    }

    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Preset Used:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Components Added:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Already Present:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Compatibility Notes:"));

    if (bUseSandboxDuplicate)
    {
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Compatibility Notes:"));
        Response.OutputLines.Add(TEXT("Readiness Notes: Sandbox observer was updated to the prepared subject."));
    }

    if (bUseAIReadyPrep)
    {
        UIFmt::AppendLinesWithPrefix(Response, AIReadyResponse, TEXT("AI-Ready Preparation:"));
        UIFmt::AppendLinesWithPrefix(Response, AIReadyResponse, TEXT("AI Controller Class:"));
        UIFmt::AppendLinesWithPrefix(Response, AIReadyResponse, TEXT("Compatibility Notes:"));
    }
    else
    {
        Response.OutputLines.Add(TEXT("AI-Ready Preparation: Not requested"));
    }

    UpdateEnhancementResultsState(
        &SourceSnapshot,
        bHasActiveSnapshot ? &ActiveSnapshot : &SourceSnapshot,
        WorkflowLabel,
        true,
        bUseSandboxDuplicate,
        true);

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ApplyStarterAndTestTarget()
{
    FWanaSelectedCharacterEnhancementSnapshot BeforeSnapshot;
    const bool bHasBeforeSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(BeforeSnapshot) && BeforeSnapshot.bHasSelectedActor;
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteApplyStarterAndTestTargetCommand(SelectedEnhancementPresetLabel);

    if (Response.bSucceeded)
    {
        FWanaSelectedCharacterEnhancementSnapshot AfterSnapshot;
        const bool bHasAfterSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(AfterSnapshot) && AfterSnapshot.bHasSelectedActor;

        UpdateEnhancementResultsState(
            bHasBeforeSnapshot ? &BeforeSnapshot : nullptr,
            bHasAfterSnapshot ? &AfterSnapshot : (bHasBeforeSnapshot ? &BeforeSnapshot : nullptr),
            TEXT("Use Original Character"),
            true,
            false,
            true);
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

bool FWanaWorksUIModule::ResolvePreferredWITReadinessPair(AActor*& OutObserverActor, AActor*& OutTargetActor, FString& OutPairSourceLabel, bool& bOutTargetFallsBackToObserver) const
{
    OutObserverActor = nullptr;
    OutTargetActor = nullptr;
    OutPairSourceLabel = TEXT("No active pair");
    bOutTargetFallsBackToObserver = false;

    FWanaSelectedRelationshipContextSnapshot SelectionSnapshot;
    const bool bHasSelectionSnapshot = WanaWorksUIEditorActions::GetSelectedRelationshipContextSnapshot(SelectionSnapshot) && SelectionSnapshot.bHasObserverActor;
    const bool bHasExplicitSelectionPair = bHasSelectionSnapshot && SelectionSnapshot.bHasTargetActor && !SelectionSnapshot.bTargetFallsBackToObserver;
    const bool bHasExplicitSandboxPair = SandboxObserverActor.IsValid() && SandboxTargetActor.IsValid();

    if (bHasExplicitSelectionPair)
    {
        OutObserverActor = SelectionSnapshot.ObserverActor.Get();
        OutTargetActor = SelectionSnapshot.TargetActor.Get();
        OutPairSourceLabel = TEXT("Current selection");
        return true;
    }

    if (bHasExplicitSandboxPair)
    {
        OutObserverActor = SandboxObserverActor.Get();
        OutTargetActor = SandboxTargetActor.Get();
        OutPairSourceLabel = TEXT("Sandbox pair");
        return true;
    }

    if (bHasSelectionSnapshot)
    {
        OutObserverActor = SelectionSnapshot.ObserverActor.Get();
        OutTargetActor = SelectionSnapshot.TargetActor.Get();
        bOutTargetFallsBackToObserver = SelectionSnapshot.bTargetFallsBackToObserver;
        OutPairSourceLabel = SelectionSnapshot.bTargetFallsBackToObserver
            ? TEXT("Current selection (observer fallback)")
            : TEXT("Current selection");
        return true;
    }

    if (SandboxObserverActor.IsValid() || SandboxTargetActor.IsValid())
    {
        OutObserverActor = SandboxObserverActor.Get();
        OutTargetActor = SandboxTargetActor.Get();
        OutPairSourceLabel = TEXT("Sandbox assignments");
        return true;
    }

    return false;
}

void FWanaWorksUIModule::ScanEnvironmentReadiness()
{
    AActor* ObserverActor = nullptr;
    AActor* TargetActor = nullptr;
    FString PairSourceLabel;
    bool bTargetFallsBackToObserver = false;

    ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteScanEnvironmentReadinessCommand(
        ObserverActor,
        TargetActor,
        PairSourceLabel,
        bTargetFallsBackToObserver);
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::EvaluateLiveTarget()
{
    FWanaSelectedCharacterEnhancementSnapshot BeforeSnapshot;
    const bool bHasBeforeSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(BeforeSnapshot) && BeforeSnapshot.bHasSelectedActor;
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteEvaluateLiveTargetCommand();

    if (Response.bSucceeded)
    {
        FWanaSelectedCharacterEnhancementSnapshot AfterSnapshot;
        const bool bHasAfterSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(AfterSnapshot) && AfterSnapshot.bHasSelectedActor;

        UpdateEnhancementResultsState(
            bHasBeforeSnapshot ? &BeforeSnapshot : nullptr,
            bHasAfterSnapshot ? &AfterSnapshot : (bHasBeforeSnapshot ? &BeforeSnapshot : nullptr),
            FString(),
            false,
            false,
            true);
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::UseSelectedActorAsSandboxObserver()
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;

    if (!WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) || !Snapshot.bHasSelectedActor)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No selected actor to assign as sandbox observer.");
        Response.OutputLines.Add(TEXT("Select an actor in the editor, then click Use Selected as Observer."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    SandboxObserverActor = Snapshot.SelectedActor;
    const bool bSelfTargetDebugPair = SandboxTargetActor.IsValid() && SandboxTargetActor.Get() == Snapshot.SelectedActor.Get();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Sandbox observer assigned.");
    Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), *Snapshot.SelectedActorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), SandboxTargetActor.IsValid() ? *SandboxTargetActor->GetActorNameOrLabel() : TEXT("(not assigned)")));
    Response.OutputLines.Add(TEXT("Readiness Notes: Sandbox observer was assigned from the current editor selection."));

    if (!SandboxTargetActor.IsValid())
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Sandbox target is still missing. Use Selected as Target before evaluating the pair."));
    }
    else if (bSelfTargetDebugPair)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Observer and target are the same actor. Self-target evaluation is active as a debug case."));
    }

    UpdateEnhancementResultsState(&Snapshot, &Snapshot, FString(), false, false, true);
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::UseSelectedActorAsSandboxTarget()
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;

    if (!WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) || !Snapshot.bHasSelectedActor)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No selected actor to assign as sandbox target.");
        Response.OutputLines.Add(TEXT("Select an actor in the editor, then click Use Selected as Target."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    SandboxTargetActor = Snapshot.SelectedActor;
    const bool bSelfTargetDebugPair = SandboxObserverActor.IsValid() && SandboxObserverActor.Get() == Snapshot.SelectedActor.Get();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Sandbox target assigned.");
    Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), SandboxObserverActor.IsValid() ? *SandboxObserverActor->GetActorNameOrLabel() : TEXT("(not assigned)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), *Snapshot.SelectedActorLabel));
    Response.OutputLines.Add(TEXT("Readiness Notes: Sandbox target was assigned from the current editor selection."));

    if (!SandboxObserverActor.IsValid())
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Sandbox observer is still missing. Use Selected as Observer before evaluating the pair."));
    }
    else if (bSelfTargetDebugPair)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Observer and target are the same actor. Self-target evaluation is active as a debug case."));
    }
    else
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Sandbox pair is ready to evaluate with separate observer and target assignments."));
    }

    if (SandboxObserverActor.IsValid())
    {
        FWanaSelectedCharacterEnhancementSnapshot ObserverSnapshot;

        if (WanaWorksUIEditorActions::GetCharacterEnhancementSnapshotForActor(SandboxObserverActor.Get(), ObserverSnapshot) && ObserverSnapshot.bHasSelectedActor)
        {
            UpdateEnhancementResultsState(&ObserverSnapshot, &ObserverSnapshot, FString(), false, false, true);
        }
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::EvaluateSandboxPair()
{
    AActor* ObserverActor = SandboxObserverActor.Get();
    AActor* TargetActor = SandboxTargetActor.Get();
    FWanaCommandResponse Response;
    FWanaSelectedCharacterEnhancementSnapshot BeforeObserverSnapshot;
    const bool bHasBeforeObserverSnapshot = ObserverActor
        && WanaWorksUIEditorActions::GetCharacterEnhancementSnapshotForActor(ObserverActor, BeforeObserverSnapshot)
        && BeforeObserverSnapshot.bHasSelectedActor;

    if (!ObserverActor || !TargetActor)
    {
        Response.StatusMessage = !ObserverActor
            ? TEXT("Status: Sandbox observer is missing.")
            : TEXT("Status: Sandbox target is missing.");
        Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), ObserverActor ? *ObserverActor->GetActorNameOrLabel() : TEXT("(not assigned)")));
        Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), TargetActor ? *TargetActor->GetActorNameOrLabel() : TEXT("(not assigned)")));
        Response.OutputLines.Add(TEXT("Readiness Notes: Sandbox evaluation uses explicit assignments only."));
        Response.OutputLines.Add(!ObserverActor
            ? TEXT("Readiness Notes: Use Selected as Observer before evaluating the sandbox pair.")
            : TEXT("Readiness Notes: Use Selected as Target before evaluating the sandbox pair."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    const bool bIsSelfTargetDebugPair = ObserverActor == TargetActor;
    Response = WanaWorksUIEditorActions::ExecuteEvaluateActorPairCommand(ObserverActor, TargetActor, false);

    if (Response.bSucceeded)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Observer Source: Stored sandbox observer."));
        Response.OutputLines.Add(TEXT("Readiness Notes: Target Source: Stored sandbox target."));
        Response.OutputLines.Add(bIsSelfTargetDebugPair
            ? TEXT("Readiness Notes: Self-target evaluation is active as a debug case.")
            : TEXT("Readiness Notes: Explicit observer-target pair evaluated."));

        FWanaSelectedCharacterEnhancementSnapshot AfterObserverSnapshot;
        const bool bHasAfterObserverSnapshot = WanaWorksUIEditorActions::GetCharacterEnhancementSnapshotForActor(ObserverActor, AfterObserverSnapshot) && AfterObserverSnapshot.bHasSelectedActor;

        UpdateEnhancementResultsState(
            bHasBeforeObserverSnapshot ? &BeforeObserverSnapshot : nullptr,
            bHasAfterObserverSnapshot ? &AfterObserverSnapshot : (bHasBeforeObserverSnapshot ? &BeforeObserverSnapshot : nullptr),
            FString(),
            false,
            false,
            true);
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::FocusSandboxObserver()
{
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteFocusActorCommand(SandboxObserverActor.Get(), TEXT("Observer"));
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::FocusSandboxTarget()
{
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteFocusActorCommand(SandboxTargetActor.Get(), TEXT("Target"));
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ApplySelectedRelationshipState()
{
    ExecuteCommandText(FString::Printf(TEXT("apply relationship state %s"), UIFmt::GetRelationshipStateLabel(SelectedRelationshipState)));
}

void FWanaWorksUIModule::RunCommand()
{
    FWanaCommandRequest Request;
    Request.CommandText = CommandText;

    FWanaCommandResponse Response = FWanaCommandDispatcher::Dispatch(Request);
    const FString ActionArgument = Response.ActionArgument;
    const FString TrimmedCommand = Request.CommandText.TrimStartAndEnd();

    switch (Response.Action)
    {
    case EWanaCommandAction::WeatherClear:
        Response = WanaWorksUIEditorActions::ExecuteWeatherPresetCommand(TEXT("Clear"));
        break;

    case EWanaCommandAction::WeatherOvercast:
        Response = WanaWorksUIEditorActions::ExecuteWeatherPresetCommand(TEXT("Overcast"));
        break;

    case EWanaCommandAction::WeatherStorm:
        Response = WanaWorksUIEditorActions::ExecuteWeatherPresetCommand(TEXT("Storm"));
        break;

    case EWanaCommandAction::SpawnCube:
        Response = WanaWorksUIEditorActions::ExecuteSpawnCubeCommand();
        break;

    case EWanaCommandAction::ListSelection:
        Response = WanaWorksUIEditorActions::ExecuteListSelectionCommand();
        break;

    case EWanaCommandAction::ClassifySelected:
        Response = WanaWorksUIEditorActions::ExecuteClassifySelectedCommand();
        break;

    case EWanaCommandAction::AddMemoryTest:
        Response = WanaWorksUIEditorActions::ExecuteAddMemoryTestCommand();
        break;

    case EWanaCommandAction::AddPreferenceTest:
        Response = WanaWorksUIEditorActions::ExecuteAddPreferenceTestCommand();
        break;

    case EWanaCommandAction::ShowMemory:
        Response = WanaWorksUIEditorActions::ExecuteShowMemoryCommand();
        break;

    case EWanaCommandAction::ShowPreferences:
        Response = WanaWorksUIEditorActions::ExecuteShowPreferencesCommand();
        break;

    case EWanaCommandAction::EnsureRelationshipProfile:
        Response = WanaWorksUIEditorActions::ExecuteEnsureRelationshipProfileCommand();
        break;

    case EWanaCommandAction::ShowRelationshipProfile:
        Response = WanaWorksUIEditorActions::ExecuteShowRelationshipProfileCommand();
        break;

    case EWanaCommandAction::ApplyRelationshipState:
        UIFmt::TryParseRelationshipStateLabel(ActionArgument, SelectedRelationshipState);
        Response = WanaWorksUIEditorActions::ExecuteApplyRelationshipStateCommand(ActionArgument);
        break;

    case EWanaCommandAction::None:
    default:
        break;
    }

    ApplyResponse(Response, TrimmedCommand);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ClearLog()
{
    LogOutput.Reset();
    StatusMessage = TEXT("Status: Log cleared.");
}

void FWanaWorksUIModule::AppendLogLine(const FString& Line)
{
    if (!LogOutput.IsEmpty())
    {
        LogOutput.Append(TEXT("\n"));
    }

    LogOutput.Append(Line);
}

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedEnhancementPresetOption() const
{
    const FString SelectedLabel = UIFmt::GetCharacterEnhancementPresetLabel(SelectedEnhancementPresetLabel);

    for (const TSharedPtr<FString>& Option : EnhancementPresetOptions)
    {
        if (Option.IsValid() && Option->Equals(SelectedLabel))
        {
            return Option;
        }
    }

    return EnhancementPresetOptions.Num() > 0 ? EnhancementPresetOptions[0] : nullptr;
}

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedEnhancementWorkflowOption() const
{
    const FString SelectedLabel = UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel);

    for (const TSharedPtr<FString>& Option : EnhancementWorkflowOptions)
    {
        if (Option.IsValid() && Option->Equals(SelectedLabel))
        {
            return Option;
        }
    }

    return EnhancementWorkflowOptions.Num() > 0 ? EnhancementWorkflowOptions[0] : nullptr;
}

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedRelationshipStateOption() const
{
    const FString SelectedLabel = UIFmt::GetRelationshipStateLabel(SelectedRelationshipState);

    for (const TSharedPtr<FString>& Option : RelationshipStateOptions)
    {
        if (Option.IsValid() && Option->Equals(SelectedLabel))
        {
            return Option;
        }
    }

    return nullptr;
}

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedIdentitySeedStateOption()
{
    RefreshIdentityEditorState();

    const FString SelectedLabel = UIFmt::GetRelationshipStateLabel(SelectedIdentitySeedState);

    for (const TSharedPtr<FString>& Option : RelationshipStateOptions)
    {
        if (Option.IsValid() && Option->Equals(SelectedLabel))
        {
            return Option;
        }
    }

    return nullptr;
}

FText FWanaWorksUIModule::GetStatusText() const
{
    return FText::FromString(StatusMessage);
}

FText FWanaWorksUIModule::GetCommandText() const
{
    return FText::FromString(CommandText);
}

FText FWanaWorksUIModule::GetLogText() const
{
    return FText::FromString(LogOutput);
}

FText FWanaWorksUIModule::GetSubjectSetupSummaryText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildSubjectSetupSummaryText(
        bHasSnapshot ? &Snapshot : nullptr,
        SelectedEnhancementWorkflowLabel,
        SelectedEnhancementPresetLabel));
}

FText FWanaWorksUIModule::GetSubjectStackSummaryText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildSubjectStackSummaryText(bHasSnapshot ? &Snapshot : nullptr));
}

FText FWanaWorksUIModule::GetSavedSubjectProgressText() const
{
    return FText::FromString(UISummary::BuildSavedSubjectProgressSummaryText(
        bSavedSubjectProgressInitialized,
        LastSavedSubjectLabel,
        LastSavedSubjectTypeLabel,
        LastSavedSubjectWorkflowLabel,
        LastSavedSubjectPresetLabel,
        LastSavedSubjectAIControllerResult,
        LastSavedSubjectAnimationResult,
        LastSavedSubjectIdentityResult,
        LastSavedSubjectWAIResult,
        LastSavedSubjectWAYResult,
        LastSavedSubjectAIReadyResult));
}

FText FWanaWorksUIModule::GetCharacterEnhancementSummaryText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildCharacterEnhancementSummaryText(bHasSnapshot ? &Snapshot : nullptr));
}

FText FWanaWorksUIModule::GetCharacterEnhancementChainText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildCharacterEnhancementChainText(bHasSnapshot ? &Snapshot : nullptr));
}

FText FWanaWorksUIModule::GetCharacterEnhancementWorkflowText() const
{
    return FText::FromString(UISummary::BuildCharacterEnhancementWorkflowText(UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel)));
}

FText FWanaWorksUIModule::GetEnhancementResultsText() const
{
    return FText::FromString(UISummary::BuildEnhancementResultsText(
        bEnhancementResultsInitialized,
        LastEnhancementResultsActorLabel,
        LastEnhancementIdentityResult,
        LastEnhancementWAYResult,
        LastEnhancementWAIResult,
        LastEnhancementAIReadyResult,
        LastEnhancementAnimationResult,
        LastEnhancementWorkflowUsed,
        bLastEnhancementSandboxCopyCreated,
        bLastEnhancementOriginalPreserved));
}

FText FWanaWorksUIModule::GetGuidedWorkflowSummaryText() const
{
    return FText::FromString(TEXT("Applies the selected WanaAI starter to the observer and immediately tests how it reacts to the target."));
}

FText FWanaWorksUIModule::GetLiveTestSummaryText() const
{
    FWanaSelectedRelationshipContextSnapshot Snapshot;

    if (!WanaWorksUIEditorActions::GetSelectedRelationshipContextSnapshot(Snapshot) || !Snapshot.bHasObserverActor)
    {
        return FText::FromString(TEXT("Observer: (none)\nTarget: (none)\nSelection Rule: First selected actor = observer. Second selected actor = target."));
    }

    const FString Summary = FString::Printf(
        TEXT("Observer: %s\nTarget: %s%s\nObserver WAY: %s\nSelection Rule: First selected actor = observer. Second selected actor = target."),
        *Snapshot.ObserverActorLabel,
        Snapshot.TargetActorLabel.IsEmpty() ? TEXT("(none)") : *Snapshot.TargetActorLabel,
        Snapshot.bTargetFallsBackToObserver ? TEXT(" (observer fallback)") : TEXT(""),
        Snapshot.bObserverHasRelationshipComponent ? TEXT("Present") : TEXT("Missing"));

    return FText::FromString(Summary);
}

FText FWanaWorksUIModule::GetBehaviorResultsText() const
{
    AActor* ObserverActor = nullptr;
    AActor* TargetActor = nullptr;
    FString PairSourceLabel;
    bool bTargetFallsBackToObserver = false;

    ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

    FWanaBehaviorResultsSnapshot Snapshot;
    WanaWorksUIEditorActions::GetBehaviorResultsSnapshotForActorPair(
        ObserverActor,
        TargetActor,
        bTargetFallsBackToObserver,
        PairSourceLabel,
        Snapshot);

    return FText::FromString(UISummary::BuildBehaviorResultsText(Snapshot));
}

FText FWanaWorksUIModule::GetWITEnvironmentReadinessText() const
{
    AActor* ObserverActor = nullptr;
    AActor* TargetActor = nullptr;
    FString PairSourceLabel;
    bool bTargetFallsBackToObserver = false;

    ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

    FWanaEnvironmentReadinessSnapshot Snapshot;
    WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
        ObserverActor,
        TargetActor,
        bTargetFallsBackToObserver,
        PairSourceLabel,
        Snapshot);

    return FText::FromString(UISummary::BuildWITEnvironmentReadinessText(Snapshot));
}

FText FWanaWorksUIModule::GetTestSandboxSummaryText() const
{
    const FString ObserverLabel = SandboxObserverActor.IsValid() ? SandboxObserverActor->GetActorNameOrLabel() : TEXT("(not assigned)");
    const FString TargetLabel = SandboxTargetActor.IsValid() ? SandboxTargetActor->GetActorNameOrLabel() : TEXT("MISSING - Use Selected as Target");

    FWanaSelectedCharacterEnhancementSnapshot SelectionSnapshot;
    const bool bHasCurrentSelection = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(SelectionSnapshot) && SelectionSnapshot.bHasSelectedActor;
    const FString SelectionLabel = bHasCurrentSelection ? SelectionSnapshot.SelectedActorLabel : TEXT("(none)");
    const bool bReadyToEvaluate = SandboxObserverActor.IsValid() && SandboxTargetActor.IsValid();
    const bool bSelfTargetDebugPair = bReadyToEvaluate && SandboxObserverActor.Get() == SandboxTargetActor.Get();
    return FText::FromString(UISummary::BuildTestSandboxSummaryText(
        SelectionLabel,
        ObserverLabel,
        TargetLabel,
        SandboxObserverActor.IsValid(),
        SandboxTargetActor.IsValid(),
        bReadyToEvaluate,
        bSelfTargetDebugPair));
}

FText FWanaWorksUIModule::GetRelationshipSummaryText() const
{
    FWanaSelectedRelationshipContextSnapshot Snapshot;

    if (!WanaWorksUIEditorActions::GetSelectedRelationshipContextSnapshot(Snapshot) || !Snapshot.bHasObserverActor)
    {
        return FText::FromString(TEXT("Observer: (none)\nTarget: (none)\nThe observer owns the relationship profile. Select one actor for the observer and a second actor for the target."));
    }

    const FString Summary = FString::Printf(
        TEXT("Observer: %s\nTarget: %s%s\nProfile Owner: The observer actor owns the WAY profile for the target."),
        *Snapshot.ObserverActorLabel,
        Snapshot.TargetActorLabel.IsEmpty() ? TEXT("(none)") : *Snapshot.TargetActorLabel,
        Snapshot.bTargetFallsBackToObserver ? TEXT(" (observer fallback)") : TEXT(""));

    return FText::FromString(Summary);
}

FText FWanaWorksUIModule::GetIdentitySummaryText()
{
    RefreshIdentityEditorState();

    FWanaSelectedActorIdentitySnapshot Snapshot;
    WanaWorksUIEditorActions::GetSelectedActorIdentitySnapshot(Snapshot);

    if (!Snapshot.bHasSelectedActor)
    {
        return FText::FromString(TEXT("Selected Actor: (none)\nIdentity Component: Missing"));
    }

    FString Summary = FString::Printf(
        TEXT("Selected Actor: %s\nIdentity Component: %s\nFaction Tag: %s\nDefault Relationship Seed: State=%s Trust=%.2f Fear=%.2f Respect=%.2f Attachment=%.2f Hostility=%.2f\nReputation Tags: %s"),
        *Snapshot.SelectedActorLabel,
        Snapshot.bHasIdentityComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot.FactionTag.IsEmpty() ? TEXT("(none)") : *Snapshot.FactionTag,
        UIFmt::GetRelationshipStateLabel(Snapshot.DefaultRelationshipSeed.RelationshipState),
        Snapshot.DefaultRelationshipSeed.Trust,
        Snapshot.DefaultRelationshipSeed.Fear,
        Snapshot.DefaultRelationshipSeed.Respect,
        Snapshot.DefaultRelationshipSeed.Attachment,
        Snapshot.DefaultRelationshipSeed.Hostility,
        *Snapshot.ReputationTagsSummary);

    return FText::FromString(Summary);
}

FText FWanaWorksUIModule::GetIdentityFactionTagText()
{
    RefreshIdentityEditorState();
    return FText::FromString(IdentityFactionTagText);
}

TSharedRef<SDockTab> FWanaWorksUIModule::SpawnWanaWorksTab(const FSpawnTabArgs& SpawnTabArgs)
{
    FWanaWorksUITabBuilderArgs BuilderArgs;
    BuilderArgs.GetStatusText = [this]() { return GetStatusText(); };
    BuilderArgs.GetCommandText = [this]() { return GetCommandText(); };
    BuilderArgs.GetLogText = [this]() { return GetLogText(); };
    BuilderArgs.GetSubjectSetupSummaryText = [this]() { return GetSubjectSetupSummaryText(); };
    BuilderArgs.GetSubjectStackSummaryText = [this]() { return GetSubjectStackSummaryText(); };
    BuilderArgs.GetSavedSubjectProgressText = [this]() { return GetSavedSubjectProgressText(); };
    BuilderArgs.GetCharacterEnhancementSummaryText = [this]() { return GetCharacterEnhancementSummaryText(); };
    BuilderArgs.GetCharacterEnhancementChainText = [this]() { return GetCharacterEnhancementChainText(); };
    BuilderArgs.GetCharacterEnhancementWorkflowText = [this]() { return GetCharacterEnhancementWorkflowText(); };
    BuilderArgs.GetEnhancementResultsText = [this]() { return GetEnhancementResultsText(); };
    BuilderArgs.GetGuidedWorkflowSummaryText = [this]() { return GetGuidedWorkflowSummaryText(); };
    BuilderArgs.GetLiveTestSummaryText = [this]() { return GetLiveTestSummaryText(); };
    BuilderArgs.GetBehaviorResultsText = [this]() { return GetBehaviorResultsText(); };
    BuilderArgs.GetTestSandboxSummaryText = [this]() { return GetTestSandboxSummaryText(); };
    BuilderArgs.GetWITEnvironmentReadinessText = [this]() { return GetWITEnvironmentReadinessText(); };
    BuilderArgs.GetRelationshipSummaryText = [this]() { return GetRelationshipSummaryText(); };
    BuilderArgs.GetIdentitySummaryText = [this]() { return GetIdentitySummaryText(); };
    BuilderArgs.GetIdentityFactionTagText = [this]() { return GetIdentityFactionTagText(); };
    BuilderArgs.EnhancementPresetOptions = &EnhancementPresetOptions;
    BuilderArgs.EnhancementWorkflowOptions = &EnhancementWorkflowOptions;
    BuilderArgs.RelationshipStateOptions = &RelationshipStateOptions;
    BuilderArgs.GetSelectedEnhancementPresetOption = [this]() { return GetSelectedEnhancementPresetOption(); };
    BuilderArgs.GetSelectedEnhancementWorkflowOption = [this]() { return GetSelectedEnhancementWorkflowOption(); };
    BuilderArgs.GetSelectedIdentitySeedStateOption = [this]() { return GetSelectedIdentitySeedStateOption(); };
    BuilderArgs.GetSelectedRelationshipStateOption = [this]() { return GetSelectedRelationshipStateOption(); };
    BuilderArgs.OnCommandTextChanged = [this](const FText& NewText) { HandleCommandTextChanged(NewText); };
    BuilderArgs.OnIdentityFactionTagTextChanged = [this](const FText& NewText) { HandleIdentityFactionTagTextChanged(NewText); };
    BuilderArgs.OnEnhancementPresetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleEnhancementPresetOptionSelected(SelectedOption); };
    BuilderArgs.OnEnhancementWorkflowOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleEnhancementWorkflowOptionSelected(SelectedOption); };
    BuilderArgs.OnIdentitySeedStateOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleIdentitySeedStateOptionSelected(SelectedOption); };
    BuilderArgs.OnRelationshipStateOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleRelationshipStateOptionSelected(SelectedOption); };
    BuilderArgs.OnRunCommand = [this]() { RunCommand(); };
    BuilderArgs.OnClearLog = [this]() { ClearLog(); };
    BuilderArgs.OnEnsureIdentityComponent = [this]() { EnsureIdentityComponent(); };
    BuilderArgs.OnApplyIdentity = [this]() { ApplyIdentity(); };
    BuilderArgs.OnCreateAIReadyController = [this]() { CreateAIReadyController(); };
    BuilderArgs.OnConvertToAIReadySubject = [this]() { ConvertSelectedSubjectToAIReady(); };
    BuilderArgs.OnSaveSubjectProgress = [this]() { SaveSubjectProgress(); };
    BuilderArgs.OnApplyCharacterEnhancement = [this]() { ApplyCharacterEnhancement(); };
    BuilderArgs.OnApplyStarterAndTestTarget = [this]() { ApplyStarterAndTestTarget(); };
    BuilderArgs.OnScanEnvironmentReadiness = [this]() { ScanEnvironmentReadiness(); };
    BuilderArgs.OnEvaluateLiveTarget = [this]() { EvaluateLiveTarget(); };
    BuilderArgs.OnUseSelectedAsSandboxObserver = [this]() { UseSelectedActorAsSandboxObserver(); };
    BuilderArgs.OnUseSelectedAsSandboxTarget = [this]() { UseSelectedActorAsSandboxTarget(); };
    BuilderArgs.OnEvaluateSandboxPair = [this]() { EvaluateSandboxPair(); };
    BuilderArgs.OnFocusSandboxObserver = [this]() { FocusSandboxObserver(); };
    BuilderArgs.OnFocusSandboxTarget = [this]() { FocusSandboxTarget(); };
    BuilderArgs.OnApplyRelationshipState = [this]() { ApplySelectedRelationshipState(); };
    BuilderArgs.OnExecuteCommandText = [this](const FString& InCommandText) { ExecuteCommandText(InCommandText); };

    TSharedRef<SDockTab> NewTab = SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            WanaWorksUITabBuilder::BuildTabContent(BuilderArgs)
        ];

    WanaWorksTab = NewTab;
    RefreshReactiveUI(true);
    return NewTab;
}

#undef LOCTEXT_NAMESPACE

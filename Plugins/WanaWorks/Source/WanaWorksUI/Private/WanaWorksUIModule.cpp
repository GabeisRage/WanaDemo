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
#include "WanaWorksUITabBuilder.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FWanaWorksUIModule"

namespace
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
}

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

    if (TryParseRelationshipStateLabel(*SelectedOption, ParsedRelationshipState))
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

    if (TryParseRelationshipStateLabel(*SelectedOption, ParsedRelationshipState))
    {
        SelectedRelationshipState = ParsedRelationshipState;
    }
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
            *GetFeedbackToneLabel(StatusMessage, Response.bSucceeded),
            *StripStatusPrefix(StatusMessage)));
    }

    for (const FString& OutputLine : Response.OutputLines)
    {
        const FString FormattedLine = FormatResponseOutputLine(OutputLine);

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

void FWanaWorksUIModule::ApplyCharacterEnhancement()
{
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteApplyCharacterEnhancementCommand(SelectedEnhancementPresetLabel);
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ApplyStarterAndTestTarget()
{
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteApplyStarterAndTestTargetCommand(SelectedEnhancementPresetLabel);
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::EvaluateLiveTarget()
{
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteEvaluateLiveTargetCommand();
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

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::EvaluateSandboxPair()
{
    AActor* ObserverActor = SandboxObserverActor.Get();
    AActor* TargetActor = SandboxTargetActor.Get();
    FWanaCommandResponse Response;

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
    ExecuteCommandText(FString::Printf(TEXT("apply relationship state %s"), GetRelationshipStateLabel(SelectedRelationshipState)));
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
        TryParseRelationshipStateLabel(ActionArgument, SelectedRelationshipState);
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
    const FString SelectedLabel = GetCharacterEnhancementPresetLabel(SelectedEnhancementPresetLabel);

    for (const TSharedPtr<FString>& Option : EnhancementPresetOptions)
    {
        if (Option.IsValid() && Option->Equals(SelectedLabel))
        {
            return Option;
        }
    }

    return EnhancementPresetOptions.Num() > 0 ? EnhancementPresetOptions[0] : nullptr;
}

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedRelationshipStateOption() const
{
    const FString SelectedLabel = GetRelationshipStateLabel(SelectedRelationshipState);

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

    const FString SelectedLabel = GetRelationshipStateLabel(SelectedIdentitySeedState);

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

FText FWanaWorksUIModule::GetCharacterEnhancementSummaryText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;

    if (!WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) || !Snapshot.bHasSelectedActor)
    {
        return FText::FromString(TEXT("Selected Actor Name: (none)\nIdentity: Missing\nWAY: Missing\nWAI: Missing\nCompatibility: Warning"));
    }

    const FString Summary = FString::Printf(
        TEXT("Selected Actor Name: %s\nIdentity: %s\nWAY: %s\nWAI: %s\nCompatibility: Safe"),
        *Snapshot.SelectedActorLabel,
        Snapshot.bHasIdentityComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot.bHasWAYComponent ? TEXT("Present") : TEXT("Missing"),
        Snapshot.bHasWAIComponent ? TEXT("Present") : TEXT("Missing"));

    return FText::FromString(Summary);
}

FText FWanaWorksUIModule::GetCharacterEnhancementChainText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) && Snapshot.bHasSelectedActor;

    const TCHAR* BehaviorGraphStatus = bHasSnapshot && Snapshot.bHasWAIComponent ? TEXT("READY") : TEXT("ACTIVE");
    const TCHAR* CombatLogicStatus = bHasSnapshot && Snapshot.bHasWAYComponent ? TEXT("READY") : TEXT("ACTIVE");
    const TCHAR* AnimationHooksStatus = bHasSnapshot && Snapshot.bHasIdentityComponent ? TEXT("READY") : TEXT("ACTIVE");

    const FString Summary = FString::Printf(
        TEXT("%s  Behavior Graph Ready\n%s  Combat Logic Synced\n%s  Animation Hooks Attached"),
        BehaviorGraphStatus,
        CombatLogicStatus,
        AnimationHooksStatus);

    return FText::FromString(Summary);
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

FText FWanaWorksUIModule::GetTestSandboxSummaryText() const
{
    const FString ObserverLabel = SandboxObserverActor.IsValid() ? SandboxObserverActor->GetActorNameOrLabel() : TEXT("(not assigned)");
    const FString TargetLabel = SandboxTargetActor.IsValid() ? SandboxTargetActor->GetActorNameOrLabel() : TEXT("MISSING - Use Selected as Target");

    FWanaSelectedCharacterEnhancementSnapshot SelectionSnapshot;
    const bool bHasCurrentSelection = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(SelectionSnapshot) && SelectionSnapshot.bHasSelectedActor;
    const FString SelectionLabel = bHasCurrentSelection ? SelectionSnapshot.SelectedActorLabel : TEXT("(none)");
    const bool bReadyToEvaluate = SandboxObserverActor.IsValid() && SandboxTargetActor.IsValid();
    const bool bSelfTargetDebugPair = bReadyToEvaluate && SandboxObserverActor.Get() == SandboxTargetActor.Get();
    const FString EvaluationMode = !bReadyToEvaluate
        ? TEXT("Assignment incomplete")
        : (bSelfTargetDebugPair ? TEXT("Self-target debug") : TEXT("Explicit observer and target"));
    const FString NextStep = !SandboxObserverActor.IsValid()
        ? TEXT("Use Selected as Observer to assign the evaluating actor.")
        : (!SandboxTargetActor.IsValid()
            ? TEXT("Use Selected as Target to assign the actor being evaluated.")
            : (bSelfTargetDebugPair
                ? TEXT("Self-target debug mode is active. Assign a different target for normal observer-target testing.")
                : TEXT("Ready to evaluate the explicit sandbox pair.")));

    const FString Summary = FString::Printf(
        TEXT("Current Selection: %s\nSandbox Observer: %s\nSandbox Target: %s\nEvaluation Mode: %s\nReady To Evaluate: %s\nNext Step: %s"),
        *SelectionLabel,
        *ObserverLabel,
        *TargetLabel,
        *EvaluationMode,
        bReadyToEvaluate ? TEXT("Yes") : TEXT("No"),
        *NextStep);

    return FText::FromString(Summary);
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
        GetRelationshipStateLabel(Snapshot.DefaultRelationshipSeed.RelationshipState),
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
    BuilderArgs.GetCharacterEnhancementSummaryText = [this]() { return GetCharacterEnhancementSummaryText(); };
    BuilderArgs.GetCharacterEnhancementChainText = [this]() { return GetCharacterEnhancementChainText(); };
    BuilderArgs.GetGuidedWorkflowSummaryText = [this]() { return GetGuidedWorkflowSummaryText(); };
    BuilderArgs.GetLiveTestSummaryText = [this]() { return GetLiveTestSummaryText(); };
    BuilderArgs.GetTestSandboxSummaryText = [this]() { return GetTestSandboxSummaryText(); };
    BuilderArgs.GetRelationshipSummaryText = [this]() { return GetRelationshipSummaryText(); };
    BuilderArgs.GetIdentitySummaryText = [this]() { return GetIdentitySummaryText(); };
    BuilderArgs.GetIdentityFactionTagText = [this]() { return GetIdentityFactionTagText(); };
    BuilderArgs.EnhancementPresetOptions = &EnhancementPresetOptions;
    BuilderArgs.RelationshipStateOptions = &RelationshipStateOptions;
    BuilderArgs.GetSelectedEnhancementPresetOption = [this]() { return GetSelectedEnhancementPresetOption(); };
    BuilderArgs.GetSelectedIdentitySeedStateOption = [this]() { return GetSelectedIdentitySeedStateOption(); };
    BuilderArgs.GetSelectedRelationshipStateOption = [this]() { return GetSelectedRelationshipStateOption(); };
    BuilderArgs.OnCommandTextChanged = [this](const FText& NewText) { HandleCommandTextChanged(NewText); };
    BuilderArgs.OnIdentityFactionTagTextChanged = [this](const FText& NewText) { HandleIdentityFactionTagTextChanged(NewText); };
    BuilderArgs.OnEnhancementPresetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleEnhancementPresetOptionSelected(SelectedOption); };
    BuilderArgs.OnIdentitySeedStateOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleIdentitySeedStateOptionSelected(SelectedOption); };
    BuilderArgs.OnRelationshipStateOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleRelationshipStateOptionSelected(SelectedOption); };
    BuilderArgs.OnRunCommand = [this]() { RunCommand(); };
    BuilderArgs.OnClearLog = [this]() { ClearLog(); };
    BuilderArgs.OnEnsureIdentityComponent = [this]() { EnsureIdentityComponent(); };
    BuilderArgs.OnApplyIdentity = [this]() { ApplyIdentity(); };
    BuilderArgs.OnApplyCharacterEnhancement = [this]() { ApplyCharacterEnhancement(); };
    BuilderArgs.OnApplyStarterAndTestTarget = [this]() { ApplyStarterAndTestTarget(); };
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

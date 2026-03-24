#include "WanaWorksUIModule.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "WanaWorksCommandDispatcher.h"
#include "WanaWorksUIEditorActions.h"
#include "WanaWorksUITabBuilder.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FWanaWorksUIModule"

namespace
{
const TCHAR* GetCharacterEnhancementPresetLabel(const FString& PresetLabel)
{
    return PresetLabel.IsEmpty() ? TEXT("Identity Only") : *PresetLabel;
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
    SelectedIdentitySeedState = EWAYRelationshipState::Neutral;
    SelectedRelationshipState = EWAYRelationshipState::Neutral;
    EnhancementPresetOptions =
    {
        MakeShared<FString>(TEXT("Identity Only")),
        MakeShared<FString>(TEXT("Relationship Starter")),
        MakeShared<FString>(TEXT("Full WanaAI Starter"))
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
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    FGlobalTabmanager::Get()->TryInvokeTab(WanaWorksTabName);
}

void FWanaWorksUIModule::ShutdownModule()
{
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WanaWorksTabName);
    }
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
        AppendLogLine(FString::Printf(TEXT("> %s"), *EchoedCommand));
    }

    StatusMessage = Response.StatusMessage;

    for (const FString& OutputLine : Response.OutputLines)
    {
        AppendLogLine(OutputLine);
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
    RefreshIdentityEditorState(true);
    ApplyResponse(Response);
}

void FWanaWorksUIModule::ApplyIdentity()
{
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteApplyIdentityCommand(IdentityFactionTagText, SelectedIdentitySeedState);
    RefreshIdentityEditorState(true);
    ApplyResponse(Response);
}

void FWanaWorksUIModule::ApplyCharacterEnhancement()
{
    const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteApplyCharacterEnhancementCommand(SelectedEnhancementPresetLabel);
    RefreshIdentityEditorState(true);
    ApplyResponse(Response);
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
    BuilderArgs.OnApplyRelationshipState = [this]() { ApplySelectedRelationshipState(); };
    BuilderArgs.OnExecuteCommandText = [this](const FString& InCommandText) { ExecuteCommandText(InCommandText); };

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            WanaWorksUITabBuilder::BuildTabContent(BuilderArgs)
        ];
}

#undef LOCTEXT_NAMESPACE

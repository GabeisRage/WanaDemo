#include "WanaWorksUIModule.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/Pawn.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "Selection.h"
#include "UObject/SoftObjectPath.h"
#include "Widgets/InvalidateWidgetReason.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "WanaWorksCommandDispatcher.h"
#include "WanaWorksUIEditorActions.h"
#include "WanaWorksUIFormattingUtils.h"
#include "WanaWorksUISummaryText.h"
#include "WanaWorksUITabBuilder.h"
#include "WAYPlayerProfileComponent.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FWanaWorksUIModule"

namespace UIFmt = WanaWorksUIFormattingUtils;
namespace UISummary = WanaWorksUISummaryText;

namespace
{
const TCHAR* SavedProgressSection = TEXT("WanaWorksUI.SavedProgress");
const TCHAR* SavedTimestampKey = TEXT("SavedTimestamp");
const TCHAR* SavedSubjectPathKey = TEXT("SavedSubjectPath");
const TCHAR* SavedSubjectLabelKey = TEXT("SavedSubjectLabel");
const TCHAR* SavedSubjectTypeKey = TEXT("SavedSubjectType");
const TCHAR* SavedWorkflowKey = TEXT("SavedWorkflow");
const TCHAR* SavedPresetKey = TEXT("SavedPreset");
const TCHAR* SavedAIControllerKey = TEXT("SavedAIController");
const TCHAR* SavedAnimationKey = TEXT("SavedAnimation");
const TCHAR* SavedIdentityKey = TEXT("SavedIdentity");
const TCHAR* SavedWAIKey = TEXT("SavedWAI");
const TCHAR* SavedWAYKey = TEXT("SavedWAY");
const TCHAR* SavedAIReadyKey = TEXT("SavedAIReady");
const TCHAR* SavedObserverPathKey = TEXT("SavedObserverPath");
const TCHAR* SavedObserverLabelKey = TEXT("SavedObserverLabel");
const TCHAR* SavedTargetPathKey = TEXT("SavedTargetPath");
const TCHAR* SavedTargetLabelKey = TEXT("SavedTargetLabel");
const TCHAR* SavedAnimationBlueprintAssetPathKey = TEXT("SavedAnimationBlueprintAssetPath");
const TCHAR* SavedAnimationBlueprintAssetLabelKey = TEXT("SavedAnimationBlueprintAssetLabel");
const TCHAR* WorkflowPresetSection = TEXT("WanaWorksUI.WorkflowPresets");
const TCHAR* SelectedWorkflowPresetKey = TEXT("SelectedWorkflowPreset");
const TCHAR* SavedWorkflowPresetTimestampKey = TEXT("SavedWorkflowPresetTimestamp");
const TCHAR* SavedWorkflowPresetSourceLabelKey = TEXT("SavedWorkflowPresetSourceLabel");
const TCHAR* SavedWorkflowPresetWorkflowKey = TEXT("SavedWorkflowPresetWorkflow");
const TCHAR* SavedWorkflowPresetEnhancementKey = TEXT("SavedWorkflowPresetEnhancement");
const TCHAR* SavedWorkflowPresetIdentitySeedKey = TEXT("SavedWorkflowPresetIdentitySeed");
const TCHAR* SavedWorkflowPresetRelationshipKey = TEXT("SavedWorkflowPresetRelationship");
const TCHAR* SavedWorkflowPresetBehaviorKey = TEXT("SavedWorkflowPresetBehavior");
const TCHAR* SavedWorkflowPresetFactionTagKey = TEXT("SavedWorkflowPresetFactionTag");
const TCHAR* SavedCustomWorkflowPresetLabel = TEXT("Saved Custom Preset");
const TCHAR* CharacterPawnPickerDefaultLabel = TEXT("(Choose Character Pawn)");
const TCHAR* AIPawnPickerDefaultLabel = TEXT("(Choose AI Pawn)");

struct FProjectAssetPickerEntry
{
    FString DisplayLabel;
    FString AssetPath;
};

struct FWanaWorkflowPresetConfig
{
    bool bValid = false;
    bool bIsCustom = false;
    bool bWantsRelationshipStarter = false;
    FString PresetLabel;
    FString SourceLabel;
    FString SavedTimestamp;
    FString WorkflowLabel;
    FString EnhancementPresetLabel;
    FString RecommendedBehaviorLabel;
    FString CoverageLabel;
    FString ApplyNotes;
    FString PersistenceNotes;
    FString FactionTagText;
    EWAYRelationshipState IdentitySeedState = EWAYRelationshipState::Neutral;
    EWAYRelationshipState RelationshipState = EWAYRelationshipState::Neutral;
};

bool DoesPresetIncludeWAY(const FString& EnhancementPresetLabel)
{
    return EnhancementPresetLabel.Equals(TEXT("Relationship Starter"))
        || EnhancementPresetLabel.Equals(TEXT("Full WanaAI Starter"));
}

FString GetPresetCoverageLabel(const FString& EnhancementPresetLabel)
{
    if (EnhancementPresetLabel.Equals(TEXT("Full WanaAI Starter")))
    {
        return TEXT("Identity, WAY, WAI");
    }

    if (EnhancementPresetLabel.Equals(TEXT("Relationship Starter")))
    {
        return TEXT("Identity, WAY");
    }

    return TEXT("Identity");
}

FString GetPresetRecommendedBehaviorLabel(const FString& EnhancementPresetLabel, EWAYRelationshipState RelationshipState)
{
    if (!DoesPresetIncludeWAY(EnhancementPresetLabel))
    {
        return TEXT("None");
    }

    const EWAYReactionState ReactionState = UWAYPlayerProfileComponent::ResolveReactionForRelationshipState(RelationshipState);
    const EWAYBehaviorPreset BehaviorPreset = UWAYPlayerProfileComponent::ResolveRecommendedBehaviorForReactionState(ReactionState);
    return UIFmt::GetBehaviorPresetSummaryLabel(BehaviorPreset);
}

FWanaWorkflowPresetConfig BuildWorkflowPresetConfig(
    const FString& SelectedPresetLabel,
    bool bHasSavedCustomPreset,
    const FString& SavedTimestamp,
    const FString& SavedSourceLabel,
    const FString& SavedWorkflowLabel,
    const FString& SavedEnhancementPresetLabel,
    const FString& SavedIdentitySeedLabel,
    const FString& SavedRelationshipLabel,
    const FString& SavedBehaviorLabel,
    const FString& SavedFactionTag)
{
    FWanaWorkflowPresetConfig Config;
    Config.PresetLabel = SelectedPresetLabel.IsEmpty() ? TEXT("Full WanaAI Starter") : SelectedPresetLabel;

    if (Config.PresetLabel.Equals(SavedCustomWorkflowPresetLabel))
    {
        Config.bIsCustom = true;
        Config.PersistenceNotes = TEXT("Stored in this project's editor settings so you can reuse this saved preset across editor sessions.");

        if (!bHasSavedCustomPreset)
        {
            Config.ApplyNotes = TEXT("Use Save Current as Preset to capture the current workflow into the saved custom preset slot.");
            return Config;
        }

        Config.bValid = true;
        Config.SourceLabel = SavedSourceLabel;
        Config.SavedTimestamp = SavedTimestamp;
        Config.WorkflowLabel = SavedWorkflowLabel.IsEmpty() ? TEXT("Use Original Character") : SavedWorkflowLabel;
        Config.EnhancementPresetLabel = SavedEnhancementPresetLabel.IsEmpty() ? TEXT("Identity Only") : SavedEnhancementPresetLabel;
        Config.FactionTagText = SavedFactionTag;
        Config.bWantsRelationshipStarter = DoesPresetIncludeWAY(Config.EnhancementPresetLabel);

        if (!SavedIdentitySeedLabel.IsEmpty())
        {
            UIFmt::TryParseRelationshipStateLabel(SavedIdentitySeedLabel, Config.IdentitySeedState);
        }

        if (!SavedRelationshipLabel.IsEmpty())
        {
            UIFmt::TryParseRelationshipStateLabel(SavedRelationshipLabel, Config.RelationshipState);
        }

        Config.CoverageLabel = GetPresetCoverageLabel(Config.EnhancementPresetLabel);
        Config.RecommendedBehaviorLabel = SavedBehaviorLabel.IsEmpty()
            ? GetPresetRecommendedBehaviorLabel(Config.EnhancementPresetLabel, Config.RelationshipState)
            : SavedBehaviorLabel;
        Config.ApplyNotes = TEXT("Reapplies the saved workflow choices and supported starter defaults without replacing the current Character BP, AI Controller, or Animation Blueprint stack.");
        return Config;
    }

    Config.bValid = true;
    Config.PersistenceNotes = TEXT("Built-in starter preset. Use Save Current as Preset to capture your own reusable variation.");

    if (Config.PresetLabel.Equals(TEXT("Hostile AI Starter")))
    {
        Config.WorkflowLabel = TEXT("Convert to AI-Ready Test Subject");
        Config.EnhancementPresetLabel = TEXT("Full WanaAI Starter");
        Config.IdentitySeedState = EWAYRelationshipState::Enemy;
        Config.RelationshipState = EWAYRelationshipState::Enemy;
        Config.bWantsRelationshipStarter = true;
        Config.ApplyNotes = TEXT("Builds the full WanaAI starter stack, prepares the subject for AI-ready testing, and seeds hostile defaults. An explicit observer/target pair is required before the hostile relationship starter is applied.");
    }
    else if (Config.PresetLabel.Equals(TEXT("Companion Starter")))
    {
        Config.WorkflowLabel = TEXT("Use Original Character");
        Config.EnhancementPresetLabel = TEXT("Full WanaAI Starter");
        Config.IdentitySeedState = EWAYRelationshipState::Friend;
        Config.RelationshipState = EWAYRelationshipState::Friend;
        Config.bWantsRelationshipStarter = true;
        Config.ApplyNotes = TEXT("Builds the full WanaAI starter stack with friendly defaults for companion-style testing. An explicit observer/target pair is required before the friendly relationship starter is applied.");
    }
    else if (Config.PresetLabel.Equals(TEXT("Relationship Starter")))
    {
        Config.WorkflowLabel = TEXT("Use Original Character");
        Config.EnhancementPresetLabel = TEXT("Relationship Starter");
        Config.IdentitySeedState = EWAYRelationshipState::Neutral;
        Config.RelationshipState = EWAYRelationshipState::Neutral;
        Config.bWantsRelationshipStarter = true;
        Config.ApplyNotes = TEXT("Adds the Identity and WAY starter layers, then seeds a neutral relationship starter when an explicit observer/target pair is available.");
    }
    else if (Config.PresetLabel.Equals(TEXT("Identity Only")))
    {
        Config.WorkflowLabel = TEXT("Use Original Character");
        Config.EnhancementPresetLabel = TEXT("Identity Only");
        Config.IdentitySeedState = EWAYRelationshipState::Neutral;
        Config.RelationshipState = EWAYRelationshipState::Neutral;
        Config.bWantsRelationshipStarter = false;
        Config.ApplyNotes = TEXT("Adds only the identity layer and safe default seed values, leaving WAY and WAI untouched.");
    }
    else
    {
        Config.PresetLabel = TEXT("Full WanaAI Starter");
        Config.WorkflowLabel = TEXT("Use Original Character");
        Config.EnhancementPresetLabel = TEXT("Full WanaAI Starter");
        Config.IdentitySeedState = EWAYRelationshipState::Neutral;
        Config.RelationshipState = EWAYRelationshipState::Neutral;
        Config.bWantsRelationshipStarter = false;
        Config.ApplyNotes = TEXT("Adds the current WanaAI starter layers without replacing existing Character BP, AI Controller, or Animation Blueprint assets.");
    }

    Config.CoverageLabel = GetPresetCoverageLabel(Config.EnhancementPresetLabel);
    Config.RecommendedBehaviorLabel = GetPresetRecommendedBehaviorLabel(Config.EnhancementPresetLabel, Config.RelationshipState);
    return Config;
}

FString MakeSavedActorPath(const AActor* Actor)
{
    return Actor ? FSoftObjectPath(Actor).ToString() : FString();
}

FString MakeProjectAssetDisplayLabel(const FAssetData& AssetData)
{
    return FString::Printf(TEXT("%s (%s)"), *AssetData.AssetName.ToString(), *AssetData.PackagePath.ToString());
}

void AddAssetPickerDefaultOption(TArray<TSharedPtr<FString>>& Options, const TCHAR* Label)
{
    Options.Add(MakeShared<FString>(Label));
}

void AddSortedAssetPickerOptions(
    const TArray<FProjectAssetPickerEntry>& Entries,
    TArray<TSharedPtr<FString>>& OutOptions,
    TMap<FString, FString>& OutPathMap)
{
    TArray<FProjectAssetPickerEntry> SortedEntries = Entries;
    SortedEntries.Sort([](const FProjectAssetPickerEntry& A, const FProjectAssetPickerEntry& B)
    {
        return A.DisplayLabel < B.DisplayLabel;
    });

    for (const FProjectAssetPickerEntry& Entry : SortedEntries)
    {
        OutOptions.Add(MakeShared<FString>(Entry.DisplayLabel));
        OutPathMap.Add(Entry.DisplayLabel, Entry.AssetPath);
    }
}

FString GetMappedAssetPath(const TMap<FString, FString>& PathMap, const FString& SelectedLabel)
{
    const FString* FoundPath = PathMap.Find(SelectedLabel);
    return FoundPath ? *FoundPath : FString();
}

AActor* ResolveSavedActorPath(const FString& SavedActorPath)
{
    if (SavedActorPath.IsEmpty())
    {
        return nullptr;
    }

    const FSoftObjectPath SoftPath(SavedActorPath);
    return Cast<AActor>(SoftPath.ResolveObject());
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
    SelectedWorkflowPresetLabel = TEXT("Full WanaAI Starter");
    SelectedEnhancementPresetLabel = TEXT("Identity Only");
    SelectedEnhancementWorkflowLabel = TEXT("Use Original Character");
    SelectedCharacterPawnAssetLabel.Reset();
    SelectedAIPawnAssetLabel.Reset();
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
    bSavedSubjectProgressInitialized = false;
    LastSavedTimestamp.Reset();
    LastSavedSubjectPath.Reset();
    LastSavedSubjectLabel.Reset();
    LastSavedSubjectTypeLabel.Reset();
    LastSavedSubjectWorkflowLabel.Reset();
    LastSavedSubjectPresetLabel.Reset();
    LastSavedSubjectAIControllerResult.Reset();
    LastSavedSubjectAnimationResult.Reset();
    LastSavedSubjectIdentityResult.Reset();
    LastSavedSubjectWAIResult.Reset();
    LastSavedSubjectWAYResult.Reset();
    LastSavedSubjectAIReadyResult.Reset();
    LastSavedObserverPath.Reset();
    LastSavedObserverLabel.Reset();
    LastSavedTargetPath.Reset();
    LastSavedTargetLabel.Reset();
    LastSavedAnimationBlueprintAssetPath.Reset();
    LastSavedAnimationBlueprintAssetLabel.Reset();
    bSavedWorkflowPresetInitialized = false;
    LastSavedWorkflowPresetTimestamp.Reset();
    LastSavedWorkflowPresetSourceLabel.Reset();
    LastSavedWorkflowPresetWorkflowLabel.Reset();
    LastSavedWorkflowPresetEnhancementPresetLabel.Reset();
    LastSavedWorkflowPresetIdentitySeedLabel.Reset();
    LastSavedWorkflowPresetRelationshipLabel.Reset();
    LastSavedWorkflowPresetBehaviorLabel.Reset();
    LastSavedWorkflowPresetFactionTag.Reset();
    SelectedIdentitySeedState = EWAYRelationshipState::Neutral;
    SelectedRelationshipState = EWAYRelationshipState::Neutral;
    SandboxObserverActor.Reset();
    SandboxTargetActor.Reset();
    WorkflowPresetOptions =
    {
        MakeShared<FString>(TEXT("Full WanaAI Starter")),
        MakeShared<FString>(TEXT("Hostile AI Starter")),
        MakeShared<FString>(TEXT("Companion Starter")),
        MakeShared<FString>(TEXT("Identity Only")),
        MakeShared<FString>(TEXT("Relationship Starter")),
        MakeShared<FString>(SavedCustomWorkflowPresetLabel)
    };
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
    RefreshProjectAssetPickerOptions();

    LoadSavedWorkflowPresets();
    LoadSavedSubjectProgress();

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

void FWanaWorksUIModule::HandleWorkflowPresetOptionSelected(TSharedPtr<FString> SelectedOption)
{
    if (SelectedOption.IsValid())
    {
        SelectedWorkflowPresetLabel = *SelectedOption;
        PersistSavedWorkflowPresets();
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
    LastSavedTimestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
    LastSavedSubjectPath = Snapshot.SelectedActor.IsValid()
        ? MakeSavedActorPath(Snapshot.SelectedActor.Get())
        : Snapshot.SubjectAssetPath;
    LastSavedAnimationBlueprintAssetPath.Reset();
    LastSavedAnimationBlueprintAssetLabel = Snapshot.LinkedAnimationBlueprintLabel;

    AActor* SavedObserverActor = SandboxObserverActor.Get();
    AActor* SavedTargetActor = SandboxTargetActor.Get();

    if (!SavedObserverActor && !SavedTargetActor)
    {
        FWanaSelectedRelationshipContextSnapshot RelationshipContext;

        if (WanaWorksUIEditorActions::GetSelectedRelationshipContextSnapshot(RelationshipContext)
            && RelationshipContext.bHasObserverActor
            && RelationshipContext.bHasTargetActor
            && !RelationshipContext.bTargetFallsBackToObserver)
        {
            SavedObserverActor = RelationshipContext.ObserverActor.Get();
            SavedTargetActor = RelationshipContext.TargetActor.Get();
        }
    }

    LastSavedObserverPath = MakeSavedActorPath(SavedObserverActor);
    LastSavedObserverLabel = SavedObserverActor ? SavedObserverActor->GetActorNameOrLabel() : FString();
    LastSavedTargetPath = MakeSavedActorPath(SavedTargetActor);
    LastSavedTargetLabel = SavedTargetActor ? SavedTargetActor->GetActorNameOrLabel() : FString();
}

void FWanaWorksUIModule::LoadSavedWorkflowPresets()
{
    if (!GConfig)
    {
        return;
    }

    GConfig->GetString(WorkflowPresetSection, SelectedWorkflowPresetKey, SelectedWorkflowPresetLabel, GEditorPerProjectIni);
    GConfig->GetString(WorkflowPresetSection, SavedWorkflowPresetTimestampKey, LastSavedWorkflowPresetTimestamp, GEditorPerProjectIni);
    GConfig->GetString(WorkflowPresetSection, SavedWorkflowPresetSourceLabelKey, LastSavedWorkflowPresetSourceLabel, GEditorPerProjectIni);
    GConfig->GetString(WorkflowPresetSection, SavedWorkflowPresetWorkflowKey, LastSavedWorkflowPresetWorkflowLabel, GEditorPerProjectIni);
    GConfig->GetString(WorkflowPresetSection, SavedWorkflowPresetEnhancementKey, LastSavedWorkflowPresetEnhancementPresetLabel, GEditorPerProjectIni);
    GConfig->GetString(WorkflowPresetSection, SavedWorkflowPresetIdentitySeedKey, LastSavedWorkflowPresetIdentitySeedLabel, GEditorPerProjectIni);
    GConfig->GetString(WorkflowPresetSection, SavedWorkflowPresetRelationshipKey, LastSavedWorkflowPresetRelationshipLabel, GEditorPerProjectIni);
    GConfig->GetString(WorkflowPresetSection, SavedWorkflowPresetBehaviorKey, LastSavedWorkflowPresetBehaviorLabel, GEditorPerProjectIni);
    GConfig->GetString(WorkflowPresetSection, SavedWorkflowPresetFactionTagKey, LastSavedWorkflowPresetFactionTag, GEditorPerProjectIni);

    if (SelectedWorkflowPresetLabel.IsEmpty())
    {
        SelectedWorkflowPresetLabel = TEXT("Full WanaAI Starter");
    }

    bSavedWorkflowPresetInitialized =
        !LastSavedWorkflowPresetTimestamp.IsEmpty()
        || !LastSavedWorkflowPresetWorkflowLabel.IsEmpty()
        || !LastSavedWorkflowPresetEnhancementPresetLabel.IsEmpty();
}

void FWanaWorksUIModule::PersistSavedWorkflowPresets() const
{
    if (!GConfig)
    {
        return;
    }

    GConfig->SetString(WorkflowPresetSection, SelectedWorkflowPresetKey, *SelectedWorkflowPresetLabel, GEditorPerProjectIni);
    GConfig->SetString(WorkflowPresetSection, SavedWorkflowPresetTimestampKey, *LastSavedWorkflowPresetTimestamp, GEditorPerProjectIni);
    GConfig->SetString(WorkflowPresetSection, SavedWorkflowPresetSourceLabelKey, *LastSavedWorkflowPresetSourceLabel, GEditorPerProjectIni);
    GConfig->SetString(WorkflowPresetSection, SavedWorkflowPresetWorkflowKey, *LastSavedWorkflowPresetWorkflowLabel, GEditorPerProjectIni);
    GConfig->SetString(WorkflowPresetSection, SavedWorkflowPresetEnhancementKey, *LastSavedWorkflowPresetEnhancementPresetLabel, GEditorPerProjectIni);
    GConfig->SetString(WorkflowPresetSection, SavedWorkflowPresetIdentitySeedKey, *LastSavedWorkflowPresetIdentitySeedLabel, GEditorPerProjectIni);
    GConfig->SetString(WorkflowPresetSection, SavedWorkflowPresetRelationshipKey, *LastSavedWorkflowPresetRelationshipLabel, GEditorPerProjectIni);
    GConfig->SetString(WorkflowPresetSection, SavedWorkflowPresetBehaviorKey, *LastSavedWorkflowPresetBehaviorLabel, GEditorPerProjectIni);
    GConfig->SetString(WorkflowPresetSection, SavedWorkflowPresetFactionTagKey, *LastSavedWorkflowPresetFactionTag, GEditorPerProjectIni);
    GConfig->Flush(false, GEditorPerProjectIni);
}

void FWanaWorksUIModule::LoadSavedSubjectProgress()
{
    if (!GConfig)
    {
        return;
    }

    GConfig->GetString(SavedProgressSection, SavedTimestampKey, LastSavedTimestamp, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedSubjectPathKey, LastSavedSubjectPath, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedSubjectLabelKey, LastSavedSubjectLabel, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedSubjectTypeKey, LastSavedSubjectTypeLabel, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedWorkflowKey, LastSavedSubjectWorkflowLabel, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedPresetKey, LastSavedSubjectPresetLabel, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedAIControllerKey, LastSavedSubjectAIControllerResult, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedAnimationKey, LastSavedSubjectAnimationResult, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedIdentityKey, LastSavedSubjectIdentityResult, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedWAIKey, LastSavedSubjectWAIResult, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedWAYKey, LastSavedSubjectWAYResult, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedAIReadyKey, LastSavedSubjectAIReadyResult, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedObserverPathKey, LastSavedObserverPath, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedObserverLabelKey, LastSavedObserverLabel, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedTargetPathKey, LastSavedTargetPath, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedTargetLabelKey, LastSavedTargetLabel, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedAnimationBlueprintAssetPathKey, LastSavedAnimationBlueprintAssetPath, GEditorPerProjectIni);
    GConfig->GetString(SavedProgressSection, SavedAnimationBlueprintAssetLabelKey, LastSavedAnimationBlueprintAssetLabel, GEditorPerProjectIni);

    bSavedSubjectProgressInitialized =
        !LastSavedSubjectLabel.IsEmpty()
        || !LastSavedSubjectPath.IsEmpty()
        || !LastSavedSubjectWorkflowLabel.IsEmpty()
        || !LastSavedSubjectPresetLabel.IsEmpty();

    if (!LastSavedSubjectWorkflowLabel.IsEmpty())
    {
        SelectedEnhancementWorkflowLabel = LastSavedSubjectWorkflowLabel;
    }

    if (!LastSavedSubjectPresetLabel.IsEmpty())
    {
        SelectedEnhancementPresetLabel = LastSavedSubjectPresetLabel;
    }
}

void FWanaWorksUIModule::HandleCharacterPawnAssetOptionSelected(TSharedPtr<FString> SelectedOption)
{
    SelectedCharacterPawnAssetLabel = SelectedOption.IsValid() ? *SelectedOption : FString();

    if (SelectedCharacterPawnAssetLabel.Equals(CharacterPawnPickerDefaultLabel))
    {
        SelectedCharacterPawnAssetLabel.Reset();
    }

    if (!SelectedCharacterPawnAssetLabel.IsEmpty())
    {
        SelectedAIPawnAssetLabel.Reset();
    }

    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::HandleAIPawnAssetOptionSelected(TSharedPtr<FString> SelectedOption)
{
    SelectedAIPawnAssetLabel = SelectedOption.IsValid() ? *SelectedOption : FString();

    if (SelectedAIPawnAssetLabel.Equals(AIPawnPickerDefaultLabel))
    {
        SelectedAIPawnAssetLabel.Reset();
    }

    if (!SelectedAIPawnAssetLabel.IsEmpty())
    {
        SelectedCharacterPawnAssetLabel.Reset();
    }

    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::RefreshProjectAssetPickerOptions()
{
    CharacterPawnAssetOptions.Reset();
    AIPawnAssetOptions.Reset();
    CharacterPawnAssetPathByLabel.Reset();
    AIPawnAssetPathByLabel.Reset();

    AddAssetPickerDefaultOption(CharacterPawnAssetOptions, CharacterPawnPickerDefaultLabel);
    AddAssetPickerDefaultOption(AIPawnAssetOptions, AIPawnPickerDefaultLabel);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> BlueprintAssets;
    AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets, true);

    TArray<FProjectAssetPickerEntry> CharacterPawnEntries;
    TArray<FProjectAssetPickerEntry> AIPawnEntries;

    for (const FAssetData& AssetData : BlueprintAssets)
    {
        UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetData.GetAsset());

        if (!BlueprintAsset || !BlueprintAsset->GeneratedClass || !BlueprintAsset->GeneratedClass->IsChildOf(APawn::StaticClass()))
        {
            continue;
        }

        const APawn* DefaultPawn = Cast<APawn>(BlueprintAsset->GeneratedClass->GetDefaultObject());
        const bool bIsAIReadyPawn = DefaultPawn
            && (DefaultPawn->AIControllerClass != nullptr || DefaultPawn->AutoPossessAI != EAutoPossessAI::Disabled);
        const FProjectAssetPickerEntry Entry
        {
            MakeProjectAssetDisplayLabel(AssetData),
            AssetData.ToSoftObjectPath().ToString()
        };

        if (bIsAIReadyPawn)
        {
            AIPawnEntries.Add(Entry);
        }
        else
        {
            CharacterPawnEntries.Add(Entry);
        }
    }

    AddSortedAssetPickerOptions(CharacterPawnEntries, CharacterPawnAssetOptions, CharacterPawnAssetPathByLabel);
    AddSortedAssetPickerOptions(AIPawnEntries, AIPawnAssetOptions, AIPawnAssetPathByLabel);
}

UObject* FWanaWorksUIModule::LoadSelectedSubjectAssetObject() const
{
    const FString SubjectAssetPath = GetSelectedSubjectAssetPath();

    if (SubjectAssetPath.IsEmpty())
    {
        return nullptr;
    }

    return FSoftObjectPath(SubjectAssetPath).TryLoad();
}

UClass* FWanaWorksUIModule::LoadSelectedSubjectActorClass() const
{
    UObject* SubjectAssetObject = LoadSelectedSubjectAssetObject();
    UBlueprint* BlueprintAsset = Cast<UBlueprint>(SubjectAssetObject);

    if (BlueprintAsset)
    {
        return BlueprintAsset->GeneratedClass.Get();
    }

    return Cast<UClass>(SubjectAssetObject);
}

FString FWanaWorksUIModule::GetSelectedSubjectAssetPath() const
{
    if (!SelectedAIPawnAssetLabel.IsEmpty())
    {
        return GetMappedAssetPath(AIPawnAssetPathByLabel, SelectedAIPawnAssetLabel);
    }

    if (!SelectedCharacterPawnAssetLabel.IsEmpty())
    {
        return GetMappedAssetPath(CharacterPawnAssetPathByLabel, SelectedCharacterPawnAssetLabel);
    }

    return FString();
}

bool FWanaWorksUIModule::ResolvePickedSubjectSnapshot(FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot) const
{
    UObject* SubjectAssetObject = LoadSelectedSubjectAssetObject();

    if (!SubjectAssetObject || !WanaWorksUIEditorActions::GetCharacterEnhancementSnapshotForSubjectObject(SubjectAssetObject, OutSnapshot))
    {
        return false;
    }

    return true;
}

bool FWanaWorksUIModule::ResolvePreferredSubjectSnapshot(FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot) const
{
    FWanaSelectedCharacterEnhancementSnapshot PickedSnapshot;
    const bool bHasPickedSnapshot = ResolvePickedSubjectSnapshot(PickedSnapshot);

    FWanaSelectedCharacterEnhancementSnapshot SelectedActorSnapshot;
    const bool bHasSelectedActorSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(SelectedActorSnapshot) && SelectedActorSnapshot.bHasSelectedActor;

    if (bHasPickedSnapshot)
    {
        const UClass* PickedSubjectClass = LoadSelectedSubjectActorClass();

        if (PickedSubjectClass
            && bHasSelectedActorSnapshot
            && SelectedActorSnapshot.SelectedActor.IsValid()
            && SelectedActorSnapshot.SelectedActor->GetClass()->IsChildOf(PickedSubjectClass))
        {
            OutSnapshot = SelectedActorSnapshot;
            return true;
        }

        OutSnapshot = PickedSnapshot;
        return true;
    }

    if (bHasSelectedActorSnapshot)
    {
        OutSnapshot = SelectedActorSnapshot;
        return true;
    }

    OutSnapshot = FWanaSelectedCharacterEnhancementSnapshot();
    return false;
}

bool FWanaWorksUIModule::PreparePickerDrivenSubjectForWorkflow(
    const FString& WorkflowContextLabel,
    FWanaCommandResponse& OutPreparationResponse,
    bool& bOutSpawnedFromPicker)
{
    OutPreparationResponse = FWanaCommandResponse();
    bOutSpawnedFromPicker = false;

    UObject* SubjectAssetObject = LoadSelectedSubjectAssetObject();
    FWanaSelectedCharacterEnhancementSnapshot CurrentSelectionSnapshot;
    const bool bHasCurrentSelection = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(CurrentSelectionSnapshot) && CurrentSelectionSnapshot.bHasSelectedActor;

    if (!SubjectAssetObject)
    {
        if (bHasCurrentSelection)
        {
            return true;
        }

        OutPreparationResponse.StatusMessage = TEXT("Status: No subject selected.");
        OutPreparationResponse.OutputLines.Add(TEXT("Choose a Character Pawn or AI Pawn in the WanaWorks project picker, or use an editor-selected actor as a fallback."));
        return false;
    }

    const UClass* PickedSubjectClass = LoadSelectedSubjectActorClass();

    if (bHasCurrentSelection
        && PickedSubjectClass
        && CurrentSelectionSnapshot.SelectedActor.IsValid()
        && CurrentSelectionSnapshot.SelectedActor->GetClass()->IsChildOf(PickedSubjectClass))
    {
        return true;
    }

    OutPreparationResponse = WanaWorksUIEditorActions::ExecuteCreateSandboxSubjectFromAssetCommand(
        SubjectAssetObject,
        nullptr);

    if (!OutPreparationResponse.bSucceeded)
    {
        return false;
    }

    if (!WorkflowContextLabel.IsEmpty())
    {
        OutPreparationResponse.OutputLines.Insert(FString::Printf(TEXT("Workflow Action: %s"), *WorkflowContextLabel), 0);
    }

    bOutSpawnedFromPicker = true;
    return true;
}

bool FWanaWorksUIModule::RestoreSubjectPickerFromAssetPath(const FString& AssetPath)
{
    if (AssetPath.IsEmpty())
    {
        return false;
    }

    const FSoftObjectPath SoftPath(AssetPath);
    const UObject* AssetObject = SoftPath.ResolveObject();
    const FString AssetLabel = AssetObject ? AssetObject->GetName() : SoftPath.GetAssetName();

    for (const TPair<FString, FString>& Entry : AIPawnAssetPathByLabel)
    {
        if (Entry.Value == AssetPath)
        {
            SelectedAIPawnAssetLabel = Entry.Key;
            SelectedCharacterPawnAssetLabel.Reset();
            return true;
        }
    }

    for (const TPair<FString, FString>& Entry : CharacterPawnAssetPathByLabel)
    {
        if (Entry.Value == AssetPath)
        {
            SelectedCharacterPawnAssetLabel = Entry.Key;
            SelectedAIPawnAssetLabel.Reset();
            return true;
        }
    }

    if (AssetLabel.IsEmpty())
    {
        return false;
    }

    return false;
}

void FWanaWorksUIModule::PersistSavedSubjectProgress() const
{
    if (!GConfig)
    {
        return;
    }

    GConfig->SetString(SavedProgressSection, SavedTimestampKey, *LastSavedTimestamp, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedSubjectPathKey, *LastSavedSubjectPath, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedSubjectLabelKey, *LastSavedSubjectLabel, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedSubjectTypeKey, *LastSavedSubjectTypeLabel, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedWorkflowKey, *LastSavedSubjectWorkflowLabel, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedPresetKey, *LastSavedSubjectPresetLabel, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedAIControllerKey, *LastSavedSubjectAIControllerResult, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedAnimationKey, *LastSavedSubjectAnimationResult, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedIdentityKey, *LastSavedSubjectIdentityResult, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedWAIKey, *LastSavedSubjectWAIResult, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedWAYKey, *LastSavedSubjectWAYResult, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedAIReadyKey, *LastSavedSubjectAIReadyResult, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedObserverPathKey, *LastSavedObserverPath, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedObserverLabelKey, *LastSavedObserverLabel, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedTargetPathKey, *LastSavedTargetPath, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedTargetLabelKey, *LastSavedTargetLabel, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedAnimationBlueprintAssetPathKey, *LastSavedAnimationBlueprintAssetPath, GEditorPerProjectIni);
    GConfig->SetString(SavedProgressSection, SavedAnimationBlueprintAssetLabelKey, *LastSavedAnimationBlueprintAssetLabel, GEditorPerProjectIni);
    GConfig->Flush(false, GEditorPerProjectIni);
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
    ResolvePreferredSubjectSnapshot(BeforeSnapshot);
    const bool bHasBeforeSnapshot = BeforeSnapshot.bHasSelectedActor;
    FWanaCommandResponse PreparationResponse;
    bool bSpawnedFromPicker = false;

    if (!PreparePickerDrivenSubjectForWorkflow(TEXT("Create AI-Ready Controller"), PreparationResponse, bSpawnedFromPicker))
    {
        ApplyResponse(PreparationResponse);
        RefreshReactiveUI(true);
        return;
    }

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
            bSpawnedFromPicker,
            true);
    }

    if (bSpawnedFromPicker)
    {
        Response.OutputLines.Insert(TEXT("Readiness Notes: WanaWorks created a sandbox subject from the project picker before applying AI-ready prep."), 2);
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ConvertSelectedSubjectToAIReady()
{
    FWanaSelectedCharacterEnhancementSnapshot BeforeSnapshot;
    ResolvePreferredSubjectSnapshot(BeforeSnapshot);
    const bool bHasBeforeSnapshot = BeforeSnapshot.bHasSelectedActor;
    FWanaCommandResponse PreparationResponse;
    bool bSpawnedFromPicker = false;

    if (!PreparePickerDrivenSubjectForWorkflow(TEXT("Convert to AI-Ready Subject"), PreparationResponse, bSpawnedFromPicker))
    {
        ApplyResponse(PreparationResponse);
        RefreshReactiveUI(true);
        return;
    }

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
            bSpawnedFromPicker,
            true);
    }

    if (bSpawnedFromPicker)
    {
        Response.OutputLines.Insert(TEXT("Readiness Notes: WanaWorks created a sandbox subject from the project picker before converting it for AI-ready testing."), 2);
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::SaveSubjectProgress()
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;

    if (!ResolvePreferredSubjectSnapshot(Snapshot) || !Snapshot.bHasSelectedActor)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No subject selected.");
        Response.OutputLines.Add(TEXT("Choose a Character Pawn or AI Pawn in the WanaWorks project picker, or use an editor-selected subject, before saving progress."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    UpdateSavedSubjectProgressState(Snapshot);
    PersistSavedSubjectProgress();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Saved subject setup progress.");
    Response.OutputLines.Add(FString::Printf(TEXT("Saved At: %s"), *LastSavedTimestamp));
    Response.OutputLines.Add(FString::Printf(TEXT("Saved Subject: %s"), *Snapshot.SelectedActorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Subject Source: %s"), Snapshot.SubjectSourceLabel.IsEmpty() ? TEXT("Editor Selection") : *Snapshot.SubjectSourceLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Detected Type: %s"), *Snapshot.ActorTypeLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Workflow Preference: %s"), UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel)));
    Response.OutputLines.Add(FString::Printf(TEXT("Starter Preset: %s"), UIFmt::GetCharacterEnhancementPresetLabel(SelectedEnhancementPresetLabel)));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Detected Animation Blueprint: %s"),
        LastSavedAnimationBlueprintAssetLabel.IsEmpty() ? TEXT("(not linked)") : *LastSavedAnimationBlueprintAssetLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Saved Observer: %s"), LastSavedObserverLabel.IsEmpty() ? TEXT("(none saved)") : *LastSavedObserverLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Saved Target: %s"), LastSavedTargetLabel.IsEmpty() ? TEXT("(none saved)") : *LastSavedTargetLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("AI Controller: %s"), *LastSavedSubjectAIControllerResult));
    Response.OutputLines.Add(FString::Printf(TEXT("Animation Blueprint: %s"), *LastSavedSubjectAnimationResult));
    Response.OutputLines.Add(FString::Printf(TEXT("Identity: %s"), *LastSavedSubjectIdentityResult));
    Response.OutputLines.Add(FString::Printf(TEXT("WAI: %s"), *LastSavedSubjectWAIResult));
    Response.OutputLines.Add(FString::Printf(TEXT("WAY: %s"), *LastSavedSubjectWAYResult));
    Response.OutputLines.Add(FString::Printf(TEXT("AI-Ready: %s"), *LastSavedSubjectAIReadyResult));
    Response.OutputLines.Add(TEXT("Readiness Notes: This snapshot is stored in the project's editor settings so you can reuse it across editor sessions."));
    Response.OutputLines.Add(TEXT("Readiness Notes: Use Restore Last Saved State to bring back the saved workflow choices and sandbox assignments when they are still available."));
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::RestoreSavedSubjectProgress()
{
    if (!bSavedSubjectProgressInitialized)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No saved Wana Works progress.");
        Response.OutputLines.Add(TEXT("Use Save Progress first to keep a reusable subject workflow snapshot."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Restored saved Wana Works progress.");

    if (!LastSavedSubjectWorkflowLabel.IsEmpty())
    {
        SelectedEnhancementWorkflowLabel = LastSavedSubjectWorkflowLabel;
        Response.OutputLines.Add(FString::Printf(TEXT("Workflow Preference: %s"), *LastSavedSubjectWorkflowLabel));
    }

    if (!LastSavedSubjectPresetLabel.IsEmpty())
    {
        SelectedEnhancementPresetLabel = LastSavedSubjectPresetLabel;
        Response.OutputLines.Add(FString::Printf(TEXT("Starter Preset: %s"), *LastSavedSubjectPresetLabel));
    }

    if (RestoreSubjectPickerFromAssetPath(LastSavedSubjectPath))
    {
        Response.OutputLines.Add(FString::Printf(
            TEXT("Restored Picker Subject: %s"),
            !SelectedAIPawnAssetLabel.IsEmpty() ? *SelectedAIPawnAssetLabel : *SelectedCharacterPawnAssetLabel));
    }

    AActor* RestoredSubjectActor = ResolveSavedActorPath(LastSavedSubjectPath);

    if (RestoredSubjectActor && GEditor)
    {
        GEditor->SelectNone(false, true, false);
        GEditor->SelectActor(RestoredSubjectActor, true, true, true);
        GEditor->NoteSelectionChange();
        Response.OutputLines.Add(FString::Printf(TEXT("Restored Subject: %s"), *RestoredSubjectActor->GetActorNameOrLabel()));
    }
    else if (!LastSavedSubjectPath.IsEmpty())
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Restored Subject: %s"), LastSavedSubjectLabel.IsEmpty() ? TEXT("Saved subject could not be found in the current level.") : *LastSavedSubjectLabel));
        Response.OutputLines.Add(TEXT("Readiness Notes: The saved subject actor is not currently loaded, so WanaWorks restored the workflow choices and any matching project picker subject instead."));
    }

    if (!LastSavedAnimationBlueprintAssetLabel.IsEmpty())
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Detected Animation Blueprint At Save: %s"), *LastSavedAnimationBlueprintAssetLabel));
    }

    SandboxObserverActor = ResolveSavedActorPath(LastSavedObserverPath);
    SandboxTargetActor = ResolveSavedActorPath(LastSavedTargetPath);
    const FString RestoredObserverLabel = SandboxObserverActor.IsValid()
        ? SandboxObserverActor->GetActorNameOrLabel()
        : (LastSavedObserverLabel.IsEmpty() ? TEXT("(none saved)") : FString::Printf(TEXT("%s (not currently available)"), *LastSavedObserverLabel));
    const FString RestoredTargetLabel = SandboxTargetActor.IsValid()
        ? SandboxTargetActor->GetActorNameOrLabel()
        : (LastSavedTargetLabel.IsEmpty() ? TEXT("(none saved)") : FString::Printf(TEXT("%s (not currently available)"), *LastSavedTargetLabel));

    Response.OutputLines.Add(FString::Printf(
        TEXT("Restored Observer: %s"),
        *RestoredObserverLabel));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Restored Target: %s"),
        *RestoredTargetLabel));

    if ((!LastSavedObserverPath.IsEmpty() && !SandboxObserverActor.IsValid())
        || (!LastSavedTargetPath.IsEmpty() && !SandboxTargetActor.IsValid()))
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Some saved actor assignments were not available in the current level, so WanaWorks kept only the assignments it could safely restore."));
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::SaveCurrentStateAsWorkflowPreset()
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;

    LastSavedWorkflowPresetTimestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
    LastSavedWorkflowPresetSourceLabel = bHasSnapshot ? Snapshot.SelectedActorLabel : TEXT("Current workflow controls");
    LastSavedWorkflowPresetWorkflowLabel = UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel);
    LastSavedWorkflowPresetEnhancementPresetLabel = UIFmt::GetCharacterEnhancementPresetLabel(SelectedEnhancementPresetLabel);
    LastSavedWorkflowPresetIdentitySeedLabel = UIFmt::GetRelationshipStateLabel(SelectedIdentitySeedState);
    LastSavedWorkflowPresetRelationshipLabel = UIFmt::GetRelationshipStateLabel(SelectedRelationshipState);
    LastSavedWorkflowPresetBehaviorLabel = GetPresetRecommendedBehaviorLabel(LastSavedWorkflowPresetEnhancementPresetLabel, SelectedRelationshipState);
    LastSavedWorkflowPresetFactionTag = IdentityFactionTagText.TrimStartAndEnd();
    bSavedWorkflowPresetInitialized = true;
    SelectedWorkflowPresetLabel = SavedCustomWorkflowPresetLabel;
    PersistSavedWorkflowPresets();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Saved current Wana Works preset.");
    Response.OutputLines.Add(FString::Printf(TEXT("Preset Slot: %s"), SavedCustomWorkflowPresetLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Saved At: %s"), *LastSavedWorkflowPresetTimestamp));
    Response.OutputLines.Add(FString::Printf(TEXT("Source Subject: %s"), *LastSavedWorkflowPresetSourceLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Workflow Path: %s"), *LastSavedWorkflowPresetWorkflowLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Starter Preset: %s"), *LastSavedWorkflowPresetEnhancementPresetLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Identity Seed: %s"), *LastSavedWorkflowPresetIdentitySeedLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Relationship Default: %s"), *LastSavedWorkflowPresetRelationshipLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Recommended Behavior: %s"), *LastSavedWorkflowPresetBehaviorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Identity Faction Tag: %s"), LastSavedWorkflowPresetFactionTag.IsEmpty() ? TEXT("(none)") : *LastSavedWorkflowPresetFactionTag));
    Response.OutputLines.Add(TEXT("Readiness Notes: The saved custom preset is stored in this project's editor settings so you can reapply it in future editor sessions."));
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ShowSelectedWorkflowPresetSummary()
{
    const FWanaWorkflowPresetConfig Config = BuildWorkflowPresetConfig(
        SelectedWorkflowPresetLabel,
        bSavedWorkflowPresetInitialized,
        LastSavedWorkflowPresetTimestamp,
        LastSavedWorkflowPresetSourceLabel,
        LastSavedWorkflowPresetWorkflowLabel,
        LastSavedWorkflowPresetEnhancementPresetLabel,
        LastSavedWorkflowPresetIdentitySeedLabel,
        LastSavedWorkflowPresetRelationshipLabel,
        LastSavedWorkflowPresetBehaviorLabel,
        LastSavedWorkflowPresetFactionTag);

    if (!Config.bValid)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No saved custom preset yet.");
        Response.OutputLines.Add(TEXT("Use Save Current as Preset to capture the current WanaWorks workflow into the custom preset slot."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Showing preset summary.");

    TArray<FString> SummaryLines;
    GetWorkflowPresetSummaryText().ToString().ParseIntoArrayLines(SummaryLines, false);

    for (const FString& SummaryLine : SummaryLines)
    {
        if (!SummaryLine.IsEmpty())
        {
            Response.OutputLines.Add(SummaryLine);
        }
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ApplySelectedWorkflowPreset()
{
    const FWanaWorkflowPresetConfig Config = BuildWorkflowPresetConfig(
        SelectedWorkflowPresetLabel,
        bSavedWorkflowPresetInitialized,
        LastSavedWorkflowPresetTimestamp,
        LastSavedWorkflowPresetSourceLabel,
        LastSavedWorkflowPresetWorkflowLabel,
        LastSavedWorkflowPresetEnhancementPresetLabel,
        LastSavedWorkflowPresetIdentitySeedLabel,
        LastSavedWorkflowPresetRelationshipLabel,
        LastSavedWorkflowPresetBehaviorLabel,
        LastSavedWorkflowPresetFactionTag);

    if (!Config.bValid)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No saved custom preset yet.");
        Response.OutputLines.Add(TEXT("Use Save Current as Preset before trying to apply the saved custom preset slot."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    SelectedEnhancementWorkflowLabel = Config.WorkflowLabel;
    SelectedEnhancementPresetLabel = Config.EnhancementPresetLabel;
    SelectedIdentitySeedState = Config.IdentitySeedState;
    SelectedRelationshipState = Config.RelationshipState;

    if (Config.bIsCustom)
    {
        IdentityFactionTagText = Config.FactionTagText;
    }

    PersistSavedWorkflowPresets();

    FWanaSelectedCharacterEnhancementSnapshot SourceSnapshot;
    ResolvePreferredSubjectSnapshot(SourceSnapshot);
    const bool bHasSourceSnapshot = SourceSnapshot.bHasSelectedActor;

    if (!bHasSourceSnapshot)
    {
        FWanaCommandResponse Response;
        Response.bSucceeded = true;
        Response.StatusMessage = TEXT("Status: Loaded preset into Wana Works controls.");
        Response.OutputLines.Add(FString::Printf(TEXT("Preset: %s"), *Config.PresetLabel));
        Response.OutputLines.Add(FString::Printf(TEXT("Workflow Path: %s"), *Config.WorkflowLabel));
        Response.OutputLines.Add(FString::Printf(TEXT("Starter Preset: %s"), *Config.EnhancementPresetLabel));
        Response.OutputLines.Add(FString::Printf(TEXT("Identity Seed: %s"), UIFmt::GetRelationshipStateLabel(Config.IdentitySeedState)));
        Response.OutputLines.Add(FString::Printf(TEXT("Relationship Default: %s"), UIFmt::GetRelationshipStateLabel(Config.RelationshipState)));
        Response.OutputLines.Add(FString::Printf(TEXT("Recommended Behavior: %s"), *Config.RecommendedBehaviorLabel));
        Response.OutputLines.Add(TEXT("Readiness Notes: Choose a Character Pawn or AI Pawn in the WanaWorks picker, or use an editor-selected actor as a fallback, before applying this preset."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    FWanaCommandResponse WorkflowResponse;
    FWanaCommandResponse EnhancementResponse;
    FWanaCommandResponse AIReadyResponse;
    FWanaCommandResponse IdentityResponse;
    FWanaCommandResponse RelationshipResponse;
    FWanaCommandResponse PreparationResponse;
    const bool bUseSandboxDuplicate = Config.WorkflowLabel == TEXT("Create Sandbox Duplicate");
    const bool bUseAIReadyPrep = Config.WorkflowLabel == TEXT("Convert to AI-Ready Test Subject");
    bool bSpawnedFromPicker = false;

    if (!PreparePickerDrivenSubjectForWorkflow(Config.WorkflowLabel, PreparationResponse, bSpawnedFromPicker))
    {
        PreparationResponse.OutputLines.Insert(FString::Printf(TEXT("Preset: %s"), *Config.PresetLabel), 0);
        ApplyResponse(PreparationResponse);
        RefreshReactiveUI(true);
        return;
    }

    if (!bHasSourceSnapshot)
    {
        ResolvePreferredSubjectSnapshot(SourceSnapshot);
    }

    if (bUseSandboxDuplicate && !bSpawnedFromPicker)
    {
        WorkflowResponse = WanaWorksUIEditorActions::ExecuteCreateSandboxDuplicateCommand();

        if (!WorkflowResponse.bSucceeded)
        {
            WorkflowResponse.OutputLines.Insert(FString::Printf(TEXT("Preset: %s"), *Config.PresetLabel), 0);
            ApplyResponse(WorkflowResponse);
            RefreshReactiveUI(true);
            return;
        }
    }

    EnhancementResponse = WanaWorksUIEditorActions::ExecuteApplyCharacterEnhancementCommand(Config.EnhancementPresetLabel);

    if (!EnhancementResponse.bSucceeded)
    {
        EnhancementResponse.OutputLines.Insert(FString::Printf(TEXT("Preset: %s"), *Config.PresetLabel), 0);
        ApplyResponse(EnhancementResponse);
        RefreshReactiveUI(true);
        return;
    }

    if (bUseAIReadyPrep)
    {
        AIReadyResponse = WanaWorksUIEditorActions::ExecutePrepareSelectedActorForAITestCommand();

        if (!AIReadyResponse.bSucceeded)
        {
            AIReadyResponse.OutputLines.Insert(FString::Printf(TEXT("Preset: %s"), *Config.PresetLabel), 0);
            ApplyResponse(AIReadyResponse);
            RefreshReactiveUI(true);
            return;
        }
    }

    IdentityResponse = WanaWorksUIEditorActions::ExecuteApplyIdentityCommand(IdentityFactionTagText, SelectedIdentitySeedState);

    if (!IdentityResponse.bSucceeded)
    {
        IdentityResponse.OutputLines.Insert(FString::Printf(TEXT("Preset: %s"), *Config.PresetLabel), 0);
        ApplyResponse(IdentityResponse);
        RefreshReactiveUI(true);
        return;
    }

    FWanaSelectedCharacterEnhancementSnapshot ActiveSnapshot;
    const bool bHasActiveSnapshot = ResolvePreferredSubjectSnapshot(ActiveSnapshot) && ActiveSnapshot.bHasSelectedActor;

    if (bHasActiveSnapshot)
    {
        SandboxObserverActor = ActiveSnapshot.SelectedActor;
    }

    FWanaSelectedRelationshipContextSnapshot RelationshipContext;
    const bool bHasExplicitRelationshipPair =
        Config.bWantsRelationshipStarter
        && WanaWorksUIEditorActions::GetSelectedRelationshipContextSnapshot(RelationshipContext)
        && RelationshipContext.bHasObserverActor
        && RelationshipContext.bHasTargetActor
        && !RelationshipContext.bTargetFallsBackToObserver;

    if (bHasExplicitRelationshipPair)
    {
        RelationshipResponse = WanaWorksUIEditorActions::ExecuteApplyRelationshipStateCommand(UIFmt::GetRelationshipStateLabel(SelectedRelationshipState));
    }

    UpdateEnhancementResultsState(
        &SourceSnapshot,
        bHasActiveSnapshot ? &ActiveSnapshot : &SourceSnapshot,
        Config.WorkflowLabel,
        true,
        bUseSandboxDuplicate || bSpawnedFromPicker,
        true);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Applied Wana Works preset.");
    Response.OutputLines.Add(FString::Printf(TEXT("Preset: %s"), *Config.PresetLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Selected Subject: %s"), *SourceSnapshot.SelectedActorLabel));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Active Subject: %s"),
        bHasActiveSnapshot ? *ActiveSnapshot.SelectedActorLabel : *SourceSnapshot.SelectedActorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Workflow Path: %s"), *Config.WorkflowLabel));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Original Or Duplicate: %s"),
        (bUseSandboxDuplicate || bSpawnedFromPicker) ? TEXT("Sandbox subject") : TEXT("Original actor")));
    Response.OutputLines.Add(FString::Printf(TEXT("Starter Preset: %s"), *Config.EnhancementPresetLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Identity Seed: %s"), UIFmt::GetRelationshipStateLabel(Config.IdentitySeedState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Relationship Default: %s"), UIFmt::GetRelationshipStateLabel(Config.RelationshipState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Recommended Behavior: %s"), *Config.RecommendedBehaviorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Coverage: %s"), *Config.CoverageLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Subject Source: %s"), SourceSnapshot.SubjectSourceLabel.IsEmpty() ? TEXT("Unknown") : *SourceSnapshot.SubjectSourceLabel));

    if (Config.bIsCustom)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Preset Source: %s"), Config.SourceLabel.IsEmpty() ? TEXT("Current workflow controls") : *Config.SourceLabel));
        Response.OutputLines.Add(FString::Printf(TEXT("Saved At: %s"), Config.SavedTimestamp.IsEmpty() ? TEXT("Unknown") : *Config.SavedTimestamp));
    }

    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Components Added:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Already Present:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Compatibility Notes:"));

    if (bSpawnedFromPicker)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: WanaWorks created a sandbox-first subject from the project picker before applying the preset."));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Picked Subject Asset:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Picked Animation Blueprint:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Animation Override Applied:"));
    }
    else if (bUseSandboxDuplicate)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: WanaWorks created a sandbox duplicate before applying the preset."));
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

    const FString TrimmedFactionTag = IdentityFactionTagText.TrimStartAndEnd();
    Response.OutputLines.Add(FString::Printf(
        TEXT("Identity Defaults: Faction Tag=%s | Seed=%s"),
        TrimmedFactionTag.IsEmpty() ? TEXT("(none)") : *TrimmedFactionTag,
        UIFmt::GetRelationshipStateLabel(SelectedIdentitySeedState)));

    if (Config.bWantsRelationshipStarter)
    {
        if (bHasExplicitRelationshipPair && RelationshipResponse.bSucceeded)
        {
            Response.OutputLines.Add(FString::Printf(
                TEXT("Relationship Starter: Applied %s for the current observer/target pair."),
                UIFmt::GetRelationshipStateLabel(SelectedRelationshipState)));
        }
        else
        {
            Response.OutputLines.Add(FString::Printf(
                TEXT("Relationship Starter: Staged %s. Select an explicit observer and target to apply it without self-target fallback."),
                UIFmt::GetRelationshipStateLabel(SelectedRelationshipState)));
        }
    }
    else
    {
        Response.OutputLines.Add(TEXT("Relationship Starter: Not included in this preset."));
    }

    Response.OutputLines.Add(FString::Printf(TEXT("Preset Notes: %s"), *Config.ApplyNotes));
    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ApplyCharacterEnhancement()
{
    FWanaSelectedCharacterEnhancementSnapshot SourceSnapshot;
    ResolvePreferredSubjectSnapshot(SourceSnapshot);
    const bool bHasSourceSnapshot = SourceSnapshot.bHasSelectedActor;

    const FString WorkflowLabel = UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel);
    FWanaCommandResponse WorkflowResponse;
    FWanaCommandResponse EnhancementResponse;
    FWanaCommandResponse AIReadyResponse;
    FWanaCommandResponse PreparationResponse;
    const bool bUseSandboxDuplicate = WorkflowLabel == TEXT("Create Sandbox Duplicate");
    const bool bUseAIReadyPrep = WorkflowLabel == TEXT("Convert to AI-Ready Test Subject");
    bool bSpawnedFromPicker = false;

    if (!PreparePickerDrivenSubjectForWorkflow(WorkflowLabel, PreparationResponse, bSpawnedFromPicker))
    {
        ApplyResponse(PreparationResponse);
        RefreshReactiveUI(true);
        return;
    }

    if (!bHasSourceSnapshot)
    {
        ResolvePreferredSubjectSnapshot(SourceSnapshot);
    }

    if (bUseSandboxDuplicate && !bSpawnedFromPicker)
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
    Response.OutputLines.Add(FString::Printf(
        TEXT("Original Or Duplicate: %s"),
        (bUseSandboxDuplicate || bSpawnedFromPicker) ? TEXT("Sandbox subject") : TEXT("Original actor")));
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

    if (bUseSandboxDuplicate && !bSpawnedFromPicker)
    {
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Compatibility Notes:"));
        Response.OutputLines.Add(TEXT("Readiness Notes: Sandbox observer was updated to the prepared subject."));
    }
    else if (bSpawnedFromPicker)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: WanaWorks created a sandbox-first subject from the project picker before applying enhancement."));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Picked Subject Asset:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Picked Animation Blueprint:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Animation Override Applied:"));
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
        bUseSandboxDuplicate || bSpawnedFromPicker,
        true);

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ApplyStarterAndTestTarget()
{
    FWanaSelectedCharacterEnhancementSnapshot BeforeSnapshot;
    ResolvePreferredSubjectSnapshot(BeforeSnapshot);
    const bool bHasBeforeSnapshot = BeforeSnapshot.bHasSelectedActor;
    FWanaCommandResponse PreparationResponse;
    bool bSpawnedFromPicker = false;

    if (!PreparePickerDrivenSubjectForWorkflow(TEXT("Apply Starter + Test Target"), PreparationResponse, bSpawnedFromPicker))
    {
        ApplyResponse(PreparationResponse);
        RefreshReactiveUI(true);
        return;
    }

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
            bSpawnedFromPicker,
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

void FWanaWorksUIModule::FinalizeSandboxBuild()
{
    FWanaSelectedCharacterEnhancementSnapshot WorkflowSnapshot;
    const bool bHasWorkflowSnapshot = ResolvePreferredSubjectSnapshot(WorkflowSnapshot) && WorkflowSnapshot.bHasSelectedActor;

    FWanaSelectedCharacterEnhancementSnapshot SandboxSnapshot;
    const bool bHasSandboxSnapshot =
        SandboxObserverActor.IsValid()
        && WanaWorksUIEditorActions::GetCharacterEnhancementSnapshotForActor(SandboxObserverActor.Get(), SandboxSnapshot)
        && SandboxSnapshot.bHasSelectedActor;

    UObject* BuildSourceObject = LoadSelectedSubjectAssetObject();
    FString SourceAssetLabel;
    FString SourceAssetPath = GetSelectedSubjectAssetPath();

    if (BuildSourceObject)
    {
        SourceAssetLabel = BuildSourceObject->GetName();
    }
    else if (bHasSandboxSnapshot && SandboxSnapshot.SelectedActor.IsValid())
    {
        BuildSourceObject = SandboxSnapshot.SelectedActor->GetClass();
        SourceAssetLabel = FString::Printf(TEXT("%s (sandbox workflow fallback)"), *SandboxSnapshot.SelectedActor->GetClass()->GetName());
    }
    else if (bHasWorkflowSnapshot && WorkflowSnapshot.SelectedActor.IsValid())
    {
        BuildSourceObject = WorkflowSnapshot.SelectedActor->GetClass();
        SourceAssetLabel = FString::Printf(TEXT("%s (selection fallback)"), *WorkflowSnapshot.SelectedActor->GetClass()->GetName());
    }

    if (!BuildSourceObject)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No sandbox build source available.");
        Response.OutputLines.Add(TEXT("Choose a Character Pawn or AI Pawn in the WanaWorks picker, or prepare a sandbox subject first, before building a final version."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    const FString BuildSourceLabel = SourceAssetLabel.IsEmpty() ? BuildSourceObject->GetName() : SourceAssetLabel;
    const FString WorkflowLabel = UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel);
    const bool bShouldPrepareAIReady =
        WorkflowLabel == TEXT("Convert to AI-Ready Test Subject")
        || (bHasSandboxSnapshot && SandboxSnapshot.bAutoPossessAIEnabled);

    FWanaCommandResponse BuildResponse = WanaWorksUIEditorActions::ExecuteCreateFinalizedBuildSubjectFromAssetCommand(
        BuildSourceObject,
        nullptr);

    if (!BuildResponse.bSucceeded)
    {
        ApplyResponse(BuildResponse);
        RefreshReactiveUI(true);
        return;
    }

    FWanaSelectedCharacterEnhancementSnapshot BuildBeforeSnapshot;
    const bool bHasBuildBeforeSnapshot =
        WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(BuildBeforeSnapshot)
        && BuildBeforeSnapshot.bHasSelectedActor;

    FWanaCommandResponse EnhancementResponse = WanaWorksUIEditorActions::ExecuteApplyCharacterEnhancementCommand(SelectedEnhancementPresetLabel);

    if (!EnhancementResponse.bSucceeded)
    {
        EnhancementResponse.OutputLines.Insert(FString::Printf(TEXT("Build Source: %s"), *BuildSourceLabel), 0);
        UIFmt::AppendLinesWithPrefix(EnhancementResponse, BuildResponse, TEXT("Finalized Output:"));
        ApplyResponse(EnhancementResponse);
        RefreshReactiveUI(true);
        return;
    }

    FWanaCommandResponse AIReadyResponse;

    if (bShouldPrepareAIReady)
    {
        AIReadyResponse = WanaWorksUIEditorActions::ExecutePrepareSelectedActorForAITestCommand();

        if (!AIReadyResponse.bSucceeded)
        {
            AIReadyResponse.OutputLines.Insert(FString::Printf(TEXT("Build Source: %s"), *BuildSourceLabel), 0);
            UIFmt::AppendLinesWithPrefix(AIReadyResponse, BuildResponse, TEXT("Finalized Output:"));
            ApplyResponse(AIReadyResponse);
            RefreshReactiveUI(true);
            return;
        }
    }

    FWanaCommandResponse IdentityResponse = WanaWorksUIEditorActions::ExecuteApplyIdentityCommand(IdentityFactionTagText, SelectedIdentitySeedState);

    if (!IdentityResponse.bSucceeded)
    {
        IdentityResponse.OutputLines.Insert(FString::Printf(TEXT("Build Source: %s"), *BuildSourceLabel), 0);
        UIFmt::AppendLinesWithPrefix(IdentityResponse, BuildResponse, TEXT("Finalized Output:"));
        ApplyResponse(IdentityResponse);
        RefreshReactiveUI(true);
        return;
    }

    FWanaSelectedCharacterEnhancementSnapshot BuildAfterSnapshot;
    const bool bHasBuildAfterSnapshot =
        WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(BuildAfterSnapshot)
        && BuildAfterSnapshot.bHasSelectedActor;

    UpdateEnhancementResultsState(
        bHasBuildBeforeSnapshot ? &BuildBeforeSnapshot : nullptr,
        bHasBuildAfterSnapshot ? &BuildAfterSnapshot : (bHasBuildBeforeSnapshot ? &BuildBeforeSnapshot : nullptr),
        TEXT("Finalize Sandbox Build"),
        true,
        false,
        true);

    FString FinalizedOutputPath = TEXT("WanaWorks/Builds");

    if (bHasBuildAfterSnapshot && BuildAfterSnapshot.SelectedActor.IsValid())
    {
        const FString OutputFolder = BuildAfterSnapshot.SelectedActor->GetFolderPath().ToString();
        FinalizedOutputPath = OutputFolder.IsEmpty()
            ? BuildAfterSnapshot.SelectedActorLabel
            : FString::Printf(TEXT("%s/%s"), *OutputFolder, *BuildAfterSnapshot.SelectedActorLabel);
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Built finalized WanaWorks subject.");
    Response.OutputLines.Add(FString::Printf(TEXT("Source Asset: %s"), *BuildSourceLabel));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Sandbox Subject: %s"),
        bHasSandboxSnapshot ? *SandboxSnapshot.SelectedActorLabel : TEXT("(not assigned - built from current workflow state)")));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Finalized Output: %s"),
        bHasBuildAfterSnapshot ? *BuildAfterSnapshot.SelectedActorLabel : TEXT("(created)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Finalized Output Path: %s"), *FinalizedOutputPath));
    Response.OutputLines.Add(TEXT("Workflow Path: Sandbox Subject -> Final Build"));
    Response.OutputLines.Add(FString::Printf(TEXT("Starter Preset: %s"), UIFmt::GetCharacterEnhancementPresetLabel(SelectedEnhancementPresetLabel)));
    Response.OutputLines.Add(FString::Printf(TEXT("Build Workflow State: %s"), *WorkflowLabel));
    UIFmt::AppendLinesWithPrefix(Response, BuildResponse, TEXT("Picked Animation Blueprint:"));
    UIFmt::AppendLinesWithPrefix(Response, BuildResponse, TEXT("Animation Override Applied:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Components Added:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Already Present:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Compatibility Notes:"));

    if (bShouldPrepareAIReady)
    {
        UIFmt::AppendLinesWithPrefix(Response, AIReadyResponse, TEXT("AI-Ready Preparation:"));
        UIFmt::AppendLinesWithPrefix(Response, AIReadyResponse, TEXT("AI Controller Class:"));
    }
    else
    {
        Response.OutputLines.Add(TEXT("AI-Ready Preparation: Not requested for this finalized build."));
    }

    const FString TrimmedFactionTag = IdentityFactionTagText.TrimStartAndEnd();
    Response.OutputLines.Add(FString::Printf(
        TEXT("Identity Defaults: Faction Tag=%s | Seed=%s"),
        TrimmedFactionTag.IsEmpty() ? TEXT("(none)") : *TrimmedFactionTag,
        UIFmt::GetRelationshipStateLabel(SelectedIdentitySeedState)));
    Response.OutputLines.Add(TEXT("Preserved: The original source asset and the sandbox working subject were both left untouched."));
    Response.OutputLines.Add(TEXT("Built: A separate finalized upgraded subject was created under WanaWorks/Builds for the next stage of use."));

    if (!SourceAssetPath.IsEmpty())
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Source Asset Path: %s"), *SourceAssetPath));
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

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedWorkflowPresetOption() const
{
    const FString SelectedLabel = SelectedWorkflowPresetLabel.IsEmpty() ? TEXT("Full WanaAI Starter") : SelectedWorkflowPresetLabel;

    for (const TSharedPtr<FString>& Option : WorkflowPresetOptions)
    {
        if (Option.IsValid() && Option->Equals(SelectedLabel))
        {
            return Option;
        }
    }

    return WorkflowPresetOptions.Num() > 0 ? WorkflowPresetOptions[0] : nullptr;
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

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedCharacterPawnAssetOption() const
{
    const FString SelectedLabel = SelectedCharacterPawnAssetLabel.IsEmpty() ? CharacterPawnPickerDefaultLabel : SelectedCharacterPawnAssetLabel;

    for (const TSharedPtr<FString>& Option : CharacterPawnAssetOptions)
    {
        if (Option.IsValid() && Option->Equals(SelectedLabel))
        {
            return Option;
        }
    }

    return CharacterPawnAssetOptions.Num() > 0 ? CharacterPawnAssetOptions[0] : nullptr;
}

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedAIPawnAssetOption() const
{
    const FString SelectedLabel = SelectedAIPawnAssetLabel.IsEmpty() ? AIPawnPickerDefaultLabel : SelectedAIPawnAssetLabel;

    for (const TSharedPtr<FString>& Option : AIPawnAssetOptions)
    {
        if (Option.IsValid() && Option->Equals(SelectedLabel))
        {
            return Option;
        }
    }

    return AIPawnAssetOptions.Num() > 0 ? AIPawnAssetOptions[0] : nullptr;
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

FText FWanaWorksUIModule::GetWorkflowPresetSummaryText() const
{
    const FWanaWorkflowPresetConfig Config = BuildWorkflowPresetConfig(
        SelectedWorkflowPresetLabel,
        bSavedWorkflowPresetInitialized,
        LastSavedWorkflowPresetTimestamp,
        LastSavedWorkflowPresetSourceLabel,
        LastSavedWorkflowPresetWorkflowLabel,
        LastSavedWorkflowPresetEnhancementPresetLabel,
        LastSavedWorkflowPresetIdentitySeedLabel,
        LastSavedWorkflowPresetRelationshipLabel,
        LastSavedWorkflowPresetBehaviorLabel,
        LastSavedWorkflowPresetFactionTag);

    return FText::FromString(UISummary::BuildWorkflowPresetSummaryText(
        Config.PresetLabel,
        Config.bIsCustom,
        bSavedWorkflowPresetInitialized,
        Config.SavedTimestamp,
        Config.SourceLabel,
        Config.WorkflowLabel,
        Config.EnhancementPresetLabel,
        UIFmt::GetRelationshipStateLabel(Config.IdentitySeedState),
        UIFmt::GetRelationshipStateLabel(Config.RelationshipState),
        Config.RecommendedBehaviorLabel,
        Config.CoverageLabel,
        Config.ApplyNotes,
        Config.PersistenceNotes));
}

FText FWanaWorksUIModule::GetSubjectSetupSummaryText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildSubjectSetupSummaryText(
        bHasSnapshot ? &Snapshot : nullptr,
        SelectedEnhancementWorkflowLabel,
        SelectedEnhancementPresetLabel));
}

FText FWanaWorksUIModule::GetSubjectStackSummaryText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildSubjectStackSummaryText(bHasSnapshot ? &Snapshot : nullptr));
}

FText FWanaWorksUIModule::GetAnimationIntegrationText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildAnimationIntegrationSummaryText(bHasSnapshot ? &Snapshot : nullptr));
}

FText FWanaWorksUIModule::GetSavedSubjectProgressText() const
{
    return FText::FromString(UISummary::BuildSavedSubjectProgressSummaryText(
        bSavedSubjectProgressInitialized,
        LastSavedTimestamp,
        LastSavedSubjectPath,
        LastSavedSubjectLabel,
        LastSavedSubjectTypeLabel,
        LastSavedSubjectWorkflowLabel,
        LastSavedSubjectPresetLabel,
        LastSavedSubjectAIControllerResult,
        LastSavedSubjectAnimationResult,
        LastSavedSubjectIdentityResult,
        LastSavedSubjectWAIResult,
        LastSavedSubjectWAYResult,
        LastSavedSubjectAIReadyResult,
        LastSavedObserverLabel,
        LastSavedTargetLabel));
}

FText FWanaWorksUIModule::GetCharacterEnhancementSummaryText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildCharacterEnhancementSummaryText(bHasSnapshot ? &Snapshot : nullptr));
}

FText FWanaWorksUIModule::GetCharacterEnhancementChainText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
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
    BuilderArgs.GetWorkflowPresetSummaryText = [this]() { return GetWorkflowPresetSummaryText(); };
    BuilderArgs.GetSubjectSetupSummaryText = [this]() { return GetSubjectSetupSummaryText(); };
    BuilderArgs.GetSubjectStackSummaryText = [this]() { return GetSubjectStackSummaryText(); };
    BuilderArgs.GetAnimationIntegrationText = [this]() { return GetAnimationIntegrationText(); };
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
    BuilderArgs.CharacterPawnAssetOptions = &CharacterPawnAssetOptions;
    BuilderArgs.AIPawnAssetOptions = &AIPawnAssetOptions;
    BuilderArgs.WorkflowPresetOptions = &WorkflowPresetOptions;
    BuilderArgs.EnhancementPresetOptions = &EnhancementPresetOptions;
    BuilderArgs.EnhancementWorkflowOptions = &EnhancementWorkflowOptions;
    BuilderArgs.RelationshipStateOptions = &RelationshipStateOptions;
    BuilderArgs.GetSelectedCharacterPawnAssetOption = [this]() { return GetSelectedCharacterPawnAssetOption(); };
    BuilderArgs.GetSelectedAIPawnAssetOption = [this]() { return GetSelectedAIPawnAssetOption(); };
    BuilderArgs.GetSelectedWorkflowPresetOption = [this]() { return GetSelectedWorkflowPresetOption(); };
    BuilderArgs.GetSelectedEnhancementPresetOption = [this]() { return GetSelectedEnhancementPresetOption(); };
    BuilderArgs.GetSelectedEnhancementWorkflowOption = [this]() { return GetSelectedEnhancementWorkflowOption(); };
    BuilderArgs.GetSelectedIdentitySeedStateOption = [this]() { return GetSelectedIdentitySeedStateOption(); };
    BuilderArgs.GetSelectedRelationshipStateOption = [this]() { return GetSelectedRelationshipStateOption(); };
    BuilderArgs.OnCommandTextChanged = [this](const FText& NewText) { HandleCommandTextChanged(NewText); };
    BuilderArgs.OnIdentityFactionTagTextChanged = [this](const FText& NewText) { HandleIdentityFactionTagTextChanged(NewText); };
    BuilderArgs.OnCharacterPawnAssetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleCharacterPawnAssetOptionSelected(SelectedOption); };
    BuilderArgs.OnAIPawnAssetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleAIPawnAssetOptionSelected(SelectedOption); };
    BuilderArgs.OnWorkflowPresetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleWorkflowPresetOptionSelected(SelectedOption); };
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
    BuilderArgs.OnRestoreSavedSubjectProgress = [this]() { RestoreSavedSubjectProgress(); };
    BuilderArgs.OnApplyWorkflowPreset = [this]() { ApplySelectedWorkflowPreset(); };
    BuilderArgs.OnSaveWorkflowPreset = [this]() { SaveCurrentStateAsWorkflowPreset(); };
    BuilderArgs.OnShowWorkflowPresetSummary = [this]() { ShowSelectedWorkflowPresetSummary(); };
    BuilderArgs.OnApplyCharacterEnhancement = [this]() { ApplyCharacterEnhancement(); };
    BuilderArgs.OnApplyStarterAndTestTarget = [this]() { ApplyStarterAndTestTarget(); };
    BuilderArgs.OnScanEnvironmentReadiness = [this]() { ScanEnvironmentReadiness(); };
    BuilderArgs.OnEvaluateLiveTarget = [this]() { EvaluateLiveTarget(); };
    BuilderArgs.OnUseSelectedAsSandboxObserver = [this]() { UseSelectedActorAsSandboxObserver(); };
    BuilderArgs.OnUseSelectedAsSandboxTarget = [this]() { UseSelectedActorAsSandboxTarget(); };
    BuilderArgs.OnEvaluateSandboxPair = [this]() { EvaluateSandboxPair(); };
    BuilderArgs.OnFinalizeSandboxBuild = [this]() { FinalizeSandboxBuild(); };
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

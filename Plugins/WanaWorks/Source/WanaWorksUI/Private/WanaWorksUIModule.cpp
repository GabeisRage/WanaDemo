#include "WanaWorksUIModule.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/Skeleton.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Textures/SlateIcon.h"
#include "ToolMenus.h"
#include "UObject/UnrealType.h"
#include "UObject/SoftObjectPath.h"
#include "Widgets/InvalidateWidgetReason.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "WanaWorksCommandDispatcher.h"
#include "WanaWorksUIEditorActions.h"
#include "WanaWorksUIFormattingUtils.h"
#include "WanaWorksUISummaryText.h"
#include "WanaWorksUIStyle.h"
#include "WanaWorksUITabBuilder.h"
#include "WanaIdentityComponent.h"
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
const TCHAR* CharacterPawnPickerDefaultLabel = TEXT("(Choose Character Blueprint)");
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

TSharedPtr<FString> FindStringOptionByLabel(const TArray<TSharedPtr<FString>>& Options, const FString& Label)
{
    for (const TSharedPtr<FString>& Option : Options)
    {
        if (Option.IsValid() && Option->Equals(Label, ESearchCase::IgnoreCase))
        {
            return Option;
        }
    }

    return nullptr;
}

EWAYRelationshipState MapCharacterIntelligenceIdentityRoleToSeedState(const FString& RoleLabel)
{
    if (RoleLabel.Equals(TEXT("Hostile"), ESearchCase::IgnoreCase))
    {
        return EWAYRelationshipState::Enemy;
    }

    if (RoleLabel.Equals(TEXT("Guard"), ESearchCase::IgnoreCase))
    {
        return EWAYRelationshipState::Partner;
    }

    if (RoleLabel.Equals(TEXT("Companion"), ESearchCase::IgnoreCase))
    {
        return EWAYRelationshipState::Friend;
    }

    if (RoleLabel.Equals(TEXT("Patrol"), ESearchCase::IgnoreCase)
        || RoleLabel.Equals(TEXT("Observer"), ESearchCase::IgnoreCase))
    {
        return EWAYRelationshipState::Acquaintance;
    }

    return EWAYRelationshipState::Neutral;
}

EWAYRelationshipState MapCharacterIntelligenceRelationshipLabelToWAYState(const FString& RelationshipLabel)
{
    if (RelationshipLabel.Equals(TEXT("Friend"), ESearchCase::IgnoreCase))
    {
        return EWAYRelationshipState::Friend;
    }

    if (RelationshipLabel.Equals(TEXT("Ally"), ESearchCase::IgnoreCase))
    {
        return EWAYRelationshipState::Partner;
    }

    if (RelationshipLabel.Equals(TEXT("Suspicious"), ESearchCase::IgnoreCase))
    {
        return EWAYRelationshipState::Acquaintance;
    }

    if (RelationshipLabel.Equals(TEXT("Enemy"), ESearchCase::IgnoreCase)
        || RelationshipLabel.Equals(TEXT("Target"), ESearchCase::IgnoreCase))
    {
        return EWAYRelationshipState::Enemy;
    }

    return EWAYRelationshipState::Neutral;
}

FString GetCharacterIntelligenceRoleFactionTag(const FString& RoleLabel)
{
    const FString TrimmedRole = RoleLabel.TrimStartAndEnd();
    return TrimmedRole.IsEmpty() ? FString(TEXT("neutral")) : TrimmedRole.ToLower();
}

FString GetCharacterIntelligenceRecommendationLabel(const FString& IdentityRoleLabel, const FString& RelationshipLabel, bool bHasTarget)
{
    if (!bHasTarget)
    {
        return TEXT("Observe Target (target needed)");
    }

    if (RelationshipLabel.Equals(TEXT("Enemy"), ESearchCase::IgnoreCase)
        || RelationshipLabel.Equals(TEXT("Target"), ESearchCase::IgnoreCase))
    {
        return TEXT("Approach Hostile");
    }

    if (RelationshipLabel.Equals(TEXT("Ally"), ESearchCase::IgnoreCase))
    {
        return TEXT("Guard Target");
    }

    if (RelationshipLabel.Equals(TEXT("Friend"), ESearchCase::IgnoreCase))
    {
        return TEXT("Follow Target");
    }

    if (RelationshipLabel.Equals(TEXT("Suspicious"), ESearchCase::IgnoreCase))
    {
        return TEXT("Observe Target");
    }

    if (IdentityRoleLabel.Equals(TEXT("Hostile"), ESearchCase::IgnoreCase))
    {
        return TEXT("Approach Hostile");
    }

    if (IdentityRoleLabel.Equals(TEXT("Guard"), ESearchCase::IgnoreCase))
    {
        return TEXT("Guard Target");
    }

    if (IdentityRoleLabel.Equals(TEXT("Companion"), ESearchCase::IgnoreCase))
    {
        return TEXT("Follow Target");
    }

    return TEXT("Observe Target");
}

AActor* GetFirstEditorSelectedActor()
{
    USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;

    if (!SelectedActors || SelectedActors->Num() == 0)
    {
        return nullptr;
    }

    for (FSelectionIterator SelectionIt(*SelectedActors); SelectionIt; ++SelectionIt)
    {
        if (AActor* SelectedActor = Cast<AActor>(*SelectionIt))
        {
            return IsValid(SelectedActor) ? SelectedActor : nullptr;
        }
    }

    return nullptr;
}

ACharacter* FindEditorPlayerCharacterCandidate()
{
    UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

    if (!EditorWorld)
    {
        return nullptr;
    }

    ACharacter* FirstCharacter = nullptr;

    for (TActorIterator<ACharacter> It(EditorWorld); It; ++It)
    {
        ACharacter* Candidate = *It;

        if (!IsValid(Candidate))
        {
            continue;
        }

        if (!FirstCharacter)
        {
            FirstCharacter = Candidate;
        }

        if (Candidate->AutoPossessPlayer != EAutoReceiveInput::Disabled)
        {
            return Candidate;
        }
    }

    return FirstCharacter;
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

bool IsCharacterFacingPawnClass(const UClass* PawnClass, const APawn* DefaultPawn)
{
    return (PawnClass && PawnClass->IsChildOf(ACharacter::StaticClass()))
        || (DefaultPawn && DefaultPawn->FindComponentByClass<USkeletalMeshComponent>() != nullptr);
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

FString GetPreviewObjectDisplayLabel(const UObject* PreviewObject)
{
    if (const AActor* PreviewActor = Cast<AActor>(PreviewObject))
    {
        return PreviewActor->GetActorNameOrLabel();
    }

    return PreviewObject ? PreviewObject->GetName() : FString();
}

struct FWanaAnalysisItem
{
    FString Category;
    FString Status;
    FString Detail;
};

struct FWanaAnalysisResult
{
    FString WorkspaceLabel;
    FString DisplayWorkspaceLabel;
    FString StatusSummary;
    FString PrimarySummary;
    FString AnimationSummary;
    FString PhysicalSummary;
    FString BehaviorSummary;
    FString WITSummary;
    FString SuggestedSummary;
    int32 ReadyCount = 0;
    int32 MissingCount = 0;
    int32 LimitedCount = 0;
    int32 NotSupportedCount = 0;
    int32 RecommendedCount = 0;
};

void TrackAnalysisStatus(FWanaAnalysisResult& Result, const FString& Status)
{
    if (Status.Equals(TEXT("Ready"), ESearchCase::IgnoreCase)
        || Status.Equals(TEXT("Present"), ESearchCase::IgnoreCase)
        || Status.Equals(TEXT("Detected"), ESearchCase::IgnoreCase)
        || Status.Equals(TEXT("Built"), ESearchCase::IgnoreCase))
    {
        ++Result.ReadyCount;
    }
    else if (Status.Equals(TEXT("Missing"), ESearchCase::IgnoreCase)
        || Status.Equals(TEXT("Blocked"), ESearchCase::IgnoreCase)
        || Status.Equals(TEXT("Needs Analyze"), ESearchCase::IgnoreCase)
        || Status.Equals(TEXT("Needs Enhance"), ESearchCase::IgnoreCase)
        || Status.Equals(TEXT("Needs Test"), ESearchCase::IgnoreCase))
    {
        ++Result.MissingCount;
    }
    else if (Status.Equals(TEXT("Not Supported"), ESearchCase::IgnoreCase))
    {
        ++Result.NotSupportedCount;
    }
    else if (Status.Equals(TEXT("Recommended"), ESearchCase::IgnoreCase))
    {
        ++Result.RecommendedCount;
    }
    else
    {
        ++Result.LimitedCount;
    }
}

void AddAnalysisItem(TArray<FWanaAnalysisItem>& Items, FWanaAnalysisResult& Result, const FString& Category, const FString& Status, const FString& Detail)
{
    Items.Add({Category, Status, Detail});
    TrackAnalysisStatus(Result, Status);
}

FString BuildAnalysisCardText(const FString& Heading, const TArray<FWanaAnalysisItem>& Items, const FString& RecommendedAction)
{
    TArray<FString> Lines;

    if (!Heading.IsEmpty())
    {
        Lines.Add(Heading);
    }

    for (const FWanaAnalysisItem& Item : Items)
    {
        Lines.Add(FString::Printf(
            TEXT("%s: %s - %s"),
            *Item.Category,
            Item.Status.IsEmpty() ? TEXT("Limited") : *Item.Status,
            Item.Detail.IsEmpty() ? TEXT("No extra detail available.") : *Item.Detail));
    }

    if (!RecommendedAction.IsEmpty())
    {
        Lines.Add(FString::Printf(TEXT("Recommended Next Step: %s"), *RecommendedAction));
    }

    return FString::Join(Lines, TEXT("\n"));
}

FString BuildAnalysisStatusText(const FWanaAnalysisResult& Result)
{
    return FString::Printf(
        TEXT("%s analysis complete: %d ready, %d missing, %d limited, %d not supported."),
        Result.DisplayWorkspaceLabel.IsEmpty() ? TEXT("Workspace") : *Result.DisplayWorkspaceLabel,
        Result.ReadyCount,
        Result.MissingCount,
        Result.LimitedCount,
        Result.NotSupportedCount);
}

FString BuildWorkspaceTestStatusText(const FWanaAnalysisResult& Result, const FString& ResultLabel)
{
    return FString::Printf(
        TEXT("%s test complete: %s. %d ready, %d missing, %d limited, %d not supported."),
        Result.DisplayWorkspaceLabel.IsEmpty() ? TEXT("Workspace") : *Result.DisplayWorkspaceLabel,
        ResultLabel.IsEmpty() ? TEXT("validation finished") : *ResultLabel,
        Result.ReadyCount,
        Result.MissingCount,
        Result.LimitedCount,
        Result.NotSupportedCount);
}

FString BuildWorkspaceBuildStatusText(const FString& WorkspaceLabel, const FString& BuildState, const FString& OutputPath)
{
    FString StatePhrase = TEXT("complete");

    if (BuildState.Equals(TEXT("Built with Limitations"), ESearchCase::IgnoreCase))
    {
        StatePhrase = TEXT("complete with limitations");
    }
    else if (BuildState.Equals(TEXT("Blocked"), ESearchCase::IgnoreCase))
    {
        StatePhrase = TEXT("blocked");
    }
    else if (BuildState.Equals(TEXT("Built"), ESearchCase::IgnoreCase))
    {
        StatePhrase = TEXT("complete");
    }
    else if (!BuildState.IsEmpty())
    {
        StatePhrase = BuildState;
    }

    const FString OutputSuffix = OutputPath.IsEmpty()
        ? FString(TEXT("."))
        : FString::Printf(TEXT(": %s"), *OutputPath);

    return FString::Printf(
        TEXT("%s build %s%s"),
        WorkspaceLabel.IsEmpty() ? TEXT("Workspace") : *WorkspaceLabel,
        *StatePhrase,
        *OutputSuffix);
}

bool HasLevelDesignWITPair(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    return Snapshot.bHasObserverActor && Snapshot.bHasTargetActor;
}

FString BuildLevelDesignPairSummary(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    if (!Snapshot.bHasObserverActor && !Snapshot.bHasTargetActor)
    {
        return TEXT("No observer-target context is assigned yet.");
    }

    if (!Snapshot.bHasObserverActor)
    {
        return FString::Printf(
            TEXT("Target %s is assigned, but no observer is available for movement-space meaning."),
            Snapshot.TargetActorLabel.IsEmpty() ? TEXT("(unnamed target)") : *Snapshot.TargetActorLabel);
    }

    if (!Snapshot.bHasTargetActor)
    {
        return FString::Printf(
            TEXT("Observer %s is assigned, but no target is available for cover, obstacle, and reachability meaning."),
            Snapshot.ObserverActorLabel.IsEmpty() ? TEXT("(unnamed observer)") : *Snapshot.ObserverActorLabel);
    }

    const FString PairSource = Snapshot.PairSourceLabel.IsEmpty() ? FString(TEXT("workspace context")) : Snapshot.PairSourceLabel;
    const FString PairLabel = FString::Printf(
        TEXT("%s to %s from %s."),
        Snapshot.ObserverActorLabel.IsEmpty() ? TEXT("(unnamed observer)") : *Snapshot.ObserverActorLabel,
        Snapshot.TargetActorLabel.IsEmpty() ? TEXT("(unnamed target)") : *Snapshot.TargetActorLabel,
        *PairSource);

    return Snapshot.bTargetFallsBackToObserver
        ? FString::Printf(TEXT("%s Observer fallback is active, so assign a separate target for standard semantic readings."), *PairLabel)
        : PairLabel;
}

FString GetLevelDesignCoverMeaningStatus(const FWanaEnvironmentReadinessSnapshot& Snapshot, int32 SelectedActorCount)
{
    if (HasLevelDesignWITPair(Snapshot))
    {
        return TEXT("Ready");
    }

    return SelectedActorCount > 0 ? TEXT("Limited") : TEXT("Missing");
}

FString GetLevelDesignCoverMeaningDetail(const FWanaEnvironmentReadinessSnapshot& Snapshot, int32 SelectedActorCount)
{
    if (!HasLevelDesignWITPair(Snapshot))
    {
        return SelectedActorCount > 0
            ? FString::Printf(TEXT("%d selected actor(s) are captured as manual cover/context candidates; assign an observer and target to score cover pressure."), SelectedActorCount)
            : TEXT("Assign an observer and target, or select scene actors, before WIT can infer cover demand.");
    }

    if (Snapshot.MovementReadiness.bMovementSpaceRestricted
        || Snapshot.MovementReadiness.bObstaclePressureDetected
        || Snapshot.MovementReadiness.bPathToTargetObstructed)
    {
        return TEXT("Nearby obstruction or restricted movement space suggests potential cover/constraint candidates for the current pair.");
    }

    return TEXT("No immediate cover pressure is detected for the current pair; WIT reads the space as open.");
}

FString GetLevelDesignObstacleMeaningStatus(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    return HasLevelDesignWITPair(Snapshot) ? TEXT("Ready") : TEXT("Limited");
}

FString GetLevelDesignObstacleMeaningDetail(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    if (!HasLevelDesignWITPair(Snapshot))
    {
        return TEXT("Obstacle meaning needs an observer-target context before WIT can score pressure around the path.");
    }

    const int32 PressurePercent = FMath::RoundToInt(FMath::Clamp(Snapshot.MovementReadiness.ObstaclePressure, 0.0f, 1.0f) * 100.0f);

    return FString::Printf(
        TEXT("%s obstacle pressure (%d%% probe pressure). Path to target: %s."),
        *UIFmt::GetObstaclePressureSummaryLabel(Snapshot.MovementReadiness),
        PressurePercent,
        Snapshot.MovementReadiness.bPathToTargetObstructed ? TEXT("obstructed") : TEXT("clear"));
}

FString GetLevelDesignMovementSpaceStatus(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    return HasLevelDesignWITPair(Snapshot) ? TEXT("Ready") : TEXT("Limited");
}

FString GetLevelDesignMovementSpaceDetail(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    if (!HasLevelDesignWITPair(Snapshot))
    {
        return TEXT("Movement-space meaning needs an observer-target context before WIT can evaluate distance, restriction, and reachability.");
    }

    const FString DistanceLabel = Snapshot.MovementReadiness.DistanceToTarget > 0.0f
        ? FString::Printf(TEXT("%.0f cm"), Snapshot.MovementReadiness.DistanceToTarget)
        : FString(TEXT("no measurable distance"));

    return FString::Printf(
        TEXT("%s movement space at %s. %s"),
        *UIFmt::GetMovementSpaceSummaryLabel(Snapshot.MovementReadiness),
        *DistanceLabel,
        Snapshot.MovementReadiness.Detail.IsEmpty() ? TEXT("No extra movement detail is available.") : *Snapshot.MovementReadiness.Detail);
}

FString GetLevelDesignBoundaryMeaningStatus(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    return HasLevelDesignWITPair(Snapshot) && Snapshot.MovementReadiness.bHasMovementContext ? TEXT("Ready") : TEXT("Limited");
}

FString GetLevelDesignBoundaryMeaningDetail(const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    if (!HasLevelDesignWITPair(Snapshot))
    {
        return TEXT("Boundary and navigation meaning need a WIT observer-target pair.");
    }

    return FString::Printf(
        TEXT("%s Navigation context: %s. Reachability: %s."),
        *UIFmt::GetEnvironmentShapingSummaryLabel(Snapshot.MovementReadiness),
        *UIFmt::GetNavigationContextSummaryLabel(Snapshot),
        *UIFmt::GetReachabilitySummaryLabel(Snapshot));
}

FString BuildAIBehaviorMovementCompatibilityDetail(const FWanaMovementReadiness& MovementReadiness)
{
    return FString::Printf(
        TEXT("%s; %s"),
        *UIFmt::GetMovementReadinessStatusSummaryLabel(MovementReadiness),
        *UIFmt::GetEnvironmentShapingSummaryLabel(MovementReadiness));
}

FString BuildAIBehaviorFallbackReasonDetail(EWAYBehaviorExecutionMode ExecutionMode, const FWanaMovementReadiness& MovementReadiness)
{
    switch (ExecutionMode)
    {
    case EWAYBehaviorExecutionMode::MovementAllowed:
        return TEXT("No fallback was needed; WIT allowed a safe movement-aware behavior.");

    case EWAYBehaviorExecutionMode::FallbackActive:
        return MovementReadiness.Detail.IsEmpty()
            ? TEXT("Full locomotion was limited, so WanaWorks used a small safe fallback movement.")
            : FString::Printf(TEXT("Full locomotion was limited, so WanaWorks used a small safe fallback movement. %s"), *MovementReadiness.Detail);

    case EWAYBehaviorExecutionMode::FacingOnly:
        return MovementReadiness.Detail.IsEmpty()
            ? TEXT("Movement was unsafe, unnecessary, or unsupported, so WanaWorks used facing/attention only.")
            : FString::Printf(TEXT("Movement was unsafe, unnecessary, or unsupported, so WanaWorks used facing/attention only. %s"), *MovementReadiness.Detail);

    case EWAYBehaviorExecutionMode::Unknown:
    default:
        return MovementReadiness.Detail.IsEmpty()
            ? TEXT("Movement compatibility was unknown.")
            : MovementReadiness.Detail;
    }
}

FString BuildAIBehaviorRecommendedNextStepDetail(EWAYBehaviorExecutionMode ExecutionMode, const FWanaMovementReadiness& MovementReadiness)
{
    if (ExecutionMode == EWAYBehaviorExecutionMode::MovementAllowed)
    {
        return TEXT("Visually verify the behavior distance and repeat Test against a different relationship target.");
    }

    if (MovementReadiness.bAnimationDrivenLocomotionDetected)
    {
        return TEXT("Keep the facing/attention fallback until animation-driven locomotion is deliberately wired for movement.");
    }

    if (MovementReadiness.ReadinessLevel == EWanaMovementReadinessLevel::Blocked)
    {
        return TEXT("Assign a reachable target or improve WIT/navigation context before expecting movement.");
    }

    if (ExecutionMode == EWAYBehaviorExecutionMode::FallbackActive)
    {
        return TEXT("Check the fallback step in the viewport, then add a locomotion-safe hook only if stronger movement is needed.");
    }

    return TEXT("Use Enhance/Test again after selecting clearer scene space or a better observer-target pair.");
}

FString BuildAIPhysicalBehaviorCommitmentDetail(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasPhysicalStateComponent)
    {
        return TEXT("Physical-state influence is limited until UWanaPhysicalStateComponent is attached.");
    }

    return FString::Printf(
        TEXT("Movement commit: %s. Attack commit: %s. Recovery: %s. Instability: %.2f."),
        Snapshot.bPhysicalCanCommitToMovement ? TEXT("ready") : TEXT("limited"),
        Snapshot.bPhysicalCanCommitToAttack ? TEXT("ready") : TEXT("limited"),
        Snapshot.bPhysicalNeedsRecovery ? TEXT("needed") : TEXT("not needed"),
        Snapshot.PhysicalInstabilityAlpha);
}

FString ExtractResponseOutputValue(const FWanaCommandResponse& Response, const FString& Prefix)
{
    const FString MatchPrefix = Prefix.EndsWith(TEXT(":")) ? Prefix : FString::Printf(TEXT("%s:"), *Prefix);

    for (const FString& Line : Response.OutputLines)
    {
        if (Line.StartsWith(MatchPrefix))
        {
            FString Value = Line.RightChop(MatchPrefix.Len()).TrimStartAndEnd();
            return Value;
        }
    }

    return FString();
}

const AActor* ResolveAnalysisActor(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot, const UClass* SubjectClass)
{
    if (Snapshot.SelectedActor.IsValid())
    {
        return Snapshot.SelectedActor.Get();
    }

    return SubjectClass ? Cast<AActor>(SubjectClass->GetDefaultObject()) : nullptr;
}

bool ComponentClassNameContainsAny(const AActor* Actor, const TArray<FString>& Tokens)
{
    if (!Actor)
    {
        return false;
    }

    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);

    for (const UActorComponent* Component : Components)
    {
        const UClass* ComponentClass = Component ? Component->GetClass() : nullptr;
        const FString ComponentClassName = ComponentClass ? ComponentClass->GetName() : FString();

        for (const FString& Token : Tokens)
        {
            if (!Token.IsEmpty() && ComponentClassName.Contains(Token, ESearchCase::IgnoreCase))
            {
                return true;
            }
        }
    }

    return false;
}

bool GetSkeletalMeshAndSkeletonLabels(const AActor* Actor, FString& OutMeshLabel, FString& OutSkeletonLabel)
{
    OutMeshLabel.Reset();
    OutSkeletonLabel.Reset();

    const USkeletalMeshComponent* MeshComponent = Actor ? Actor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;

    if (!MeshComponent)
    {
        return false;
    }

    const USkeletalMesh* SkeletalMesh = MeshComponent->GetSkeletalMeshAsset();

    if (!SkeletalMesh)
    {
        OutMeshLabel = TEXT("Skeletal mesh component present, mesh asset not assigned");
        OutSkeletonLabel = TEXT("(no skeleton asset)");
        return true;
    }

    OutMeshLabel = SkeletalMesh->GetName();
    const USkeleton* Skeleton = SkeletalMesh->GetSkeleton();
    OutSkeletonLabel = Skeleton ? Skeleton->GetName() : TEXT("(no skeleton asset)");
    return true;
}

bool HasPawnMovementComponent(const AActor* Actor)
{
    const APawn* Pawn = Cast<APawn>(Actor);
    return Pawn && Pawn->GetMovementComponent() != nullptr;
}

const UClass* GetAIControllerClassForAnalysis(const AActor* Actor)
{
    const APawn* Pawn = Cast<APawn>(Actor);
    return Pawn ? Pawn->AIControllerClass : nullptr;
}

bool TryFindLinkedObjectLabelByTokens(const UObject* OwnerObject, const TArray<FString>& PropertyTokens, const TArray<FString>& ClassTokens, FString& OutLabel, bool& bOutPropertyFound)
{
    OutLabel.Reset();
    bOutPropertyFound = false;

    if (!OwnerObject)
    {
        return false;
    }

    for (TFieldIterator<FProperty> PropertyIt(OwnerObject->GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        const FProperty* Property = *PropertyIt;
        const FString PropertyName = Property ? Property->GetName() : FString();
        bool bTokenMatched = false;

        for (const FString& Token : PropertyTokens)
        {
            if (!Token.IsEmpty() && PropertyName.Contains(Token, ESearchCase::IgnoreCase))
            {
                bTokenMatched = true;
                break;
            }
        }

        if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
        {
            const UClass* PropertyClass = ObjectProperty->PropertyClass;
            const FString PropertyClassName = PropertyClass ? PropertyClass->GetName() : FString();

            for (const FString& Token : ClassTokens)
            {
                if (!Token.IsEmpty() && PropertyClassName.Contains(Token, ESearchCase::IgnoreCase))
                {
                    bTokenMatched = true;
                    break;
                }
            }

            if (!bTokenMatched)
            {
                continue;
            }

            bOutPropertyFound = true;
            const UObject* LinkedObject = ObjectProperty->GetObjectPropertyValue_InContainer(OwnerObject);

            if (LinkedObject)
            {
                OutLabel = LinkedObject->GetName();
                return true;
            }
        }
    }

    return false;
}

int32 GetEditorSelectedActorCount()
{
    USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;
    return SelectedActors ? SelectedActors->Num() : 0;
}

FString GetEditorWorldLabel()
{
    UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

    if (!EditorWorld)
    {
        return FString();
    }

    return EditorWorld->GetMapName().IsEmpty() ? EditorWorld->GetName() : EditorWorld->GetMapName();
}

FString GetWorkspaceDisplayLabel(const FString& WorkspaceLabel)
{
    return WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase)
        ? FString(TEXT("Character Intelligence"))
        : WorkspaceLabel;
}
}

const FName FWanaWorksUIModule::WanaWorksTabName(TEXT("WanaWorksTab"));

IMPLEMENT_MODULE(FWanaWorksUIModule, WanaWorksUI)

void FWanaWorksUIModule::StartupModule()
{
    WanaWorksUIStyle::Register();

    StatusMessage = TEXT("Status: Wana Works Initialized");
    SelectedWorkspaceLabel = TEXT("AI");
    SelectedPreviewStageViewLabel = TEXT("Overview");
    CommandText.Reset();
    LogOutput = TEXT("Wana Works Initialized");
    IdentityFactionTagText.Reset();
    SelectedWorkflowPresetLabel = TEXT("Full WanaAI Starter");
    SelectedEnhancementPresetLabel = TEXT("Identity Only");
    SelectedEnhancementWorkflowLabel = TEXT("Create Working Copy");
    SelectedCharacterPawnAssetLabel.Reset();
    SelectedAIPawnAssetLabel.Reset();
    SelectedCharacterIntelligenceIdentityRoleLabel = TEXT("Neutral");
    SelectedCharacterIntelligenceRelationshipLabel = TEXT("Unknown");
    SelectedCharacterIntelligenceTargetLabel = TEXT("No Target");
    bWorkspaceAnalysisInitialized = false;
    LastAnalysisWorkspaceLabel.Reset();
    LastAnalysisStatusSummary = TEXT("Analyze has not run yet.");
    LastAnalysisPrimarySummary.Reset();
    LastAnalysisAnimationSummary.Reset();
    LastAnalysisPhysicalSummary.Reset();
    LastAnalysisBehaviorSummary.Reset();
    LastAnalysisWITSummary.Reset();
    LastAnalysisSuggestedSummary.Reset();
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
        MakeShared<FString>(TEXT("Create Working Copy")),
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
    CharacterIntelligenceIdentityRoleOptions =
    {
        MakeShared<FString>(TEXT("Neutral")),
        MakeShared<FString>(TEXT("Hostile")),
        MakeShared<FString>(TEXT("Guard")),
        MakeShared<FString>(TEXT("Companion")),
        MakeShared<FString>(TEXT("Civilian")),
        MakeShared<FString>(TEXT("Patrol")),
        MakeShared<FString>(TEXT("Observer"))
    };
    CharacterIntelligenceRelationshipOptions =
    {
        MakeShared<FString>(TEXT("Unknown")),
        MakeShared<FString>(TEXT("Neutral")),
        MakeShared<FString>(TEXT("Friend")),
        MakeShared<FString>(TEXT("Ally")),
        MakeShared<FString>(TEXT("Suspicious")),
        MakeShared<FString>(TEXT("Enemy")),
        MakeShared<FString>(TEXT("Target"))
    };
    CharacterIntelligenceTargetOptions =
    {
        MakeShared<FString>(TEXT("No Target")),
        MakeShared<FString>(TEXT("Current Editor Selection")),
        MakeShared<FString>(TEXT("Player Character"))
    };
    RefreshProjectAssetPickerOptions();

    LoadSavedWorkflowPresets();
    LoadSavedSubjectProgress();

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        WanaWorksTabName,
        FOnSpawnTab::CreateRaw(this, &FWanaWorksUIModule::SpawnWanaWorksTab))
        .SetDisplayName(LOCTEXT("WanaWorksTabTitle", "WanaWorks Studio"))
        .SetTooltipText(LOCTEXT("WanaWorksTabTooltip", "Open WanaWorks Studio."))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
        .SetIcon(FSlateIcon(WanaWorksUIStyle::GetStyleSetName(), WanaWorksUIStyle::GetLauncherIconName()))
        .SetMenuType(ETabSpawnerMenuType::Enabled);

    if (UToolMenus::IsToolMenuUIEnabled())
    {
        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FWanaWorksUIModule::RegisterEditorLauncherMenus));
    }

    SelectionChangedHandle = USelection::SelectionChangedEvent.AddRaw(this, &FWanaWorksUIModule::HandleEditorSelectionChanged);
    SelectObjectHandle = USelection::SelectObjectEvent.AddRaw(this, &FWanaWorksUIModule::HandleEditorSelectionChanged);

    FGlobalTabmanager::Get()->TryInvokeTab(WanaWorksTabName);
}

void FWanaWorksUIModule::ShutdownModule()
{
    if (UToolMenus::IsToolMenuUIEnabled())
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
    }

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

    WanaWorksUIStyle::Unregister();
}

void FWanaWorksUIModule::RegisterEditorLauncherMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    const FUIAction OpenStudioAction(FExecuteAction::CreateRaw(this, &FWanaWorksUIModule::OpenWanaWorksStudioTab));
    const FSlateIcon LauncherIcon(WanaWorksUIStyle::GetStyleSetName(), WanaWorksUIStyle::GetLauncherIconName());

    if (UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar")))
    {
        FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection(TEXT("WanaWorks"));
        ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
            FName(TEXT("WanaWorksStudioToolbarLauncher")),
            OpenStudioAction,
            LOCTEXT("WanaWorksStudioToolbarLauncherLabel", "WanaWorks"),
            LOCTEXT("WanaWorksStudioToolbarLauncherTooltip", "Open or focus WanaWorks Studio."),
            LauncherIcon));
    }

    if (UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools")))
    {
        FToolMenuSection& ToolsSection = ToolsMenu->FindOrAddSection(TEXT("WanaWorks"));
        ToolsSection.AddMenuEntry(
            FName(TEXT("WanaWorksStudioToolsLauncher")),
            LOCTEXT("WanaWorksStudioToolsLauncherLabel", "WanaWorks Studio"),
            LOCTEXT("WanaWorksStudioToolsLauncherTooltip", "Open or focus WanaWorks Studio."),
            LauncherIcon,
            OpenStudioAction);
    }

    if (UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window")))
    {
        FToolMenuSection& WindowSection = WindowMenu->FindOrAddSection(TEXT("WanaWorks"));
        WindowSection.AddMenuEntry(
            FName(TEXT("WanaWorksStudioWindowLauncher")),
            LOCTEXT("WanaWorksStudioWindowLauncherLabel", "WanaWorks Studio"),
            LOCTEXT("WanaWorksStudioWindowLauncherTooltip", "Open or focus WanaWorks Studio."),
            LauncherIcon,
            OpenStudioAction);
    }
}

void FWanaWorksUIModule::OpenWanaWorksStudioTab()
{
    FGlobalTabmanager::Get()->TryInvokeTab(WanaWorksTabName);
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
    bWorkspaceAnalysisInitialized = false;
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::HandleCommandTextChanged(const FText& NewText)
{
    CommandText = NewText.ToString();
}

void FWanaWorksUIModule::HandleWorkspaceSelected(const FString& WorkspaceLabel)
{
    if (!WorkspaceLabel.IsEmpty())
    {
        if (!SelectedWorkspaceLabel.Equals(WorkspaceLabel, ESearchCase::IgnoreCase))
        {
            bWorkspaceAnalysisInitialized = false;
        }

        SelectedWorkspaceLabel = WorkspaceLabel;
        RefreshReactiveUI(true);
    }
}

void FWanaWorksUIModule::HandlePreviewStageViewSelected(const FString& PreviewViewLabel)
{
    if (!PreviewViewLabel.IsEmpty())
    {
        SelectedPreviewStageViewLabel = PreviewViewLabel;
        RefreshReactiveUI(true);
    }
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
        SyncSelectedWorkflowPresetIntoControls();
        PersistSavedWorkflowPresets();
    }

    RefreshReactiveUI(true);
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

void FWanaWorksUIModule::HandleCharacterIntelligenceIdentityRoleOptionSelected(TSharedPtr<FString> SelectedOption)
{
    if (!SelectedOption.IsValid())
    {
        return;
    }

    SelectedCharacterIntelligenceIdentityRoleLabel = *SelectedOption;
    SelectedIdentitySeedState = MapCharacterIntelligenceIdentityRoleToSeedState(SelectedCharacterIntelligenceIdentityRoleLabel);
    IdentityFactionTagText = GetCharacterIntelligenceRoleFactionTag(SelectedCharacterIntelligenceIdentityRoleLabel);
    bWorkspaceAnalysisInitialized = false;

    AActor* ObserverActor = nullptr;
    FString ObserverSourceLabel;
    ResolveCharacterIntelligenceObserverActor(ObserverActor, ObserverSourceLabel);

    AActor* TargetActor = nullptr;
    FString TargetSourceLabel;
    ResolveCharacterIntelligenceTargetActor(TargetActor, TargetSourceLabel);

    TArray<FString> ControlNotes;
    ApplyCharacterIntelligenceControlState(ObserverActor, TargetActor, &ControlNotes);

    const FString RecommendedBehavior = GetCharacterIntelligenceRecommendationLabel(
        SelectedCharacterIntelligenceIdentityRoleLabel,
        SelectedCharacterIntelligenceRelationshipLabel,
        TargetActor != nullptr);

    FWanaCommandResponse Response;
    Response.bSucceeded = ObserverActor != nullptr;
    Response.StatusMessage = ObserverActor
        ? FString::Printf(TEXT("Status: AI identity set to %s."), *SelectedCharacterIntelligenceIdentityRoleLabel)
        : FString::Printf(TEXT("Status: AI identity set to %s. No compatible AI subject selected."), *SelectedCharacterIntelligenceIdentityRoleLabel);
    Response.OutputLines.Add(FString::Printf(TEXT("AI Subject: %s"), ObserverActor ? *ObserverActor->GetActorNameOrLabel() : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("AI Identity: %s"), *SelectedCharacterIntelligenceIdentityRoleLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Default Seed: %s"), *UIFmt::GetRelationshipStateLabel(SelectedIdentitySeedState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Recommended Behavior: %s"), *RecommendedBehavior));

    for (const FString& ControlNote : ControlNotes)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Readiness Notes: %s"), *ControlNote));
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::HandleCharacterIntelligenceRelationshipOptionSelected(TSharedPtr<FString> SelectedOption)
{
    if (!SelectedOption.IsValid())
    {
        return;
    }

    SelectedCharacterIntelligenceRelationshipLabel = *SelectedOption;
    SelectedRelationshipState = MapCharacterIntelligenceRelationshipLabelToWAYState(SelectedCharacterIntelligenceRelationshipLabel);
    bWorkspaceAnalysisInitialized = false;

    AActor* ObserverActor = nullptr;
    FString ObserverSourceLabel;
    ResolveCharacterIntelligenceObserverActor(ObserverActor, ObserverSourceLabel);

    AActor* TargetActor = nullptr;
    FString TargetSourceLabel;
    ResolveCharacterIntelligenceTargetActor(TargetActor, TargetSourceLabel);

    TArray<FString> ControlNotes;
    ApplyCharacterIntelligenceControlState(ObserverActor, TargetActor, &ControlNotes);

    const FString RecommendedBehavior = GetCharacterIntelligenceRecommendationLabel(
        SelectedCharacterIntelligenceIdentityRoleLabel,
        SelectedCharacterIntelligenceRelationshipLabel,
        TargetActor != nullptr);

    FWanaCommandResponse Response;
    Response.bSucceeded = ObserverActor != nullptr && TargetActor != nullptr;

    if (!ObserverActor)
    {
        Response.StatusMessage = FString::Printf(TEXT("Status: Relationship set to %s. No compatible AI subject selected."), *SelectedCharacterIntelligenceRelationshipLabel);
    }
    else if (!TargetActor)
    {
        Response.StatusMessage = FString::Printf(TEXT("Status: Relationship set to %s. No relationship target selected."), *SelectedCharacterIntelligenceRelationshipLabel);
    }
    else
    {
        Response.StatusMessage = FString::Printf(TEXT("Status: Relationship set to %s. Recommended behavior: %s."), *SelectedCharacterIntelligenceRelationshipLabel, *RecommendedBehavior);
    }

    Response.OutputLines.Add(FString::Printf(TEXT("AI Subject: %s"), ObserverActor ? *ObserverActor->GetActorNameOrLabel() : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Relationship Target: %s"), TargetActor ? *TargetActor->GetActorNameOrLabel() : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("WAY Relationship: %s"), *SelectedCharacterIntelligenceRelationshipLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("WAY Runtime State: %s"), *UIFmt::GetRelationshipStateLabel(SelectedRelationshipState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Recommended Behavior: %s"), *RecommendedBehavior));

    for (const FString& ControlNote : ControlNotes)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Readiness Notes: %s"), *ControlNote));
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::HandleCharacterIntelligenceTargetOptionSelected(TSharedPtr<FString> SelectedOption)
{
    if (!SelectedOption.IsValid())
    {
        return;
    }

    SelectedCharacterIntelligenceTargetLabel = *SelectedOption;
    bWorkspaceAnalysisInitialized = false;

    AActor* ObserverActor = nullptr;
    FString ObserverSourceLabel;
    ResolveCharacterIntelligenceObserverActor(ObserverActor, ObserverSourceLabel);

    AActor* TargetActor = nullptr;
    FString TargetSourceLabel;

    if (SelectedCharacterIntelligenceTargetLabel.Equals(TEXT("No Target"), ESearchCase::IgnoreCase))
    {
        SandboxTargetActor.Reset();
        TargetSourceLabel = TEXT("No Target");
    }
    else if (SelectedCharacterIntelligenceTargetLabel.Equals(TEXT("Current Editor Selection"), ESearchCase::IgnoreCase))
    {
        TargetActor = GetFirstEditorSelectedActor();
        TargetSourceLabel = TEXT("Current Editor Selection");
    }
    else if (SelectedCharacterIntelligenceTargetLabel.Equals(TEXT("Player Character"), ESearchCase::IgnoreCase))
    {
        TargetActor = FindEditorPlayerCharacterCandidate();
        TargetSourceLabel = TEXT("Player Character");
    }

    if (TargetActor)
    {
        SandboxTargetActor = TargetActor;
    }
    else if (!SelectedCharacterIntelligenceTargetLabel.Equals(TEXT("No Target"), ESearchCase::IgnoreCase))
    {
        SandboxTargetActor.Reset();
    }

    TArray<FString> ControlNotes;
    ApplyCharacterIntelligenceControlState(ObserverActor, SandboxTargetActor.Get(), &ControlNotes);

    const FString RecommendedBehavior = GetCharacterIntelligenceRecommendationLabel(
        SelectedCharacterIntelligenceIdentityRoleLabel,
        SelectedCharacterIntelligenceRelationshipLabel,
        SandboxTargetActor.IsValid());

    FWanaCommandResponse Response;
    Response.bSucceeded = SelectedCharacterIntelligenceTargetLabel.Equals(TEXT("No Target"), ESearchCase::IgnoreCase)
        || SandboxTargetActor.IsValid();

    if (SelectedCharacterIntelligenceTargetLabel.Equals(TEXT("No Target"), ESearchCase::IgnoreCase))
    {
        Response.StatusMessage = TEXT("Status: Relationship target cleared.");
    }
    else if (!SandboxTargetActor.IsValid())
    {
        Response.StatusMessage = FString::Printf(TEXT("Status: No valid %s target found."), *SelectedCharacterIntelligenceTargetLabel);
    }
    else
    {
        Response.StatusMessage = FString::Printf(TEXT("Status: Target set to %s."), *SandboxTargetActor->GetActorNameOrLabel());
    }

    Response.OutputLines.Add(FString::Printf(TEXT("AI Subject: %s"), ObserverActor ? *ObserverActor->GetActorNameOrLabel() : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Relationship Target: %s"), SandboxTargetActor.IsValid() ? *SandboxTargetActor->GetActorNameOrLabel() : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Target Source: %s"), TargetSourceLabel.IsEmpty() ? *SelectedCharacterIntelligenceTargetLabel : *TargetSourceLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("WAY Relationship: %s"), *SelectedCharacterIntelligenceRelationshipLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Recommended Behavior: %s"), *RecommendedBehavior));

    for (const FString& ControlNote : ControlNotes)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Readiness Notes: %s"), *ControlNote));
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
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
    LastEnhancementAnimationResult =
        EffectiveSnapshot->AnimationAutomaticIntegrationStatus != EWAYAutomaticAnimationIntegrationStatus::NotSupported
            ? UIFmt::GetAutomaticAnimationIntegrationStatusLabel(EffectiveSnapshot->AnimationAutomaticIntegrationStatus)
            : UIFmt::GetAnimationReadinessResultLabel(EffectiveSnapshot->bHasSkeletalMeshComponent, EffectiveSnapshot->bHasAnimBlueprint);

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
        SelectedWorkspaceLabel = TEXT("Character Building");
    }

    bWorkspaceAnalysisInitialized = false;
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
        SelectedWorkspaceLabel = TEXT("AI");
    }

    bWorkspaceAnalysisInitialized = false;
    RefreshReactiveUI(true);
}

bool FWanaWorksUIModule::SyncSelectedWorkflowPresetIntoControls(FString* OutPresetLabel, FString* OutPresetNotes)
{
    if (OutPresetLabel)
    {
        OutPresetLabel->Reset();
    }

    if (OutPresetNotes)
    {
        OutPresetNotes->Reset();
    }

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

    if (OutPresetLabel)
    {
        *OutPresetLabel = Config.PresetLabel;
    }

    if (!Config.bValid)
    {
        if (OutPresetNotes)
        {
            *OutPresetNotes = TEXT("Saved Custom Preset has not been captured yet, so WanaWorks kept the current workflow controls.");
        }

        return false;
    }

    SelectedEnhancementWorkflowLabel = Config.WorkflowLabel;
    SelectedEnhancementPresetLabel = Config.EnhancementPresetLabel;
    SelectedIdentitySeedState = Config.IdentitySeedState;
    SelectedRelationshipState = Config.RelationshipState;

    if (Config.bIsCustom)
    {
        IdentityFactionTagText = Config.FactionTagText;
    }

    if (OutPresetNotes)
    {
        *OutPresetNotes = FString::Printf(
            TEXT("WanaWorks staged the %s profile automatically before running the core workflow."),
            *Config.PresetLabel);
    }

    return true;
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
        const bool bIsCharacterFacingPawn = IsCharacterFacingPawnClass(BlueprintAsset->GeneratedClass.Get(), DefaultPawn);
        const FProjectAssetPickerEntry Entry
        {
            MakeProjectAssetDisplayLabel(AssetData),
            AssetData.ToSoftObjectPath().ToString()
        };

        if (bIsAIReadyPawn)
        {
            AIPawnEntries.Add(Entry);
        }

        if (bIsCharacterFacingPawn || !bIsAIReadyPawn)
        {
            // AI-ready Character Blueprints still belong in Character Building as authorable character assets.
            CharacterPawnEntries.Add(Entry);
        }
    }

    AddSortedAssetPickerOptions(CharacterPawnEntries, CharacterPawnAssetOptions, CharacterPawnAssetPathByLabel);
    AddSortedAssetPickerOptions(AIPawnEntries, AIPawnAssetOptions, AIPawnAssetPathByLabel);
}

UObject* FWanaWorksUIModule::LoadSelectedSubjectAssetObject() const
{
    return LoadSelectedSubjectAssetObjectForWorkspace(GetSelectedWorkspaceLabel());
}

UObject* FWanaWorksUIModule::LoadSelectedSubjectAssetObjectForWorkspace(const FString& WorkspaceLabel) const
{
    const FString SubjectAssetPath = GetSelectedSubjectAssetPathForWorkspace(WorkspaceLabel);

    if (SubjectAssetPath.IsEmpty())
    {
        return nullptr;
    }

    const FSoftObjectPath SubjectSoftPath(SubjectAssetPath);

    if (UObject* ResolvedObject = SubjectSoftPath.ResolveObject())
    {
        return ResolvedObject;
    }

    return SubjectSoftPath.TryLoad();
}

UClass* FWanaWorksUIModule::LoadSelectedSubjectActorClass() const
{
    return LoadSelectedSubjectActorClassForWorkspace(GetSelectedWorkspaceLabel());
}

UClass* FWanaWorksUIModule::LoadSelectedSubjectActorClassForWorkspace(const FString& WorkspaceLabel) const
{
    UObject* SubjectAssetObject = LoadSelectedSubjectAssetObjectForWorkspace(WorkspaceLabel);
    UBlueprint* BlueprintAsset = Cast<UBlueprint>(SubjectAssetObject);

    if (BlueprintAsset)
    {
        return BlueprintAsset->GeneratedClass.Get();
    }

    return Cast<UClass>(SubjectAssetObject);
}

FString FWanaWorksUIModule::GetSelectedSubjectAssetPath() const
{
    return GetSelectedSubjectAssetPathForWorkspace(GetSelectedWorkspaceLabel());
}

FString FWanaWorksUIModule::GetSelectedSubjectAssetPathForWorkspace(const FString& WorkspaceLabel) const
{
    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return FString();
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return !SelectedCharacterPawnAssetLabel.IsEmpty()
            ? GetMappedAssetPath(CharacterPawnAssetPathByLabel, SelectedCharacterPawnAssetLabel)
            : FString();
    }

    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase)
        || WorkspaceLabel.Equals(TEXT("Character Intelligence"), ESearchCase::IgnoreCase))
    {
        return !SelectedAIPawnAssetLabel.IsEmpty()
            ? GetMappedAssetPath(AIPawnAssetPathByLabel, SelectedAIPawnAssetLabel)
            : FString();
    }

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

UObject* FWanaWorksUIModule::GetSandboxPreviewObject() const
{
    const FString WorkspaceLabel = GetSelectedWorkspaceLabel();

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return nullptr;
    }

    FString PreviewModeLabel;
    AActor* PreviewActor = nullptr;

    if (ResolvePreferredSandboxPreviewActor(PreviewActor, PreviewModeLabel) && PreviewActor)
    {
        return PreviewActor;
    }

    if (UObject* PickedSubjectAsset = LoadSelectedSubjectAssetObject())
    {
        return PickedSubjectAsset;
    }

    return nullptr;
}

bool FWanaWorksUIModule::IsActorCompatibleWithWorkspacePreview(const AActor* Actor, const FString& WorkspaceLabel) const
{
    if (!IsValid(Actor) || WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return false;
    }

    const UClass* PickedSubjectClass = LoadSelectedSubjectActorClassForWorkspace(WorkspaceLabel);

    if (PickedSubjectClass)
    {
        return Actor->GetClass()->IsChildOf(PickedSubjectClass);
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return Actor->IsA<ACharacter>() || Actor->FindComponentByClass<USkeletalMeshComponent>() != nullptr;
    }

    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase)
        || WorkspaceLabel.Equals(TEXT("Character Intelligence"), ESearchCase::IgnoreCase))
    {
        const APawn* Pawn = Cast<APawn>(Actor);
        return Pawn && (Pawn->AIControllerClass != nullptr || Pawn->AutoPossessAI != EAutoPossessAI::Disabled);
    }

    return true;
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
    const FString WorkspaceLabel = GetSelectedWorkspaceLabel();

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        OutSnapshot = FWanaSelectedCharacterEnhancementSnapshot();
        return false;
    }

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
            && IsActorCompatibleWithWorkspacePreview(SelectedActorSnapshot.SelectedActor.Get(), WorkspaceLabel)
            && SelectedActorSnapshot.SelectedActor->GetClass()->IsChildOf(PickedSubjectClass))
        {
            OutSnapshot = SelectedActorSnapshot;
            return true;
        }

        OutSnapshot = PickedSnapshot;
        return true;
    }

    if (bHasSelectedActorSnapshot
        && SelectedActorSnapshot.SelectedActor.IsValid()
        && IsActorCompatibleWithWorkspacePreview(SelectedActorSnapshot.SelectedActor.Get(), WorkspaceLabel))
    {
        OutSnapshot = SelectedActorSnapshot;
        return true;
    }

    OutSnapshot = FWanaSelectedCharacterEnhancementSnapshot();
    return false;
}

bool FWanaWorksUIModule::ResolvePreferredSandboxPreviewActor(AActor*& OutActor, FString& OutPreviewModeLabel) const
{
    OutActor = nullptr;
    OutPreviewModeLabel = TEXT("Picked subject preview");
    const FString WorkspaceLabel = GetSelectedWorkspaceLabel();

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return false;
    }

    if (LastEnhancementResultsActor.IsValid()
        && IsActorCompatibleWithWorkspacePreview(LastEnhancementResultsActor.Get(), WorkspaceLabel))
    {
        OutActor = LastEnhancementResultsActor.Get();
        const FString ActorLabel = OutActor->GetActorNameOrLabel();
        OutPreviewModeLabel = ActorLabel.StartsWith(TEXT("WW_Final_"))
            ? TEXT("Finalized build preview")
            : TEXT("Working/generated subject preview");
        return true;
    }

    if (SandboxObserverActor.IsValid()
        && IsActorCompatibleWithWorkspacePreview(SandboxObserverActor.Get(), WorkspaceLabel))
    {
        OutActor = SandboxObserverActor.Get();
        OutPreviewModeLabel = TEXT("Working subject preview");
        return true;
    }

    FWanaSelectedCharacterEnhancementSnapshot Snapshot;

    if (ResolvePreferredSubjectSnapshot(Snapshot)
        && Snapshot.bHasSelectedActor
        && Snapshot.SelectedActor.IsValid()
        && IsActorCompatibleWithWorkspacePreview(Snapshot.SelectedActor.Get(), WorkspaceLabel))
    {
        OutActor = Snapshot.SelectedActor.Get();
        OutPreviewModeLabel = Snapshot.bIsProjectAsset
            ? TEXT("Picked asset match preview")
            : TEXT("Editor selection fallback preview");
        return true;
    }

    return false;
}

bool FWanaWorksUIModule::ResolveCharacterIntelligenceObserverActor(AActor*& OutObserverActor, FString& OutSourceLabel) const
{
    OutObserverActor = nullptr;
    OutSourceLabel = TEXT("No compatible AI subject selected");
    const FString AIWorkspaceLabel(TEXT("AI"));

    if (LastEnhancementResultsActor.IsValid()
        && IsActorCompatibleWithWorkspacePreview(LastEnhancementResultsActor.Get(), AIWorkspaceLabel))
    {
        OutObserverActor = LastEnhancementResultsActor.Get();
        OutSourceLabel = TEXT("Working/enhanced AI subject");
        return true;
    }

    if (SandboxObserverActor.IsValid()
        && IsActorCompatibleWithWorkspacePreview(SandboxObserverActor.Get(), AIWorkspaceLabel))
    {
        OutObserverActor = SandboxObserverActor.Get();
        OutSourceLabel = TEXT("Assigned Character Intelligence subject");
        return true;
    }

    FWanaSelectedCharacterEnhancementSnapshot Snapshot;

    if (ResolvePreferredSubjectSnapshot(Snapshot)
        && Snapshot.bHasSelectedActor
        && Snapshot.SelectedActor.IsValid()
        && IsActorCompatibleWithWorkspacePreview(Snapshot.SelectedActor.Get(), AIWorkspaceLabel))
    {
        OutObserverActor = Snapshot.SelectedActor.Get();
        OutSourceLabel = Snapshot.bIsProjectAsset ? TEXT("Picked AI asset") : TEXT("Editor-selected AI subject");
        return true;
    }

    return false;
}

bool FWanaWorksUIModule::ResolveCharacterIntelligenceTargetActor(AActor*& OutTargetActor, FString& OutSourceLabel) const
{
    OutTargetActor = nullptr;
    OutSourceLabel = SelectedCharacterIntelligenceTargetLabel.IsEmpty()
        ? TEXT("No Target")
        : SelectedCharacterIntelligenceTargetLabel;

    if (SelectedCharacterIntelligenceTargetLabel.Equals(TEXT("No Target"), ESearchCase::IgnoreCase))
    {
        return false;
    }

    if (SandboxTargetActor.IsValid())
    {
        OutTargetActor = SandboxTargetActor.Get();
        OutSourceLabel = SelectedCharacterIntelligenceTargetLabel.IsEmpty()
            ? TEXT("Assigned relationship target")
            : SelectedCharacterIntelligenceTargetLabel;
        return true;
    }

    if (SelectedCharacterIntelligenceTargetLabel.Equals(TEXT("Current Editor Selection"), ESearchCase::IgnoreCase))
    {
        if (AActor* SelectedActor = GetFirstEditorSelectedActor())
        {
            OutTargetActor = SelectedActor;
            OutSourceLabel = TEXT("Current Editor Selection");
            return true;
        }
    }

    if (SelectedCharacterIntelligenceTargetLabel.Equals(TEXT("Player Character"), ESearchCase::IgnoreCase))
    {
        if (ACharacter* PlayerCharacter = FindEditorPlayerCharacterCandidate())
        {
            OutTargetActor = PlayerCharacter;
            OutSourceLabel = TEXT("Player Character");
            return true;
        }
    }

    return false;
}

void FWanaWorksUIModule::ApplyCharacterIntelligenceControlState(AActor* ObserverActor, AActor* TargetActor, TArray<FString>* OutControlNotes)
{
    auto AddControlNote = [OutControlNotes](const FString& Note)
    {
        if (OutControlNotes && !Note.IsEmpty())
        {
            OutControlNotes->Add(Note);
        }
    };

    if (!IsValid(ObserverActor))
    {
        AddControlNote(TEXT("No compatible AI subject selected. Choose an AI Pawn/NPC or click Enhance to prepare a working subject."));
        return;
    }

    if (UWanaIdentityComponent* IdentityComponent = ObserverActor->FindComponentByClass<UWanaIdentityComponent>())
    {
        const FString RoleFactionTag = GetCharacterIntelligenceRoleFactionTag(SelectedCharacterIntelligenceIdentityRoleLabel);
        FScopedTransaction Transaction(LOCTEXT("WanaWorksApplyCharacterIntelligenceIdentityControlTransaction", "Apply WanaWorks Character Intelligence Identity Control"));
        ObserverActor->Modify();
        IdentityComponent->SetFlags(RF_Transactional);
        IdentityComponent->Modify();
        IdentityComponent->FactionTag = FName(*RoleFactionTag);
        IdentityComponent->DefaultRelationshipSeed.RelationshipState = SelectedIdentitySeedState;
        ObserverActor->MarkPackageDirty();
        AddControlNote(FString::Printf(TEXT("WAI/WAMI identity control applied as %s with %s default seed."), *SelectedCharacterIntelligenceIdentityRoleLabel, *UIFmt::GetRelationshipStateLabel(SelectedIdentitySeedState)));
    }
    else
    {
        AddControlNote(TEXT("WAI/WAMI identity component missing. Click Enhance to prepare identity support before applying role data to the subject."));
    }

    if (!IsValid(TargetActor))
    {
        AddControlNote(TEXT("No relationship target selected. WAY-lite behavior will use a safe observe-style recommendation until a target is assigned."));
        return;
    }

    if (ObserverActor->GetWorld() && TargetActor->GetWorld() && ObserverActor->GetWorld() != TargetActor->GetWorld())
    {
        AddControlNote(TEXT("Selected target belongs to a different world context. WAY-lite relationship data was not applied."));
        return;
    }

    if (UWAYPlayerProfileComponent* ProfileComponent = ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>())
    {
        FScopedTransaction Transaction(LOCTEXT("WanaWorksApplyCharacterIntelligenceRelationshipControlTransaction", "Apply WanaWorks Character Intelligence Relationship Control"));
        ObserverActor->Modify();
        ProfileComponent->SetFlags(RF_Transactional);
        ProfileComponent->Modify();
        ProfileComponent->EnsureRelationshipProfileForTarget(TargetActor);
        ProfileComponent->SetRelationshipStateForTarget(TargetActor, SelectedRelationshipState);
        ObserverActor->MarkPackageDirty();
        AddControlNote(FString::Printf(TEXT("WAY-lite relationship set to %s for %s."), *SelectedCharacterIntelligenceRelationshipLabel, *TargetActor->GetActorNameOrLabel()));
    }
    else
    {
        AddControlNote(TEXT("WAY component missing. Click Enhance to prepare relationship support before applying target-specific relationship data."));
    }
}

bool FWanaWorksUIModule::PreparePickerDrivenSubjectForWorkflow(
    const FString& WorkflowContextLabel,
    FWanaCommandResponse& OutPreparationResponse,
    bool& bOutSpawnedFromPicker)
{
    OutPreparationResponse = FWanaCommandResponse();
    bOutSpawnedFromPicker = false;

    const FString WorkspaceLabel = GetSelectedWorkspaceLabel();
    UObject* SubjectAssetObject = LoadSelectedSubjectAssetObject();
    FWanaSelectedCharacterEnhancementSnapshot CurrentSelectionSnapshot;
    const bool bHasCurrentSelection = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(CurrentSelectionSnapshot) && CurrentSelectionSnapshot.bHasSelectedActor;

    if (!SubjectAssetObject)
    {
        if (bHasCurrentSelection
            && CurrentSelectionSnapshot.SelectedActor.IsValid()
            && IsActorCompatibleWithWorkspacePreview(CurrentSelectionSnapshot.SelectedActor.Get(), WorkspaceLabel))
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
        && IsActorCompatibleWithWorkspacePreview(CurrentSelectionSnapshot.SelectedActor.Get(), WorkspaceLabel)
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

bool FWanaWorksUIModule::ApplyWorkspaceSubjectEnhancement(
    const FString& WorkflowLabel,
    const FString& EnhancementPresetLabel,
    const FString& WorkspaceActionLabel)
{
    FWanaSelectedCharacterEnhancementSnapshot SourceSnapshot;
    ResolvePreferredSubjectSnapshot(SourceSnapshot);
    const bool bHasSourceSnapshot = SourceSnapshot.bHasSelectedActor;
    FWanaCommandResponse WorkflowResponse;
    FWanaCommandResponse EnhancementResponse;
    FWanaCommandResponse AIReadyResponse;
    FWanaCommandResponse PreparationResponse;
    const bool bUseSandboxDuplicate = WorkflowLabel == TEXT("Create Sandbox Duplicate")
        || WorkflowLabel == TEXT("Create Working Copy");
    const bool bUseAIReadyPrep = WorkflowLabel == TEXT("Convert to AI-Ready Test Subject");
    bool bSpawnedFromPicker = false;

    if (!PreparePickerDrivenSubjectForWorkflow(WorkflowLabel, PreparationResponse, bSpawnedFromPicker))
    {
        PreparationResponse.OutputLines.Insert(FString::Printf(TEXT("Workspace Action: %s"), *WorkspaceActionLabel), 0);
        ApplyResponse(PreparationResponse);
        RefreshReactiveUI(true);
        return false;
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
            WorkflowResponse.OutputLines.Insert(FString::Printf(TEXT("Workspace Action: %s"), *WorkspaceActionLabel), 0);
            ApplyResponse(WorkflowResponse);
            RefreshReactiveUI(true);
            return false;
        }
    }

    EnhancementResponse = WanaWorksUIEditorActions::ExecuteApplyCharacterEnhancementCommand(EnhancementPresetLabel);

    if (!EnhancementResponse.bSucceeded)
    {
        EnhancementResponse.OutputLines.Insert(FString::Printf(TEXT("Workspace Action: %s"), *WorkspaceActionLabel), 0);
        ApplyResponse(EnhancementResponse);
        RefreshReactiveUI(true);
        return false;
    }

    if (bUseAIReadyPrep)
    {
        AIReadyResponse = WanaWorksUIEditorActions::ExecutePrepareSelectedActorForAITestCommand();

        if (!AIReadyResponse.bSucceeded)
        {
            AIReadyResponse.OutputLines.Insert(FString::Printf(TEXT("Workspace Action: %s"), *WorkspaceActionLabel), 0);
            ApplyResponse(AIReadyResponse);
            RefreshReactiveUI(true);
            return false;
        }
    }

    FWanaSelectedCharacterEnhancementSnapshot ActiveSnapshot;
    const bool bHasActiveSnapshot = WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(ActiveSnapshot) && ActiveSnapshot.bHasSelectedActor;

    if (bHasActiveSnapshot)
    {
        SandboxObserverActor = ActiveSnapshot.SelectedActor;
    }

    const FString SourceSubjectLabel = SourceSnapshot.SelectedActorLabel.IsEmpty()
        ? TEXT("(picked subject)")
        : SourceSnapshot.SelectedActorLabel;
    const FString ActiveSubjectLabel = bHasActiveSnapshot ? ActiveSnapshot.SelectedActorLabel : SourceSubjectLabel;

    UpdateEnhancementResultsState(
        &SourceSnapshot,
        bHasActiveSnapshot ? &ActiveSnapshot : &SourceSnapshot,
        WorkflowLabel,
        true,
        bUseSandboxDuplicate || bSpawnedFromPicker,
        true);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = FString::Printf(TEXT("Status: %s complete. Original preserved."), *WorkspaceActionLabel);
    Response.OutputLines.Add(FString::Printf(TEXT("Workspace Action: %s"), *WorkspaceActionLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Source Subject: %s"), *SourceSubjectLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Active Subject: %s"), *ActiveSubjectLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Workflow Path: %s"), *WorkflowLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Preset Used: %s"), *EnhancementPresetLabel));
    Response.OutputLines.Add(TEXT("Original Source: Preserved"));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Working Subject: %s"),
        (bUseSandboxDuplicate || bSpawnedFromPicker) ? TEXT("Prepared") : TEXT("Existing selected actor")));

    if (bHasActiveSnapshot)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Identity / Profile: %s"), ActiveSnapshot.bHasIdentityComponent ? TEXT("Ready") : TEXT("Limited")));
        Response.OutputLines.Add(FString::Printf(TEXT("WAY-lite Relationship: %s"), ActiveSnapshot.bHasWAYComponent ? TEXT("Ready") : TEXT("Limited")));
        Response.OutputLines.Add(FString::Printf(TEXT("WAI / WAMI: %s"), ActiveSnapshot.bHasWAIComponent ? TEXT("Ready") : TEXT("Limited")));
        Response.OutputLines.Add(FString::Printf(TEXT("WanaAnimation: %s"), *UIFmt::GetAutomaticAnimationIntegrationStatusLabel(ActiveSnapshot.AnimationAutomaticIntegrationStatus)));
        Response.OutputLines.Add(FString::Printf(TEXT("Physical State: %s"), ActiveSnapshot.bHasPhysicalStateComponent ? TEXT("Ready") : TEXT("Limited")));
        Response.OutputLines.Add(FString::Printf(TEXT("AI Readiness: %s"), *ActiveSnapshot.AIReadinessSummary));
    }

    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Components Added:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Already Present:"));
    UIFmt::AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Compatibility Notes:"));

    if (bSpawnedFromPicker)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: WanaWorks generated a working subject from the picked asset before applying enhancement."));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Picked Subject Asset:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Detected Source Anim BP:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Workspace Animation Integration Target:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Automatic Anim Integration:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Auto-Attach:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Auto-Wire:"));
    }
    else if (bUseSandboxDuplicate)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: WanaWorks created a working copy before applying enhancement."));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Detected Source Anim BP:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Workspace Animation Integration Target:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Automatic Anim Integration:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Auto-Attach:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Auto-Wire:"));
    }

    if (bUseAIReadyPrep)
    {
        UIFmt::AppendLinesWithPrefix(Response, AIReadyResponse, TEXT("AI-Ready Preparation:"));
        UIFmt::AppendLinesWithPrefix(Response, AIReadyResponse, TEXT("AI Controller Class:"));
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
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
        Response.OutputLines.Insert(TEXT("Readiness Notes: WanaWorks created a working subject from the project picker before applying AI-ready prep."), 2);
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
        Response.OutputLines.Insert(TEXT("Readiness Notes: WanaWorks created a working subject from the project picker before converting it for AI-ready testing."), 2);
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
    Response.OutputLines.Add(TEXT("Readiness Notes: Use Restore Last Saved State to bring back the saved workflow choices and working-pair assignments when they are still available."));
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
    const bool bUseSandboxDuplicate = Config.WorkflowLabel == TEXT("Create Sandbox Duplicate")
        || Config.WorkflowLabel == TEXT("Create Working Copy");
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
        (bUseSandboxDuplicate || bSpawnedFromPicker) ? TEXT("Working subject") : TEXT("Original actor")));
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
        Response.OutputLines.Add(TEXT("Readiness Notes: WanaWorks created a working subject from the project picker before applying the preset."));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Picked Subject Asset:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Picked Animation Blueprint:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Detected Source Anim BP:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Animation Override Applied:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Workspace Animation Integration Target:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Automatic Anim Integration:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Auto-Attach:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Auto-Wire:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Generated Runtime Layer:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Animation Integration Notes:"));
    }
    else if (bUseSandboxDuplicate)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: WanaWorks created a working copy before applying the preset."));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Detected Source Anim BP:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Workspace Animation Integration Target:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Automatic Anim Integration:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Auto-Attach:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Auto-Wire:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Generated Runtime Layer:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Animation Integration Notes:"));
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

void FWanaWorksUIModule::CreateWorkingCopy()
{
    FWanaSelectedCharacterEnhancementSnapshot BeforeSnapshot;
    const bool bHasBeforeSnapshot = ResolvePreferredSubjectSnapshot(BeforeSnapshot) && BeforeSnapshot.bHasSelectedActor;
    FWanaCommandResponse PreparationResponse;
    bool bSpawnedFromPicker = false;

    if (!PreparePickerDrivenSubjectForWorkflow(TEXT("Create Copy"), PreparationResponse, bSpawnedFromPicker))
    {
        ApplyResponse(PreparationResponse);
        RefreshReactiveUI(true);
        return;
    }

    FWanaCommandResponse Response;

    if (bSpawnedFromPicker)
    {
        Response = PreparationResponse;
        Response.StatusMessage = TEXT("Status: Created working copy.");
        Response.OutputLines.Insert(TEXT("Workspace Action: Create Copy"), 0);
        Response.OutputLines.Add(TEXT("Workspace Notes: WanaWorks created a working subject from the picked project asset so the original source stays untouched."));
    }
    else
    {
        Response = WanaWorksUIEditorActions::ExecuteCreateSandboxDuplicateCommand();

        if (bHasBeforeSnapshot)
        {
            Response.OutputLines.Insert(FString::Printf(TEXT("Selected Subject: %s"), *BeforeSnapshot.SelectedActorLabel), 0);
            Response.OutputLines.Insert(TEXT("Workspace Action: Create Copy"), 1);
        }

        if (Response.bSucceeded)
        {
            Response.StatusMessage = TEXT("Status: Created working copy.");
            Response.OutputLines.Add(TEXT("Workspace Notes: WanaWorks created a working copy inside the workspace so the original actor remains preserved."));
        }
    }

    if (Response.bSucceeded)
    {
        FWanaSelectedCharacterEnhancementSnapshot WorkingSnapshot;

        if (WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(WorkingSnapshot)
            && WorkingSnapshot.bHasSelectedActor
            && WorkingSnapshot.SelectedActor.IsValid())
        {
            SandboxObserverActor = WorkingSnapshot.SelectedActor.Get();
        }
    }

    ApplyResponse(Response);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::EnhanceActiveWorkspace()
{
    const FString WorkspaceLabel = GetSelectedWorkspaceLabel();

    if (!HasCurrentWorkspaceAnalysis())
    {
        AnalyzeActiveWorkspace();
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        const FString WorldLabel = GetEditorWorldLabel();
        const int32 SelectedActorCount = GetEditorSelectedActorCount();
        const FString SceneSelectionLabel = FString::Printf(TEXT("%d actor(s) captured as optional scene context."), SelectedActorCount);
        const FString WorldContextLabel = WorldLabel.IsEmpty() ? TEXT("No editor world available yet.") : WorldLabel;
        AActor* ObserverActor = nullptr;
        AActor* TargetActor = nullptr;
        FString PairSourceLabel;
        bool bTargetFallsBackToObserver = false;
        ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

        FWanaEnvironmentReadinessSnapshot EnvironmentSnapshot;
        WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
            ObserverActor,
            TargetActor,
            bTargetFallsBackToObserver,
            PairSourceLabel,
            EnvironmentSnapshot);
        const bool bHasWITPair = HasLevelDesignWITPair(EnvironmentSnapshot);
        const FString CoverStatus = GetLevelDesignCoverMeaningStatus(EnvironmentSnapshot, SelectedActorCount);
        const FString ObstacleStatus = GetLevelDesignObstacleMeaningStatus(EnvironmentSnapshot);
        const FString MovementSpaceStatus = GetLevelDesignMovementSpaceStatus(EnvironmentSnapshot);
        const FString BoundaryStatus = GetLevelDesignBoundaryMeaningStatus(EnvironmentSnapshot);

        bWorkspaceAnalysisInitialized = true;
        LastAnalysisWorkspaceLabel = WorkspaceLabel;
        LastAnalysisStatusSummary = bHasWITPair
            ? TEXT("Level Design prepared with WIT semantic world context.")
            : TEXT("Level Design prepared; assign observer-target context for richer WIT readings.");
        LastAnalysisPrimarySummary = FString::Printf(
            TEXT("Semantic World Preparation\nWorld Context: %s\nScene Selection: %s\nScene Pair: %s\nOriginal Level: Preserved\nGenerated Assets: None\nWorkspace Mode: WIT semantic environment understanding is ready for scan-based analysis."),
            *WorldContextLabel,
            *SceneSelectionLabel,
            *BuildLevelDesignPairSummary(EnvironmentSnapshot));
        LastAnalysisWITSummary = FString::Printf(
            TEXT("WIT Preparation\nEnvironment Scan Readiness: %s - %s\nCover Meaning: %s - %s\nObstacle Meaning: %s - %s\nMovement Space: %s - %s\nBoundary / Navigation Context: %s - %s\nScene Context: %s\nRecommended Next Step: Run Analyze to refresh the semantic world reading, then Test before Build."),
            bHasWITPair ? TEXT("Ready") : TEXT("Limited"),
            *BuildLevelDesignPairSummary(EnvironmentSnapshot),
            *CoverStatus,
            *GetLevelDesignCoverMeaningDetail(EnvironmentSnapshot, SelectedActorCount),
            *ObstacleStatus,
            *GetLevelDesignObstacleMeaningDetail(EnvironmentSnapshot),
            *MovementSpaceStatus,
            *GetLevelDesignMovementSpaceDetail(EnvironmentSnapshot),
            *BoundaryStatus,
            *GetLevelDesignBoundaryMeaningDetail(EnvironmentSnapshot),
            *SceneSelectionLabel);
        LastAnalysisBehaviorSummary = LastAnalysisWITSummary;
        LastAnalysisAnimationSummary = LastAnalysisWITSummary;
        LastAnalysisPhysicalSummary = LastAnalysisWITSummary;
        LastAnalysisSuggestedSummary = TEXT("Level Design Enhancement Result\nPrepared Systems: WIT semantic world context, scene pair, cover demand, obstacle pressure, movement-space meaning, boundary/navigation hints\nApplied Changes: None\nOriginal Level: Preserved\nRecommended Next Step: Analyze the scene, then use Build for an in-app semantic-world output summary.");
        StatusMessage = TEXT("Status: Level Design WIT context prepared. Original level preserved.");
        RefreshReactiveUI(true);
        return;
    }

    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSubject = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;

    if (!bHasSubject)
    {
        StatusMessage = WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase)
            ? TEXT("Status: Character enhancement limited. Choose a Character Blueprint or Pawn first.")
            : TEXT("Status: Character Intelligence enhancement limited. Choose an AI Pawn or NPC first.");
        RefreshReactiveUI(true);
        return;
    }

    const bool bCharacterBuildingWorkspace = WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase);
    const FString EnhancementPresetLabel = bCharacterBuildingWorkspace
        ? FString(TEXT("Identity Only"))
        : FString(TEXT("Full WanaAI Starter"));
    const FString WorkspaceActionLabel = bCharacterBuildingWorkspace
        ? FString(TEXT("Character Building enhancement"))
        : FString(TEXT("Character Intelligence enhancement"));

    if (!ApplyWorkspaceSubjectEnhancement(TEXT("Create Working Copy"), EnhancementPresetLabel, WorkspaceActionLabel))
    {
        return;
    }

    AnalyzeActiveWorkspace();

    FWanaSelectedCharacterEnhancementSnapshot ActiveSnapshot;
    const bool bHasActiveSnapshot = ResolvePreferredSubjectSnapshot(ActiveSnapshot) && ActiveSnapshot.bHasSelectedActor;

    if (bCharacterBuildingWorkspace)
    {
        LastAnalysisSuggestedSummary = FString::Printf(
            TEXT("Character Build Preparation Result\nOriginal Source: Preserved\nWorking Subject: %s\nCharacter Profile: %s\nRig & Animation: %s\nMovement / Playability: %s\nBuild Readiness: Prepared for clean WanaWorks output\nRecommended Next Step: Test the prepared character, then Build when the preview and readiness cards look correct."),
            bHasActiveSnapshot ? *ActiveSnapshot.SelectedActorLabel : TEXT("(working subject)"),
            bHasActiveSnapshot && ActiveSnapshot.bHasIdentityComponent ? TEXT("Ready") : TEXT("Limited"),
            bHasActiveSnapshot ? *UIFmt::GetAutomaticAnimationIntegrationStatusLabel(ActiveSnapshot.AnimationAutomaticIntegrationStatus) : TEXT("Limited"),
            bHasActiveSnapshot && ActiveSnapshot.bIsPawnActor ? TEXT("Ready") : TEXT("Limited"));
        StatusMessage = TEXT("Status: Character build preparation complete. Original preserved.");
    }
    else
    {
        LastAnalysisSuggestedSummary = FString::Printf(
            TEXT("Character Intelligence Enhancement Result\nOriginal Source: Preserved\nWorking Subject: %s\nWAI / WAMI: %s\nWAY-lite: %s\nWanaAnimation: %s\nWanaCombat-lite / Physical State: %s\nWIT Awareness: Prepared for semantic readiness analysis\nRecommended Next Step: Test the enhanced NPC/AI, then Analyze again after behavior or environment context changes."),
            bHasActiveSnapshot ? *ActiveSnapshot.SelectedActorLabel : TEXT("(working subject)"),
            bHasActiveSnapshot && ActiveSnapshot.bHasWAIComponent ? TEXT("Ready") : TEXT("Limited"),
            bHasActiveSnapshot && ActiveSnapshot.bHasWAYComponent ? TEXT("Ready") : TEXT("Limited"),
            bHasActiveSnapshot ? *UIFmt::GetAutomaticAnimationIntegrationStatusLabel(ActiveSnapshot.AnimationAutomaticIntegrationStatus) : TEXT("Limited"),
            bHasActiveSnapshot && ActiveSnapshot.bHasPhysicalStateComponent ? TEXT("Ready") : TEXT("Limited"));
        StatusMessage = TEXT("Status: Character Intelligence enhancement applied. Original preserved.");
    }

    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::TestActiveWorkspace()
{
    const FString WorkspaceLabel = GetSelectedWorkspaceLabel();

    if (!HasCurrentWorkspaceAnalysis())
    {
        AnalyzeActiveWorkspace();
    }

    FWanaAnalysisResult Result;
    Result.WorkspaceLabel = WorkspaceLabel;
    Result.DisplayWorkspaceLabel = GetWorkspaceDisplayLabel(WorkspaceLabel);

    TArray<FWanaAnalysisItem> PrimaryItems;
    TArray<FWanaAnalysisItem> AnimationItems;
    TArray<FWanaAnalysisItem> PhysicalItems;
    TArray<FWanaAnalysisItem> BehaviorItems;
    TArray<FWanaAnalysisItem> WITItems;
    TArray<FWanaAnalysisItem> SuggestedItems;
    FString RecommendedAction;
    FString ResultLabel;

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        const FString WorldLabel = GetEditorWorldLabel();
        const int32 SelectedActorCount = GetEditorSelectedActorCount();
        AActor* ObserverActor = nullptr;
        AActor* TargetActor = nullptr;
        FString PairSourceLabel;
        bool bTargetFallsBackToObserver = false;
        ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

        FWanaEnvironmentReadinessSnapshot EnvironmentSnapshot;
        WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
            ObserverActor,
            TargetActor,
            bTargetFallsBackToObserver,
            PairSourceLabel,
            EnvironmentSnapshot);

        const bool bHasWorldContext = !WorldLabel.IsEmpty();
        const bool bHasWITPair = HasLevelDesignWITPair(EnvironmentSnapshot);
        const bool bHasMovementContext = bHasWITPair && EnvironmentSnapshot.MovementReadiness.bHasMovementContext;

        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("World Context"),
            bHasWorldContext ? TEXT("Ready") : TEXT("Missing"),
            bHasWorldContext ? WorldLabel : TEXT("No editor world is available for semantic testing yet."));
        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("Selected Scene Context"),
            SelectedActorCount > 0 ? TEXT("Ready") : TEXT("Limited"),
            FString::Printf(TEXT("%d actor(s) selected. Selection is optional for this safe Level Design test."), SelectedActorCount));
        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("Scene Pair"),
            bHasWITPair ? TEXT("Ready") : TEXT("Limited"),
            BuildLevelDesignPairSummary(EnvironmentSnapshot));
        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("Level Modification"),
            TEXT("Ready"),
            TEXT("Test V1 is read-only: no modular assets are spawned and the current level is preserved."));

        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("WIT Scan Readiness"),
            bHasWITPair ? TEXT("Ready") : TEXT("Limited"),
            bHasWITPair ? TEXT("Observer-target context is available for semantic world testing.") : TEXT("WIT scan context is not assigned yet; prepare or select scene actors for richer semantic testing."));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Navigation Context"),
            bHasMovementContext ? TEXT("Ready") : TEXT("Limited"),
            bHasWITPair ? UIFmt::GetNavigationContextSummaryLabel(EnvironmentSnapshot) : TEXT("Navigation remains unknown until a WIT movement context is available."));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Cover Meaning"),
            GetLevelDesignCoverMeaningStatus(EnvironmentSnapshot, SelectedActorCount),
            GetLevelDesignCoverMeaningDetail(EnvironmentSnapshot, SelectedActorCount));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Obstacle Meaning"),
            GetLevelDesignObstacleMeaningStatus(EnvironmentSnapshot),
            GetLevelDesignObstacleMeaningDetail(EnvironmentSnapshot));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Movement Space"),
            GetLevelDesignMovementSpaceStatus(EnvironmentSnapshot),
            GetLevelDesignMovementSpaceDetail(EnvironmentSnapshot));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Level Generation"),
            TEXT("Not Supported"),
            TEXT("Prompt-based level generation is intentionally outside this Test V1 phase."));

        RecommendedAction = bHasWITPair
            ? TEXT("Build can prepare an in-app semantic-world summary; future WIT passes can persist cover and obstacle classification as assets.")
            : TEXT("Prepare or select scene context, then run Analyze before future WIT scan work.");
        ResultLabel = !bHasWorldContext
            ? TEXT("world context is missing")
            : (bHasWITPair ? TEXT("WIT semantic world context is readable") : TEXT("world context is available but WIT scan context is limited"));
        AddAnalysisItem(SuggestedItems, Result, TEXT("Next Step"), TEXT("Recommended"), RecommendedAction);

        Result.PrimarySummary = BuildAnalysisCardText(TEXT("Semantic World Test Result"), PrimaryItems, RecommendedAction);
        Result.WITSummary = BuildAnalysisCardText(TEXT("WIT Readiness Test"), WITItems, RecommendedAction);
        Result.AnimationSummary = Result.WITSummary;
        Result.PhysicalSummary = Result.WITSummary;
        Result.BehaviorSummary = Result.WITSummary;
        Result.SuggestedSummary = BuildAnalysisCardText(TEXT("Level Test Recommendations"), SuggestedItems, RecommendedAction);
    }
    else
    {
        FWanaSelectedCharacterEnhancementSnapshot Snapshot;
        const bool bHasSubject = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
        const UClass* SubjectClass = LoadSelectedSubjectActorClassForWorkspace(WorkspaceLabel);
        const AActor* AnalysisActor = bHasSubject ? ResolveAnalysisActor(Snapshot, SubjectClass) : nullptr;
        const bool bCharacterBuildingWorkspace = WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase);
        FString SkeletalMeshLabel;
        FString SkeletonLabel;
        const bool bHasSkeletalMeshAsset = GetSkeletalMeshAndSkeletonLabels(AnalysisActor, SkeletalMeshLabel, SkeletonLabel);
        const bool bHasMovementComponent = HasPawnMovementComponent(AnalysisActor);
        const bool bHasCameraOrControlComponent = ComponentClassNameContainsAny(AnalysisActor, {TEXT("Camera"), TEXT("SpringArm"), TEXT("Control"), TEXT("Input")});
        AActor* PreviewActor = nullptr;
        FString PreviewModeLabel;
        const bool bPreviewReady = ResolvePreferredSandboxPreviewActor(PreviewActor, PreviewModeLabel) && PreviewActor != nullptr;
        const bool bWorkingSubjectReady = bEnhancementResultsInitialized && LastEnhancementResultsActor.IsValid();

        if (!bHasSubject)
        {
            AddAnalysisItem(
                PrimaryItems,
                Result,
                bCharacterBuildingWorkspace ? TEXT("Character Subject") : TEXT("AI Subject"),
                TEXT("Missing"),
                bCharacterBuildingWorkspace ? TEXT("Choose a Character Blueprint or playable Character Pawn before testing.") : TEXT("Choose an AI Pawn or NPC subject before testing."));
            AddAnalysisItem(
                SuggestedItems,
                Result,
                TEXT("Test Result"),
                TEXT("Limited"),
                TEXT("Test blocked until a compatible subject is selected in this workspace."));

            RecommendedAction = bCharacterBuildingWorkspace
                ? TEXT("Select a Character Blueprint or Pawn, then Analyze or Enhance before testing again.")
                : TEXT("Select an AI Pawn or NPC, then Analyze or Enhance before testing again.");
            ResultLabel = TEXT("test blocked because no compatible subject is selected");
            Result.PrimarySummary = BuildAnalysisCardText(bCharacterBuildingWorkspace ? TEXT("Character Test Blocked") : TEXT("AI Test Blocked"), PrimaryItems, RecommendedAction);
            Result.AnimationSummary = Result.PrimarySummary;
            Result.PhysicalSummary = Result.PrimarySummary;
            Result.BehaviorSummary = Result.PrimarySummary;
            Result.WITSummary = Result.PrimarySummary;
            Result.SuggestedSummary = BuildAnalysisCardText(TEXT("Test Recommendations"), SuggestedItems, RecommendedAction);
        }
        else if (bCharacterBuildingWorkspace)
        {
            const bool bRigReady = !SkeletonLabel.IsEmpty() && !SkeletonLabel.Equals(TEXT("(no skeleton asset)"));
            const bool bAnimationReady = Snapshot.bHasAnimBlueprint;
            const bool bBuildReady = Snapshot.bHasSkeletalMeshComponent && Snapshot.bHasAnimBlueprint;

            AddAnalysisItem(PrimaryItems, Result, TEXT("Character Subject"), TEXT("Ready"), Snapshot.SelectedActorLabel);
            AddAnalysisItem(
                PrimaryItems,
                Result,
                TEXT("Preview Stage"),
                bPreviewReady ? TEXT("Ready") : TEXT("Limited"),
                bPreviewReady ? FString::Printf(TEXT("Preview is showing %s."), *PreviewModeLabel) : TEXT("No live preview subject is available yet."));
            AddAnalysisItem(
                PrimaryItems,
                Result,
                TEXT("Character Profile"),
                Snapshot.bHasIdentityComponent ? TEXT("Ready") : TEXT("Limited"),
                Snapshot.bHasIdentityComponent ? TEXT("Character profile/readiness layer is available.") : TEXT("Character profile is not attached yet; Enhance can prepare it on a working copy."));

            AddAnalysisItem(AnimationItems, Result, TEXT("Skeletal Mesh"), Snapshot.bHasSkeletalMeshComponent ? TEXT("Ready") : TEXT("Missing"), bHasSkeletalMeshAsset ? SkeletalMeshLabel : TEXT("No skeletal mesh asset detected."));
            AddAnalysisItem(AnimationItems, Result, TEXT("Skeleton / Rig"), bRigReady ? TEXT("Ready") : TEXT("Limited"), bRigReady ? SkeletonLabel : TEXT("Skeleton or rig could not be confirmed."));
            AddAnalysisItem(AnimationItems, Result, TEXT("Animation Blueprint"), bAnimationReady ? TEXT("Ready") : TEXT("Limited"), Snapshot.LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("No linked Animation Blueprint detected.") : Snapshot.LinkedAnimationBlueprintLabel);

            AddAnalysisItem(PhysicalItems, Result, TEXT("Movement Component"), bHasMovementComponent ? TEXT("Ready") : TEXT("Limited"), bHasMovementComponent ? TEXT("Movement component is present for playability validation.") : TEXT("No movement component detected; playability is limited."));
            AddAnalysisItem(PhysicalItems, Result, TEXT("Camera / Control Setup"), bHasCameraOrControlComponent ? TEXT("Ready") : TEXT("Limited"), bHasCameraOrControlComponent ? TEXT("Camera, spring arm, control, or input-style component detected.") : TEXT("Camera/control setup is unknown and preserved."));
            AddAnalysisItem(PhysicalItems, Result, TEXT("Playability"), Snapshot.bIsPawnActor && Snapshot.bHasSkeletalMeshComponent && bHasMovementComponent ? TEXT("Ready") : TEXT("Limited"), TEXT("Test validates playable-character signals without replacing movement or input."));

            AddAnalysisItem(SuggestedItems, Result, TEXT("Build Readiness"), bBuildReady ? TEXT("Ready") : TEXT("Limited"), bBuildReady ? TEXT("Mesh and animation are detected; character is ready for build preparation.") : TEXT("Build readiness is limited until mesh and animation are both detected."));
            AddAnalysisItem(SuggestedItems, Result, TEXT("Output Status"), bWorkingSubjectReady ? TEXT("Ready") : TEXT("Limited"), bWorkingSubjectReady ? TEXT("A WanaWorks working subject is available for safe output flow.") : TEXT("Enhancement has not prepared a working subject in this session yet."));

            RecommendedAction = bBuildReady
                ? TEXT("Build can proceed after any desired character-facing enhancement or visual review.")
                : TEXT("Run Enhance to prepare profile/readiness state, then fix missing mesh or animation setup before Build.");
            ResultLabel = bBuildReady ? TEXT("character preview, rig, and animation are ready") : TEXT("character test is limited by missing build-readiness signals");
            Result.PrimarySummary = BuildAnalysisCardText(TEXT("Character Test Result"), PrimaryItems, RecommendedAction);
            Result.AnimationSummary = BuildAnalysisCardText(TEXT("Rig & Animation Test"), AnimationItems, RecommendedAction);
            Result.PhysicalSummary = BuildAnalysisCardText(TEXT("Movement / Playability Test"), PhysicalItems, RecommendedAction);
            Result.BehaviorSummary = Result.PrimarySummary;
            Result.WITSummary = Result.PrimarySummary;
            Result.SuggestedSummary = BuildAnalysisCardText(TEXT("Build Readiness Test"), SuggestedItems, RecommendedAction);
        }
        else
        {
            const bool bAIControllerReady = Snapshot.bHasAIControllerClass;
            const bool bMovementReady = Snapshot.bIsPawnActor && (bHasMovementComponent || Snapshot.bAutoPossessAIEnabled);
            const bool bAnimationReady =
                Snapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Applied
                || Snapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Ready
                || Snapshot.bHasAnimBlueprint;
            const bool bPhysicalReady = Snapshot.bHasPhysicalStateComponent;
            const bool bBehaviorReady = Snapshot.bHasWAIComponent && Snapshot.bHasWAYComponent;
            const bool bReadyForBuild = bAIControllerReady && bAnimationReady && bPhysicalReady && bBehaviorReady;

            AActor* ObserverActor = nullptr;
            AActor* TargetActor = nullptr;
            FString PairSourceLabel;
            bool bTargetFallsBackToObserver = false;
            ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

            TArray<FString> ControlNotes;
            ApplyCharacterIntelligenceControlState(ObserverActor, TargetActor, &ControlNotes);
            const FString ControlRecommendedBehavior = GetCharacterIntelligenceRecommendationLabel(
                SelectedCharacterIntelligenceIdentityRoleLabel,
                SelectedCharacterIntelligenceRelationshipLabel,
                TargetActor != nullptr);

            FWanaEnvironmentReadinessSnapshot EnvironmentSnapshot;
            WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
                ObserverActor,
                TargetActor,
                bTargetFallsBackToObserver,
                PairSourceLabel,
                EnvironmentSnapshot);

            FWanaCommandResponse BehaviorExecutionResponse;
            const bool bBehaviorExecutionAttempted = ObserverActor != nullptr && TargetActor != nullptr;
            const bool bBehaviorExecutionPrepared = ObserverActor && ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>() != nullptr;
            bool bBehaviorExecutionSucceeded = false;

            if (bBehaviorExecutionAttempted && bBehaviorExecutionPrepared)
            {
                BehaviorExecutionResponse = WanaWorksUIEditorActions::ExecuteEvaluateActorPairCommand(
                    ObserverActor,
                    TargetActor,
                    bTargetFallsBackToObserver);
                bBehaviorExecutionSucceeded = BehaviorExecutionResponse.bSucceeded;

                WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
                    ObserverActor,
                    TargetActor,
                    bTargetFallsBackToObserver,
                    PairSourceLabel,
                    EnvironmentSnapshot);
            }
            else if (bBehaviorExecutionAttempted)
            {
                BehaviorExecutionResponse.StatusMessage = TEXT("Status: Behavior test needs Enhance before visible execution.");
            }

            FWanaBehaviorResultsSnapshot BehaviorSnapshot;
            WanaWorksUIEditorActions::GetBehaviorResultsSnapshotForActorPair(
                ObserverActor,
                TargetActor,
                bTargetFallsBackToObserver,
                PairSourceLabel,
                BehaviorSnapshot);

            const bool bHasWITPair = EnvironmentSnapshot.bHasObserverActor && EnvironmentSnapshot.bHasTargetActor;

            AddAnalysisItem(PrimaryItems, Result, TEXT("AI Test Subject"), TEXT("Ready"), Snapshot.SelectedActorLabel);
            AddAnalysisItem(PrimaryItems, Result, TEXT("Working / Enhanced Subject"), bWorkingSubjectReady ? TEXT("Ready") : TEXT("Limited"), bWorkingSubjectReady ? FString::Printf(TEXT("%s is available as the WanaWorks working subject."), *LastEnhancementResultsActorLabel) : TEXT("Enhancement has not prepared a working subject in this session; testing selected subject readiness instead."));
            AddAnalysisItem(PrimaryItems, Result, TEXT("Preview Stage"), bPreviewReady ? TEXT("Ready") : TEXT("Limited"), bPreviewReady ? FString::Printf(TEXT("Stage can show %s."), *PreviewModeLabel) : TEXT("Preview subject is not available yet."));

            AddAnalysisItem(BehaviorItems, Result, TEXT("AI Controller"), bAIControllerReady ? TEXT("Ready") : TEXT("Missing"), Snapshot.LinkedAIControllerLabel.IsEmpty() ? TEXT("No linked AI Controller detected.") : Snapshot.LinkedAIControllerLabel);
            AddAnalysisItem(BehaviorItems, Result, TEXT("WAI / WAMI"), Snapshot.bHasWAIComponent ? TEXT("Ready") : TEXT("Limited"), Snapshot.bHasWAIComponent ? TEXT("Identity, memory, emotion, and role layer is readable.") : TEXT("AI can be tested, but WAI/WAMI is not attached yet."));
            AddAnalysisItem(BehaviorItems, Result, TEXT("AI Identity Role"), ObserverActor && ObserverActor->FindComponentByClass<UWanaIdentityComponent>() ? TEXT("Ready") : TEXT("Limited"), FString::Printf(TEXT("%s biases behavior toward %s."), SelectedCharacterIntelligenceIdentityRoleLabel.IsEmpty() ? TEXT("Neutral") : *SelectedCharacterIntelligenceIdentityRoleLabel, *ControlRecommendedBehavior));
            AddAnalysisItem(BehaviorItems, Result, TEXT("WAY-lite Relationship"), Snapshot.bHasWAYComponent ? TEXT("Ready") : TEXT("Limited"), Snapshot.bHasWAYComponent ? TEXT("Relationship/adaptation layer is readable.") : TEXT("Relationship behavior is limited until WAY-lite is attached."));
            AddAnalysisItem(BehaviorItems, Result, TEXT("Relationship Target"), TargetActor ? TEXT("Ready") : TEXT("Limited"), TargetActor ? TargetActor->GetActorNameOrLabel() : TEXT("No relationship target selected."));
            AddAnalysisItem(BehaviorItems, Result, TEXT("Selected Relationship"), TargetActor && ObserverActor && ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>() ? TEXT("Ready") : TEXT("Limited"), FString::Printf(TEXT("%s maps to runtime state %s."), SelectedCharacterIntelligenceRelationshipLabel.IsEmpty() ? TEXT("Unknown") : *SelectedCharacterIntelligenceRelationshipLabel, *UIFmt::GetRelationshipStateLabel(SelectedRelationshipState)));
            AddAnalysisItem(BehaviorItems, Result, TEXT("Behavior Results"), BehaviorSnapshot.bHasWAYComponent && BehaviorSnapshot.bHasRelationshipProfile ? TEXT("Ready") : TEXT("Limited"), BehaviorSnapshot.bHasWAYComponent ? TEXT("Behavior result data is readable.") : TEXT("Behavior result is limited until WAY-lite has usable relationship data."));
            AddAnalysisItem(BehaviorItems, Result, TEXT("Requested Behavior"), BehaviorSnapshot.RecommendedBehavior != EWAYBehaviorPreset::None ? TEXT("Ready") : TEXT("Limited"), UIFmt::GetBehaviorPresetSummaryLabel(BehaviorSnapshot.RecommendedBehavior));
            AddAnalysisItem(BehaviorItems, Result, TEXT("Visible Behavior"), !BehaviorSnapshot.VisibleBehaviorLabel.IsEmpty() ? TEXT("Ready") : TEXT("Limited"), BehaviorSnapshot.VisibleBehaviorLabel.IsEmpty() ? TEXT("No visible starter behavior was applied.") : BehaviorSnapshot.VisibleBehaviorLabel);
            AddAnalysisItem(BehaviorItems, Result, TEXT("Execution Mode"), bBehaviorExecutionSucceeded ? TEXT("Ready") : TEXT("Limited"), UIFmt::GetBehaviorExecutionModeSummaryLabel(BehaviorSnapshot.ExecutionMode));
            AddAnalysisItem(BehaviorItems, Result, TEXT("Movement Compatibility"), BehaviorSnapshot.MovementReadiness.bCanAttemptMovement ? TEXT("Ready") : TEXT("Limited"), BuildAIBehaviorMovementCompatibilityDetail(BehaviorSnapshot.MovementReadiness));
            AddAnalysisItem(BehaviorItems, Result, TEXT("Fallback Reason"), BehaviorSnapshot.ExecutionMode == EWAYBehaviorExecutionMode::MovementAllowed ? TEXT("Ready") : TEXT("Limited"), BuildAIBehaviorFallbackReasonDetail(BehaviorSnapshot.ExecutionMode, BehaviorSnapshot.MovementReadiness));
            AddAnalysisItem(BehaviorItems, Result, TEXT("Result Notes"), bBehaviorExecutionSucceeded ? TEXT("Ready") : TEXT("Limited"), BehaviorSnapshot.BehaviorExecutionDetail.IsEmpty() ? TEXT("No behavior test detail was available.") : BehaviorSnapshot.BehaviorExecutionDetail);
            AddAnalysisItem(BehaviorItems, Result, TEXT("Behavior Next Step"), TEXT("Recommended"), BuildAIBehaviorRecommendedNextStepDetail(BehaviorSnapshot.ExecutionMode, BehaviorSnapshot.MovementReadiness));
            AddAnalysisItem(BehaviorItems, Result, TEXT("Behavior Test"), bBehaviorExecutionSucceeded ? TEXT("Ready") : TEXT("Limited"), bBehaviorExecutionAttempted ? BehaviorExecutionResponse.StatusMessage : TEXT("No observer-target pair was available for visible behavior execution."));

            if (!ControlNotes.IsEmpty())
            {
                const bool bControlStateLimited = ControlNotes.ContainsByPredicate([](const FString& Note)
                {
                    return Note.Contains(TEXT("missing"), ESearchCase::IgnoreCase)
                        || Note.Contains(TEXT("No "), ESearchCase::CaseSensitive)
                        || Note.Contains(TEXT("different world"), ESearchCase::IgnoreCase)
                        || Note.Contains(TEXT("Needs"), ESearchCase::IgnoreCase);
                });
                AddAnalysisItem(BehaviorItems, Result, TEXT("Control Safety"), bControlStateLimited ? TEXT("Limited") : TEXT("Ready"), FString::Join(ControlNotes, TEXT(" ")));
            }

            AddAnalysisItem(AnimationItems, Result, TEXT("Animation Blueprint"), Snapshot.bHasAnimBlueprint ? TEXT("Ready") : TEXT("Limited"), Snapshot.LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("No linked Animation Blueprint detected.") : Snapshot.LinkedAnimationBlueprintLabel);
            AddAnalysisItem(AnimationItems, Result, TEXT("WanaAnimation"), bAnimationReady ? TEXT("Ready") : TEXT("Limited"), Snapshot.AnimationAutomaticIntegrationDetail.IsEmpty() ? TEXT("Animation integration is not fully prepared yet.") : Snapshot.AnimationAutomaticIntegrationDetail);

            AddAnalysisItem(PhysicalItems, Result, TEXT("Movement / Facing"), bMovementReady ? TEXT("Ready") : TEXT("Limited"), bMovementReady ? TEXT("Movement or facing readiness is available without forcing locomotion.") : TEXT("Movement-safe execution is limited; fallback facing/attention behavior should be used."));
            AddAnalysisItem(PhysicalItems, Result, TEXT("Physical State"), bPhysicalReady ? TEXT("Ready") : TEXT("Limited"), bPhysicalReady ? TEXT("Readable body-state layer is available for physical response testing.") : TEXT("Physical state layer is missing; visible disruption testing is limited."));
            AddAnalysisItem(PhysicalItems, Result, TEXT("Behavior Commitment"), bPhysicalReady && Snapshot.bPhysicalCanCommitToMovement && Snapshot.bPhysicalCanCommitToAttack ? TEXT("Ready") : TEXT("Limited"), BuildAIPhysicalBehaviorCommitmentDetail(Snapshot));
            AddAnalysisItem(PhysicalItems, Result, TEXT("WanaCombat-lite"), bPhysicalReady && Snapshot.bHasWAYComponent ? TEXT("Ready") : TEXT("Limited"), TEXT("Combat-lite validation stays additive and does not replace Behavior Trees, controllers, or locomotion."));

            AddAnalysisItem(WITItems, Result, TEXT("WIT Awareness"), bHasWITPair ? TEXT("Ready") : TEXT("Limited"), bHasWITPair ? UIFmt::GetEnvironmentShapingSummaryLabel(EnvironmentSnapshot.MovementReadiness) : TEXT("No active WIT observer-target context is assigned yet."));
            AddAnalysisItem(WITItems, Result, TEXT("Navigation Context"), bHasWITPair && EnvironmentSnapshot.MovementReadiness.bHasMovementContext ? TEXT("Ready") : TEXT("Limited"), bHasWITPair ? UIFmt::GetNavigationContextSummaryLabel(EnvironmentSnapshot) : TEXT("Navigation remains unknown until WIT context is available."));

            AddAnalysisItem(SuggestedItems, Result, TEXT("Ready for Build"), bReadyForBuild ? TEXT("Ready") : TEXT("Limited"), bReadyForBuild ? TEXT("AI subject has the core readable systems expected for this V1 workflow.") : TEXT("Run Enhance or resolve limited systems before treating the subject as build-ready."));

            RecommendedAction = bReadyForBuild
                ? TEXT("Use Build after visual review, or Test again after changing behavior/environment context.")
                : TEXT("Run Enhance to prepare missing WanaWorks layers, then Test again before Build.");
            ResultLabel = bBehaviorExecutionSucceeded
                ? FString::Printf(
                    TEXT("%s via %s"),
                    *UIFmt::GetBehaviorPresetSummaryLabel(BehaviorSnapshot.RecommendedBehavior),
                    *UIFmt::GetBehaviorExecutionModeSummaryLabel(BehaviorSnapshot.ExecutionMode))
                : (bReadyForBuild ? TEXT("AI subject is enhanced and ready for build review") : TEXT("AI behavior readiness is limited but safe fallback data is available"));
            Result.PrimarySummary = BuildAnalysisCardText(TEXT("Character Intelligence Test Result"), PrimaryItems, RecommendedAction);
            Result.AnimationSummary = BuildAnalysisCardText(TEXT("Animation Integration Test"), AnimationItems, RecommendedAction);
            Result.PhysicalSummary = BuildAnalysisCardText(TEXT("Physical / Movement Test"), PhysicalItems, RecommendedAction);
            Result.BehaviorSummary = BuildAnalysisCardText(TEXT("Behavior Readiness Test"), BehaviorItems, RecommendedAction);
            Result.WITSummary = BuildAnalysisCardText(TEXT("WIT Environment Test"), WITItems, RecommendedAction);
            Result.SuggestedSummary = BuildAnalysisCardText(TEXT("AI Test Recommendations"), SuggestedItems, RecommendedAction);
        }
    }

    Result.StatusSummary = BuildWorkspaceTestStatusText(Result, ResultLabel);
    bWorkspaceAnalysisInitialized = true;
    LastAnalysisWorkspaceLabel = Result.WorkspaceLabel;
    LastAnalysisStatusSummary = Result.StatusSummary;
    LastAnalysisPrimarySummary = Result.PrimarySummary;
    LastAnalysisAnimationSummary = Result.AnimationSummary;
    LastAnalysisPhysicalSummary = Result.PhysicalSummary;
    LastAnalysisBehaviorSummary = Result.BehaviorSummary;
    LastAnalysisWITSummary = Result.WITSummary;
    LastAnalysisSuggestedSummary = Result.SuggestedSummary;
    StatusMessage = FString::Printf(TEXT("Status: %s"), *Result.StatusSummary);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::ApplyCharacterEnhancement()
{
    FString PresetProfileLabel;
    FString PresetProfileNotes;
    SyncSelectedWorkflowPresetIntoControls(&PresetProfileLabel, &PresetProfileNotes);

    FWanaSelectedCharacterEnhancementSnapshot SourceSnapshot;
    ResolvePreferredSubjectSnapshot(SourceSnapshot);
    const bool bHasSourceSnapshot = SourceSnapshot.bHasSelectedActor;

    const FString WorkflowLabel = UIFmt::GetCharacterEnhancementWorkflowLabel(SelectedEnhancementWorkflowLabel);
    FWanaCommandResponse WorkflowResponse;
    FWanaCommandResponse EnhancementResponse;
    FWanaCommandResponse AIReadyResponse;
    FWanaCommandResponse PreparationResponse;
    const bool bUseSandboxDuplicate = WorkflowLabel == TEXT("Create Sandbox Duplicate")
        || WorkflowLabel == TEXT("Create Working Copy");
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
        ? TEXT("Status: Prepared working copy for WanaAI testing.")
        : (bUseAIReadyPrep
            ? (bAIReadyNotApplicable
                ? TEXT("Status: Applied WanaAI enhancement with AI-ready notes.")
                : TEXT("Status: Prepared AI-ready WanaAI test subject."))
            : TEXT("Status: Applied WanaAI enhancement to the original actor."));
    Response.OutputLines.Add(FString::Printf(TEXT("Selected Actor: %s"), *SourceSnapshot.SelectedActorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Workflow Path: %s"), *WorkflowLabel));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Preset Profile: %s"),
        PresetProfileLabel.IsEmpty() ? TEXT("Current workflow controls") : *PresetProfileLabel));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Original Or Duplicate: %s"),
        (bUseSandboxDuplicate || bSpawnedFromPicker) ? TEXT("Working subject") : TEXT("Original actor")));
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
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Detected Source Anim BP:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Workspace Animation Integration Target:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Automatic Anim Integration:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Auto-Attach:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Auto-Wire:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Generated Runtime Layer:"));
        UIFmt::AppendLinesWithPrefix(Response, WorkflowResponse, TEXT("Animation Integration Notes:"));
        Response.OutputLines.Add(TEXT("Readiness Notes: The working subject was updated as the active observer."));
    }
    else if (bSpawnedFromPicker)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: WanaWorks created a working subject from the project picker before applying enhancement."));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Picked Subject Asset:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Picked Animation Blueprint:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Detected Source Anim BP:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Animation Override Applied:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Workspace Animation Integration Target:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Automatic Anim Integration:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Auto-Attach:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Auto-Wire:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Generated Runtime Layer:"));
        UIFmt::AppendLinesWithPrefix(Response, PreparationResponse, TEXT("Animation Integration Notes:"));
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

    if (!PresetProfileNotes.IsEmpty())
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Workflow Notes: %s"), *PresetProfileNotes));
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
    FString PresetProfileLabel;
    FString PresetProfileNotes;
    SyncSelectedWorkflowPresetIntoControls(&PresetProfileLabel, &PresetProfileNotes);

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

    FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteApplyStarterAndTestTargetCommand(SelectedEnhancementPresetLabel);

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

    Response.OutputLines.Insert(FString::Printf(
        TEXT("Preset Profile: %s"),
        PresetProfileLabel.IsEmpty() ? TEXT("Current workflow controls") : *PresetProfileLabel), 0);

    if (!PresetProfileNotes.IsEmpty())
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Workflow Notes: %s"), *PresetProfileNotes));
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

    const FString WorkspaceLabel = GetSelectedWorkspaceLabel();

    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase)
        || WorkspaceLabel.Equals(TEXT("Character Intelligence"), ESearchCase::IgnoreCase))
    {
        AActor* ControlObserverActor = nullptr;
        FString ObserverSourceLabel;
        ResolveCharacterIntelligenceObserverActor(ControlObserverActor, ObserverSourceLabel);

        AActor* ControlTargetActor = nullptr;
        FString TargetSourceLabel;
        ResolveCharacterIntelligenceTargetActor(ControlTargetActor, TargetSourceLabel);

        if (ControlObserverActor || ControlTargetActor)
        {
            OutObserverActor = ControlObserverActor;
            OutTargetActor = ControlTargetActor;
            OutPairSourceLabel = FString::Printf(
                TEXT("Character Intelligence controls (%s / %s)"),
                ObserverSourceLabel.IsEmpty() ? TEXT("no subject") : *ObserverSourceLabel,
                TargetSourceLabel.IsEmpty() ? TEXT("no target") : *TargetSourceLabel);
            return true;
        }
    }

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
        OutPairSourceLabel = TEXT("Working pair");
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
        OutPairSourceLabel = TEXT("Working-pair assignments");
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

void FWanaWorksUIModule::AnalyzeActiveWorkspace()
{
    const FString WorkspaceLabel = GetSelectedWorkspaceLabel();
    FWanaAnalysisResult Result;
    Result.WorkspaceLabel = WorkspaceLabel;
    Result.DisplayWorkspaceLabel = GetWorkspaceDisplayLabel(WorkspaceLabel);

    TArray<FWanaAnalysisItem> PrimaryItems;
    TArray<FWanaAnalysisItem> AnimationItems;
    TArray<FWanaAnalysisItem> PhysicalItems;
    TArray<FWanaAnalysisItem> BehaviorItems;
    TArray<FWanaAnalysisItem> WITItems;
    TArray<FWanaAnalysisItem> SuggestedItems;
    FString RecommendedAction;

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        const FString WorldLabel = GetEditorWorldLabel();
        const int32 SelectedActorCount = GetEditorSelectedActorCount();

        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("World Context"),
            WorldLabel.IsEmpty() ? TEXT("Missing") : TEXT("Ready"),
            WorldLabel.IsEmpty() ? TEXT("No editor world is available for WIT analysis yet.") : WorldLabel);
        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("Selected Actors"),
            SelectedActorCount > 0 ? TEXT("Ready") : TEXT("Limited"),
            FString::Printf(TEXT("%d actor(s) selected as optional scene context."), SelectedActorCount));

        AActor* ObserverActor = nullptr;
        AActor* TargetActor = nullptr;
        FString PairSourceLabel;
        bool bTargetFallsBackToObserver = false;
        ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

        FWanaEnvironmentReadinessSnapshot EnvironmentSnapshot;
        WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
            ObserverActor,
            TargetActor,
            bTargetFallsBackToObserver,
            PairSourceLabel,
            EnvironmentSnapshot);

        const bool bHasWITPair = HasLevelDesignWITPair(EnvironmentSnapshot);
        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("Scene Pair"),
            bHasWITPair ? TEXT("Ready") : TEXT("Limited"),
            BuildLevelDesignPairSummary(EnvironmentSnapshot));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("WIT Environment Scan"),
            bHasWITPair ? TEXT("Ready") : TEXT("Limited"),
            bHasWITPair ? TEXT("Observer-target context is available for semantic world analysis.") : TEXT("Assign an observer and target to turn selected scene context into richer WIT meaning."));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Navigation Context"),
            bHasWITPair && EnvironmentSnapshot.MovementReadiness.bHasMovementContext ? TEXT("Ready") : TEXT("Limited"),
            bHasWITPair ? UIFmt::GetNavigationContextSummaryLabel(EnvironmentSnapshot) : TEXT("Unknown until WIT has an active observer-target context."));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Cover Meaning"),
            GetLevelDesignCoverMeaningStatus(EnvironmentSnapshot, SelectedActorCount),
            GetLevelDesignCoverMeaningDetail(EnvironmentSnapshot, SelectedActorCount));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Obstacle Meaning"),
            GetLevelDesignObstacleMeaningStatus(EnvironmentSnapshot),
            GetLevelDesignObstacleMeaningDetail(EnvironmentSnapshot));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Movement Space"),
            GetLevelDesignMovementSpaceStatus(EnvironmentSnapshot),
            GetLevelDesignMovementSpaceDetail(EnvironmentSnapshot));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Boundary / Navigation"),
            GetLevelDesignBoundaryMeaningStatus(EnvironmentSnapshot),
            GetLevelDesignBoundaryMeaningDetail(EnvironmentSnapshot));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Modular Asset Context"),
            SelectedActorCount > 0 ? TEXT("Limited") : TEXT("Missing"),
            SelectedActorCount > 0 ? TEXT("Selected actors can be used as manual scene context, but modular classification is not automated yet.") : TEXT("No selected modular assets are available as scene context."));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Atmosphere / Vibe Context"),
            TEXT("Not Supported"),
            TEXT("Atmosphere analysis is reserved for a later Level Design pass."));

        RecommendedAction = bHasWITPair
            ? TEXT("Use Test to validate this semantic world reading, then Build an in-app WIT output summary before future generation work.")
            : TEXT("Assign or select an observer-target scene context, then Analyze again to classify cover, obstacles, boundaries, and movement space.");
        AddAnalysisItem(SuggestedItems, Result, TEXT("Next Improvement"), TEXT("Recommended"), RecommendedAction);

        Result.PrimarySummary = BuildAnalysisCardText(TEXT("Semantic World Context"), PrimaryItems, FString());
        Result.WITSummary = BuildAnalysisCardText(TEXT("WIT / What Is This? Diagnosis"), WITItems, RecommendedAction);
        Result.BehaviorSummary = Result.WITSummary;
        Result.AnimationSummary = Result.WITSummary;
        Result.PhysicalSummary = Result.WITSummary;
        Result.SuggestedSummary = BuildAnalysisCardText(TEXT("Suggested Level Improvements"), SuggestedItems, RecommendedAction);
    }
    else
    {
        FWanaSelectedCharacterEnhancementSnapshot Snapshot;
        const bool bHasSubject = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
        const UClass* SubjectClass = LoadSelectedSubjectActorClassForWorkspace(WorkspaceLabel);
        const AActor* AnalysisActor = bHasSubject ? ResolveAnalysisActor(Snapshot, SubjectClass) : nullptr;
        FString SkeletalMeshLabel;
        FString SkeletonLabel;
        const bool bHasSkeletalMeshAsset = GetSkeletalMeshAndSkeletonLabels(AnalysisActor, SkeletalMeshLabel, SkeletonLabel);
        const bool bHasMovementComponent = HasPawnMovementComponent(AnalysisActor);
        const bool bHasCameraOrControlComponent = ComponentClassNameContainsAny(AnalysisActor, {TEXT("Camera"), TEXT("SpringArm"), TEXT("Control"), TEXT("Input")});

        if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
        {
            AddAnalysisItem(
                PrimaryItems,
                Result,
                TEXT("Character Blueprint"),
                bHasSubject ? TEXT("Ready") : TEXT("Missing"),
                bHasSubject ? Snapshot.SelectedActorLabel : TEXT("Choose a Character Blueprint or playable Character Pawn in WanaWorks."));
            AddAnalysisItem(
                PrimaryItems,
                Result,
                TEXT("Character Profile"),
                Snapshot.bHasIdentityComponent ? TEXT("Ready") : TEXT("Missing"),
                Snapshot.bHasIdentityComponent ? TEXT("WanaWorks identity/profile layer is present.") : TEXT("Character profile is not attached yet."));
            AddAnalysisItem(
                PrimaryItems,
                Result,
                TEXT("Character Systems"),
                (Snapshot.bHasIdentityComponent && Snapshot.bHasWAYComponent) ? TEXT("Ready") : TEXT("Limited"),
                FString::Printf(
                    TEXT("Identity=%s, Relationship Context=%s, Physical State=%s"),
                    Snapshot.bHasIdentityComponent ? TEXT("Present") : TEXT("Missing"),
                    Snapshot.bHasWAYComponent ? TEXT("Present") : TEXT("Missing"),
                    Snapshot.bHasPhysicalStateComponent ? TEXT("Present") : TEXT("Missing")));

            AddAnalysisItem(
                AnimationItems,
                Result,
                TEXT("Skeletal Mesh"),
                Snapshot.bHasSkeletalMeshComponent ? TEXT("Ready") : TEXT("Missing"),
                bHasSkeletalMeshAsset ? SkeletalMeshLabel : TEXT("No skeletal mesh asset detected."));
            AddAnalysisItem(
                AnimationItems,
                Result,
                TEXT("Skeleton / Rig"),
                !SkeletonLabel.IsEmpty() && !SkeletonLabel.Equals(TEXT("(no skeleton asset)")) ? TEXT("Ready") : TEXT("Limited"),
                SkeletonLabel.IsEmpty() ? TEXT("Skeleton or rig not detected from the selected character stack.") : SkeletonLabel);
            AddAnalysisItem(
                AnimationItems,
                Result,
                TEXT("Animation Blueprint"),
                Snapshot.bHasAnimBlueprint ? TEXT("Ready") : (Snapshot.bHasSkeletalMeshComponent ? TEXT("Limited") : TEXT("Missing")),
                Snapshot.LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("No linked Animation Blueprint detected.") : Snapshot.LinkedAnimationBlueprintLabel);

            AddAnalysisItem(
                PhysicalItems,
                Result,
                TEXT("Movement Component"),
                bHasMovementComponent ? TEXT("Ready") : TEXT("Limited"),
                bHasMovementComponent ? TEXT("Pawn movement component is present for playability checks.") : TEXT("No pawn movement component was detected."));
            AddAnalysisItem(
                PhysicalItems,
                Result,
                TEXT("Camera / Control Setup"),
                bHasCameraOrControlComponent ? TEXT("Ready") : TEXT("Limited"),
                bHasCameraOrControlComponent ? TEXT("Camera, spring arm, control, or input-style component detected.") : TEXT("Camera/control setup is unknown; WanaWorks will preserve the existing project path."));
            AddAnalysisItem(
                PhysicalItems,
                Result,
                TEXT("Playable Character Readiness"),
                bHasSubject && Snapshot.bIsPawnActor && Snapshot.bHasSkeletalMeshComponent ? TEXT("Ready") : TEXT("Limited"),
                TEXT("Analyze checks playability signals without replacing input, camera, or movement architecture."));

            AddAnalysisItem(
                SuggestedItems,
                Result,
                TEXT("Build Readiness"),
                bHasSubject && Snapshot.bHasSkeletalMeshComponent && Snapshot.bHasAnimBlueprint ? TEXT("Ready") : TEXT("Limited"),
                TEXT("Final output can be prepared once the character stack has mesh, animation, and profile readiness."));
            AddAnalysisItem(
                SuggestedItems,
                Result,
                TEXT("Output Path"),
                bHasSubject ? TEXT("Ready") : TEXT("Missing"),
                bHasSubject ? TEXT("Build outputs remain separate under the WanaWorks build folder.") : TEXT("Choose a character subject before building final output."));
            AddAnalysisItem(
                SuggestedItems,
                Result,
                TEXT("Safe Enhancement"),
                (!Snapshot.bHasIdentityComponent || !Snapshot.bHasPhysicalStateComponent || !Snapshot.bHasAnimBlueprint) ? TEXT("Recommended") : TEXT("Ready"),
                TEXT("Enhance can prepare missing profile, animation-readiness, and build-output context without replacing the original asset."));

            RecommendedAction = TEXT("Apply Character Building enhancement to prepare the character profile, playability checks, rig/animation readiness, and final output path.");
            Result.PrimarySummary = BuildAnalysisCardText(TEXT("Character Asset Diagnosis"), PrimaryItems, RecommendedAction);
            Result.AnimationSummary = BuildAnalysisCardText(TEXT("Rig & Animation Readiness"), AnimationItems, RecommendedAction);
            Result.PhysicalSummary = BuildAnalysisCardText(TEXT("Movement / Playability Diagnosis"), PhysicalItems, RecommendedAction);
            Result.BehaviorSummary = Result.PrimarySummary;
            Result.WITSummary = Result.PrimarySummary;
            Result.SuggestedSummary = BuildAnalysisCardText(TEXT("Build Readiness"), SuggestedItems, RecommendedAction);
        }
        else
        {
            const UClass* AIControllerClass = GetAIControllerClassForAnalysis(AnalysisActor);
            const UObject* ControllerDefaultObject = AIControllerClass ? AIControllerClass->GetDefaultObject() : nullptr;
            FString BehaviorTreeLabel;
            FString StateTreeLabel;
            FString BlackboardLabel;
            bool bBehaviorTreePropertyFound = false;
            bool bStateTreePropertyFound = false;
            bool bBlackboardPropertyFound = false;
            const bool bBehaviorTreeDetected = TryFindLinkedObjectLabelByTokens(
                ControllerDefaultObject,
                {TEXT("BehaviorTree"), TEXT("Behavior"), TEXT("BT")},
                {TEXT("BehaviorTree")},
                BehaviorTreeLabel,
                bBehaviorTreePropertyFound);
            const bool bStateTreeDetected = TryFindLinkedObjectLabelByTokens(
                ControllerDefaultObject,
                {TEXT("StateTree")},
                {TEXT("StateTree")},
                StateTreeLabel,
                bStateTreePropertyFound);
            const bool bBlackboardDetected = TryFindLinkedObjectLabelByTokens(
                ControllerDefaultObject,
                {TEXT("Blackboard")},
                {TEXT("Blackboard")},
                BlackboardLabel,
                bBlackboardPropertyFound);

            AddAnalysisItem(
                PrimaryItems,
                Result,
                TEXT("AI Subject"),
                bHasSubject ? TEXT("Ready") : TEXT("Missing"),
                bHasSubject ? Snapshot.SelectedActorLabel : TEXT("Choose an AI Pawn or NPC subject in WanaWorks."));
            AddAnalysisItem(
                PrimaryItems,
                Result,
                TEXT("Pawn Type"),
                Snapshot.bIsPawnActor ? TEXT("Ready") : TEXT("Not Supported"),
                Snapshot.ActorTypeLabel.IsEmpty() ? TEXT("No pawn type detected.") : Snapshot.ActorTypeLabel);
            AddAnalysisItem(
                BehaviorItems,
                Result,
                TEXT("AI Controller"),
                Snapshot.bHasAIControllerClass ? TEXT("Ready") : TEXT("Missing"),
                Snapshot.LinkedAIControllerLabel.IsEmpty() ? TEXT("No linked AI Controller class detected.") : Snapshot.LinkedAIControllerLabel);
            AddAnalysisItem(
                BehaviorItems,
                Result,
                TEXT("Behavior Tree"),
                bBehaviorTreeDetected ? TEXT("Ready") : (AIControllerClass ? TEXT("Limited") : TEXT("Missing")),
                bBehaviorTreeDetected ? BehaviorTreeLabel : (bBehaviorTreePropertyFound ? TEXT("Behavior Tree property exists but no asset is assigned.") : TEXT("No Behavior Tree asset was detected on the controller defaults.")));
            AddAnalysisItem(
                BehaviorItems,
                Result,
                TEXT("State Tree"),
                bStateTreeDetected ? TEXT("Ready") : (AIControllerClass ? TEXT("Limited") : TEXT("Missing")),
                bStateTreeDetected ? StateTreeLabel : (bStateTreePropertyFound ? TEXT("State Tree property exists but no asset is assigned.") : TEXT("No State Tree asset was detected on the controller defaults.")));
            AddAnalysisItem(
                BehaviorItems,
                Result,
                TEXT("Blackboard"),
                bBlackboardDetected ? TEXT("Ready") : (AIControllerClass ? TEXT("Limited") : TEXT("Missing")),
                bBlackboardDetected ? BlackboardLabel : (bBlackboardPropertyFound ? TEXT("Blackboard property exists but no asset is assigned.") : TEXT("No Blackboard asset was detected on the controller defaults.")));
            AddAnalysisItem(
                BehaviorItems,
                Result,
                TEXT("WAI / WAMI"),
                Snapshot.bHasWAIComponent ? TEXT("Ready") : TEXT("Missing"),
                Snapshot.bHasWAIComponent ? TEXT("Personality/memory component is available.") : TEXT("AI identity, memory, emotion, and role layer is not attached yet."));
            AddAnalysisItem(
                BehaviorItems,
                Result,
                TEXT("WAY-lite Relationship"),
                Snapshot.bHasWAYComponent ? TEXT("Ready") : TEXT("Missing"),
                Snapshot.bHasWAYComponent ? TEXT("Relationship profile component is available.") : TEXT("Relationship/adaptation layer is not attached yet."));

            AddAnalysisItem(
                AnimationItems,
                Result,
                TEXT("Animation Blueprint"),
                Snapshot.bHasAnimBlueprint ? TEXT("Ready") : (Snapshot.bHasSkeletalMeshComponent ? TEXT("Limited") : TEXT("Not Supported")),
                Snapshot.LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("No linked Animation Blueprint detected.") : Snapshot.LinkedAnimationBlueprintLabel);
            AddAnalysisItem(
                AnimationItems,
                Result,
                TEXT("WanaAnimation"),
                (Snapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Applied
                    || Snapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Ready)
                    ? TEXT("Ready")
                    : (Snapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Limited ? TEXT("Limited") : TEXT("Missing")),
                Snapshot.AnimationAutomaticIntegrationDetail.IsEmpty() ? TEXT("Animation hook integration has not been prepared yet.") : Snapshot.AnimationAutomaticIntegrationDetail);

            AddAnalysisItem(
                PhysicalItems,
                Result,
                TEXT("Physical State"),
                Snapshot.bHasPhysicalStateComponent ? TEXT("Ready") : TEXT("Missing"),
                Snapshot.bHasPhysicalStateComponent ? TEXT("Readable body-state layer is available.") : TEXT("UWanaPhysicalStateComponent is missing."));
            AddAnalysisItem(
                PhysicalItems,
                Result,
                TEXT("WanaCombat-lite"),
                (Snapshot.bHasPhysicalStateComponent && Snapshot.bHasWAYComponent) ? TEXT("Ready") : TEXT("Limited"),
                TEXT("Combat-lite readiness depends on relationship context plus physical state without replacing AI logic."));
            AddAnalysisItem(
                PhysicalItems,
                Result,
                TEXT("Movement / Facing"),
                Snapshot.bIsPawnActor && (bHasMovementComponent || Snapshot.bAutoPossessAIEnabled) ? TEXT("Ready") : TEXT("Limited"),
                TEXT("Facing and movement confidence are evaluated without forcing locomotion."));

            AActor* ObserverActor = nullptr;
            AActor* TargetActor = nullptr;
            FString PairSourceLabel;
            bool bTargetFallsBackToObserver = false;
            ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

            FWanaEnvironmentReadinessSnapshot EnvironmentSnapshot;
            WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
                ObserverActor,
                TargetActor,
                bTargetFallsBackToObserver,
                PairSourceLabel,
                EnvironmentSnapshot);
            const bool bHasWITPair = EnvironmentSnapshot.bHasObserverActor && EnvironmentSnapshot.bHasTargetActor;

            FWanaBehaviorResultsSnapshot BehaviorSnapshot;
            WanaWorksUIEditorActions::GetBehaviorResultsSnapshotForActorPair(
                ObserverActor,
                TargetActor,
                bTargetFallsBackToObserver,
                PairSourceLabel,
                BehaviorSnapshot);

            AddAnalysisItem(
                WITItems,
                Result,
                TEXT("WIT Awareness"),
                bHasWITPair ? TEXT("Ready") : TEXT("Limited"),
                bHasWITPair ? UIFmt::GetEnvironmentShapingSummaryLabel(EnvironmentSnapshot.MovementReadiness) : TEXT("No active WIT observer-target context is assigned yet."));
            AddAnalysisItem(
                WITItems,
                Result,
                TEXT("Navigation Context"),
                bHasWITPair && EnvironmentSnapshot.MovementReadiness.bHasMovementContext ? TEXT("Ready") : TEXT("Limited"),
                bHasWITPair ? UIFmt::GetNavigationContextSummaryLabel(EnvironmentSnapshot) : TEXT("Unknown until WIT context is available."));
            AddAnalysisItem(
                WITItems,
                Result,
                TEXT("Behavior Results"),
                BehaviorSnapshot.bHasWAYComponent && BehaviorSnapshot.bHasRelationshipProfile ? TEXT("Ready") : TEXT("Limited"),
                BehaviorSnapshot.bHasWAYComponent ? TEXT("Relationship-aware behavior data is readable.") : TEXT("Behavior result is limited until WAY-lite is attached and evaluated."));

            AddAnalysisItem(
                SuggestedItems,
                Result,
                TEXT("Safe Enhancement"),
                (!Snapshot.bHasIdentityComponent || !Snapshot.bHasWAIComponent || !Snapshot.bHasWAYComponent || !Snapshot.bHasPhysicalStateComponent || !Snapshot.bHasAnimBlueprint)
                    ? TEXT("Recommended")
                    : TEXT("Ready"),
                TEXT("Enhance can attach missing WanaWorks layers and prepare animation/behavior hooks on the safe workflow path."));

            RecommendedAction = TEXT("Apply Character Intelligence enhancement to attach missing WanaWorks components and prepare behavior, animation, physical-state, and WIT hooks.");
            Result.PrimarySummary = BuildAnalysisCardText(TEXT("AI Subject Diagnosis"), PrimaryItems, RecommendedAction);
            Result.AnimationSummary = BuildAnalysisCardText(TEXT("Animation Readiness Diagnosis"), AnimationItems, RecommendedAction);
            Result.PhysicalSummary = BuildAnalysisCardText(TEXT("Physical / Combat-lite Diagnosis"), PhysicalItems, RecommendedAction);
            Result.BehaviorSummary = BuildAnalysisCardText(TEXT("Behavior Structure Diagnosis"), BehaviorItems, RecommendedAction);
            Result.WITSummary = BuildAnalysisCardText(TEXT("Environment Awareness Diagnosis"), WITItems, RecommendedAction);
            Result.SuggestedSummary = BuildAnalysisCardText(TEXT("Suggested AI Improvements"), SuggestedItems, RecommendedAction);
        }
    }

    Result.StatusSummary = BuildAnalysisStatusText(Result);
    bWorkspaceAnalysisInitialized = true;
    LastAnalysisWorkspaceLabel = Result.WorkspaceLabel;
    LastAnalysisStatusSummary = Result.StatusSummary;
    LastAnalysisPrimarySummary = Result.PrimarySummary;
    LastAnalysisAnimationSummary = Result.AnimationSummary;
    LastAnalysisPhysicalSummary = Result.PhysicalSummary;
    LastAnalysisBehaviorSummary = Result.BehaviorSummary;
    LastAnalysisWITSummary = Result.WITSummary;
    LastAnalysisSuggestedSummary = Result.SuggestedSummary;
    StatusMessage = FString::Printf(TEXT("Status: %s"), *Result.StatusSummary);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::UseSelectedActorAsSandboxObserver()
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;

    if (!WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(Snapshot) || !Snapshot.bHasSelectedActor)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: No selected actor to assign as the working observer.");
        Response.OutputLines.Add(TEXT("Select an actor in the editor, then click Use Selected as Observer."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    SandboxObserverActor = Snapshot.SelectedActor;
    const bool bSelfTargetDebugPair = SandboxTargetActor.IsValid() && SandboxTargetActor.Get() == Snapshot.SelectedActor.Get();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Working observer assigned.");
    Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), *Snapshot.SelectedActorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), SandboxTargetActor.IsValid() ? *SandboxTargetActor->GetActorNameOrLabel() : TEXT("(not assigned)")));
    Response.OutputLines.Add(TEXT("Readiness Notes: The working observer was assigned from the current editor selection."));

    if (!SandboxTargetActor.IsValid())
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: The working target is still missing. Use Selected as Target before evaluating the pair."));
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
        Response.StatusMessage = TEXT("Status: No selected actor to assign as the working target.");
        Response.OutputLines.Add(TEXT("Select an actor in the editor, then click Use Selected as Target."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    SandboxTargetActor = Snapshot.SelectedActor;
    const bool bSelfTargetDebugPair = SandboxObserverActor.IsValid() && SandboxObserverActor.Get() == Snapshot.SelectedActor.Get();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Working target assigned.");
    Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), SandboxObserverActor.IsValid() ? *SandboxObserverActor->GetActorNameOrLabel() : TEXT("(not assigned)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), *Snapshot.SelectedActorLabel));
    Response.OutputLines.Add(TEXT("Readiness Notes: The working target was assigned from the current editor selection."));

    if (!SandboxObserverActor.IsValid())
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: The working observer is still missing. Use Selected as Observer before evaluating the pair."));
    }
    else if (bSelfTargetDebugPair)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Observer and target are the same actor. Self-target evaluation is active as a debug case."));
    }
    else
    {
    Response.OutputLines.Add(TEXT("Readiness Notes: The working pair is ready to evaluate with separate observer and target assignments."));
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
            ? TEXT("Status: Working observer is missing.")
            : TEXT("Status: Working target is missing.");
        Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), ObserverActor ? *ObserverActor->GetActorNameOrLabel() : TEXT("(not assigned)")));
        Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), TargetActor ? *TargetActor->GetActorNameOrLabel() : TEXT("(not assigned)")));
        Response.OutputLines.Add(TEXT("Readiness Notes: Working-pair evaluation uses explicit assignments only."));
        Response.OutputLines.Add(!ObserverActor
            ? TEXT("Readiness Notes: Use Selected as Observer before evaluating the working pair.")
            : TEXT("Readiness Notes: Use Selected as Target before evaluating the working pair."));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    const bool bIsSelfTargetDebugPair = ObserverActor == TargetActor;
    Response = WanaWorksUIEditorActions::ExecuteEvaluateActorPairCommand(ObserverActor, TargetActor, false);

    if (Response.bSucceeded)
    {
    Response.OutputLines.Add(TEXT("Readiness Notes: Observer Source: Stored working observer."));
    Response.OutputLines.Add(TEXT("Readiness Notes: Target Source: Stored working target."));
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
    const FString WorkspaceLabel = GetSelectedWorkspaceLabel();
    const FString DisplayWorkspaceLabel = GetWorkspaceDisplayLabel(WorkspaceLabel);

    if (!HasCurrentWorkspaceAnalysis())
    {
        AnalyzeActiveWorkspace();
    }

    const bool bHadTestResult = HasCurrentWorkspaceAnalysis()
        && LastAnalysisStatusSummary.Contains(TEXT("test complete"), ESearchCase::IgnoreCase);

    if (!bHadTestResult)
    {
        TestActiveWorkspace();
    }

    FWanaAnalysisResult Result;
    Result.WorkspaceLabel = WorkspaceLabel;
    Result.DisplayWorkspaceLabel = DisplayWorkspaceLabel;

    TArray<FWanaAnalysisItem> PrimaryItems;
    TArray<FWanaAnalysisItem> AnimationItems;
    TArray<FWanaAnalysisItem> PhysicalItems;
    TArray<FWanaAnalysisItem> BehaviorItems;
    TArray<FWanaAnalysisItem> WITItems;
    TArray<FWanaAnalysisItem> SuggestedItems;

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        const FString WorldLabel = GetEditorWorldLabel();
        const int32 SelectedActorCount = GetEditorSelectedActorCount();
        AActor* ObserverActor = nullptr;
        AActor* TargetActor = nullptr;
        FString PairSourceLabel;
        bool bTargetFallsBackToObserver = false;
        ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

        FWanaEnvironmentReadinessSnapshot EnvironmentSnapshot;
        WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
            ObserverActor,
            TargetActor,
            bTargetFallsBackToObserver,
            PairSourceLabel,
            EnvironmentSnapshot);

        const bool bHasWorldContext = !WorldLabel.IsEmpty();
        const bool bHasWITPair = HasLevelDesignWITPair(EnvironmentSnapshot);
        const bool bHasMovementContext = bHasWITPair && EnvironmentSnapshot.MovementReadiness.bHasMovementContext;
        const FString ReportPath = TEXT("/Game/WanaWorks/Builds");
        const FString BuildState = bHasWorldContext ? TEXT("Built with Limitations") : TEXT("Blocked");
        const FString RecommendedAction = bHasWorldContext
            ? TEXT("Use this semantic-world build summary as the V1 output, then run a later WIT pass for persisted cover, obstacle, and movement-space assets.")
            : TEXT("Open a valid editor world before preparing a Level Design build summary.");

        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("World Context"),
            bHasWorldContext ? TEXT("Ready") : TEXT("Blocked"),
            bHasWorldContext ? WorldLabel : TEXT("No editor world is available for Level Design build output."));
        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("Selected Actors"),
            SelectedActorCount > 0 ? TEXT("Ready") : TEXT("Limited"),
            FString::Printf(TEXT("%d actor(s) captured as optional scene context."), SelectedActorCount));
        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("Scene Pair"),
            bHasWITPair ? TEXT("Ready") : TEXT("Limited"),
            BuildLevelDesignPairSummary(EnvironmentSnapshot));
        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("Output / Report"),
            bHasWorldContext ? TEXT("Built with Limitations") : TEXT("Blocked"),
            bHasWorldContext ? TEXT("In-app WIT semantic-world summary prepared. No level assets were generated in Build V1.") : TEXT("No report was prepared because world context is missing."));
        AddAnalysisItem(
            PrimaryItems,
            Result,
            TEXT("Output Location"),
            TEXT("Limited"),
            FString::Printf(TEXT("%s is reserved for future persisted WIT report assets; this phase reports in-app only."), *ReportPath));

        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("WIT Preparation"),
            bHasWITPair ? TEXT("Ready") : TEXT("Limited"),
            bHasWITPair ? TEXT("Semantic scan context is available for WIT world understanding.") : TEXT("WIT scan context is limited; no cover/obstacle report asset was generated."));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Navigation Context"),
            bHasMovementContext ? TEXT("Ready") : TEXT("Limited"),
            bHasWITPair ? UIFmt::GetNavigationContextSummaryLabel(EnvironmentSnapshot) : TEXT("Navigation context remains unknown until a WIT movement pair is available."));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Cover Meaning"),
            GetLevelDesignCoverMeaningStatus(EnvironmentSnapshot, SelectedActorCount),
            GetLevelDesignCoverMeaningDetail(EnvironmentSnapshot, SelectedActorCount));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Obstacle Meaning"),
            GetLevelDesignObstacleMeaningStatus(EnvironmentSnapshot),
            GetLevelDesignObstacleMeaningDetail(EnvironmentSnapshot));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Movement Space"),
            GetLevelDesignMovementSpaceStatus(EnvironmentSnapshot),
            GetLevelDesignMovementSpaceDetail(EnvironmentSnapshot));
        AddAnalysisItem(
            WITItems,
            Result,
            TEXT("Full Level Generation"),
            TEXT("Not Supported"),
            TEXT("Prompt-based modular level generation is not implemented in Build V1."));

        AddAnalysisItem(SuggestedItems, Result, TEXT("Build State"), BuildState, bHasWorldContext ? TEXT("Semantic world output summary is ready in WanaWorks.") : TEXT("Build is blocked until an editor world is available."));
        AddAnalysisItem(SuggestedItems, Result, TEXT("Original Level"), TEXT("Ready"), TEXT("Preserved. Build V1 did not spawn or move level actors."));
        AddAnalysisItem(SuggestedItems, Result, TEXT("Recommended Next Step"), TEXT("Recommended"), RecommendedAction);

        Result.PrimarySummary = BuildAnalysisCardText(TEXT("Semantic World Build Summary"), PrimaryItems, RecommendedAction);
        Result.WITSummary = BuildAnalysisCardText(TEXT("WIT Output Readiness"), WITItems, RecommendedAction);
        Result.AnimationSummary = Result.WITSummary;
        Result.PhysicalSummary = Result.WITSummary;
        Result.BehaviorSummary = Result.WITSummary;
        Result.SuggestedSummary = BuildAnalysisCardText(TEXT("Level Design Build Result"), SuggestedItems, RecommendedAction);
        Result.StatusSummary = BuildWorkspaceBuildStatusText(DisplayWorkspaceLabel, BuildState, bHasWorldContext ? TEXT("in-app WIT summary prepared") : FString());
        bWorkspaceAnalysisInitialized = true;
        LastAnalysisWorkspaceLabel = Result.WorkspaceLabel;
        LastAnalysisStatusSummary = Result.StatusSummary;
        LastAnalysisPrimarySummary = Result.PrimarySummary;
        LastAnalysisAnimationSummary = Result.AnimationSummary;
        LastAnalysisPhysicalSummary = Result.PhysicalSummary;
        LastAnalysisBehaviorSummary = Result.BehaviorSummary;
        LastAnalysisWITSummary = Result.WITSummary;
        LastAnalysisSuggestedSummary = Result.SuggestedSummary;
        StatusMessage = FString::Printf(TEXT("Status: %s"), *Result.StatusSummary);
        RefreshReactiveUI(true);
        return;
    }

    FWanaSelectedCharacterEnhancementSnapshot SourceSnapshot;
    const bool bHasSourceSnapshot = ResolvePreferredSubjectSnapshot(SourceSnapshot) && SourceSnapshot.bHasSelectedActor;

    FWanaCommandResponse PreparationResponse;
    bool bSpawnedFromPicker = false;

    if (!PreparePickerDrivenSubjectForWorkflow(TEXT("Build Final"), PreparationResponse, bSpawnedFromPicker))
    {
        ApplyResponse(PreparationResponse);
        RefreshReactiveUI(true);
        return;
    }

    FWanaSelectedCharacterEnhancementSnapshot WorkingSnapshot;
    const bool bHasWorkingSnapshot =
        WanaWorksUIEditorActions::GetSelectedCharacterEnhancementSnapshot(WorkingSnapshot)
        && WorkingSnapshot.bHasSelectedActor
        && WorkingSnapshot.SelectedActor.IsValid();

    AActor* BuildSourceActor = nullptr;

    if (SandboxObserverActor.IsValid())
    {
        BuildSourceActor = SandboxObserverActor.Get();
    }
    else if (bHasWorkingSnapshot)
    {
        BuildSourceActor = WorkingSnapshot.SelectedActor.Get();
    }

    if (!BuildSourceActor)
    {
        FWanaCommandResponse Response;
        Response.StatusMessage = TEXT("Status: Build blocked. No working subject available.");
        Response.OutputLines.Add(TEXT("Choose a subject or run Enhance before building the final output asset."));
        ApplyResponse(Response);

        AddAnalysisItem(PrimaryItems, Result, TEXT("Build Source"), TEXT("Blocked"), TEXT("No compatible working or selected subject is available."));
        AddAnalysisItem(SuggestedItems, Result, TEXT("Recommended Next Step"), TEXT("Recommended"), TEXT("Select a subject, run Enhance or Test, then Build again."));
        Result.PrimarySummary = BuildAnalysisCardText(TEXT("Build Blocked"), PrimaryItems, TEXT("Select a subject, run Enhance or Test, then Build again."));
        Result.AnimationSummary = Result.PrimarySummary;
        Result.PhysicalSummary = Result.PrimarySummary;
        Result.BehaviorSummary = Result.PrimarySummary;
        Result.WITSummary = Result.PrimarySummary;
        Result.SuggestedSummary = BuildAnalysisCardText(TEXT("Build Result"), SuggestedItems, FString());
        Result.StatusSummary = BuildWorkspaceBuildStatusText(DisplayWorkspaceLabel, TEXT("Blocked"), FString());
        bWorkspaceAnalysisInitialized = true;
        LastAnalysisWorkspaceLabel = Result.WorkspaceLabel;
        LastAnalysisStatusSummary = Result.StatusSummary;
        LastAnalysisPrimarySummary = Result.PrimarySummary;
        LastAnalysisAnimationSummary = Result.AnimationSummary;
        LastAnalysisPhysicalSummary = Result.PhysicalSummary;
        LastAnalysisBehaviorSummary = Result.BehaviorSummary;
        LastAnalysisWITSummary = Result.WITSummary;
        LastAnalysisSuggestedSummary = Result.SuggestedSummary;
        StatusMessage = FString::Printf(TEXT("Status: %s"), *Result.StatusSummary);
        RefreshReactiveUI(true);
        return;
    }

    FWanaCommandResponse BuildResponse = WanaWorksUIEditorActions::ExecuteCreateFinalizedBuildAssetFromActorCommand(BuildSourceActor);

    if (bSpawnedFromPicker)
    {
        BuildResponse.OutputLines.Insert(TEXT("Workspace Notes: WanaWorks created a working subject from the project picker before building the final asset."), 1);
    }

    const FString OutputName = ExtractResponseOutputValue(BuildResponse, TEXT("Finalized Output"));
    const FString OutputPath = ExtractResponseOutputValue(BuildResponse, TEXT("Finalized Output Path"));
    FString OutputDetail = TEXT("No output path was returned.");
    if (!OutputPath.IsEmpty())
    {
        OutputDetail = FString::Printf(
            TEXT("%s saved at %s"),
            OutputName.IsEmpty() ? TEXT("Finalized Blueprint") : *OutputName,
            *OutputPath);
    }
    const bool bCharacterBuildingWorkspace = WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase);
    const FWanaSelectedCharacterEnhancementSnapshot& EffectiveSnapshot = bHasWorkingSnapshot ? WorkingSnapshot : SourceSnapshot;
    const bool bHasEffectiveSnapshot = bHasWorkingSnapshot || bHasSourceSnapshot;
    const AActor* AnalysisActor = bHasEffectiveSnapshot ? ResolveAnalysisActor(EffectiveSnapshot, EffectiveSnapshot.SelectedActor.IsValid() ? EffectiveSnapshot.SelectedActor->GetClass() : nullptr) : BuildSourceActor;
    FString SkeletalMeshLabel;
    FString SkeletonLabel;
    const bool bHasSkeletalMeshAsset = GetSkeletalMeshAndSkeletonLabels(AnalysisActor, SkeletalMeshLabel, SkeletonLabel);
    const bool bHasMovementComponent = HasPawnMovementComponent(AnalysisActor);
    const bool bHasCameraOrControlComponent = ComponentClassNameContainsAny(AnalysisActor, {TEXT("Camera"), TEXT("SpringArm"), TEXT("Control"), TEXT("Input")});
    const bool bTestWasAvailable = LastAnalysisStatusSummary.Contains(TEXT("test complete"), ESearchCase::IgnoreCase);
    const bool bAnimationReady = EffectiveSnapshot.bHasAnimBlueprint
        || EffectiveSnapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Applied
        || EffectiveSnapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Ready;

    bool bBuildHasLimitations = !BuildResponse.bSucceeded || !bTestWasAvailable;

    if (bCharacterBuildingWorkspace)
    {
        bBuildHasLimitations = bBuildHasLimitations
            || !EffectiveSnapshot.bHasSkeletalMeshComponent
            || !EffectiveSnapshot.bHasAnimBlueprint
            || !bHasMovementComponent
            || !bHasCameraOrControlComponent;
    }
    else
    {
        bBuildHasLimitations = bBuildHasLimitations
            || !EffectiveSnapshot.bHasAIControllerClass
            || !EffectiveSnapshot.bHasWAIComponent
            || !EffectiveSnapshot.bHasWAYComponent
            || !EffectiveSnapshot.bHasPhysicalStateComponent
            || !bAnimationReady;
    }

    const FString BuildState = !BuildResponse.bSucceeded
        ? FString(TEXT("Blocked"))
        : (bBuildHasLimitations ? FString(TEXT("Built with Limitations")) : FString(TEXT("Built")));
    const FString RecommendedAction = BuildResponse.bSucceeded
        ? TEXT("Review the finalized output in /Game/WanaWorks/Builds, then iterate with Enhance/Test if any limited systems matter for this subject.")
        : TEXT("Resolve the blocked build condition, then run Build again.");

    if (bCharacterBuildingWorkspace)
    {
        AddAnalysisItem(PrimaryItems, Result, TEXT("Source Character"), bHasSourceSnapshot ? TEXT("Ready") : TEXT("Limited"), bHasSourceSnapshot ? SourceSnapshot.SelectedActorLabel : TEXT("Source subject was inferred from the active working subject."));
        AddAnalysisItem(PrimaryItems, Result, TEXT("Working Character Used"), TEXT("Ready"), BuildSourceActor->GetActorNameOrLabel());
        AddAnalysisItem(PrimaryItems, Result, TEXT("Original Source"), TEXT("Ready"), TEXT("Preserved. Build created a separate finalized output."));
        AddAnalysisItem(PrimaryItems, Result, TEXT("Final Output"), BuildResponse.bSucceeded ? BuildState : TEXT("Blocked"), OutputDetail);

        AddAnalysisItem(AnimationItems, Result, TEXT("Skeletal Mesh"), EffectiveSnapshot.bHasSkeletalMeshComponent ? TEXT("Ready") : TEXT("Limited"), bHasSkeletalMeshAsset ? SkeletalMeshLabel : TEXT("No skeletal mesh asset detected."));
        AddAnalysisItem(AnimationItems, Result, TEXT("Skeleton / Rig"), !SkeletonLabel.IsEmpty() && !SkeletonLabel.Equals(TEXT("(no skeleton asset)")) ? TEXT("Ready") : TEXT("Limited"), SkeletonLabel.IsEmpty() ? TEXT("Skeleton or rig could not be confirmed.") : SkeletonLabel);
        AddAnalysisItem(AnimationItems, Result, TEXT("Animation Blueprint"), EffectiveSnapshot.bHasAnimBlueprint ? TEXT("Ready") : TEXT("Limited"), EffectiveSnapshot.LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("No linked Animation Blueprint detected.") : EffectiveSnapshot.LinkedAnimationBlueprintLabel);

        AddAnalysisItem(PhysicalItems, Result, TEXT("Movement / Playability"), bHasMovementComponent ? TEXT("Ready") : TEXT("Limited"), bHasMovementComponent ? TEXT("Movement component is present.") : TEXT("Movement component was not detected."));
        AddAnalysisItem(PhysicalItems, Result, TEXT("Camera / Control"), bHasCameraOrControlComponent ? TEXT("Ready") : TEXT("Limited"), bHasCameraOrControlComponent ? TEXT("Camera, spring arm, control, or input-style component detected.") : TEXT("Camera/control setup is unknown and was preserved."));
        AddAnalysisItem(BehaviorItems, Result, TEXT("Character Profile"), EffectiveSnapshot.bHasIdentityComponent ? TEXT("Ready") : TEXT("Limited"), EffectiveSnapshot.bHasIdentityComponent ? TEXT("Character profile/readiness layer is present.") : TEXT("Character profile was not detected."));
        AddAnalysisItem(BehaviorItems, Result, TEXT("Character Systems"), EffectiveSnapshot.bHasIdentityComponent && EffectiveSnapshot.bHasWAYComponent ? TEXT("Ready") : TEXT("Limited"), TEXT("Character-side identity and relationship context remain additive."));

        AddAnalysisItem(SuggestedItems, Result, TEXT("Build Status"), BuildState, BuildResponse.bSucceeded ? TEXT("Character build output completed without touching the original.") : TEXT("Character build did not complete."));
        AddAnalysisItem(SuggestedItems, Result, TEXT("Output Path"), BuildResponse.bSucceeded ? TEXT("Built") : TEXT("Blocked"), OutputPath.IsEmpty() ? TEXT("/Game/WanaWorks/Builds") : OutputPath);
        AddAnalysisItem(SuggestedItems, Result, TEXT("Validation"), bTestWasAvailable ? TEXT("Ready") : TEXT("Limited"), bTestWasAvailable ? TEXT("Test readiness was available before Build.") : TEXT("Build used lightweight readiness because Test had not been run yet."));

        Result.PrimarySummary = BuildAnalysisCardText(TEXT("Character Build Output"), PrimaryItems, RecommendedAction);
        Result.AnimationSummary = BuildAnalysisCardText(TEXT("Rig & Animation Build Readiness"), AnimationItems, RecommendedAction);
        Result.PhysicalSummary = BuildAnalysisCardText(TEXT("Movement / Playability Build Readiness"), PhysicalItems, RecommendedAction);
        Result.BehaviorSummary = BuildAnalysisCardText(TEXT("Character Systems Build Readiness"), BehaviorItems, RecommendedAction);
        Result.WITSummary = Result.BehaviorSummary;
        Result.SuggestedSummary = BuildAnalysisCardText(TEXT("Character Build Result"), SuggestedItems, RecommendedAction);
    }
    else
    {
        AActor* ObserverActor = nullptr;
        AActor* TargetActor = nullptr;
        FString PairSourceLabel;
        bool bTargetFallsBackToObserver = false;
        ResolvePreferredWITReadinessPair(ObserverActor, TargetActor, PairSourceLabel, bTargetFallsBackToObserver);

        FWanaEnvironmentReadinessSnapshot EnvironmentSnapshot;
        WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
            ObserverActor,
            TargetActor,
            bTargetFallsBackToObserver,
            PairSourceLabel,
            EnvironmentSnapshot);

        FWanaBehaviorResultsSnapshot BehaviorSnapshot;
        WanaWorksUIEditorActions::GetBehaviorResultsSnapshotForActorPair(
            ObserverActor,
            TargetActor,
            bTargetFallsBackToObserver,
            PairSourceLabel,
            BehaviorSnapshot);

        const bool bHasWITPair = EnvironmentSnapshot.bHasObserverActor && EnvironmentSnapshot.bHasTargetActor;

        AddAnalysisItem(PrimaryItems, Result, TEXT("Source AI / NPC"), bHasSourceSnapshot ? TEXT("Ready") : TEXT("Limited"), bHasSourceSnapshot ? SourceSnapshot.SelectedActorLabel : TEXT("Source subject was inferred from the active working subject."));
        AddAnalysisItem(PrimaryItems, Result, TEXT("Working AI Used"), TEXT("Ready"), BuildSourceActor->GetActorNameOrLabel());
        AddAnalysisItem(PrimaryItems, Result, TEXT("Original Source"), TEXT("Ready"), TEXT("Preserved. Build created a separate finalized output."));
        AddAnalysisItem(PrimaryItems, Result, TEXT("Final Output"), BuildResponse.bSucceeded ? BuildState : TEXT("Blocked"), OutputDetail);

        AddAnalysisItem(BehaviorItems, Result, TEXT("AI Controller"), EffectiveSnapshot.bHasAIControllerClass ? TEXT("Ready") : TEXT("Limited"), EffectiveSnapshot.LinkedAIControllerLabel.IsEmpty() ? TEXT("No linked AI Controller detected.") : EffectiveSnapshot.LinkedAIControllerLabel);
        AddAnalysisItem(BehaviorItems, Result, TEXT("WAI / WAMI"), EffectiveSnapshot.bHasWAIComponent ? TEXT("Ready") : TEXT("Limited"), EffectiveSnapshot.bHasWAIComponent ? TEXT("Identity, memory, emotion, and role layer is present.") : TEXT("WAI/WAMI was not detected."));
        AddAnalysisItem(BehaviorItems, Result, TEXT("WAY-lite"), EffectiveSnapshot.bHasWAYComponent ? TEXT("Ready") : TEXT("Limited"), EffectiveSnapshot.bHasWAYComponent ? TEXT("Relationship/adaptation layer is present.") : TEXT("WAY-lite was not detected."));
        AddAnalysisItem(BehaviorItems, Result, TEXT("Behavior Results"), BehaviorSnapshot.bHasWAYComponent && BehaviorSnapshot.bHasRelationshipProfile ? TEXT("Ready") : TEXT("Limited"), BehaviorSnapshot.bHasWAYComponent ? TEXT("Relationship-aware behavior data is readable.") : TEXT("Behavior result is limited until WAY-lite data is available."));

        AddAnalysisItem(AnimationItems, Result, TEXT("Animation Blueprint"), EffectiveSnapshot.bHasAnimBlueprint ? TEXT("Ready") : TEXT("Limited"), EffectiveSnapshot.LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("No linked Animation Blueprint detected.") : EffectiveSnapshot.LinkedAnimationBlueprintLabel);
        AddAnalysisItem(AnimationItems, Result, TEXT("WanaAnimation"), bAnimationReady ? TEXT("Ready") : TEXT("Limited"), EffectiveSnapshot.AnimationAutomaticIntegrationDetail.IsEmpty() ? TEXT("Animation integration is limited or not prepared.") : EffectiveSnapshot.AnimationAutomaticIntegrationDetail);
        AddAnalysisItem(PhysicalItems, Result, TEXT("Physical State"), EffectiveSnapshot.bHasPhysicalStateComponent ? TEXT("Ready") : TEXT("Limited"), EffectiveSnapshot.bHasPhysicalStateComponent ? TEXT("Readable body-state layer is available.") : TEXT("Physical state layer was not detected."));
        AddAnalysisItem(PhysicalItems, Result, TEXT("WanaCombat-lite"), EffectiveSnapshot.bHasPhysicalStateComponent && EffectiveSnapshot.bHasWAYComponent ? TEXT("Ready") : TEXT("Limited"), TEXT("Combat-lite readiness stays additive and does not replace AI logic."));

        AddAnalysisItem(WITItems, Result, TEXT("WIT Awareness"), bHasWITPair ? TEXT("Ready") : TEXT("Limited"), bHasWITPair ? UIFmt::GetEnvironmentShapingSummaryLabel(EnvironmentSnapshot.MovementReadiness) : TEXT("WIT observer-target context is not assigned yet."));
        AddAnalysisItem(WITItems, Result, TEXT("Navigation Context"), bHasWITPair && EnvironmentSnapshot.MovementReadiness.bHasMovementContext ? TEXT("Ready") : TEXT("Limited"), bHasWITPair ? UIFmt::GetNavigationContextSummaryLabel(EnvironmentSnapshot) : TEXT("Navigation context remains unknown until WIT context is available."));

        AddAnalysisItem(SuggestedItems, Result, TEXT("Build Status"), BuildState, BuildResponse.bSucceeded ? TEXT("Character Intelligence output completed without touching the original.") : TEXT("Character Intelligence build did not complete."));
        AddAnalysisItem(SuggestedItems, Result, TEXT("Output Path"), BuildResponse.bSucceeded ? TEXT("Built") : TEXT("Blocked"), OutputPath.IsEmpty() ? TEXT("/Game/WanaWorks/Builds") : OutputPath);
        AddAnalysisItem(SuggestedItems, Result, TEXT("Validation"), bTestWasAvailable ? TEXT("Ready") : TEXT("Limited"), bTestWasAvailable ? TEXT("Test readiness was available before Build.") : TEXT("Build used lightweight readiness because Test had not been run yet."));

        Result.PrimarySummary = BuildAnalysisCardText(TEXT("Character Intelligence Build Output"), PrimaryItems, RecommendedAction);
        Result.AnimationSummary = BuildAnalysisCardText(TEXT("WanaAnimation Build Readiness"), AnimationItems, RecommendedAction);
        Result.PhysicalSummary = BuildAnalysisCardText(TEXT("Physical / Combat-lite Build Readiness"), PhysicalItems, RecommendedAction);
        Result.BehaviorSummary = BuildAnalysisCardText(TEXT("AI Systems Build Readiness"), BehaviorItems, RecommendedAction);
        Result.WITSummary = BuildAnalysisCardText(TEXT("WIT Build Readiness"), WITItems, RecommendedAction);
        Result.SuggestedSummary = BuildAnalysisCardText(TEXT("AI Build Result"), SuggestedItems, RecommendedAction);
    }

    Result.StatusSummary = BuildWorkspaceBuildStatusText(DisplayWorkspaceLabel, BuildState, OutputPath);
    BuildResponse.StatusMessage = FString::Printf(TEXT("Status: %s"), *Result.StatusSummary);
    BuildResponse.OutputLines.Insert(FString::Printf(TEXT("Workspace Build State: %s"), *BuildState), 0);
    BuildResponse.OutputLines.Insert(FString::Printf(TEXT("Workspace: %s"), *DisplayWorkspaceLabel), 0);

    ApplyResponse(BuildResponse);

    bWorkspaceAnalysisInitialized = true;
    LastAnalysisWorkspaceLabel = Result.WorkspaceLabel;
    LastAnalysisStatusSummary = Result.StatusSummary;
    LastAnalysisPrimarySummary = Result.PrimarySummary;
    LastAnalysisAnimationSummary = Result.AnimationSummary;
    LastAnalysisPhysicalSummary = Result.PhysicalSummary;
    LastAnalysisBehaviorSummary = Result.BehaviorSummary;
    LastAnalysisWITSummary = Result.WITSummary;
    LastAnalysisSuggestedSummary = Result.SuggestedSummary;
    StatusMessage = FString::Printf(TEXT("Status: %s"), *Result.StatusSummary);
    RefreshReactiveUI(true);
}

void FWanaWorksUIModule::FocusSandboxPreviewSubject()
{
    FString PreviewModeLabel;
    AActor* PreviewActor = nullptr;

    if (ResolvePreferredSandboxPreviewActor(PreviewActor, PreviewModeLabel) && PreviewActor)
    {
        const FWanaCommandResponse Response = WanaWorksUIEditorActions::ExecuteFocusActorCommand(PreviewActor, TEXT("Preview Subject"));
        ApplyResponse(Response);
        RefreshReactiveUI(true);
        return;
    }

    FWanaCommandResponse Response;
    Response.StatusMessage = TEXT("Status: No live preview subject is available.");
    Response.OutputLines.Add(TEXT("The embedded preview is currently showing a picker-driven asset card. Generate or select a working subject to focus it in the main editor viewport."));
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

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedCharacterIntelligenceIdentityRoleOption() const
{
    const FString SelectedLabel = SelectedCharacterIntelligenceIdentityRoleLabel.IsEmpty()
        ? TEXT("Neutral")
        : SelectedCharacterIntelligenceIdentityRoleLabel;
    return FindStringOptionByLabel(CharacterIntelligenceIdentityRoleOptions, SelectedLabel);
}

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedCharacterIntelligenceRelationshipOption() const
{
    const FString SelectedLabel = SelectedCharacterIntelligenceRelationshipLabel.IsEmpty()
        ? TEXT("Unknown")
        : SelectedCharacterIntelligenceRelationshipLabel;
    return FindStringOptionByLabel(CharacterIntelligenceRelationshipOptions, SelectedLabel);
}

TSharedPtr<FString> FWanaWorksUIModule::GetSelectedCharacterIntelligenceTargetOption() const
{
    const FString SelectedLabel = SelectedCharacterIntelligenceTargetLabel.IsEmpty()
        ? TEXT("No Target")
        : SelectedCharacterIntelligenceTargetLabel;
    return FindStringOptionByLabel(CharacterIntelligenceTargetOptions, SelectedLabel);
}

bool FWanaWorksUIModule::HasCurrentWorkspaceAnalysis() const
{
    return bWorkspaceAnalysisInitialized
        && LastAnalysisWorkspaceLabel.Equals(GetSelectedWorkspaceLabel(), ESearchCase::IgnoreCase);
}

FText FWanaWorksUIModule::GetStatusText() const
{
    return FText::FromString(StatusMessage);
}

FString FWanaWorksUIModule::GetSelectedWorkspaceLabel() const
{
    return SelectedWorkspaceLabel.IsEmpty() ? TEXT("AI") : SelectedWorkspaceLabel;
}

FString FWanaWorksUIModule::GetSelectedPreviewStageViewLabel() const
{
    return SelectedPreviewStageViewLabel.IsEmpty() ? TEXT("Overview") : SelectedPreviewStageViewLabel;
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

FText FWanaWorksUIModule::GetSandboxPreviewSummaryText() const
{
    if (GetSelectedWorkspaceLabel().Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        if (HasCurrentWorkspaceAnalysis() && !LastAnalysisPrimarySummary.IsEmpty())
        {
            return FText::FromString(LastAnalysisPrimarySummary);
        }

        return FText::FromString(TEXT("Stage: Semantic World Stage Ready\nContext: Level Design / WIT\nPreview: Environment context\nNext Step: Scan environment or select modular assets to begin."));
    }

    if (HasCurrentWorkspaceAnalysis()
        && LastAnalysisStatusSummary.Contains(TEXT("test complete"), ESearchCase::IgnoreCase)
        && !LastAnalysisPrimarySummary.IsEmpty())
    {
        return FText::FromString(LastAnalysisPrimarySummary);
    }

    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    FString PreviewModeLabel;
    AActor* PreviewActor = nullptr;
    const bool bHasLivePreviewSubject = ResolvePreferredSandboxPreviewActor(PreviewActor, PreviewModeLabel) && PreviewActor != nullptr;
    UObject* PreviewObject = GetSandboxPreviewObject();
    const FString PreviewAssetLabel = GetPreviewObjectDisplayLabel(PreviewObject);

    return FText::FromString(UISummary::BuildSandboxPreviewSummaryText(
        bHasSnapshot ? &Snapshot : nullptr,
        PreviewModeLabel,
        PreviewAssetLabel,
        bHasLivePreviewSubject));
}

FText FWanaWorksUIModule::GetAnimationIntegrationText() const
{
    if (HasCurrentWorkspaceAnalysis() && !LastAnalysisAnimationSummary.IsEmpty())
    {
        return FText::FromString(LastAnalysisAnimationSummary);
    }

    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildAnimationIntegrationSummaryText(bHasSnapshot ? &Snapshot : nullptr));
}

FText FWanaWorksUIModule::GetAnimationHookUsageText() const
{
    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildAnimationHookUsageText(bHasSnapshot ? &Snapshot : nullptr));
}

FText FWanaWorksUIModule::GetPhysicalStateText() const
{
    if (HasCurrentWorkspaceAnalysis() && !LastAnalysisPhysicalSummary.IsEmpty())
    {
        return FText::FromString(LastAnalysisPhysicalSummary);
    }

    FWanaSelectedCharacterEnhancementSnapshot Snapshot;
    const bool bHasSnapshot = ResolvePreferredSubjectSnapshot(Snapshot) && Snapshot.bHasSelectedActor;
    return FText::FromString(UISummary::BuildPhysicalStateSummaryText(bHasSnapshot ? &Snapshot : nullptr));
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
    if (HasCurrentWorkspaceAnalysis() && !LastAnalysisPrimarySummary.IsEmpty())
    {
        return FText::FromString(LastAnalysisPrimarySummary);
    }

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
    if (HasCurrentWorkspaceAnalysis() && !LastAnalysisSuggestedSummary.IsEmpty())
    {
        return FText::FromString(LastAnalysisSuggestedSummary);
    }

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
    return FText::FromString(TEXT("Test uses the current preset-aware setup, runs the guided reaction pass, and evaluates the live subject without making you step through the helper controls manually."));
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
    if (HasCurrentWorkspaceAnalysis() && !LastAnalysisBehaviorSummary.IsEmpty())
    {
        return FText::FromString(LastAnalysisBehaviorSummary);
    }

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
    if (HasCurrentWorkspaceAnalysis() && !LastAnalysisWITSummary.IsEmpty())
    {
        return FText::FromString(LastAnalysisWITSummary);
    }

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

FText FWanaWorksUIModule::GetCharacterIntelligenceControlSummaryText() const
{
    AActor* ObserverActor = nullptr;
    FString ObserverSourceLabel;
    ResolveCharacterIntelligenceObserverActor(ObserverActor, ObserverSourceLabel);

    AActor* TargetActor = nullptr;
    FString TargetSourceLabel;
    ResolveCharacterIntelligenceTargetActor(TargetActor, TargetSourceLabel);

    const UWanaIdentityComponent* IdentityComponent = IsValid(ObserverActor)
        ? ObserverActor->FindComponentByClass<UWanaIdentityComponent>()
        : nullptr;
    const UWAYPlayerProfileComponent* ProfileComponent = IsValid(ObserverActor)
        ? ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>()
        : nullptr;
    const FString RecommendationLabel = GetCharacterIntelligenceRecommendationLabel(
        SelectedCharacterIntelligenceIdentityRoleLabel,
        SelectedCharacterIntelligenceRelationshipLabel,
        TargetActor != nullptr);

    FString Summary = FString::Printf(
        TEXT("AI Subject: %s\nSubject Source: %s\nAI Identity Role: %s\nWAI / WAMI: %s\nRelationship Target: %s\nTarget Source: %s\nWAY Relationship: %s\nWAY-lite: %s\nCurrent Recommendation: %s"),
        IsValid(ObserverActor) ? *ObserverActor->GetActorNameOrLabel() : TEXT("(none)"),
        ObserverSourceLabel.IsEmpty() ? TEXT("(none)") : *ObserverSourceLabel,
        SelectedCharacterIntelligenceIdentityRoleLabel.IsEmpty() ? TEXT("Neutral") : *SelectedCharacterIntelligenceIdentityRoleLabel,
        IdentityComponent ? TEXT("Ready") : TEXT("Needs Enhance"),
        IsValid(TargetActor) ? *TargetActor->GetActorNameOrLabel() : TEXT("(none)"),
        TargetSourceLabel.IsEmpty() ? TEXT("(none)") : *TargetSourceLabel,
        SelectedCharacterIntelligenceRelationshipLabel.IsEmpty() ? TEXT("Unknown") : *SelectedCharacterIntelligenceRelationshipLabel,
        ProfileComponent ? TEXT("Ready") : TEXT("Needs Enhance"),
        *RecommendationLabel);

    if (!IsValid(ObserverActor))
    {
        Summary += TEXT("\nStatus: No compatible AI subject selected.");
    }
    else if (!IsValid(TargetActor))
    {
        Summary += TEXT("\nStatus: Select a target to let WAY-lite drive Follow, Guard, Observe, or Approach Hostile.");
    }

    return FText::FromString(Summary);
}

TSharedRef<SDockTab> FWanaWorksUIModule::SpawnWanaWorksTab(const FSpawnTabArgs& SpawnTabArgs)
{
    FWanaWorksUITabBuilderArgs BuilderArgs;
    BuilderArgs.GetSelectedWorkspaceLabel = [this]() { return GetSelectedWorkspaceLabel(); };
    BuilderArgs.GetSelectedPreviewViewLabel = [this]() { return GetSelectedPreviewStageViewLabel(); };
    BuilderArgs.GetStatusText = [this]() { return GetStatusText(); };
    BuilderArgs.GetCommandText = [this]() { return GetCommandText(); };
    BuilderArgs.GetLogText = [this]() { return GetLogText(); };
    BuilderArgs.GetWorkflowPresetSummaryText = [this]() { return GetWorkflowPresetSummaryText(); };
    BuilderArgs.GetSubjectSetupSummaryText = [this]() { return GetSubjectSetupSummaryText(); };
    BuilderArgs.GetSubjectStackSummaryText = [this]() { return GetSubjectStackSummaryText(); };
    BuilderArgs.GetSandboxPreviewSummaryText = [this]() { return GetSandboxPreviewSummaryText(); };
    BuilderArgs.GetSandboxPreviewObject = [this]() { return GetSandboxPreviewObject(); };
    BuilderArgs.GetAnimationIntegrationText = [this]() { return GetAnimationIntegrationText(); };
    BuilderArgs.GetAnimationHookUsageText = [this]() { return GetAnimationHookUsageText(); };
    BuilderArgs.GetPhysicalStateText = [this]() { return GetPhysicalStateText(); };
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
    BuilderArgs.GetCharacterIntelligenceControlSummaryText = [this]() { return GetCharacterIntelligenceControlSummaryText(); };
    BuilderArgs.CharacterPawnAssetOptions = &CharacterPawnAssetOptions;
    BuilderArgs.AIPawnAssetOptions = &AIPawnAssetOptions;
    BuilderArgs.WorkflowPresetOptions = &WorkflowPresetOptions;
    BuilderArgs.EnhancementPresetOptions = &EnhancementPresetOptions;
    BuilderArgs.EnhancementWorkflowOptions = &EnhancementWorkflowOptions;
    BuilderArgs.RelationshipStateOptions = &RelationshipStateOptions;
    BuilderArgs.CharacterIntelligenceIdentityRoleOptions = &CharacterIntelligenceIdentityRoleOptions;
    BuilderArgs.CharacterIntelligenceRelationshipOptions = &CharacterIntelligenceRelationshipOptions;
    BuilderArgs.CharacterIntelligenceTargetOptions = &CharacterIntelligenceTargetOptions;
    BuilderArgs.GetSelectedCharacterPawnAssetOption = [this]() { return GetSelectedCharacterPawnAssetOption(); };
    BuilderArgs.GetSelectedAIPawnAssetOption = [this]() { return GetSelectedAIPawnAssetOption(); };
    BuilderArgs.GetSelectedWorkflowPresetOption = [this]() { return GetSelectedWorkflowPresetOption(); };
    BuilderArgs.GetSelectedEnhancementPresetOption = [this]() { return GetSelectedEnhancementPresetOption(); };
    BuilderArgs.GetSelectedEnhancementWorkflowOption = [this]() { return GetSelectedEnhancementWorkflowOption(); };
    BuilderArgs.GetSelectedIdentitySeedStateOption = [this]() { return GetSelectedIdentitySeedStateOption(); };
    BuilderArgs.GetSelectedRelationshipStateOption = [this]() { return GetSelectedRelationshipStateOption(); };
    BuilderArgs.GetSelectedCharacterIntelligenceIdentityRoleOption = [this]() { return GetSelectedCharacterIntelligenceIdentityRoleOption(); };
    BuilderArgs.GetSelectedCharacterIntelligenceRelationshipOption = [this]() { return GetSelectedCharacterIntelligenceRelationshipOption(); };
    BuilderArgs.GetSelectedCharacterIntelligenceTargetOption = [this]() { return GetSelectedCharacterIntelligenceTargetOption(); };
    BuilderArgs.OnCommandTextChanged = [this](const FText& NewText) { HandleCommandTextChanged(NewText); };
    BuilderArgs.OnIdentityFactionTagTextChanged = [this](const FText& NewText) { HandleIdentityFactionTagTextChanged(NewText); };
    BuilderArgs.OnWorkspaceSelected = [this](const FString& WorkspaceLabel) { HandleWorkspaceSelected(WorkspaceLabel); };
    BuilderArgs.OnPreviewViewSelected = [this](const FString& PreviewViewLabel) { HandlePreviewStageViewSelected(PreviewViewLabel); };
    BuilderArgs.OnCharacterPawnAssetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleCharacterPawnAssetOptionSelected(SelectedOption); };
    BuilderArgs.OnAIPawnAssetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleAIPawnAssetOptionSelected(SelectedOption); };
    BuilderArgs.OnWorkflowPresetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleWorkflowPresetOptionSelected(SelectedOption); };
    BuilderArgs.OnEnhancementPresetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleEnhancementPresetOptionSelected(SelectedOption); };
    BuilderArgs.OnEnhancementWorkflowOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleEnhancementWorkflowOptionSelected(SelectedOption); };
    BuilderArgs.OnIdentitySeedStateOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleIdentitySeedStateOptionSelected(SelectedOption); };
    BuilderArgs.OnRelationshipStateOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleRelationshipStateOptionSelected(SelectedOption); };
    BuilderArgs.OnCharacterIntelligenceIdentityRoleOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleCharacterIntelligenceIdentityRoleOptionSelected(SelectedOption); };
    BuilderArgs.OnCharacterIntelligenceRelationshipOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleCharacterIntelligenceRelationshipOptionSelected(SelectedOption); };
    BuilderArgs.OnCharacterIntelligenceTargetOptionSelected = [this](TSharedPtr<FString> SelectedOption) { HandleCharacterIntelligenceTargetOptionSelected(SelectedOption); };
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
    BuilderArgs.OnCreateWorkingCopy = [this]() { CreateWorkingCopy(); };
    BuilderArgs.OnEnhanceWorkspace = [this]() { EnhanceActiveWorkspace(); };
    BuilderArgs.OnTestWorkspace = [this]() { TestActiveWorkspace(); };
    BuilderArgs.OnApplyCharacterEnhancement = [this]() { ApplyCharacterEnhancement(); };
    BuilderArgs.OnApplyStarterAndTestTarget = [this]() { ApplyStarterAndTestTarget(); };
    BuilderArgs.OnAnalyzeWorkspace = [this]() { AnalyzeActiveWorkspace(); };
    BuilderArgs.OnScanEnvironmentReadiness = [this]() { ScanEnvironmentReadiness(); };
    BuilderArgs.OnEvaluateLiveTarget = [this]() { EvaluateLiveTarget(); };
    BuilderArgs.OnUseSelectedAsSandboxObserver = [this]() { UseSelectedActorAsSandboxObserver(); };
    BuilderArgs.OnUseSelectedAsSandboxTarget = [this]() { UseSelectedActorAsSandboxTarget(); };
    BuilderArgs.OnEvaluateSandboxPair = [this]() { EvaluateSandboxPair(); };
    BuilderArgs.OnFinalizeSandboxBuild = [this]() { FinalizeSandboxBuild(); };
    BuilderArgs.OnFocusSandboxPreviewSubject = [this]() { FocusSandboxPreviewSubject(); };
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

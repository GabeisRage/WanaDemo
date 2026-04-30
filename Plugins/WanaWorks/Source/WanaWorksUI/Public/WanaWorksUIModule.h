#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "WAYRelationshipTypes.h"

class AActor;
class SDockTab;
class FSpawnTabArgs;
class UObject;
class UClass;
struct FWanaCommandResponse;
struct FWanaSelectedCharacterEnhancementSnapshot;

class WANAWORKSUI_API FWanaWorksUIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void ApplyResponse(const FWanaCommandResponse& Response, const FString& EchoedCommand = FString());
    void RefreshIdentityEditorState(bool bForceRefresh = false);
    void RefreshReactiveUI(bool bForceRefresh = false);
    void HandleEditorSelectionChanged(UObject* NewSelection);
    void HandleCommandTextChanged(const FText& NewText);
    void HandleIdentityFactionTagTextChanged(const FText& NewText);
    void HandleWorkspaceSelected(const FString& WorkspaceLabel);
    void HandlePreviewStageViewSelected(const FString& PreviewViewLabel);
    void HandleIdentitySeedStateOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleWorkflowPresetOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleEnhancementPresetOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleEnhancementWorkflowOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleRelationshipStateOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleCharacterPawnAssetOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleAIPawnAssetOptionSelected(TSharedPtr<FString> SelectedOption);
    void RegisterEditorLauncherMenus();
    void OpenWanaWorksStudioTab();
    bool SyncSelectedWorkflowPresetIntoControls(FString* OutPresetLabel = nullptr, FString* OutPresetNotes = nullptr);
    void UpdateEnhancementResultsState(
        const FWanaSelectedCharacterEnhancementSnapshot* BeforeSnapshot,
        const FWanaSelectedCharacterEnhancementSnapshot* AfterSnapshot,
        const FString& WorkflowUsed,
        bool bUpdateWorkflowMetadata,
        bool bSandboxCopyCreated,
        bool bOriginalPreserved);
    void UpdateSavedSubjectProgressState(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot);
    void EnsureIdentityComponent();
    void ExecuteCommandText(const FString& InCommandText);
    void ApplyIdentity();
    void CreateAIReadyController();
    void ConvertSelectedSubjectToAIReady();
    void SaveSubjectProgress();
    void RestoreSavedSubjectProgress();
    void ApplySelectedWorkflowPreset();
    void SaveCurrentStateAsWorkflowPreset();
    void ShowSelectedWorkflowPresetSummary();
    void CreateWorkingCopy();
    void EnhanceActiveWorkspace();
    void TestActiveWorkspace();
    void ApplyCharacterEnhancement();
    void ApplyStarterAndTestTarget();
    void AnalyzeActiveWorkspace();
    void ScanEnvironmentReadiness();
    void EvaluateLiveTarget();
    void UseSelectedActorAsSandboxObserver();
    void UseSelectedActorAsSandboxTarget();
    void EvaluateSandboxPair();
    void FinalizeSandboxBuild();
    void FocusSandboxPreviewSubject();
    void FocusSandboxObserver();
    void FocusSandboxTarget();
    void ApplySelectedRelationshipState();
    void LoadSavedSubjectProgress();
    void PersistSavedSubjectProgress() const;
    void LoadSavedWorkflowPresets();
    void PersistSavedWorkflowPresets() const;
    void RunCommand();
    void ClearLog();
    void AppendLogLine(const FString& Line);
    TSharedPtr<FString> GetSelectedWorkflowPresetOption() const;
    TSharedPtr<FString> GetSelectedEnhancementPresetOption() const;
    TSharedPtr<FString> GetSelectedEnhancementWorkflowOption() const;
    TSharedPtr<FString> GetSelectedCharacterPawnAssetOption() const;
    TSharedPtr<FString> GetSelectedAIPawnAssetOption() const;
    TSharedPtr<FString> GetSelectedIdentitySeedStateOption();
    TSharedPtr<FString> GetSelectedRelationshipStateOption() const;
    void RefreshProjectAssetPickerOptions();
    bool ResolvePreferredSubjectSnapshot(FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot) const;
    bool ResolvePickedSubjectSnapshot(FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot) const;
    bool ResolvePreferredSandboxPreviewActor(AActor*& OutActor, FString& OutPreviewModeLabel) const;
    bool PreparePickerDrivenSubjectForWorkflow(const FString& WorkflowContextLabel, FWanaCommandResponse& OutPreparationResponse, bool& bOutSpawnedFromPicker);
    bool ApplyWorkspaceSubjectEnhancement(const FString& WorkflowLabel, const FString& EnhancementPresetLabel, const FString& WorkspaceActionLabel);
    UObject* LoadSelectedSubjectAssetObject() const;
    UObject* LoadSelectedSubjectAssetObjectForWorkspace(const FString& WorkspaceLabel) const;
    UClass* LoadSelectedSubjectActorClass() const;
    UClass* LoadSelectedSubjectActorClassForWorkspace(const FString& WorkspaceLabel) const;
    FString GetSelectedSubjectAssetPath() const;
    FString GetSelectedSubjectAssetPathForWorkspace(const FString& WorkspaceLabel) const;
    UObject* GetSandboxPreviewObject() const;
    bool IsActorCompatibleWithWorkspacePreview(const AActor* Actor, const FString& WorkspaceLabel) const;
    bool RestoreSubjectPickerFromAssetPath(const FString& AssetPath);
    bool HasCurrentWorkspaceAnalysis() const;

    FText GetStatusText() const;
    FString GetSelectedWorkspaceLabel() const;
    FString GetSelectedPreviewStageViewLabel() const;
    FText GetCommandText() const;
    FText GetLogText() const;
    FText GetWorkflowPresetSummaryText() const;
    FText GetSubjectSetupSummaryText() const;
    FText GetSubjectStackSummaryText() const;
    FText GetSandboxPreviewSummaryText() const;
    FText GetAnimationIntegrationText() const;
    FText GetAnimationHookUsageText() const;
    FText GetPhysicalStateText() const;
    FText GetSavedSubjectProgressText() const;
    FText GetCharacterEnhancementSummaryText() const;
    FText GetCharacterEnhancementChainText() const;
    FText GetCharacterEnhancementWorkflowText() const;
    FText GetEnhancementResultsText() const;
    FText GetGuidedWorkflowSummaryText() const;
    FText GetLiveTestSummaryText() const;
    FText GetBehaviorResultsText() const;
    FText GetTestSandboxSummaryText() const;
    FText GetWITEnvironmentReadinessText() const;
    FText GetRelationshipSummaryText() const;
    FText GetIdentitySummaryText();
    FText GetIdentityFactionTagText();
    bool ResolvePreferredWITReadinessPair(AActor*& OutObserverActor, AActor*& OutTargetActor, FString& OutPairSourceLabel, bool& bOutTargetFallsBackToObserver) const;

    TSharedRef<SDockTab> SpawnWanaWorksTab(const FSpawnTabArgs& SpawnTabArgs);

    FString StatusMessage;
    FString SelectedWorkspaceLabel;
    FString SelectedPreviewStageViewLabel;
    FString CommandText;
    FString LogOutput;
    FString IdentityFactionTagText;
    FString SelectedWorkflowPresetLabel;
    FString SelectedEnhancementPresetLabel;
    FString SelectedEnhancementWorkflowLabel;
    FString SelectedCharacterPawnAssetLabel;
    FString SelectedAIPawnAssetLabel;
    EWAYRelationshipState SelectedIdentitySeedState = EWAYRelationshipState::Neutral;
    EWAYRelationshipState SelectedRelationshipState = EWAYRelationshipState::Neutral;
    TArray<TSharedPtr<FString>> WorkflowPresetOptions;
    TArray<TSharedPtr<FString>> EnhancementPresetOptions;
    TArray<TSharedPtr<FString>> EnhancementWorkflowOptions;
    TArray<TSharedPtr<FString>> CharacterPawnAssetOptions;
    TArray<TSharedPtr<FString>> AIPawnAssetOptions;
    TArray<TSharedPtr<FString>> RelationshipStateOptions;
    TMap<FString, FString> CharacterPawnAssetPathByLabel;
    TMap<FString, FString> AIPawnAssetPathByLabel;
    TWeakObjectPtr<AActor> CachedIdentityActor;
    TWeakObjectPtr<AActor> LastEnhancementResultsActor;
    TWeakObjectPtr<AActor> SandboxObserverActor;
    TWeakObjectPtr<AActor> SandboxTargetActor;
    TWeakPtr<SDockTab> WanaWorksTab;
    FDelegateHandle SelectionChangedHandle;
    FDelegateHandle SelectObjectHandle;
    bool bIdentityStateInitialized = false;
    bool bWorkspaceAnalysisInitialized = false;
    bool bEnhancementResultsInitialized = false;
    FString LastAnalysisWorkspaceLabel;
    FString LastAnalysisStatusSummary;
    FString LastAnalysisPrimarySummary;
    FString LastAnalysisAnimationSummary;
    FString LastAnalysisPhysicalSummary;
    FString LastAnalysisBehaviorSummary;
    FString LastAnalysisWITSummary;
    FString LastAnalysisSuggestedSummary;
    FString LastEnhancementResultsActorLabel;
    FString LastEnhancementWorkflowUsed;
    FString LastEnhancementIdentityResult;
    FString LastEnhancementWAYResult;
    FString LastEnhancementWAIResult;
    FString LastEnhancementAIReadyResult;
    FString LastEnhancementAnimationResult;
    bool bLastEnhancementSandboxCopyCreated = false;
    bool bLastEnhancementOriginalPreserved = true;
    bool bSavedSubjectProgressInitialized = false;
    FString LastSavedTimestamp;
    FString LastSavedSubjectPath;
    FString LastSavedSubjectLabel;
    FString LastSavedSubjectTypeLabel;
    FString LastSavedSubjectWorkflowLabel;
    FString LastSavedSubjectPresetLabel;
    FString LastSavedSubjectAIControllerResult;
    FString LastSavedSubjectAnimationResult;
    FString LastSavedSubjectIdentityResult;
    FString LastSavedSubjectWAIResult;
    FString LastSavedSubjectWAYResult;
    FString LastSavedSubjectAIReadyResult;
    FString LastSavedObserverPath;
    FString LastSavedObserverLabel;
    FString LastSavedTargetPath;
    FString LastSavedTargetLabel;
    FString LastSavedAnimationBlueprintAssetPath;
    FString LastSavedAnimationBlueprintAssetLabel;
    bool bSavedWorkflowPresetInitialized = false;
    FString LastSavedWorkflowPresetTimestamp;
    FString LastSavedWorkflowPresetSourceLabel;
    FString LastSavedWorkflowPresetWorkflowLabel;
    FString LastSavedWorkflowPresetEnhancementPresetLabel;
    FString LastSavedWorkflowPresetIdentitySeedLabel;
    FString LastSavedWorkflowPresetRelationshipLabel;
    FString LastSavedWorkflowPresetBehaviorLabel;
    FString LastSavedWorkflowPresetFactionTag;

    static const FName WanaWorksTabName;
};

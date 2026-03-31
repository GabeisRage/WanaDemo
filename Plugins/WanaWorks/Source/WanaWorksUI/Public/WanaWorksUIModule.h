#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "WAYRelationshipTypes.h"

class AActor;
class SDockTab;
class FSpawnTabArgs;
class UObject;
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
    void HandleIdentitySeedStateOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleEnhancementPresetOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleEnhancementWorkflowOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleRelationshipStateOptionSelected(TSharedPtr<FString> SelectedOption);
    void UpdateEnhancementResultsState(
        const FWanaSelectedCharacterEnhancementSnapshot* BeforeSnapshot,
        const FWanaSelectedCharacterEnhancementSnapshot* AfterSnapshot,
        const FString& WorkflowUsed,
        bool bUpdateWorkflowMetadata,
        bool bSandboxCopyCreated,
        bool bOriginalPreserved);
    void EnsureIdentityComponent();
    void ExecuteCommandText(const FString& InCommandText);
    void ApplyIdentity();
    void ApplyCharacterEnhancement();
    void ApplyStarterAndTestTarget();
    void EvaluateLiveTarget();
    void UseSelectedActorAsSandboxObserver();
    void UseSelectedActorAsSandboxTarget();
    void EvaluateSandboxPair();
    void FocusSandboxObserver();
    void FocusSandboxTarget();
    void ApplySelectedRelationshipState();
    void RunCommand();
    void ClearLog();
    void AppendLogLine(const FString& Line);
    TSharedPtr<FString> GetSelectedEnhancementPresetOption() const;
    TSharedPtr<FString> GetSelectedEnhancementWorkflowOption() const;
    TSharedPtr<FString> GetSelectedIdentitySeedStateOption();
    TSharedPtr<FString> GetSelectedRelationshipStateOption() const;

    FText GetStatusText() const;
    FText GetCommandText() const;
    FText GetLogText() const;
    FText GetCharacterEnhancementSummaryText() const;
    FText GetCharacterEnhancementChainText() const;
    FText GetCharacterEnhancementWorkflowText() const;
    FText GetEnhancementResultsText() const;
    FText GetGuidedWorkflowSummaryText() const;
    FText GetLiveTestSummaryText() const;
    FText GetTestSandboxSummaryText() const;
    FText GetRelationshipSummaryText() const;
    FText GetIdentitySummaryText();
    FText GetIdentityFactionTagText();

    TSharedRef<SDockTab> SpawnWanaWorksTab(const FSpawnTabArgs& SpawnTabArgs);

    FString StatusMessage;
    FString CommandText;
    FString LogOutput;
    FString IdentityFactionTagText;
    FString SelectedEnhancementPresetLabel;
    FString SelectedEnhancementWorkflowLabel;
    EWAYRelationshipState SelectedIdentitySeedState = EWAYRelationshipState::Neutral;
    EWAYRelationshipState SelectedRelationshipState = EWAYRelationshipState::Neutral;
    TArray<TSharedPtr<FString>> EnhancementPresetOptions;
    TArray<TSharedPtr<FString>> EnhancementWorkflowOptions;
    TArray<TSharedPtr<FString>> RelationshipStateOptions;
    TWeakObjectPtr<AActor> CachedIdentityActor;
    TWeakObjectPtr<AActor> LastEnhancementResultsActor;
    TWeakObjectPtr<AActor> SandboxObserverActor;
    TWeakObjectPtr<AActor> SandboxTargetActor;
    TWeakPtr<SDockTab> WanaWorksTab;
    FDelegateHandle SelectionChangedHandle;
    FDelegateHandle SelectObjectHandle;
    bool bIdentityStateInitialized = false;
    bool bEnhancementResultsInitialized = false;
    FString LastEnhancementResultsActorLabel;
    FString LastEnhancementWorkflowUsed;
    FString LastEnhancementIdentityResult;
    FString LastEnhancementWAYResult;
    FString LastEnhancementWAIResult;
    FString LastEnhancementAIReadyResult;
    FString LastEnhancementAnimationResult;
    bool bLastEnhancementSandboxCopyCreated = false;
    bool bLastEnhancementOriginalPreserved = true;

    static const FName WanaWorksTabName;
};

#pragma once

#include "CoreMinimal.h"

class SWidget;

struct FWanaWorksUITabBuilderArgs
{
    TFunction<FText(void)> GetStatusText;
    TFunction<FText(void)> GetCommandText;
    TFunction<FText(void)> GetLogText;
    TFunction<FText(void)> GetWorkflowPresetSummaryText;
    TFunction<FText(void)> GetSubjectSetupSummaryText;
    TFunction<FText(void)> GetSubjectStackSummaryText;
    TFunction<FText(void)> GetAnimationIntegrationText;
    TFunction<FText(void)> GetSavedSubjectProgressText;
    TFunction<FText(void)> GetCharacterEnhancementSummaryText;
    TFunction<FText(void)> GetCharacterEnhancementChainText;
    TFunction<FText(void)> GetCharacterEnhancementWorkflowText;
    TFunction<FText(void)> GetEnhancementResultsText;
    TFunction<FText(void)> GetGuidedWorkflowSummaryText;
    TFunction<FText(void)> GetLiveTestSummaryText;
    TFunction<FText(void)> GetBehaviorResultsText;
    TFunction<FText(void)> GetTestSandboxSummaryText;
    TFunction<FText(void)> GetWITEnvironmentReadinessText;
    TFunction<FText(void)> GetRelationshipSummaryText;
    TFunction<FText(void)> GetIdentitySummaryText;
    TFunction<FText(void)> GetIdentityFactionTagText;
    const TArray<TSharedPtr<FString>>* CharacterPawnAssetOptions = nullptr;
    const TArray<TSharedPtr<FString>>* AIPawnAssetOptions = nullptr;
    const TArray<TSharedPtr<FString>>* WorkflowPresetOptions = nullptr;
    const TArray<TSharedPtr<FString>>* EnhancementPresetOptions = nullptr;
    const TArray<TSharedPtr<FString>>* EnhancementWorkflowOptions = nullptr;
    const TArray<TSharedPtr<FString>>* RelationshipStateOptions = nullptr;
    TFunction<TSharedPtr<FString>(void)> GetSelectedCharacterPawnAssetOption;
    TFunction<TSharedPtr<FString>(void)> GetSelectedAIPawnAssetOption;
    TFunction<TSharedPtr<FString>(void)> GetSelectedWorkflowPresetOption;
    TFunction<TSharedPtr<FString>(void)> GetSelectedEnhancementPresetOption;
    TFunction<TSharedPtr<FString>(void)> GetSelectedEnhancementWorkflowOption;
    TFunction<TSharedPtr<FString>(void)> GetSelectedIdentitySeedStateOption;
    TFunction<TSharedPtr<FString>(void)> GetSelectedRelationshipStateOption;
    TFunction<void(const FText&)> OnCommandTextChanged;
    TFunction<void(const FText&)> OnIdentityFactionTagTextChanged;
    TFunction<void(TSharedPtr<FString>)> OnCharacterPawnAssetOptionSelected;
    TFunction<void(TSharedPtr<FString>)> OnAIPawnAssetOptionSelected;
    TFunction<void(TSharedPtr<FString>)> OnWorkflowPresetOptionSelected;
    TFunction<void(TSharedPtr<FString>)> OnEnhancementPresetOptionSelected;
    TFunction<void(TSharedPtr<FString>)> OnEnhancementWorkflowOptionSelected;
    TFunction<void(TSharedPtr<FString>)> OnIdentitySeedStateOptionSelected;
    TFunction<void(TSharedPtr<FString>)> OnRelationshipStateOptionSelected;
    TFunction<void(void)> OnRunCommand;
    TFunction<void(void)> OnClearLog;
    TFunction<void(void)> OnEnsureIdentityComponent;
    TFunction<void(void)> OnApplyIdentity;
    TFunction<void(void)> OnCreateAIReadyController;
    TFunction<void(void)> OnConvertToAIReadySubject;
    TFunction<void(void)> OnSaveSubjectProgress;
    TFunction<void(void)> OnRestoreSavedSubjectProgress;
    TFunction<void(void)> OnApplyWorkflowPreset;
    TFunction<void(void)> OnSaveWorkflowPreset;
    TFunction<void(void)> OnShowWorkflowPresetSummary;
    TFunction<void(void)> OnApplyCharacterEnhancement;
    TFunction<void(void)> OnApplyStarterAndTestTarget;
    TFunction<void(void)> OnScanEnvironmentReadiness;
    TFunction<void(void)> OnEvaluateLiveTarget;
    TFunction<void(void)> OnUseSelectedAsSandboxObserver;
    TFunction<void(void)> OnUseSelectedAsSandboxTarget;
    TFunction<void(void)> OnEvaluateSandboxPair;
    TFunction<void(void)> OnFinalizeSandboxBuild;
    TFunction<void(void)> OnFocusSandboxObserver;
    TFunction<void(void)> OnFocusSandboxTarget;
    TFunction<void(void)> OnApplyRelationshipState;
    TFunction<void(const FString&)> OnExecuteCommandText;
};

namespace WanaWorksUITabBuilder
{
    TSharedRef<SWidget> BuildTabContent(const FWanaWorksUITabBuilderArgs& Args);
}

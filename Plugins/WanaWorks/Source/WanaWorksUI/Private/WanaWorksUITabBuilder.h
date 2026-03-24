#pragma once

#include "CoreMinimal.h"

class SWidget;

struct FWanaWorksUITabBuilderArgs
{
    TFunction<FText(void)> GetStatusText;
    TFunction<FText(void)> GetCommandText;
    TFunction<FText(void)> GetLogText;
    TFunction<FText(void)> GetRelationshipSummaryText;
    TFunction<FText(void)> GetIdentitySummaryText;
    TFunction<FText(void)> GetIdentityFactionTagText;
    const TArray<TSharedPtr<FString>>* EnhancementPresetOptions = nullptr;
    const TArray<TSharedPtr<FString>>* RelationshipStateOptions = nullptr;
    TFunction<TSharedPtr<FString>(void)> GetSelectedEnhancementPresetOption;
    TFunction<TSharedPtr<FString>(void)> GetSelectedIdentitySeedStateOption;
    TFunction<TSharedPtr<FString>(void)> GetSelectedRelationshipStateOption;
    TFunction<void(const FText&)> OnCommandTextChanged;
    TFunction<void(const FText&)> OnIdentityFactionTagTextChanged;
    TFunction<void(TSharedPtr<FString>)> OnEnhancementPresetOptionSelected;
    TFunction<void(TSharedPtr<FString>)> OnIdentitySeedStateOptionSelected;
    TFunction<void(TSharedPtr<FString>)> OnRelationshipStateOptionSelected;
    TFunction<void(void)> OnRunCommand;
    TFunction<void(void)> OnClearLog;
    TFunction<void(void)> OnEnsureIdentityComponent;
    TFunction<void(void)> OnApplyIdentity;
    TFunction<void(void)> OnApplyCharacterEnhancement;
    TFunction<void(void)> OnApplyRelationshipState;
    TFunction<void(const FString&)> OnExecuteCommandText;
};

namespace WanaWorksUITabBuilder
{
    TSharedRef<SWidget> BuildTabContent(const FWanaWorksUITabBuilderArgs& Args);
}

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "WAYRelationshipTypes.h"

class AActor;
class SDockTab;
class FSpawnTabArgs;
struct FWanaCommandResponse;

class WANAWORKSUI_API FWanaWorksUIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void ApplyResponse(const FWanaCommandResponse& Response, const FString& EchoedCommand = FString());
    void RefreshIdentityEditorState(bool bForceRefresh = false);
    void HandleCommandTextChanged(const FText& NewText);
    void HandleIdentityFactionTagTextChanged(const FText& NewText);
    void HandleIdentitySeedStateOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleEnhancementPresetOptionSelected(TSharedPtr<FString> SelectedOption);
    void HandleRelationshipStateOptionSelected(TSharedPtr<FString> SelectedOption);
    void EnsureIdentityComponent();
    void ExecuteCommandText(const FString& InCommandText);
    void ApplyIdentity();
    void ApplyCharacterEnhancement();
    void ApplySelectedRelationshipState();
    void RunCommand();
    void ClearLog();
    void AppendLogLine(const FString& Line);
    TSharedPtr<FString> GetSelectedEnhancementPresetOption() const;
    TSharedPtr<FString> GetSelectedIdentitySeedStateOption();
    TSharedPtr<FString> GetSelectedRelationshipStateOption() const;

    FText GetStatusText() const;
    FText GetCommandText() const;
    FText GetLogText() const;
    FText GetRelationshipSummaryText() const;
    FText GetIdentitySummaryText();
    FText GetIdentityFactionTagText();

    TSharedRef<SDockTab> SpawnWanaWorksTab(const FSpawnTabArgs& SpawnTabArgs);

    FString StatusMessage;
    FString CommandText;
    FString LogOutput;
    FString IdentityFactionTagText;
    FString SelectedEnhancementPresetLabel;
    EWAYRelationshipState SelectedIdentitySeedState = EWAYRelationshipState::Neutral;
    EWAYRelationshipState SelectedRelationshipState = EWAYRelationshipState::Neutral;
    TArray<TSharedPtr<FString>> EnhancementPresetOptions;
    TArray<TSharedPtr<FString>> RelationshipStateOptions;
    TWeakObjectPtr<AActor> CachedIdentityActor;
    bool bIdentityStateInitialized = false;

    static const FName WanaWorksTabName;
};

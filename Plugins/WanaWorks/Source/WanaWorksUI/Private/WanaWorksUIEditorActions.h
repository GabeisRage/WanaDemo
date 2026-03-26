#pragma once

#include "CoreMinimal.h"
#include "WanaWorksCommandDispatcher.h"
#include "WAYRelationshipTypes.h"

class AActor;

struct FWanaSelectedActorIdentitySnapshot
{
    bool bHasSelectedActor = false;
    bool bHasIdentityComponent = false;
    TWeakObjectPtr<AActor> SelectedActor;
    FString SelectedActorLabel;
    FString FactionTag;
    FWAYRelationshipSeed DefaultRelationshipSeed;
    FString ReputationTagsSummary;
};

struct FWanaSelectedCharacterEnhancementSnapshot
{
    bool bHasSelectedActor = false;
    bool bHasIdentityComponent = false;
    bool bHasWAIComponent = false;
    bool bHasWAYComponent = false;
    TWeakObjectPtr<AActor> SelectedActor;
    FString SelectedActorLabel;
};

struct FWanaSelectedRelationshipContextSnapshot
{
    bool bHasObserverActor = false;
    bool bHasTargetActor = false;
    bool bTargetFallsBackToObserver = false;
    bool bObserverHasRelationshipComponent = false;
    TWeakObjectPtr<AActor> ObserverActor;
    TWeakObjectPtr<AActor> TargetActor;
    FString ObserverActorLabel;
    FString TargetActorLabel;
};

namespace WanaWorksUIEditorActions
{
    bool GetSelectedActorIdentitySnapshot(FWanaSelectedActorIdentitySnapshot& OutSnapshot);
    bool GetSelectedCharacterEnhancementSnapshot(FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot);
    bool GetSelectedRelationshipContextSnapshot(FWanaSelectedRelationshipContextSnapshot& OutSnapshot);
    FWanaCommandResponse ExecuteWeatherPresetCommand(const FString& PresetName);
    FWanaCommandResponse ExecuteSpawnCubeCommand();
    FWanaCommandResponse ExecuteListSelectionCommand();
    FWanaCommandResponse ExecuteClassifySelectedCommand();
    FWanaCommandResponse ExecuteAddMemoryTestCommand();
    FWanaCommandResponse ExecuteAddPreferenceTestCommand();
    FWanaCommandResponse ExecuteShowMemoryCommand();
    FWanaCommandResponse ExecuteShowPreferencesCommand();
    FWanaCommandResponse ExecuteEnsureRelationshipProfileCommand();
    FWanaCommandResponse ExecuteShowRelationshipProfileCommand();
    FWanaCommandResponse ExecuteApplyRelationshipStateCommand(const FString& RelationshipStateText);
    FWanaCommandResponse ExecuteEnsureIdentityComponentCommand();
    FWanaCommandResponse ExecuteApplyIdentityCommand(const FString& FactionTagText, EWAYRelationshipState DefaultRelationshipState);
    FWanaCommandResponse ExecuteApplyCharacterEnhancementCommand(const FString& PresetLabel);
    FWanaCommandResponse ExecuteEvaluateLiveTargetCommand();
    FWanaCommandResponse ExecuteEvaluateActorPairCommand(AActor* ObserverActor, AActor* TargetActor, bool bTargetFallsBackToObserver = false);
    FWanaCommandResponse ExecuteFocusActorCommand(AActor* Actor, const FString& RoleLabel);
    FWanaCommandResponse ExecuteApplyStarterAndTestTargetCommand(const FString& PresetLabel);
}

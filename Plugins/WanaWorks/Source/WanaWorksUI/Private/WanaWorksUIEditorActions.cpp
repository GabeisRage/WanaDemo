#include "WanaWorksUIEditorActions.h"

#include "Editor.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "ScopedTransaction.h"
#include "WAIPersonalityComponent.h"
#include "WAYPlayerProfileComponent.h"
#include "WanaIdentityComponent.h"
#include "WanaWorksWeatherController.h"
#include "WITBlueprintLibrary.h"

#define LOCTEXT_NAMESPACE "WanaWorksUIEditorActions"

namespace
{
FWanaCommandResponse MakeEditorFailureResponse(const FString& StatusMessage, const FString& OutputLine)
{
    FWanaCommandResponse Response;
    Response.StatusMessage = StatusMessage;
    Response.OutputLines.Add(OutputLine);
    return Response;
}

FString GetRelationshipStateDisplayLabel(EWAYRelationshipState RelationshipState)
{
    const UEnum* RelationshipEnum = StaticEnum<EWAYRelationshipState>();
    return RelationshipEnum
        ? RelationshipEnum->GetDisplayNameTextByValue(static_cast<int64>(RelationshipState)).ToString()
        : TEXT("Neutral");
}

FString GetReactionStateDisplayLabel(EWAYReactionState ReactionState)
{
    const UEnum* ReactionEnum = StaticEnum<EWAYReactionState>();
    return ReactionEnum
        ? ReactionEnum->GetDisplayNameTextByValue(static_cast<int64>(ReactionState)).ToString()
        : TEXT("Observational");
}

bool TryParseRelationshipState(const FString& RelationshipStateText, EWAYRelationshipState& OutRelationshipState)
{
    const FString NormalizedRelationshipState = RelationshipStateText.TrimStartAndEnd().ToLower();

    if (NormalizedRelationshipState == TEXT("acquaintance"))
    {
        OutRelationshipState = EWAYRelationshipState::Acquaintance;
        return true;
    }

    if (NormalizedRelationshipState == TEXT("friend"))
    {
        OutRelationshipState = EWAYRelationshipState::Friend;
        return true;
    }

    if (NormalizedRelationshipState == TEXT("partner"))
    {
        OutRelationshipState = EWAYRelationshipState::Partner;
        return true;
    }

    if (NormalizedRelationshipState == TEXT("enemy"))
    {
        OutRelationshipState = EWAYRelationshipState::Enemy;
        return true;
    }

    if (NormalizedRelationshipState == TEXT("neutral"))
    {
        OutRelationshipState = EWAYRelationshipState::Neutral;
        return true;
    }

    return false;
}

bool GetSelectedActorPair(const AActor*& OutObserverActor, const AActor*& OutTargetActor, bool& bOutTargetFallsBackToObserver)
{
    OutObserverActor = nullptr;
    OutTargetActor = nullptr;
    bOutTargetFallsBackToObserver = false;

    if (!GEditor)
    {
        return false;
    }

    USelection* SelectedActors = GEditor->GetSelectedActors();

    if (!SelectedActors || SelectedActors->Num() == 0)
    {
        return false;
    }

    TArray<const AActor*> ActorSelection;
    ActorSelection.Reserve(2);

    for (FSelectionIterator SelectionIt(*SelectedActors); SelectionIt && ActorSelection.Num() < 2; ++SelectionIt)
    {
        if (const AActor* SelectedActor = Cast<AActor>(*SelectionIt))
        {
            ActorSelection.Add(SelectedActor);
        }
    }

    if (ActorSelection.Num() == 0)
    {
        return false;
    }

    OutObserverActor = ActorSelection[0];

    if (ActorSelection.Num() > 1 && ActorSelection[1] && ActorSelection[1] != OutObserverActor)
    {
        OutTargetActor = ActorSelection[1];
        return true;
    }

    OutTargetActor = OutObserverActor;
    bOutTargetFallsBackToObserver = true;
    return true;
}

const AActor* GetFirstSelectedActor()
{
    const AActor* ObserverActor = nullptr;
    const AActor* TargetActor = nullptr;
    bool bTargetFallsBackToObserver = false;
    GetSelectedActorPair(ObserverActor, TargetActor, bTargetFallsBackToObserver);
    return ObserverActor;
}

AActor* GetFirstSelectedActorMutable()
{
    return const_cast<AActor*>(GetFirstSelectedActor());
}

UWAIPersonalityComponent* FindOrAddMemoryComponent(AActor* Actor)
{
    if (!Actor)
    {
        return nullptr;
    }

    if (UWAIPersonalityComponent* ExistingComponent = Actor->FindComponentByClass<UWAIPersonalityComponent>())
    {
        ExistingComponent->SetFlags(RF_Transactional);
        return ExistingComponent;
    }

    Actor->Modify();

    UWAIPersonalityComponent* NewComponent = NewObject<UWAIPersonalityComponent>(Actor, UWAIPersonalityComponent::StaticClass(), NAME_None, RF_Transactional);

    if (!NewComponent)
    {
        return nullptr;
    }

    NewComponent->SetFlags(RF_Transactional);
    Actor->AddInstanceComponent(NewComponent);
    NewComponent->OnComponentCreated();
    NewComponent->RegisterComponent();
    return NewComponent;
}

UWAYPlayerProfileComponent* FindOrAddPreferenceComponent(AActor* Actor)
{
    if (!Actor)
    {
        return nullptr;
    }

    if (UWAYPlayerProfileComponent* ExistingComponent = Actor->FindComponentByClass<UWAYPlayerProfileComponent>())
    {
        ExistingComponent->SetFlags(RF_Transactional);
        return ExistingComponent;
    }

    Actor->Modify();

    UWAYPlayerProfileComponent* NewComponent = NewObject<UWAYPlayerProfileComponent>(Actor, UWAYPlayerProfileComponent::StaticClass(), NAME_None, RF_Transactional);

    if (!NewComponent)
    {
        return nullptr;
    }

    NewComponent->SetFlags(RF_Transactional);
    Actor->AddInstanceComponent(NewComponent);
    NewComponent->OnComponentCreated();
    NewComponent->RegisterComponent();
    return NewComponent;
}

UWanaIdentityComponent* FindOrAddIdentityComponent(AActor* Actor)
{
    if (!Actor)
    {
        return nullptr;
    }

    if (UWanaIdentityComponent* ExistingComponent = Actor->FindComponentByClass<UWanaIdentityComponent>())
    {
        ExistingComponent->SetFlags(RF_Transactional);
        return ExistingComponent;
    }

    Actor->Modify();

    UWanaIdentityComponent* NewComponent = NewObject<UWanaIdentityComponent>(Actor, UWanaIdentityComponent::StaticClass(), NAME_None, RF_Transactional);

    if (!NewComponent)
    {
        return nullptr;
    }

    NewComponent->SetFlags(RF_Transactional);
    Actor->AddInstanceComponent(NewComponent);
    NewComponent->OnComponentCreated();
    NewComponent->RegisterComponent();
    return NewComponent;
}

FString MakeReputationTagsSummary(const TArray<FName>& ReputationTags)
{
    return ReputationTags.Num() > 0
        ? FString::JoinBy(ReputationTags, TEXT(", "), [](const FName& Tag) { return Tag.ToString(); })
        : TEXT("(none)");
}

FString JoinOrNone(const TArray<FString>& Entries)
{
    return Entries.Num() > 0 ? FString::Join(Entries, TEXT(", ")) : TEXT("(none)");
}

FString GetActorTypeLabel(const AActor* Actor)
{
    if (Cast<ACharacter>(Actor))
    {
        return TEXT("Character");
    }

    if (Cast<APawn>(Actor))
    {
        return TEXT("Pawn");
    }

    return TEXT("Actor");
}

FString MakeAnimationCompatibilitySummary(bool bHasSkeletalMeshComponent, bool bHasAnimBlueprint)
{
    if (!bHasSkeletalMeshComponent)
    {
        return TEXT("No skeletal mesh animation setup detected.");
    }

    if (bHasAnimBlueprint)
    {
        return TEXT("Anim Blueprint is assigned.");
    }

    return TEXT("Skeletal mesh found, but no Anim Blueprint is assigned.");
}

FString MakeAIReadinessSummary(bool bIsPawnActor, bool bAutoPossessAIEnabled, bool bHasAIControllerClass)
{
    if (!bIsPawnActor)
    {
        return TEXT("Not a Pawn/Character test subject.");
    }

    if (bAutoPossessAIEnabled && bHasAIControllerClass)
    {
        return TEXT("AI-ready for lightweight testing.");
    }

    if (bAutoPossessAIEnabled)
    {
        return TEXT("Auto Possess AI is enabled. Assign an AIControllerClass if you want autonomous controller behavior.");
    }

    return TEXT("Can be prepared for AI-ready testing.");
}

bool BuildCharacterEnhancementSnapshot(const AActor* Actor, FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot)
{
    OutSnapshot = FWanaSelectedCharacterEnhancementSnapshot();

    if (!Actor)
    {
        return false;
    }

    OutSnapshot.bHasSelectedActor = true;
    OutSnapshot.SelectedActor = const_cast<AActor*>(Actor);
    OutSnapshot.SelectedActorLabel = Actor->GetActorNameOrLabel();
    OutSnapshot.bHasIdentityComponent = Actor->FindComponentByClass<UWanaIdentityComponent>() != nullptr;
    OutSnapshot.bHasWAIComponent = Actor->FindComponentByClass<UWAIPersonalityComponent>() != nullptr;
    OutSnapshot.bHasWAYComponent = Actor->FindComponentByClass<UWAYPlayerProfileComponent>() != nullptr;
    OutSnapshot.ActorTypeLabel = GetActorTypeLabel(Actor);

    if (const APawn* Pawn = Cast<APawn>(Actor))
    {
        OutSnapshot.bIsPawnActor = true;
        OutSnapshot.bHasAIControllerClass = Pawn->AIControllerClass != nullptr;
        OutSnapshot.bAutoPossessAIEnabled = Pawn->AutoPossessAI != EAutoPossessAI::Disabled;
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
    OutSnapshot.bHasSkeletalMeshComponent = SkeletalMeshComponents.Num() > 0;

    for (const USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (SkeletalMeshComponent && SkeletalMeshComponent->GetAnimClass() != nullptr)
        {
            OutSnapshot.bHasAnimBlueprint = true;
            break;
        }
    }

    OutSnapshot.AnimationCompatibilitySummary = MakeAnimationCompatibilitySummary(OutSnapshot.bHasSkeletalMeshComponent, OutSnapshot.bHasAnimBlueprint);
    OutSnapshot.AIReadinessSummary = MakeAIReadinessSummary(OutSnapshot.bIsPawnActor, OutSnapshot.bAutoPossessAIEnabled, OutSnapshot.bHasAIControllerClass);
    return true;
}

bool TryResolveCharacterEnhancementPreset(
    const FString& PresetLabel,
    bool& bOutNeedsIdentity,
    bool& bOutNeedsRelationship,
    bool& bOutNeedsWAI,
    FString& OutResolvedPresetLabel)
{
    const FString NormalizedPreset = PresetLabel.TrimStartAndEnd().ToLower();

    bOutNeedsIdentity = false;
    bOutNeedsRelationship = false;
    bOutNeedsWAI = false;

    if (NormalizedPreset == TEXT("identity only"))
    {
        bOutNeedsIdentity = true;
        OutResolvedPresetLabel = TEXT("Identity Only");
        return true;
    }

    if (NormalizedPreset == TEXT("relationship starter"))
    {
        bOutNeedsIdentity = true;
        bOutNeedsRelationship = true;
        OutResolvedPresetLabel = TEXT("Relationship Starter");
        return true;
    }

    if (NormalizedPreset == TEXT("full wanaai starter"))
    {
        bOutNeedsIdentity = true;
        bOutNeedsRelationship = true;
        bOutNeedsWAI = true;
        OutResolvedPresetLabel = TEXT("Full WanaAI Starter");
        return true;
    }

    return false;
}

void AppendRelationshipProfileLines(FWanaCommandResponse& Response, const AActor* SelectedActor, const FWAYRelationshipProfile& Profile)
{
    const EWAYReactionState ReactionState = UWAYPlayerProfileComponent::ResolveReactionForRelationshipState(Profile.RelationshipState);

    Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), *SelectedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), Profile.TargetActor ? *Profile.TargetActor->GetActorNameOrLabel() : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Relationship State: %s"), *GetRelationshipStateDisplayLabel(Profile.RelationshipState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Reaction: %s"), *GetReactionStateDisplayLabel(ReactionState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Trust: %.2f"), Profile.Trust));
    Response.OutputLines.Add(FString::Printf(TEXT("Fear: %.2f"), Profile.Fear));
    Response.OutputLines.Add(FString::Printf(TEXT("Respect: %.2f"), Profile.Respect));
    Response.OutputLines.Add(FString::Printf(TEXT("Attachment: %.2f"), Profile.Attachment));
    Response.OutputLines.Add(FString::Printf(TEXT("Hostility: %.2f"), Profile.Hostility));
}

void AppendIdentityLines(FWanaCommandResponse& Response, const FWanaSelectedActorIdentitySnapshot& Snapshot)
{
    Response.OutputLines.Add(FString::Printf(TEXT("Selected actor: %s"), Snapshot.bHasSelectedActor ? *Snapshot.SelectedActorLabel : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Identity Component: %s"), Snapshot.bHasIdentityComponent ? TEXT("Present") : TEXT("Missing")));

    if (Snapshot.bHasIdentityComponent)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Faction Tag: %s"), Snapshot.FactionTag.IsEmpty() ? TEXT("(none)") : *Snapshot.FactionTag));
        Response.OutputLines.Add(FString::Printf(
            TEXT("Default Relationship Seed: State=%s Trust=%.2f Fear=%.2f Respect=%.2f Attachment=%.2f Hostility=%.2f"),
            *GetRelationshipStateDisplayLabel(Snapshot.DefaultRelationshipSeed.RelationshipState),
            Snapshot.DefaultRelationshipSeed.Trust,
            Snapshot.DefaultRelationshipSeed.Fear,
            Snapshot.DefaultRelationshipSeed.Respect,
            Snapshot.DefaultRelationshipSeed.Attachment,
            Snapshot.DefaultRelationshipSeed.Hostility));
        Response.OutputLines.Add(FString::Printf(TEXT("Reputation Tags: %s"), *Snapshot.ReputationTagsSummary));
    }
}

FString MakeIdentitySeedSummary(const AActor* TargetActor)
{
    if (!TargetActor)
    {
        return TEXT("Missing (no target actor was available).");
    }

    const UWanaIdentityComponent* IdentityComponent = TargetActor->FindComponentByClass<UWanaIdentityComponent>();

    if (!IdentityComponent)
    {
        return TEXT("Missing on target. Neutral defaults were used.");
    }

    const FWAYRelationshipSeed& Seed = IdentityComponent->DefaultRelationshipSeed;
    return FString::Printf(
        TEXT("State=%s Trust=%.2f Fear=%.2f Respect=%.2f Attachment=%.2f Hostility=%.2f"),
        *GetRelationshipStateDisplayLabel(Seed.RelationshipState),
        Seed.Trust,
        Seed.Fear,
        Seed.Respect,
        Seed.Attachment,
        Seed.Hostility);
}

void AppendLinesWithPrefix(FWanaCommandResponse& Response, const FWanaCommandResponse& SourceResponse, const FString& Prefix)
{
    for (const FString& OutputLine : SourceResponse.OutputLines)
    {
        if (OutputLine.StartsWith(Prefix))
        {
            Response.OutputLines.Add(OutputLine);
        }
    }
}

FWanaCommandResponse ExecuteEvaluateActorPairInternal(AActor* ObserverActor, AActor* TargetActor, bool bTargetFallsBackToObserver)
{
    if (!ObserverActor || !TargetActor)
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Observer or target is missing."),
            TEXT("Select or assign both an observer and target actor before evaluating the pair."));
    }

    UWAYPlayerProfileComponent* ProfileComponent = ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>();
    FWAYRelationshipProfile ExistingProfile;
    const bool bHadRelationshipProfile = ProfileComponent && ProfileComponent->GetRelationshipProfileForTarget(TargetActor, ExistingProfile);
    const bool bNeedsWayComponent = ProfileComponent == nullptr;
    const bool bNeedsTransaction = bNeedsWayComponent || !bHadRelationshipProfile;

    TUniquePtr<FScopedTransaction> Transaction;

    if (bNeedsTransaction)
    {
        Transaction = MakeUnique<FScopedTransaction>(LOCTEXT("WanaWorksEvaluateLiveTargetTransaction", "Evaluate WanaAI Live Target"));
        ObserverActor->Modify();
    }

    if (!ProfileComponent)
    {
        ProfileComponent = FindOrAddPreferenceComponent(ObserverActor);
    }

    if (!ProfileComponent)
    {
        if (Transaction)
        {
            Transaction->Cancel();
        }

        return MakeEditorFailureResponse(
            TEXT("Status: Could not prepare observer WAY component."),
            TEXT("Failed to create or find UWAYPlayerProfileComponent for the observer."));
    }

    ProfileComponent->SetFlags(RF_Transactional);

    if (bNeedsTransaction)
    {
        ProfileComponent->Modify();
    }

    const FWAYTargetEvaluation Evaluation = ProfileComponent->EvaluateTarget(TargetActor);

    if (bNeedsTransaction)
    {
        ObserverActor->MarkPackageDirty();
    }

    TArray<FString> ReadinessNotes;

    if (bNeedsWayComponent)
    {
        ReadinessNotes.Add(TEXT("Observer was missing UWAYPlayerProfileComponent, so it was added automatically for the evaluation."));
    }

    if (!bHadRelationshipProfile)
    {
        ReadinessNotes.Add(TEXT("A relationship profile was created for this observer-target pair during evaluation."));
    }

    if (bTargetFallsBackToObserver)
    {
        ReadinessNotes.Add(TEXT("Target Source: Observer fallback (assign a separate target to test another actor)."));
    }
    else
    {
        ReadinessNotes.Add(TEXT("Target Source: Explicit target actor."));
    }

    if (!TargetActor->FindComponentByClass<UWanaIdentityComponent>())
    {
        ReadinessNotes.Add(TEXT("Target has no UWanaIdentityComponent, so neutral seed defaults were used."));
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Live target evaluation complete.");
    Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), *ObserverActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), *TargetActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(FString::Printf(TEXT("Identity Seed: %s"), *MakeIdentitySeedSummary(TargetActor)));
    Response.OutputLines.Add(FString::Printf(TEXT("Relationship State: %s"), *GetRelationshipStateDisplayLabel(Evaluation.RelationshipProfile.RelationshipState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Reaction State: %s"), *GetReactionStateDisplayLabel(Evaluation.ReactionState)));

    for (const FString& ReadinessNote : ReadinessNotes)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Readiness Notes: %s"), *ReadinessNote));
    }

    return Response;
}
}

namespace WanaWorksUIEditorActions
{
bool GetSelectedActorIdentitySnapshot(FWanaSelectedActorIdentitySnapshot& OutSnapshot)
{
    OutSnapshot = FWanaSelectedActorIdentitySnapshot();

    const AActor* SelectedActor = GetFirstSelectedActor();

    if (!SelectedActor)
    {
        return false;
    }

    OutSnapshot.bHasSelectedActor = true;
    OutSnapshot.SelectedActor = const_cast<AActor*>(SelectedActor);
    OutSnapshot.SelectedActorLabel = SelectedActor->GetActorNameOrLabel();

    if (const UWanaIdentityComponent* IdentityComponent = SelectedActor->FindComponentByClass<UWanaIdentityComponent>())
    {
        OutSnapshot.bHasIdentityComponent = true;
        OutSnapshot.FactionTag = IdentityComponent->FactionTag.IsNone() ? FString() : IdentityComponent->FactionTag.ToString();
        OutSnapshot.DefaultRelationshipSeed = IdentityComponent->DefaultRelationshipSeed;
        OutSnapshot.ReputationTagsSummary = MakeReputationTagsSummary(IdentityComponent->ReputationTags);
    }
    else
    {
        OutSnapshot.ReputationTagsSummary = TEXT("(none)");
    }

    return true;
}

bool GetSelectedCharacterEnhancementSnapshot(FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot)
{
    const AActor* SelectedActor = GetFirstSelectedActor();
    return BuildCharacterEnhancementSnapshot(SelectedActor, OutSnapshot);
}

bool GetSelectedRelationshipContextSnapshot(FWanaSelectedRelationshipContextSnapshot& OutSnapshot)
{
    OutSnapshot = FWanaSelectedRelationshipContextSnapshot();

    const AActor* ObserverActor = nullptr;
    const AActor* TargetActor = nullptr;
    bool bTargetFallsBackToObserver = false;

    if (!GetSelectedActorPair(ObserverActor, TargetActor, bTargetFallsBackToObserver) || !ObserverActor)
    {
        return false;
    }

    OutSnapshot.bHasObserverActor = true;
    OutSnapshot.ObserverActor = const_cast<AActor*>(ObserverActor);
    OutSnapshot.ObserverActorLabel = ObserverActor->GetActorNameOrLabel();
    OutSnapshot.bObserverHasRelationshipComponent = ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>() != nullptr;

    if (TargetActor)
    {
        OutSnapshot.bHasTargetActor = true;
        OutSnapshot.TargetActor = const_cast<AActor*>(TargetActor);
        OutSnapshot.TargetActorLabel = TargetActor->GetActorNameOrLabel();
    }

    OutSnapshot.bTargetFallsBackToObserver = bTargetFallsBackToObserver;
    return true;
}

FWanaCommandResponse ExecuteWeatherPresetCommand(const FString& PresetName)
{
    UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

    EWanaWeatherPreset Preset = EWanaWeatherPreset::Clear;

    if (PresetName.Equals(TEXT("Overcast"), ESearchCase::IgnoreCase))
    {
        Preset = EWanaWeatherPreset::Overcast;
    }
    else if (PresetName.Equals(TEXT("Storm"), ESearchCase::IgnoreCase))
    {
        Preset = EWanaWeatherPreset::Storm;
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksApplyWeatherTransaction", "Apply Wana Works Weather"));

    const FWanaWeatherApplyResult Result = FWanaWorksWeatherController::ApplyPreset(EditorWorld, Preset);

    if (!Result.bSucceeded)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(
            TEXT("Status: Weather update failed."),
            Result.ErrorMessage.IsEmpty() ? TEXT("Weather update failed.") : Result.ErrorMessage);
    }

    if (GEditor)
    {
        GEditor->RedrawLevelEditingViewports();
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = FString::Printf(TEXT("Status: Applied %s weather."), *Result.PresetName);
    Response.OutputLines.Add(FString::Printf(TEXT("Applied weather preset: %s"), *Result.PresetName));
    Response.OutputLines.Add(FString::Printf(TEXT("Updated scene systems: %s"), *FString::Join(Result.UpdatedSystems, TEXT(", "))));
    return Response;
}

FWanaCommandResponse ExecuteSpawnCubeCommand()
{
    UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

    if (!EditorWorld)
    {
        return MakeEditorFailureResponse(
            TEXT("Status: No editor world available."),
            TEXT("Could not spawn cube because no editor world is available."));
    }

    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (!CubeMesh)
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Could not load cube mesh."),
            TEXT("Could not load /Engine/BasicShapes/Cube.Cube."));
    }

    AStaticMeshActor* CubeActor = GEditor
        ? Cast<AStaticMeshActor>(GEditor->AddActor(EditorWorld->GetCurrentLevel(), AStaticMeshActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 50.0f))))
        : nullptr;

    if (!CubeActor)
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Cube spawn failed."),
            TEXT("Failed to spawn cube actor in the current editor world."));
    }

    CubeActor->Modify();

    if (UStaticMeshComponent* StaticMeshComponent = CubeActor->GetStaticMeshComponent())
    {
        StaticMeshComponent->SetFlags(RF_Transactional);
        StaticMeshComponent->Modify();
        StaticMeshComponent->SetStaticMesh(CubeMesh);
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Spawned cube.");
    Response.OutputLines.Add(FString::Printf(TEXT("Spawned cube actor: %s"), *CubeActor->GetActorNameOrLabel()));
    return Response;
}

FWanaCommandResponse ExecuteListSelectionCommand()
{
    if (!GEditor)
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Editor selection is unavailable."),
            TEXT("Could not inspect selection because GEditor is unavailable."));
    }

    USelection* SelectedActors = GEditor->GetSelectedActors();

    if (!SelectedActors || SelectedActors->Num() == 0)
    {
        FWanaCommandResponse Response;
        Response.bSucceeded = true;
        Response.StatusMessage = TEXT("Status: No actors selected.");
        Response.OutputLines.Add(TEXT("No actors selected."));
        return Response;
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Listed selection.");
    Response.OutputLines.Add(TEXT("Selected actors:"));

    for (FSelectionIterator SelectionIt(*SelectedActors); SelectionIt; ++SelectionIt)
    {
        if (const AActor* SelectedActor = Cast<AActor>(*SelectionIt))
        {
            Response.OutputLines.Add(SelectedActor->GetActorNameOrLabel());
        }
    }

    return Response;
}

FWanaCommandResponse ExecuteClassifySelectedCommand()
{
    const AActor* SelectedActor = GetFirstSelectedActor();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(
            TEXT("Status: No actors selected."),
            TEXT("No actors selected."));
    }

    TArray<FName> ActorTags = SelectedActor->Tags;

    TArray<UActorComponent*> Components;
    SelectedActor->GetComponents(Components);

    TArray<FName> ComponentClassNames;
    ComponentClassNames.Reserve(Components.Num());

    for (const UActorComponent* Component : Components)
    {
        if (Component)
        {
            ComponentClassNames.Add(Component->GetClass()->GetFName());
        }
    }

    const FWanaPredictionProfile Profile = UWITBlueprintLibrary::ClassifyActorHeuristically(
        FName(*SelectedActor->GetActorNameOrLabel()),
        SelectedActor->GetClass()->GetFName(),
        ActorTags,
        ComponentClassNames);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Classified selected actor.");
    Response.OutputLines.Add(FString::Printf(TEXT("Actor: %s"), *SelectedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(FString::Printf(TEXT("Class: %s"), *SelectedActor->GetClass()->GetName()));
    Response.OutputLines.Add(FString::Printf(TEXT("Tags: %s"), ActorTags.Num() > 0 ? *FString::JoinBy(ActorTags, TEXT(", "), [](const FName& Tag) { return Tag.ToString(); }) : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Components: %s"), ComponentClassNames.Num() > 0 ? *FString::JoinBy(ComponentClassNames, TEXT(", "), [](const FName& ComponentClassName) { return ComponentClassName.ToString(); }) : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Tag: %s"), *Profile.ClassificationTag.ToString()));
    Response.OutputLines.Add(FString::Printf(TEXT("Domain: %s"), *StaticEnum<EWanaBehaviorDomain>()->GetValueAsString(Profile.Domain)));
    Response.OutputLines.Add(FString::Printf(TEXT("Confidence: %.2f"), Profile.Confidence));
    return Response;
}

FWanaCommandResponse ExecuteAddMemoryTestCommand()
{
    AActor* SelectedActor = GetFirstSelectedActorMutable();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksAddMemoryTransaction", "Add Wana Works Memory Test"));
    SelectedActor->Modify();

    UWAIPersonalityComponent* MemoryComponent = FindOrAddMemoryComponent(SelectedActor);

    if (!MemoryComponent)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Could not add memory component."), TEXT("Failed to create or find UWAIPersonalityComponent."));
    }

    MemoryComponent->Modify();
    MemoryComponent->RememberEvent(TEXT("Test memory event"), 0.50f);
    SelectedActor->MarkPackageDirty();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Added memory test.");
    Response.OutputLines.Add(FString::Printf(TEXT("Actor: %s"), *SelectedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(TEXT("Added memory event: Test memory event"));
    Response.OutputLines.Add(TEXT("Emotional impact: 0.50"));
    return Response;
}

FWanaCommandResponse ExecuteAddPreferenceTestCommand()
{
    AActor* SelectedActor = GetFirstSelectedActorMutable();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksAddPreferenceTransaction", "Add Wana Works Preference Test"));
    SelectedActor->Modify();

    UWAYPlayerProfileComponent* PreferenceComponent = FindOrAddPreferenceComponent(SelectedActor);

    if (!PreferenceComponent)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Could not add preference component."), TEXT("Failed to create or find UWAYPlayerProfileComponent."));
    }

    PreferenceComponent->Modify();
    PreferenceComponent->RecordPreferenceSignal(TEXT("TestPreference"), 0.75f);
    SelectedActor->MarkPackageDirty();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Added preference test.");
    Response.OutputLines.Add(FString::Printf(TEXT("Actor: %s"), *SelectedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(TEXT("Added preference signal: TestPreference"));
    Response.OutputLines.Add(TEXT("Weight: 0.75"));
    return Response;
}

FWanaCommandResponse ExecuteShowMemoryCommand()
{
    const AActor* SelectedActor = GetFirstSelectedActor();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    const UWAIPersonalityComponent* MemoryComponent = SelectedActor->FindComponentByClass<UWAIPersonalityComponent>();

    if (!MemoryComponent)
    {
        return MakeEditorFailureResponse(TEXT("Status: No memory component found."), TEXT("Selected actor does not have a WAI personality component."));
    }

    const TArray<FWAIMemoryEvent>& Memories = MemoryComponent->GetMemories();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Showing memory.");
    Response.OutputLines.Add(FString::Printf(TEXT("Actor: %s"), *SelectedActor->GetActorNameOrLabel()));

    if (Memories.Num() == 0)
    {
        Response.OutputLines.Add(TEXT("Memory entries: none"));
        return Response;
    }

    Response.OutputLines.Add(TEXT("Memory entries:"));

    for (int32 Index = 0; Index < Memories.Num(); ++Index)
    {
        const FWAIMemoryEvent& Memory = Memories[Index];
        Response.OutputLines.Add(FString::Printf(TEXT("[%d] %s (impact: %.2f)"), Index, *Memory.Summary, Memory.EmotionalImpact));
    }

    return Response;
}

FWanaCommandResponse ExecuteShowPreferencesCommand()
{
    const AActor* SelectedActor = GetFirstSelectedActor();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    const UWAYPlayerProfileComponent* PreferenceComponent = SelectedActor->FindComponentByClass<UWAYPlayerProfileComponent>();

    if (!PreferenceComponent)
    {
        return MakeEditorFailureResponse(TEXT("Status: No preference component found."), TEXT("Selected actor does not have a WAY player profile component."));
    }

    const TArray<FWAYPreferenceSignal>& Signals = PreferenceComponent->GetSignals();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Showing preferences.");
    Response.OutputLines.Add(FString::Printf(TEXT("Actor: %s"), *SelectedActor->GetActorNameOrLabel()));

    if (Signals.Num() == 0)
    {
        Response.OutputLines.Add(TEXT("Preference entries: none"));
        return Response;
    }

    Response.OutputLines.Add(TEXT("Preference entries:"));

    for (int32 Index = 0; Index < Signals.Num(); ++Index)
    {
        const FWAYPreferenceSignal& Signal = Signals[Index];
        Response.OutputLines.Add(FString::Printf(TEXT("[%d] %s (weight: %.2f)"), Index, *Signal.SignalName.ToString(), Signal.Weight));
    }

    return Response;
}

FWanaCommandResponse ExecuteEnsureRelationshipProfileCommand()
{
    FWanaSelectedRelationshipContextSnapshot RelationshipContext;

    if (!GetSelectedRelationshipContextSnapshot(RelationshipContext))
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    AActor* ObserverActor = RelationshipContext.ObserverActor.Get();
    AActor* TargetActor = RelationshipContext.TargetActor.Get();

    FWAYRelationshipProfile ExistingProfile;
    const UWAYPlayerProfileComponent* ExistingComponent = ObserverActor ? ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>() : nullptr;
    const bool bProfileAlreadyExists = ExistingComponent && ExistingComponent->GetRelationshipProfileForTarget(TargetActor, ExistingProfile);

    if (bProfileAlreadyExists)
    {
        FWanaCommandResponse Response;
        Response.bSucceeded = true;
        Response.StatusMessage = TEXT("Status: Relationship profile already exists.");
        Response.OutputLines.Add(TEXT("Relationship profile already exists for the current observer and target."));

        if (RelationshipContext.bTargetFallsBackToObserver)
        {
            Response.OutputLines.Add(TEXT("Target Source: Observer fallback (select a second actor to use a separate target)."));
        }

        AppendRelationshipProfileLines(Response, ObserverActor, ExistingProfile);
        return Response;
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksEnsureRelationshipProfileTransaction", "Ensure Wana Works Relationship Profile"));
    ObserverActor->Modify();

    UWAYPlayerProfileComponent* ProfileComponent = FindOrAddPreferenceComponent(ObserverActor);

    if (!ProfileComponent)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Could not add relationship component."), TEXT("Failed to create or find UWAYPlayerProfileComponent."));
    }

    ProfileComponent->Modify();
    const FWAYRelationshipProfile Profile = ProfileComponent->EnsureRelationshipProfileForTarget(TargetActor);
    ObserverActor->MarkPackageDirty();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Ensured relationship profile.");
    Response.OutputLines.Add(TEXT("Ensured relationship profile for the current observer and target."));

    if (RelationshipContext.bTargetFallsBackToObserver)
    {
        Response.OutputLines.Add(TEXT("Target Source: Observer fallback (select a second actor to use a separate target)."));
    }

    AppendRelationshipProfileLines(Response, ObserverActor, Profile);
    return Response;
}

FWanaCommandResponse ExecuteShowRelationshipProfileCommand()
{
    FWanaSelectedRelationshipContextSnapshot RelationshipContext;

    if (!GetSelectedRelationshipContextSnapshot(RelationshipContext))
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    const AActor* ObserverActor = RelationshipContext.ObserverActor.Get();
    const AActor* TargetActor = RelationshipContext.TargetActor.Get();

    const UWAYPlayerProfileComponent* ProfileComponent = ObserverActor ? ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>() : nullptr;

    if (!ProfileComponent)
    {
        return MakeEditorFailureResponse(TEXT("Status: No relationship component found."), TEXT("The selected observer does not have a WAY player profile component."));
    }

    FWAYRelationshipProfile Profile;

    if (!ProfileComponent->GetRelationshipProfileForTarget(const_cast<AActor*>(TargetActor), Profile))
    {
        return MakeEditorFailureResponse(TEXT("Status: No relationship profile found."), TEXT("Run Ensure Relationship Profile first for the current observer and target."));
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Showing relationship profile.");

    if (RelationshipContext.bTargetFallsBackToObserver)
    {
        Response.OutputLines.Add(TEXT("Target Source: Observer fallback (select a second actor to use a separate target)."));
    }

    AppendRelationshipProfileLines(Response, ObserverActor, Profile);
    return Response;
}

FWanaCommandResponse ExecuteApplyRelationshipStateCommand(const FString& RelationshipStateText)
{
    FWanaSelectedRelationshipContextSnapshot RelationshipContext;

    if (!GetSelectedRelationshipContextSnapshot(RelationshipContext))
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    AActor* ObserverActor = RelationshipContext.ObserverActor.Get();
    AActor* TargetActor = RelationshipContext.TargetActor.Get();

    EWAYRelationshipState RelationshipState = EWAYRelationshipState::Neutral;

    if (!TryParseRelationshipState(RelationshipStateText, RelationshipState))
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Invalid relationship state."),
            TEXT("Use one of: Acquaintance, Friend, Partner, Enemy, Neutral."));
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksApplyRelationshipStateTransaction", "Apply Wana Works Relationship State"));
    ObserverActor->Modify();

    UWAYPlayerProfileComponent* ProfileComponent = FindOrAddPreferenceComponent(ObserverActor);

    if (!ProfileComponent)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Could not add relationship component."), TEXT("Failed to create or find UWAYPlayerProfileComponent."));
    }

    ProfileComponent->Modify();
    ProfileComponent->EnsureRelationshipProfileForTarget(TargetActor);
    ProfileComponent->SetRelationshipStateForTarget(TargetActor, RelationshipState);
    ObserverActor->MarkPackageDirty();

    FWAYRelationshipProfile UpdatedProfile;
    ProfileComponent->GetRelationshipProfileForTarget(TargetActor, UpdatedProfile);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Applied relationship state.");
    Response.OutputLines.Add(FString::Printf(TEXT("Applied relationship state: %s"), *GetRelationshipStateDisplayLabel(RelationshipState)));

    if (RelationshipContext.bTargetFallsBackToObserver)
    {
        Response.OutputLines.Add(TEXT("Target Source: Observer fallback (select a second actor to use a separate target)."));
    }

    AppendRelationshipProfileLines(Response, ObserverActor, UpdatedProfile);
    return Response;
}

FWanaCommandResponse ExecuteEnsureIdentityComponentCommand()
{
    AActor* SelectedActor = GetFirstSelectedActorMutable();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    if (SelectedActor->FindComponentByClass<UWanaIdentityComponent>())
    {
        FWanaSelectedActorIdentitySnapshot Snapshot;
        GetSelectedActorIdentitySnapshot(Snapshot);

        FWanaCommandResponse Response;
        Response.bSucceeded = true;
        Response.StatusMessage = TEXT("Status: Identity component already exists.");
        Response.OutputLines.Add(TEXT("Identity component already exists for the selected actor."));
        AppendIdentityLines(Response, Snapshot);
        return Response;
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksEnsureIdentityComponentTransaction", "Ensure Wana Works Identity Component"));
    SelectedActor->Modify();

    UWanaIdentityComponent* IdentityComponent = FindOrAddIdentityComponent(SelectedActor);

    if (!IdentityComponent)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Could not add identity component."), TEXT("Failed to create or find UWanaIdentityComponent."));
    }

    SelectedActor->MarkPackageDirty();

    FWanaSelectedActorIdentitySnapshot Snapshot;
    GetSelectedActorIdentitySnapshot(Snapshot);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Ensured identity component.");
    Response.OutputLines.Add(TEXT("Ensured identity component for the selected actor."));
    AppendIdentityLines(Response, Snapshot);
    return Response;
}

FWanaCommandResponse ExecuteApplyIdentityCommand(const FString& FactionTagText, EWAYRelationshipState DefaultRelationshipState)
{
    AActor* SelectedActor = GetFirstSelectedActorMutable();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksApplyIdentityTransaction", "Apply Wana Works Identity"));
    SelectedActor->Modify();

    UWanaIdentityComponent* IdentityComponent = FindOrAddIdentityComponent(SelectedActor);

    if (!IdentityComponent)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Could not add identity component."), TEXT("Failed to create or find UWanaIdentityComponent."));
    }

    IdentityComponent->Modify();
    IdentityComponent->FactionTag = FactionTagText.TrimStartAndEnd().IsEmpty() ? NAME_None : FName(*FactionTagText.TrimStartAndEnd());
    IdentityComponent->DefaultRelationshipSeed.RelationshipState = DefaultRelationshipState;
    SelectedActor->MarkPackageDirty();

    FWanaSelectedActorIdentitySnapshot Snapshot;
    GetSelectedActorIdentitySnapshot(Snapshot);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Applied identity values.");
    Response.OutputLines.Add(TEXT("Applied identity values to the selected actor."));
    AppendIdentityLines(Response, Snapshot);
    return Response;
}

FWanaCommandResponse ExecuteApplyCharacterEnhancementCommand(const FString& PresetLabel)
{
    AActor* SelectedActor = GetFirstSelectedActorMutable();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    bool bNeedsIdentity = false;
    bool bNeedsRelationship = false;
    bool bNeedsWAI = false;
    FString ResolvedPresetLabel;

    if (!TryResolveCharacterEnhancementPreset(PresetLabel, bNeedsIdentity, bNeedsRelationship, bNeedsWAI, ResolvedPresetLabel))
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Invalid enhancement preset."),
            TEXT("Use one of: Identity Only, Relationship Starter, Full WanaAI Starter."));
    }

    const bool bHadIdentity = SelectedActor->FindComponentByClass<UWanaIdentityComponent>() != nullptr;
    const bool bHadRelationship = SelectedActor->FindComponentByClass<UWAYPlayerProfileComponent>() != nullptr;
    const bool bHadWAI = SelectedActor->FindComponentByClass<UWAIPersonalityComponent>() != nullptr;

    TArray<FString> AddedComponents;
    TArray<FString> ExistingComponents;
    TArray<FString> CompatibilityNotes;

    if (bNeedsIdentity && bHadIdentity)
    {
        ExistingComponents.Add(TEXT("UWanaIdentityComponent"));
    }

    if (bNeedsRelationship && bHadRelationship)
    {
        ExistingComponents.Add(TEXT("UWAYPlayerProfileComponent"));
    }

    if (bNeedsWAI && bHadWAI)
    {
        ExistingComponents.Add(TEXT("UWAIPersonalityComponent"));
    }

    if ((!bNeedsIdentity || bHadIdentity) &&
        (!bNeedsRelationship || bHadRelationship) &&
        (!bNeedsWAI || bHadWAI))
    {
        FWanaCommandResponse Response;
        Response.bSucceeded = true;
        Response.StatusMessage = TEXT("Status: WanaAI enhancement already applied.");
        Response.OutputLines.Add(FString::Printf(TEXT("Selected Actor: %s"), *SelectedActor->GetActorNameOrLabel()));
        Response.OutputLines.Add(FString::Printf(TEXT("Preset Used: %s"), *ResolvedPresetLabel));
        Response.OutputLines.Add(TEXT("Components Added: (none)"));
        Response.OutputLines.Add(FString::Printf(TEXT("Already Present: %s"), *JoinOrNone(ExistingComponents)));
        Response.OutputLines.Add(TEXT("Compatibility Notes: Existing setup preserved. No duplicate components were added."));

        if (bNeedsWAI)
        {
            Response.OutputLines.Add(TEXT("Compatibility Notes: Full WanaAI Starter uses UWAIPersonalityComponent as the current WAI starter component."));
        }

        return Response;
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksApplyCharacterEnhancementTransaction", "Apply WanaAI Character Enhancement"));
    SelectedActor->Modify();

    if (bNeedsIdentity && !bHadIdentity)
    {
        if (!FindOrAddIdentityComponent(SelectedActor))
        {
            Transaction.Cancel();
            return MakeEditorFailureResponse(TEXT("Status: Could not add identity component."), TEXT("Failed to create UWanaIdentityComponent."));
        }

        AddedComponents.Add(TEXT("UWanaIdentityComponent"));
    }

    if (bNeedsRelationship && !bHadRelationship)
    {
        if (!FindOrAddPreferenceComponent(SelectedActor))
        {
            Transaction.Cancel();
            return MakeEditorFailureResponse(TEXT("Status: Could not add WAY component."), TEXT("Failed to create UWAYPlayerProfileComponent."));
        }

        AddedComponents.Add(TEXT("UWAYPlayerProfileComponent"));
    }

    if (bNeedsWAI && !bHadWAI)
    {
        if (!FindOrAddMemoryComponent(SelectedActor))
        {
            Transaction.Cancel();
            return MakeEditorFailureResponse(TEXT("Status: Could not add WAI component."), TEXT("Failed to create UWAIPersonalityComponent."));
        }

        AddedComponents.Add(TEXT("UWAIPersonalityComponent"));
    }

    SelectedActor->MarkPackageDirty();

    CompatibilityNotes.Add(TEXT("Existing setup preserved. No existing components were replaced."));
    CompatibilityNotes.Add(TEXT("This enhancement only adds missing starter components for the selected actor."));

    if (bNeedsWAI)
    {
        CompatibilityNotes.Add(TEXT("Full WanaAI Starter uses UWAIPersonalityComponent as the current WAI starter component."));
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = FString::Printf(TEXT("Status: Applied %s preset."), *ResolvedPresetLabel);
    Response.OutputLines.Add(FString::Printf(TEXT("Selected Actor: %s"), *SelectedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(FString::Printf(TEXT("Preset Used: %s"), *ResolvedPresetLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Components Added: %s"), *JoinOrNone(AddedComponents)));
    Response.OutputLines.Add(FString::Printf(TEXT("Already Present: %s"), *JoinOrNone(ExistingComponents)));

    for (const FString& CompatibilityNote : CompatibilityNotes)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Compatibility Notes: %s"), *CompatibilityNote));
    }

    return Response;
}

FWanaCommandResponse ExecuteCreateSandboxDuplicateCommand()
{
    AActor* SelectedActor = GetFirstSelectedActorMutable();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("Select a character or pawn before creating a sandbox duplicate."));
    }

    UWorld* EditorWorld = SelectedActor->GetWorld();

    if (!GEditor || !EditorWorld || !SelectedActor->GetLevel())
    {
        return MakeEditorFailureResponse(TEXT("Status: Sandbox duplicate failed."), TEXT("Could not access the editor world for sandbox duplication."));
    }

    const FTransform SourceTransform = SelectedActor->GetActorTransform();
    FTransform DuplicateTransform = SourceTransform;
    DuplicateTransform.AddToTranslation(FVector(180.0f, 0.0f, 0.0f));

    FScopedTransaction Transaction(LOCTEXT("WanaWorksCreateSandboxDuplicateTransaction", "Create WanaWorks Sandbox Duplicate"));
    AActor* DuplicateActor = GEditor->AddActor(SelectedActor->GetLevel(), SelectedActor->GetClass(), DuplicateTransform);

    if (!DuplicateActor)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Sandbox duplicate failed."), TEXT("Could not create a sandbox duplicate for the selected actor."));
    }

    DuplicateActor->SetFlags(RF_Transactional);
    DuplicateActor->Modify();
    DuplicateActor->SetActorLabel(FString::Printf(TEXT("WW_Sandbox_%s"), *SelectedActor->GetActorNameOrLabel()), true);
    DuplicateActor->SetFolderPath(FName(TEXT("WanaWorks/Sandbox")));
    DuplicateActor->MarkPackageDirty();

    GEditor->SelectNone(false, true, false);
    GEditor->SelectActor(DuplicateActor, true, true, true);
    GEditor->NoteSelectionChange();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Created sandbox duplicate.");
    Response.OutputLines.Add(FString::Printf(TEXT("Selected Actor: %s"), *SelectedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(TEXT("Workflow Path: Create Sandbox Duplicate"));
    Response.OutputLines.Add(TEXT("Subject Mode: Sandbox duplicate"));
    Response.OutputLines.Add(FString::Printf(TEXT("Active Subject: %s"), *DuplicateActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(TEXT("Compatibility Notes: The original actor was left untouched."));
    Response.OutputLines.Add(TEXT("Compatibility Notes: The sandbox copy was placed in the WanaWorks/Sandbox folder and selected for continued setup."));
    return Response;
}

FWanaCommandResponse ExecutePrepareSelectedActorForAITestCommand()
{
    AActor* SelectedActor = GetFirstSelectedActorMutable();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("Select a character or pawn before preparing an AI-ready test subject."));
    }

    APawn* SelectedPawn = Cast<APawn>(SelectedActor);

    if (!SelectedPawn)
    {
        FWanaCommandResponse Response;
        Response.bSucceeded = true;
        Response.StatusMessage = TEXT("Status: AI-ready prep not applicable.");
        Response.OutputLines.Add(FString::Printf(TEXT("Selected Actor: %s"), *SelectedActor->GetActorNameOrLabel()));
        Response.OutputLines.Add(TEXT("AI-Ready Preparation: Not applied"));
        Response.OutputLines.Add(TEXT("Compatibility Notes: AI-ready prep only applies to Pawn or Character actors."));
        return Response;
    }

    const bool bAutoPossessAIAlreadyEnabled = SelectedPawn->AutoPossessAI != EAutoPossessAI::Disabled;

    if (!bAutoPossessAIAlreadyEnabled)
    {
        FScopedTransaction Transaction(LOCTEXT("WanaWorksPrepareAITestSubjectTransaction", "Prepare WanaWorks AI-Ready Test Subject"));
        SelectedPawn->Modify();
        SelectedPawn->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
        SelectedPawn->MarkPackageDirty();
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = bAutoPossessAIAlreadyEnabled
        ? TEXT("Status: AI-ready prep already configured.")
        : TEXT("Status: Applied AI-ready prep.");
    Response.OutputLines.Add(FString::Printf(TEXT("Selected Actor: %s"), *SelectedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(FString::Printf(TEXT("AI-Ready Preparation: %s"), bAutoPossessAIAlreadyEnabled ? TEXT("Already configured") : TEXT("Applied")));
    Response.OutputLines.Add(FString::Printf(TEXT("AI Controller Class: %s"), SelectedPawn->AIControllerClass ? TEXT("Present") : TEXT("Missing")));
    Response.OutputLines.Add(TEXT("Compatibility Notes: Existing Character BP and animation setup were preserved."));

    if (!SelectedPawn->AIControllerClass)
    {
        Response.OutputLines.Add(TEXT("Compatibility Notes: Assign an AIControllerClass later if you want autonomous controller-driven behavior."));
    }

    return Response;
}

FWanaCommandResponse ExecuteEvaluateLiveTargetCommand()
{
    FWanaSelectedRelationshipContextSnapshot RelationshipContext;

    if (!GetSelectedRelationshipContextSnapshot(RelationshipContext))
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("No actors selected."));
    }

    AActor* ObserverActor = RelationshipContext.ObserverActor.Get();
    AActor* TargetActor = RelationshipContext.TargetActor.Get();

    return ExecuteEvaluateActorPairInternal(ObserverActor, TargetActor, RelationshipContext.bTargetFallsBackToObserver);
}

FWanaCommandResponse ExecuteEvaluateActorPairCommand(AActor* ObserverActor, AActor* TargetActor, bool bTargetFallsBackToObserver)
{
    return ExecuteEvaluateActorPairInternal(ObserverActor, TargetActor, bTargetFallsBackToObserver);
}

FWanaCommandResponse ExecuteFocusActorCommand(AActor* Actor, const FString& RoleLabel)
{
    if (!GEditor)
    {
        return MakeEditorFailureResponse(TEXT("Status: Editor viewport is unavailable."), TEXT("Could not focus actor because GEditor is unavailable."));
    }

    if (!Actor)
    {
        return MakeEditorFailureResponse(
            FString::Printf(TEXT("Status: No %s is assigned."), *RoleLabel.ToLower()),
            FString::Printf(TEXT("Assign a %s in the sandbox before focusing it."), *RoleLabel.ToLower()));
    }

    GEditor->MoveViewportCamerasToActor(*Actor, false);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = FString::Printf(TEXT("Status: Focused %s."), *RoleLabel.ToLower());
    Response.OutputLines.Add(FString::Printf(TEXT("%s: %s"), *RoleLabel, *Actor->GetActorNameOrLabel()));
    Response.OutputLines.Add(TEXT("Readiness Notes: Viewport camera moved to the assigned sandbox actor."));
    return Response;
}

FWanaCommandResponse ExecuteApplyStarterAndTestTargetCommand(const FString& PresetLabel)
{
    const FWanaCommandResponse EnhancementResponse = ExecuteApplyCharacterEnhancementCommand(PresetLabel);

    if (!EnhancementResponse.bSucceeded)
    {
        return EnhancementResponse;
    }

    const FWanaCommandResponse EvaluationResponse = ExecuteEvaluateLiveTargetCommand();

    if (!EvaluationResponse.bSucceeded)
    {
        FWanaCommandResponse Response = EvaluationResponse;
        Response.OutputLines.Insert(TEXT("Guided Workflow: Starter preset was applied before the live test stopped."), 0);
        AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Preset Used:"));
        AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Components Added:"));
        AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Already Present:"));
        AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Compatibility Notes:"));
        return Response;
    }

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Applied starter and evaluated target.");
    Response.OutputLines.Add(TEXT("Guided Workflow: Applied the selected starter preset and immediately evaluated the current target."));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Observer:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Target:"));
    AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Preset Used:"));
    AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Components Added:"));
    AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Already Present:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Identity Seed:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Relationship State:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Reaction State:"));
    AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Compatibility Notes:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Readiness Notes:"));
    return Response;
}
}

#undef LOCTEXT_NAMESPACE

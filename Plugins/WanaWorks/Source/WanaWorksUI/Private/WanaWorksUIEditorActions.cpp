#include "WanaWorksUIEditorActions.h"

#include "Editor.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "ScopedTransaction.h"
#include "UObject/SoftObjectPath.h"
#include "WAIPersonalityComponent.h"
#include "WAYPlayerProfileComponent.h"
#include "WanaAutoAnimationIntegrationComponent.h"
#include "WanaPhysicalStateComponent.h"
#include "WanaIdentityComponent.h"
#include "WanaWorksUIFormattingUtils.h"
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

FString GetBehaviorPresetDisplayLabel(EWAYBehaviorPreset BehaviorPreset)
{
    const UEnum* BehaviorEnum = StaticEnum<EWAYBehaviorPreset>();
    return BehaviorEnum
        ? BehaviorEnum->GetDisplayNameTextByValue(static_cast<int64>(BehaviorPreset)).ToString()
        : TEXT("None");
}

FString GetBehaviorExecutionModeDisplayLabel(EWAYBehaviorExecutionMode ExecutionMode)
{
    const UEnum* ExecutionModeEnum = StaticEnum<EWAYBehaviorExecutionMode>();
    return ExecutionModeEnum
        ? ExecutionModeEnum->GetDisplayNameTextByValue(static_cast<int64>(ExecutionMode)).ToString()
        : TEXT("Unknown");
}

FString GetAnimationHookApplicationStatusDisplayLabel(EWAYAnimationHookApplicationStatus ApplicationStatus)
{
    const UEnum* ApplicationStatusEnum = StaticEnum<EWAYAnimationHookApplicationStatus>();
    return ApplicationStatusEnum
        ? ApplicationStatusEnum->GetDisplayNameTextByValue(static_cast<int64>(ApplicationStatus)).ToString()
        : TEXT("Not Available");
}

FString GetAutomaticAnimationIntegrationStatusDisplayLabel(EWAYAutomaticAnimationIntegrationStatus IntegrationStatus)
{
    const UEnum* IntegrationStatusEnum = StaticEnum<EWAYAutomaticAnimationIntegrationStatus>();
    return IntegrationStatusEnum
        ? IntegrationStatusEnum->GetDisplayNameTextByValue(static_cast<int64>(IntegrationStatus)).ToString()
        : TEXT("Not Supported");
}

const TCHAR* GetAnimationHookRequestDisplayLabel(bool bRequested)
{
    return bRequested ? TEXT("Requested") : TEXT("Idle");
}

void AppendBehaviorExecutionLines(
    FWanaCommandResponse& Response,
    EWAYBehaviorExecutionMode ExecutionMode,
    const FWanaMovementReadiness& MovementReadiness);

struct FWanaAutomaticAnimationPreparationResult
{
    bool bHasSkeletalAnimationStack = false;
    bool bAutoAttachSucceeded = false;
    bool bAutoWireSucceeded = false;
    bool bAddedPhysicalStateComponent = false;
    bool bAddedAutomaticIntegrationComponent = false;
    FString DetectedSourceAnimBlueprintLabel;
    FString IntegrationTargetLabel;
    FString Detail;
    EWAYAutomaticAnimationIntegrationStatus IntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::NotSupported;
};

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

UWanaPhysicalStateComponent* FindOrAddPhysicalStateComponent(AActor* Actor)
{
    if (!Actor)
    {
        return nullptr;
    }

    if (UWanaPhysicalStateComponent* ExistingComponent = Actor->FindComponentByClass<UWanaPhysicalStateComponent>())
    {
        ExistingComponent->SetFlags(RF_Transactional);
        return ExistingComponent;
    }

    Actor->Modify();

    UWanaPhysicalStateComponent* NewComponent = NewObject<UWanaPhysicalStateComponent>(Actor, UWanaPhysicalStateComponent::StaticClass(), NAME_None, RF_Transactional);

    if (!NewComponent)
    {
        return nullptr;
    }

    NewComponent->SetFlags(RF_Transactional);
    Actor->AddInstanceComponent(NewComponent);
    NewComponent->OnComponentCreated();
    NewComponent->RegisterComponent();
    NewComponent->RefreshPhysicalReadiness();
    return NewComponent;
}

UWanaAutoAnimationIntegrationComponent* FindOrAddAutomaticAnimationIntegrationComponent(AActor* Actor)
{
    if (!Actor)
    {
        return nullptr;
    }

    if (UWanaAutoAnimationIntegrationComponent* ExistingComponent = Actor->FindComponentByClass<UWanaAutoAnimationIntegrationComponent>())
    {
        ExistingComponent->SetFlags(RF_Transactional);
        return ExistingComponent;
    }

    Actor->Modify();

    UWanaAutoAnimationIntegrationComponent* NewComponent = NewObject<UWanaAutoAnimationIntegrationComponent>(Actor, UWanaAutoAnimationIntegrationComponent::StaticClass(), NAME_None, RF_Transactional);

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

FWanaAutomaticAnimationPreparationResult PrepareAutomaticAnimationIntegrationForActor(AActor* Actor)
{
    FWanaAutomaticAnimationPreparationResult Result;

    if (!Actor)
    {
        Result.Detail = TEXT("Automatic Anim BP integration could not be prepared because no working subject was available.");
        return Result;
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
    Result.bHasSkeletalAnimationStack = SkeletalMeshComponents.Num() > 0;

    if (!Result.bHasSkeletalAnimationStack)
    {
        Result.Detail = TEXT("Automatic Anim BP integration is not supported for this subject because no skeletal animation stack was detected.");
        Result.IntegrationTargetLabel = FString::Printf(TEXT("%s -> (no skeletal animation stack)"), *Actor->GetActorNameOrLabel());
        return Result;
    }

    bool bHasLinkedAnimationClass = false;

    for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (!SkeletalMeshComponent)
        {
            continue;
        }

        if (UClass* AnimClass = SkeletalMeshComponent->GetAnimClass())
        {
            Result.DetectedSourceAnimBlueprintLabel = AnimClass->GetName();
            Result.IntegrationTargetLabel = FString::Printf(TEXT("%s -> %s"), *SkeletalMeshComponent->GetName(), *AnimClass->GetName());
            bHasLinkedAnimationClass = true;
            break;
        }

        if (UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance())
        {
            Result.DetectedSourceAnimBlueprintLabel = AnimInstance->GetClass()->GetName();
            Result.IntegrationTargetLabel = FString::Printf(TEXT("%s -> %s"), *SkeletalMeshComponent->GetName(), *AnimInstance->GetClass()->GetName());
            bHasLinkedAnimationClass = true;
            break;
        }
    }

    if (!bHasLinkedAnimationClass)
    {
        Result.IntegrationTargetLabel = FString::Printf(TEXT("%s -> (no Animation Blueprint detected)"), *Actor->GetActorNameOrLabel());
        Result.IntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::NotSupported;
        Result.Detail = TEXT("Automatic Anim BP integration is not supported yet because the working subject has no linked Animation Blueprint.");
        return Result;
    }

    const bool bHadPhysicalStateComponent = Actor->FindComponentByClass<UWanaPhysicalStateComponent>() != nullptr;
    const bool bHadAutomaticIntegrationComponent = Actor->FindComponentByClass<UWanaAutoAnimationIntegrationComponent>() != nullptr;

    UWanaPhysicalStateComponent* PhysicalStateComponent = FindOrAddPhysicalStateComponent(Actor);
    UWanaAutoAnimationIntegrationComponent* AutomaticIntegrationComponent = FindOrAddAutomaticAnimationIntegrationComponent(Actor);

    if (!PhysicalStateComponent || !AutomaticIntegrationComponent)
    {
        Result.IntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Limited;
        Result.Detail = TEXT("Automatic Anim BP integration could only be prepared partially because a required WanaWorks runtime helper component could not be attached.");
        Result.bAutoAttachSucceeded = PhysicalStateComponent != nullptr || AutomaticIntegrationComponent != nullptr;
        return Result;
    }

    Result.bAddedPhysicalStateComponent = !bHadPhysicalStateComponent;
    Result.bAddedAutomaticIntegrationComponent = !bHadAutomaticIntegrationComponent;
    Result.bAutoAttachSucceeded = true;

    PhysicalStateComponent->RefreshPhysicalReadiness();
    AutomaticIntegrationComponent->RefreshAutomaticAnimationIntegration();

    Result.bAutoWireSucceeded = AutomaticIntegrationComponent->bAutoWireSucceeded;
    Result.IntegrationStatus = AutomaticIntegrationComponent->AutomaticIntegrationStatus;

    if (!AutomaticIntegrationComponent->SourceAnimationBlueprintLabel.IsEmpty())
    {
        Result.DetectedSourceAnimBlueprintLabel = AutomaticIntegrationComponent->SourceAnimationBlueprintLabel;
    }

    if (!AutomaticIntegrationComponent->IntegrationTargetLabel.IsEmpty())
    {
        Result.IntegrationTargetLabel = AutomaticIntegrationComponent->IntegrationTargetLabel;
    }

    Result.Detail = AutomaticIntegrationComponent->Detail;
    return Result;
}

void AppendAutomaticAnimationIntegrationLines(FWanaCommandResponse& Response, const FWanaAutomaticAnimationPreparationResult& Result)
{
    Response.OutputLines.Add(FString::Printf(
        TEXT("Detected Source Anim BP: %s"),
        Result.DetectedSourceAnimBlueprintLabel.IsEmpty() ? TEXT("(not detected)") : *Result.DetectedSourceAnimBlueprintLabel));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Workspace Animation Integration Target: %s"),
        Result.IntegrationTargetLabel.IsEmpty() ? TEXT("(not prepared)") : *Result.IntegrationTargetLabel));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Automatic Anim Integration: %s"),
        *GetAutomaticAnimationIntegrationStatusDisplayLabel(Result.IntegrationStatus)));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Auto-Attach: %s"),
        Result.bAutoAttachSucceeded ? TEXT("Yes") : TEXT("No")));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Auto-Wire: %s"),
        Result.bAutoWireSucceeded ? TEXT("Yes") : TEXT("No")));

    if (Result.bAddedPhysicalStateComponent)
    {
        Response.OutputLines.Add(TEXT("Generated Runtime Layer: Added UWanaPhysicalStateComponent to the working subject."));
    }

    if (Result.bAddedAutomaticIntegrationComponent)
    {
        Response.OutputLines.Add(TEXT("Generated Runtime Layer: Added UWanaAutoAnimationIntegrationComponent to the working subject."));
    }

    if (!Result.Detail.IsEmpty())
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Animation Integration Notes: %s"), *Result.Detail));
    }
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

FString GetActorTypeLabel(
    const AActor* Actor,
    bool bHasAIControllerClass,
    bool bAutoPossessAIEnabled,
    bool bHasSkeletalMeshComponent)
{
    if (Cast<APawn>(Actor))
    {
        return (bHasAIControllerClass || bAutoPossessAIEnabled) ? TEXT("AI Pawn") : TEXT("Character Pawn");
    }

    if (bHasSkeletalMeshComponent)
    {
        return TEXT("Character Actor");
    }

    return TEXT("Unknown / Unsupported");
}

FString GetAnimBlueprintLabelFromActor(const AActor* Actor)
{
    if (!Actor)
    {
        return TEXT("(not linked)");
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);

    for (const USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (!SkeletalMeshComponent)
        {
            continue;
        }

        if (const UClass* AnimClass = const_cast<USkeletalMeshComponent*>(SkeletalMeshComponent)->GetAnimClass())
        {
            return AnimClass->GetName();
        }

        if (const UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance())
        {
            return AnimInstance->GetClass()->GetName();
        }
    }

    return TEXT("(not linked)");
}

FString GetAIControllerLabelFromActor(const AActor* Actor)
{
    const APawn* Pawn = Cast<APawn>(Actor);

    if (!Pawn || !Pawn->AIControllerClass)
    {
        return TEXT("(not linked)");
    }

    return Pawn->AIControllerClass->GetName();
}

FString GetAutoPossessPlayerLabelFromPawn(const APawn* Pawn)
{
    if (!Pawn)
    {
        return TEXT("(not a Pawn)");
    }

    if (Pawn->AutoPossessPlayer == EAutoReceiveInput::Disabled)
    {
        return TEXT("Disabled");
    }

    if (Pawn->AutoPossessPlayer == EAutoReceiveInput::Player0)
    {
        return TEXT("Player 0");
    }

    if (Pawn->AutoPossessPlayer == EAutoReceiveInput::Player1)
    {
        return TEXT("Player 1");
    }

    if (Pawn->AutoPossessPlayer == EAutoReceiveInput::Player2)
    {
        return TEXT("Player 2");
    }

    if (Pawn->AutoPossessPlayer == EAutoReceiveInput::Player3)
    {
        return TEXT("Player 3");
    }

    if (Pawn->AutoPossessPlayer == EAutoReceiveInput::Player4)
    {
        return TEXT("Player 4");
    }

    if (Pawn->AutoPossessPlayer == EAutoReceiveInput::Player5)
    {
        return TEXT("Player 5");
    }

    if (Pawn->AutoPossessPlayer == EAutoReceiveInput::Player6)
    {
        return TEXT("Player 6");
    }

    if (Pawn->AutoPossessPlayer == EAutoReceiveInput::Player7)
    {
        return TEXT("Player 7");
    }

    return TEXT("Enabled");
}

bool ActorHasComponentClassNameToken(const AActor* Actor, const FString& Token)
{
    if (!Actor || Token.IsEmpty())
    {
        return false;
    }

    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);

    for (const UActorComponent* Component : Components)
    {
        const UClass* ComponentClass = Component ? Component->GetClass() : nullptr;
        const FString ComponentClassName = ComponentClass ? ComponentClass->GetName() : FString();

        if (ComponentClassName.Contains(Token, ESearchCase::IgnoreCase))
        {
            return true;
        }
    }

    return false;
}

bool GetSkeletalMeshAndSkeletonLabelsFromActor(const AActor* Actor, FString& OutMeshLabel, FString& OutSkeletonLabel)
{
    OutMeshLabel.Reset();
    OutSkeletonLabel.Reset();

    const USkeletalMeshComponent* SkeletalMeshComponent = Actor ? Actor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;

    if (!SkeletalMeshComponent)
    {
        return false;
    }

    const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();

    if (!SkeletalMesh)
    {
        OutMeshLabel = TEXT("(mesh component, no mesh asset)");
        OutSkeletonLabel = TEXT("(no skeleton asset)");
        return true;
    }

    OutMeshLabel = SkeletalMesh->GetName();
    const USkeleton* Skeleton = SkeletalMesh->GetSkeleton();
    OutSkeletonLabel = Skeleton ? Skeleton->GetName() : TEXT("(no skeleton asset)");
    return true;
}

FString GetPlayerControllerLabelFromActor(const AActor* Actor)
{
    const APawn* Pawn = Cast<APawn>(Actor);

    if (!Pawn)
    {
        return TEXT("(not a Pawn)");
    }

    const AController* Controller = Pawn->GetController();

    if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        return PlayerController->GetClass() ? PlayerController->GetClass()->GetName() : TEXT("PlayerController");
    }

    if (Controller)
    {
        return FString::Printf(
            TEXT("Runtime controller is %s, not a PlayerController"),
            Controller->GetClass() ? *Controller->GetClass()->GetName() : TEXT("Controller"));
    }

    if (Pawn->AutoPossessPlayer != EAutoReceiveInput::Disabled)
    {
        return FString::Printf(
            TEXT("Auto Possess Player %s; PlayerController class is resolved by GameMode/runtime"),
            *GetAutoPossessPlayerLabelFromPawn(Pawn));
    }

    return TEXT("(not detected)");
}

FString MakeCompatibleSkeletonSummary(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bHasSkeletalMeshComponent)
    {
        return TEXT("Not Supported: no skeletal mesh component was detected.");
    }

    if (Snapshot.SkeletonLabel.IsEmpty() || Snapshot.SkeletonLabel.Equals(TEXT("(no skeleton asset)")))
    {
        return TEXT("Limited: skeletal mesh exists, but no skeleton asset could be confirmed.");
    }

    if (Snapshot.bHasAnimBlueprint)
    {
        return FString::Printf(
            TEXT("Ready: %s is using skeleton %s with the assigned Anim BP %s."),
            Snapshot.SkeletalMeshLabel.IsEmpty() ? TEXT("Selected skeletal mesh") : *Snapshot.SkeletalMeshLabel,
            *Snapshot.SkeletonLabel,
            Snapshot.LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("(linked Animation Blueprint)") : *Snapshot.LinkedAnimationBlueprintLabel);
    }

    return FString::Printf(
        TEXT("Limited: skeleton %s is detected, but no assigned Anim BP was found on the selected mesh stack."),
        *Snapshot.SkeletonLabel);
}

FString MakeCharacterControlModeLabel(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    const bool bAISignal = Snapshot.bHasAIControllerClass || Snapshot.bAutoPossessAIEnabled;

    if (!Snapshot.bIsPawnActor)
    {
        return Snapshot.bHasSkeletalMeshComponent ? TEXT("Limited") : TEXT("Unknown");
    }

    if (Snapshot.bHasPlayerControllerSignal && bAISignal)
    {
        return TEXT("Shared Character BP");
    }

    if (Snapshot.bHasPlayerControllerSignal && Snapshot.bHasPawnMovementComponent)
    {
        return TEXT("Player Ready");
    }

    if (bAISignal)
    {
        return TEXT("AI Ready");
    }

    if (Snapshot.bHasPawnMovementComponent || Snapshot.bHasCameraComponent || Snapshot.bHasSpringArmComponent)
    {
        return TEXT("Limited");
    }

    return TEXT("Unknown");
}

FString MakePlayableControlSummary(const FWanaSelectedCharacterEnhancementSnapshot& Snapshot)
{
    if (!Snapshot.bIsPawnActor)
    {
        return TEXT("Playable control readiness is unknown because the subject is not a Pawn or Character.");
    }

    const FString CameraLabel = Snapshot.bHasCameraComponent
        ? (Snapshot.bHasSpringArmComponent ? TEXT("Camera and spring arm detected") : TEXT("Camera detected"))
        : (Snapshot.bHasSpringArmComponent ? TEXT("Spring arm detected without a camera component") : TEXT("No camera/spring arm detected"));
    const FString PlayerControllerLabel = Snapshot.LinkedPlayerControllerLabel.IsEmpty()
        ? TEXT("(not detected)")
        : Snapshot.LinkedPlayerControllerLabel;
    const FString MovementLabel = Snapshot.bHasPawnMovementComponent
        ? TEXT("movement component present")
        : TEXT("movement component not detected");

    return FString::Printf(
        TEXT("%s. Auto Possess Player: %s. Player Controller: %s. %s. Control Mode: %s."),
        *MovementLabel,
        Snapshot.AutoPossessPlayerLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot.AutoPossessPlayerLabel,
        *PlayerControllerLabel,
        *CameraLabel,
        Snapshot.CharacterControlModeLabel.IsEmpty() ? TEXT("Unknown") : *Snapshot.CharacterControlModeLabel);
}

bool IsSandboxOrFinalizedWanaWorksActor(const AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    const FString ActorLabel = Actor->GetActorNameOrLabel();
    const FString FolderPath = Actor->GetFolderPath().ToString();

    return ActorLabel.StartsWith(TEXT("WW_Sandbox_"))
        || ActorLabel.StartsWith(TEXT("WW_Final_"))
        || FolderPath.StartsWith(TEXT("WanaWorks/Sandbox"))
        || FolderPath.StartsWith(TEXT("WanaWorks/Builds"));
}

FString BuildFinalizedActorLabel(const FString& SourceLabel)
{
    FString BaseLabel = SourceLabel;
    BaseLabel.RemoveFromStart(TEXT("WW_Sandbox_"));
    BaseLabel.RemoveFromStart(TEXT("WW_Final_"));
    return FString::Printf(TEXT("WW_Final_%s"), *BaseLabel);
}

FString BuildFinalizedAssetName(const FString& SourceLabel)
{
    FString BaseLabel = SourceLabel;
    BaseLabel.RemoveFromStart(TEXT("WW_Sandbox_"));
    BaseLabel.RemoveFromStart(TEXT("WW_Final_"));
    BaseLabel = ObjectTools::SanitizeObjectName(BaseLabel);

    if (BaseLabel.IsEmpty())
    {
        BaseLabel = TEXT("Subject");
    }

    return FString::Printf(TEXT("WW_Final_%s"), *BaseLabel);
}

FString BuildUniqueFinalizedAssetPath(const FString& SourceLabel)
{
    const FString BaseAssetName = BuildFinalizedAssetName(SourceLabel);
    FString CandidateAssetName = BaseAssetName;
    FString CandidateAssetPath = FString::Printf(TEXT("/Game/WanaWorks/Builds/%s"), *CandidateAssetName);
    int32 SuffixIndex = 1;

    while (FPackageName::DoesPackageExist(CandidateAssetPath))
    {
        CandidateAssetName = FString::Printf(TEXT("%s_%02d"), *BaseAssetName, SuffixIndex++);
        CandidateAssetPath = FString::Printf(TEXT("/Game/WanaWorks/Builds/%s"), *CandidateAssetName);
    }

    return CandidateAssetPath;
}

void CaptureAnimationStackInspection(
    const USkeletalMeshComponent* SkeletalMeshComponent,
    FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot)
{
    if (!SkeletalMeshComponent)
    {
        return;
    }

    if (OutSnapshot.AnimationMeshComponentLabel.IsEmpty())
    {
        OutSnapshot.AnimationMeshComponentLabel = SkeletalMeshComponent->GetName();
    }

    const UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance();

    if (AnimInstance && OutSnapshot.AnimationInstanceClassLabel.IsEmpty())
    {
        OutSnapshot.AnimationInstanceClassLabel = AnimInstance->GetClass() ? AnimInstance->GetClass()->GetName() : TEXT("AnimInstance");
    }

    const UClass* AnimClass = const_cast<USkeletalMeshComponent*>(SkeletalMeshComponent)->GetAnimClass();

    if (!AnimClass && AnimInstance)
    {
        AnimClass = AnimInstance->GetClass();
    }

    if (!AnimClass)
    {
        return;
    }

    OutSnapshot.bHasAnimationBlueprintGeneratedClass = true;

    if (OutSnapshot.AnimationAssignedClassLabel.IsEmpty())
    {
        OutSnapshot.AnimationAssignedClassLabel = AnimClass->GetName();
    }

    if (OutSnapshot.AnimationAssignedClassPath.IsEmpty())
    {
        OutSnapshot.AnimationAssignedClassPath = FSoftObjectPath(AnimClass).ToString();
    }

    if (OutSnapshot.AnimationGeneratedClassLabel.IsEmpty())
    {
        OutSnapshot.AnimationGeneratedClassLabel = AnimClass->GetName();
    }

    if (const UObject* GeneratedBy = AnimClass->ClassGeneratedBy)
    {
        OutSnapshot.bHasAnimationBlueprintAsset = true;

        if (OutSnapshot.AnimationBlueprintAssetLabel.IsEmpty())
        {
            OutSnapshot.AnimationBlueprintAssetLabel = GeneratedBy->GetName();
        }

        if (OutSnapshot.AnimationBlueprintAssetPath.IsEmpty())
        {
            OutSnapshot.AnimationBlueprintAssetPath = FSoftObjectPath(GeneratedBy).ToString();
        }
    }

    if (const UClass* ParentClass = AnimClass->GetSuperClass())
    {
        OutSnapshot.bHasAnimationParentInstanceClass = true;

        if (OutSnapshot.AnimationParentInstanceClassLabel.IsEmpty())
        {
            OutSnapshot.AnimationParentInstanceClassLabel = ParentClass->GetName();
        }
    }
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
    OutSnapshot.SubjectSourceLabel = TEXT("Editor Selection");
    OutSnapshot.SubjectAssetPath = FSoftObjectPath(Actor).ToString();
    OutSnapshot.bHasIdentityComponent = Actor->FindComponentByClass<UWanaIdentityComponent>() != nullptr;
    OutSnapshot.bHasWAIComponent = Actor->FindComponentByClass<UWAIPersonalityComponent>() != nullptr;
    OutSnapshot.bHasWAYComponent = Actor->FindComponentByClass<UWAYPlayerProfileComponent>() != nullptr;

    if (const APawn* Pawn = Cast<APawn>(Actor))
    {
        OutSnapshot.bIsPawnActor = true;
        OutSnapshot.bHasAIControllerClass = Pawn->AIControllerClass != nullptr;
        OutSnapshot.bAutoPossessAIEnabled = Pawn->AutoPossessAI != EAutoPossessAI::Disabled;
        OutSnapshot.bAutoPossessPlayerEnabled = Pawn->AutoPossessPlayer != EAutoReceiveInput::Disabled;
        OutSnapshot.bHasPawnMovementComponent = Pawn->GetMovementComponent() != nullptr;
        OutSnapshot.AutoPossessPlayerLabel = GetAutoPossessPlayerLabelFromPawn(Pawn);
        OutSnapshot.LinkedPlayerControllerLabel = GetPlayerControllerLabelFromActor(Actor);

        if (const AController* Controller = Pawn->GetController())
        {
            OutSnapshot.bHasLivePlayerController = Controller->IsA<APlayerController>();
        }

        OutSnapshot.bHasPlayerControllerSignal = OutSnapshot.bHasLivePlayerController || OutSnapshot.bAutoPossessPlayerEnabled;
    }

    OutSnapshot.bHasCameraComponent = ActorHasComponentClassNameToken(Actor, TEXT("Camera"));
    OutSnapshot.bHasSpringArmComponent = ActorHasComponentClassNameToken(Actor, TEXT("SpringArm"));

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
    OutSnapshot.bHasSkeletalMeshComponent = SkeletalMeshComponents.Num() > 0;
    GetSkeletalMeshAndSkeletonLabelsFromActor(Actor, OutSnapshot.SkeletalMeshLabel, OutSnapshot.SkeletonLabel);

    for (const USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (!SkeletalMeshComponent)
        {
            continue;
        }

        if (SkeletalMeshComponent->GetAnimInstance() != nullptr)
        {
            OutSnapshot.bHasAnimationInstance = true;
        }

        CaptureAnimationStackInspection(SkeletalMeshComponent, OutSnapshot);

        if (SkeletalMeshComponent
            && (SkeletalMeshComponent->GetAnimationMode() == EAnimationMode::AnimationBlueprint
                || SkeletalMeshComponent->GetAnimInstance() != nullptr))
        {
            OutSnapshot.bHasAnimBlueprint = true;
        }
    }

    if (const UWanaPhysicalStateComponent* PhysicalStateComponent = Actor->FindComponentByClass<UWanaPhysicalStateComponent>())
    {
        OutSnapshot.bHasPhysicalStateComponent = true;
        OutSnapshot.PhysicalState = PhysicalStateComponent->PhysicalState;
        OutSnapshot.PhysicalStabilityScore = PhysicalStateComponent->StabilityScore;
        OutSnapshot.PhysicalRecoveryProgress = PhysicalStateComponent->RecoveryProgress;
        OutSnapshot.PhysicalInstabilityAlpha = PhysicalStateComponent->InstabilityAlpha;
        OutSnapshot.PhysicalLastImpactDirection = PhysicalStateComponent->LastImpactDirection;
        OutSnapshot.PhysicalLastImpactStrength = PhysicalStateComponent->LastImpactStrength;
        OutSnapshot.bPhysicalBracing = PhysicalStateComponent->bBracing;
        OutSnapshot.bPhysicalCanCommitToMovement = PhysicalStateComponent->bCanCommitToMovement;
        OutSnapshot.bPhysicalCanCommitToAttack = PhysicalStateComponent->bCanCommitToAttack;
        OutSnapshot.bPhysicalNeedsRecovery = PhysicalStateComponent->bNeedsRecovery;
    }

    OutSnapshot.ActorTypeLabel = GetActorTypeLabel(
        Actor,
        OutSnapshot.bHasAIControllerClass,
        OutSnapshot.bAutoPossessAIEnabled,
        OutSnapshot.bHasSkeletalMeshComponent);
    OutSnapshot.LinkedAnimationBlueprintLabel = GetAnimBlueprintLabelFromActor(Actor);
    OutSnapshot.LinkedAIControllerLabel = GetAIControllerLabelFromActor(Actor);
    OutSnapshot.CharacterControlModeLabel = MakeCharacterControlModeLabel(OutSnapshot);
    OutSnapshot.CompatibleSkeletonSummary = MakeCompatibleSkeletonSummary(OutSnapshot);
    OutSnapshot.PlayableControlSummary = MakePlayableControlSummary(OutSnapshot);
    OutSnapshot.AnimationCompatibilitySummary = MakeAnimationCompatibilitySummary(OutSnapshot.bHasSkeletalMeshComponent, OutSnapshot.bHasAnimBlueprint);
    OutSnapshot.AIReadinessSummary = MakeAIReadinessSummary(OutSnapshot.bIsPawnActor, OutSnapshot.bAutoPossessAIEnabled, OutSnapshot.bHasAIControllerClass);

    if (UWanaAutoAnimationIntegrationComponent* AutomaticIntegrationComponent = const_cast<AActor*>(Actor)->FindComponentByClass<UWanaAutoAnimationIntegrationComponent>())
    {
        AutomaticIntegrationComponent->RefreshAutomaticAnimationIntegration();
        OutSnapshot.bHasAutomaticAnimationIntegrationComponent = true;
        OutSnapshot.bAnimationAutoAttachSucceeded = AutomaticIntegrationComponent->bAutoAttachSucceeded;
        OutSnapshot.bAnimationAutoWireSucceeded = AutomaticIntegrationComponent->bAutoWireSucceeded;
        OutSnapshot.AnimationSupportedAutoWireFieldCount = AutomaticIntegrationComponent->SupportedFieldCount;
        OutSnapshot.AnimationLastAppliedAutoWireFieldCount = AutomaticIntegrationComponent->LastAppliedFieldCount;
        OutSnapshot.AnimationAutomaticIntegrationStatus = AutomaticIntegrationComponent->AutomaticIntegrationStatus;
        OutSnapshot.AnimationIntegrationTargetLabel = AutomaticIntegrationComponent->IntegrationTargetLabel;
        OutSnapshot.AnimationAutomaticIntegrationDetail = AutomaticIntegrationComponent->Detail;

        if (!AutomaticIntegrationComponent->SourceAnimationBlueprintLabel.IsEmpty())
        {
            OutSnapshot.LinkedAnimationBlueprintLabel = AutomaticIntegrationComponent->SourceAnimationBlueprintLabel;
        }
    }
    else if (!OutSnapshot.bHasSkeletalMeshComponent)
    {
        OutSnapshot.AnimationAutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::NotSupported;
        OutSnapshot.AnimationIntegrationTargetLabel = FString::Printf(TEXT("%s -> (no skeletal animation stack)"), *OutSnapshot.SelectedActorLabel);
        OutSnapshot.AnimationAutomaticIntegrationDetail = TEXT("Automatic Anim BP integration is not supported because this subject has no skeletal animation stack.");
    }
    else if (!OutSnapshot.bHasAnimBlueprint)
    {
        OutSnapshot.AnimationAutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Limited;
        OutSnapshot.AnimationIntegrationTargetLabel = FString::Printf(TEXT("%s -> (no Animation Blueprint detected)"), *OutSnapshot.SelectedActorLabel);
        OutSnapshot.AnimationAutomaticIntegrationDetail = TEXT("Automatic Anim BP integration is limited because no linked Animation Blueprint was detected for this subject yet.");
    }
    else
    {
        OutSnapshot.AnimationAutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Ready;
        OutSnapshot.AnimationIntegrationTargetLabel = FString::Printf(
            TEXT("%s -> %s"),
            *OutSnapshot.SelectedActorLabel,
            OutSnapshot.LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("(linked Animation Blueprint)") : *OutSnapshot.LinkedAnimationBlueprintLabel);
        OutSnapshot.AnimationAutomaticIntegrationDetail = TEXT("Automatic Anim BP integration is ready for the working-copy or finalized workflow. WanaWorks will auto-attach its safe animation bridge there without touching the original source asset.");
    }

    if (const UWAYPlayerProfileComponent* WAYComponent = Actor->FindComponentByClass<UWAYPlayerProfileComponent>())
    {
        const FWAYAnimationHookState AnimationHookState = WAYComponent->GetCurrentAnimationHookState();
        OutSnapshot.bAnimationHookStateReadable = true;
        OutSnapshot.bAnimationFacingHookRequested = AnimationHookState.bFacingHookRequested;
        OutSnapshot.bAnimationTurnToTargetRequested = AnimationHookState.bTurnToTargetRequested;
        OutSnapshot.bAnimationLocomotionHintSafe = AnimationHookState.bLocomotionSafeExecutionHint;
        OutSnapshot.bAnimationMovementLimitedFallbackHint = AnimationHookState.bMovementLimitedFallbackHint;
        OutSnapshot.bAnimationOutwardGuardHintRequested = AnimationHookState.bOutwardGuardHintRequested;
        OutSnapshot.bAnimationPhysicalReactionStateAvailable = AnimationHookState.bPhysicalReactionStateAvailable;
        OutSnapshot.AnimationHookApplicationStatus = AnimationHookState.ApplicationStatus;
        OutSnapshot.AnimationReactionState = AnimationHookState.ReactionState;
        OutSnapshot.AnimationRecommendedBehavior = AnimationHookState.RecommendedBehavior;
        OutSnapshot.AnimationExecutionMode = AnimationHookState.ExecutionMode;
        OutSnapshot.AnimationRelationshipState = AnimationHookState.RelationshipState;
        OutSnapshot.AnimationPhysicalState = AnimationHookState.PhysicalState;
        OutSnapshot.AnimationPhysicalStabilityScore = AnimationHookState.PhysicalStabilityScore;
        OutSnapshot.AnimationPhysicalRecoveryProgress = AnimationHookState.PhysicalRecoveryProgress;
        OutSnapshot.AnimationPhysicalInstabilityAlpha = AnimationHookState.PhysicalInstabilityAlpha;
        OutSnapshot.AnimationPhysicalImpactDirection = AnimationHookState.PhysicalImpactDirection;
        OutSnapshot.AnimationPhysicalImpactStrength = AnimationHookState.PhysicalImpactStrength;
        OutSnapshot.AnimationBehaviorIntent = AnimationHookState.BehaviorIntent;
        OutSnapshot.AnimationVisibleBehaviorLabel = AnimationHookState.VisibleBehaviorLabel;
        OutSnapshot.AnimationIdentityRoleHint = AnimationHookState.IdentityRoleHint;
        OutSnapshot.AnimationPostureHint = AnimationHookState.PostureHint;
        OutSnapshot.AnimationPostureCategory = AnimationHookState.PostureCategory;
        OutSnapshot.AnimationFallbackHint = AnimationHookState.FallbackHint;
        OutSnapshot.AnimationHookDetail = AnimationHookState.Detail;

        if (OutSnapshot.AnimationHookApplicationStatus == EWAYAnimationHookApplicationStatus::NotAvailable)
        {
            if (!OutSnapshot.bHasSkeletalMeshComponent)
            {
                OutSnapshot.AnimationHookDetail = TEXT("Animation hook application is not available because this subject has no skeletal animation stack.");
            }
            else if (!OutSnapshot.bHasAnimBlueprint)
            {
                OutSnapshot.AnimationHookApplicationStatus = EWAYAnimationHookApplicationStatus::Limited;
                OutSnapshot.AnimationHookDetail = TEXT("Animation hook application is limited because no Animation Blueprint is currently assigned.");
            }
            else
            {
                OutSnapshot.AnimationHookApplicationStatus = EWAYAnimationHookApplicationStatus::Limited;
                OutSnapshot.AnimationHookDetail = TEXT("Animation hook application is ready, but no current target has driven it yet.");
            }
        }
    }

    return true;
}

bool BuildCharacterEnhancementSnapshotFromSubjectObject(const UObject* SubjectObject, FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot)
{
    OutSnapshot = FWanaSelectedCharacterEnhancementSnapshot();

    if (!SubjectObject)
    {
        return false;
    }

    if (const AActor* Actor = Cast<AActor>(SubjectObject))
    {
        return BuildCharacterEnhancementSnapshot(Actor, OutSnapshot);
    }

    const UBlueprint* BlueprintAsset = Cast<UBlueprint>(SubjectObject);
    const UClass* SubjectClass = BlueprintAsset ? BlueprintAsset->GeneratedClass : Cast<UClass>(SubjectObject);

    if (!SubjectClass || !SubjectClass->IsChildOf(AActor::StaticClass()))
    {
        return false;
    }

    const AActor* DefaultActor = Cast<AActor>(SubjectClass->GetDefaultObject());

    if (!DefaultActor || !BuildCharacterEnhancementSnapshot(DefaultActor, OutSnapshot))
    {
        return false;
    }

    OutSnapshot.SelectedActor = nullptr;
    OutSnapshot.bIsProjectAsset = true;
    OutSnapshot.SubjectSourceLabel = TEXT("Project Asset Picker");
    OutSnapshot.SubjectAssetPath = FSoftObjectPath(SubjectObject).ToString();
    OutSnapshot.SelectedActorLabel = SubjectObject->GetName();

    if (!OutSnapshot.bHasAutomaticAnimationIntegrationComponent)
    {
        OutSnapshot.AnimationIntegrationTargetLabel = FString::Printf(
            TEXT("%s -> %s"),
            *OutSnapshot.SelectedActorLabel,
            OutSnapshot.LinkedAnimationBlueprintLabel.IsEmpty() ? TEXT("(linked Animation Blueprint)") : *OutSnapshot.LinkedAnimationBlueprintLabel);

        if (OutSnapshot.AnimationAutomaticIntegrationStatus == EWAYAutomaticAnimationIntegrationStatus::Ready)
        {
        OutSnapshot.AnimationAutomaticIntegrationDetail = TEXT("Automatic Anim BP integration is ready for the picker-driven workspace workflow. WanaWorks will attach the runtime bridge on the generated working or finalized subject instead of changing the original asset.");
        }
    }

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
    const UWAYPlayerProfileComponent* ProfileComponent = SelectedActor ? SelectedActor->FindComponentByClass<UWAYPlayerProfileComponent>() : nullptr;
    const EWAYReactionState ReactionState = ProfileComponent && Profile.TargetActor
        ? ProfileComponent->GetReactionForTarget(Profile.TargetActor)
        : UWAYPlayerProfileComponent::ResolveReactionForRelationshipState(Profile.RelationshipState);
    const EWAYBehaviorPreset RecommendedBehavior = ProfileComponent && Profile.TargetActor
        ? ProfileComponent->GetRecommendedBehaviorForTarget(Profile.TargetActor)
        : UWAYPlayerProfileComponent::ResolveRecommendedBehaviorForReactionState(ReactionState);
    const bool bStarterHookAvailable = ProfileComponent && Profile.TargetActor
        ? ProfileComponent->HasStarterBehaviorHookForTarget(Profile.TargetActor)
        : RecommendedBehavior != EWAYBehaviorPreset::None;
    const EWAYBehaviorPreset LastAppliedHook = ProfileComponent && Profile.TargetActor
        ? ProfileComponent->GetLastAppliedBehaviorHookForTarget(Profile.TargetActor)
        : EWAYBehaviorPreset::None;
    const EWAYBehaviorExecutionMode ExecutionMode = ProfileComponent && Profile.TargetActor
        ? ProfileComponent->GetBehaviorExecutionModeForTarget(Profile.TargetActor)
        : EWAYBehaviorExecutionMode::Unknown;
    const FString VisibleBehaviorLabel = ProfileComponent && Profile.TargetActor
        ? ProfileComponent->GetVisibleBehaviorLabelForTarget(Profile.TargetActor)
        : FString();
    const FString BehaviorExecutionDetail = ProfileComponent && Profile.TargetActor
        ? ProfileComponent->GetBehaviorExecutionDetailForTarget(Profile.TargetActor)
        : FString();
    const FWanaMovementReadiness MovementReadiness = (SelectedActor && Profile.TargetActor)
        ? UWITBlueprintLibrary::GetMovementReadinessForObserverTarget(const_cast<AActor*>(SelectedActor), Profile.TargetActor)
        : FWanaMovementReadiness();
    const FWAYAnimationHookState AnimationHookState = ProfileComponent
        ? ProfileComponent->GetCurrentAnimationHookState()
        : FWAYAnimationHookState();
    const bool bAnimationHookMatchesTarget = !Profile.TargetActor
        || AnimationHookState.TargetActor == Profile.TargetActor;
    const EWAYAnimationHookApplicationStatus AnimationHookApplicationStatus = bAnimationHookMatchesTarget
        ? AnimationHookState.ApplicationStatus
        : EWAYAnimationHookApplicationStatus::Limited;
    const FString AnimationHookDetail = bAnimationHookMatchesTarget
        ? AnimationHookState.Detail
        : TEXT("Animation hook application is ready, but the current target has not driven it yet.");

    Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), *SelectedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), Profile.TargetActor ? *Profile.TargetActor->GetActorNameOrLabel() : TEXT("(none)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Relationship State: %s"), *GetRelationshipStateDisplayLabel(Profile.RelationshipState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Reaction: %s"), *GetReactionStateDisplayLabel(ReactionState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Recommended Behavior: %s"), *GetBehaviorPresetDisplayLabel(RecommendedBehavior)));
    Response.OutputLines.Add(FString::Printf(TEXT("Starter Hook: %s"), bStarterHookAvailable ? TEXT("Available") : TEXT("Not Available")));
    Response.OutputLines.Add(FString::Printf(TEXT("Visible Behavior: %s"), VisibleBehaviorLabel.IsEmpty() ? TEXT("(not applied yet)") : *VisibleBehaviorLabel));
    Response.OutputLines.Add(FString::Printf(TEXT("Last Applied Hook: %s"), *GetBehaviorPresetDisplayLabel(LastAppliedHook)));
    Response.OutputLines.Add(FString::Printf(TEXT("Execution Mode: %s"), *GetBehaviorExecutionModeDisplayLabel(ExecutionMode)));
    AppendBehaviorExecutionLines(Response, ExecutionMode, MovementReadiness);
    Response.OutputLines.Add(FString::Printf(TEXT("Environment Shaping: %s"), *WanaWorksUIFormattingUtils::GetEnvironmentShapingSummaryLabel(MovementReadiness)));
    Response.OutputLines.Add(FString::Printf(TEXT("Result Notes: %s"), BehaviorExecutionDetail.IsEmpty() ? TEXT("No visible behavior has been applied to this target yet.") : *BehaviorExecutionDetail));
    Response.OutputLines.Add(FString::Printf(TEXT("Movement Confidence: %s"), *WanaWorksUIFormattingUtils::GetMovementReadinessStatusSummaryLabel(MovementReadiness)));
    Response.OutputLines.Add(FString::Printf(TEXT("Environment Shaping: %s"), *WanaWorksUIFormattingUtils::GetEnvironmentShapingSummaryLabel(MovementReadiness)));
    Response.OutputLines.Add(FString::Printf(TEXT("Animation Hook Application: %s"), *GetAnimationHookApplicationStatusDisplayLabel(AnimationHookApplicationStatus)));
    Response.OutputLines.Add(FString::Printf(TEXT("Animation Behavior Intent: %s"), bAnimationHookMatchesTarget ? *AnimationHookState.BehaviorIntent : TEXT("(not driven yet)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Animation Posture Hint: %s"), bAnimationHookMatchesTarget ? *AnimationHookState.PostureHint : TEXT("(not driven yet)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Animation Posture Category: %s"), bAnimationHookMatchesTarget ? *AnimationHookState.PostureCategory : TEXT("(not driven yet)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Animation Identity Role Hint: %s"), bAnimationHookMatchesTarget ? *AnimationHookState.IdentityRoleHint : TEXT("(limited context)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Animation Fallback Hint: %s"), bAnimationHookMatchesTarget ? *AnimationHookState.FallbackHint : TEXT("(not driven yet)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Facing Hook: %s"), GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bFacingHookRequested)));
    Response.OutputLines.Add(FString::Printf(TEXT("Turn-To-Target Hook: %s"), GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bTurnToTargetRequested)));
    Response.OutputLines.Add(FString::Printf(TEXT("Movement-Limited Animation Hook: %s"), GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bMovementLimitedFallbackHint)));
    Response.OutputLines.Add(FString::Printf(TEXT("Outward Guard Hook: %s"), GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bOutwardGuardHintRequested)));
    Response.OutputLines.Add(FString::Printf(TEXT("Physical Reaction Hook: %s"), GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bPhysicalReactionStateAvailable)));
    Response.OutputLines.Add(FString::Printf(TEXT("Animation Hook Notes: %s"), AnimationHookDetail.IsEmpty() ? TEXT("No animation hook notes available.") : *AnimationHookDetail));
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

FString GetMovementCapabilityLabel(const FWanaMovementReadiness& MovementReadiness, bool bHasObserverActor)
{
    if (!bHasObserverActor)
    {
        return TEXT("Unknown");
    }

    return MovementReadiness.bHasUsableMovementCapability ? TEXT("Yes") : TEXT("No");
}

FString GetNavigationContextLabel(const FWanaMovementReadiness& MovementReadiness, bool bHasObserverActor)
{
    if (!bHasObserverActor)
    {
        return TEXT("Unknown");
    }

    return MovementReadiness.bHasMovementContext ? TEXT("Detected") : TEXT("Missing");
}

FString GetReachabilityLabel(const FWanaMovementReadiness& MovementReadiness, bool bHasObserverActor, bool bHasTargetActor)
{
    if (!bHasObserverActor || !bHasTargetActor)
    {
        return TEXT("Unknown");
    }

    return MovementReadiness.bTargetReachableHint ? TEXT("Likely") : TEXT("Blocked");
}

FString GetFallbackModeLabel(const FWanaMovementReadiness& MovementReadiness, bool bHasObserverActor, bool bHasTargetActor)
{
    if (!bHasObserverActor || !bHasTargetActor)
    {
        return TEXT("Facing Only");
    }

    if (MovementReadiness.ReadinessLevel == EWanaMovementReadinessLevel::Allowed)
    {
        return TEXT("Movement Allowed");
    }

    if (MovementReadiness.bSupportsDirectActorMove && MovementReadiness.ReadinessLevel != EWanaMovementReadinessLevel::Blocked)
    {
        return TEXT("Fallback Active");
    }

    return TEXT("Facing Only");
}

void AppendEnvironmentReadinessLines(FWanaCommandResponse& Response, const FWanaEnvironmentReadinessSnapshot& Snapshot)
{
    Response.OutputLines.Add(FString::Printf(TEXT("Movement Confidence: %s"), *WanaWorksUIFormattingUtils::GetMovementReadinessStatusSummaryLabel(Snapshot.MovementReadiness)));
    Response.OutputLines.Add(FString::Printf(TEXT("Movement Capability: %s"), *GetMovementCapabilityLabel(Snapshot.MovementReadiness, Snapshot.bHasObserverActor)));
    Response.OutputLines.Add(FString::Printf(TEXT("Navigation Context: %s"), *GetNavigationContextLabel(Snapshot.MovementReadiness, Snapshot.bHasObserverActor)));
    Response.OutputLines.Add(FString::Printf(TEXT("Reachability: %s"), *GetReachabilityLabel(Snapshot.MovementReadiness, Snapshot.bHasObserverActor, Snapshot.bHasTargetActor)));
    Response.OutputLines.Add(FString::Printf(TEXT("Obstacle Pressure: %s"), *WanaWorksUIFormattingUtils::GetObstaclePressureSummaryLabel(Snapshot.MovementReadiness)));
    Response.OutputLines.Add(FString::Printf(TEXT("Movement Space: %s"), *WanaWorksUIFormattingUtils::GetMovementSpaceSummaryLabel(Snapshot.MovementReadiness)));
    Response.OutputLines.Add(FString::Printf(TEXT("Fallback Mode: %s"), *GetFallbackModeLabel(Snapshot.MovementReadiness, Snapshot.bHasObserverActor, Snapshot.bHasTargetActor)));

    if (!Snapshot.PairSourceLabel.IsEmpty())
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Readiness Notes: Pair Source: %s"), *Snapshot.PairSourceLabel));
    }

    if (Snapshot.bTargetFallsBackToObserver)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Observer fallback is active. Assign a separate target for standard movement-aware reactions."));
    }

    if (!Snapshot.bHasObserverActor)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: No observer actor is assigned for readiness scanning."));
    }
    else if (!Snapshot.bHasTargetActor)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: No target actor is assigned, so movement-based reactions are blocked until a target is chosen."));
    }

    if (!Snapshot.MovementReadiness.Detail.IsEmpty())
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Readiness Notes: %s"), *Snapshot.MovementReadiness.Detail));
    }

    if (Snapshot.MovementReadiness.bMovementSpaceRestricted)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Nearby space is restricted, so visible behaviors should stay small or fall back to facing-only responses."));
    }
    else if (Snapshot.MovementReadiness.bObstaclePressureDetected)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Nearby obstacle pressure was detected, so movement should stay deliberate and short."));
    }
    else if (Snapshot.MovementReadiness.bLowConfidenceContext)
    {
        Response.OutputLines.Add(TEXT("Readiness Notes: Movement context confidence is lower than normal, so fallback behavior may be safer than locomotion."));
    }
}

FString BuildBehaviorMovementCompatibilityDetail(const FWanaMovementReadiness& MovementReadiness)
{
    return FString::Printf(
        TEXT("%s; %s"),
        *WanaWorksUIFormattingUtils::GetMovementReadinessStatusSummaryLabel(MovementReadiness),
        *WanaWorksUIFormattingUtils::GetEnvironmentShapingSummaryLabel(MovementReadiness));
}

FString BuildBehaviorFallbackReasonDetail(EWAYBehaviorExecutionMode ExecutionMode, const FWanaMovementReadiness& MovementReadiness)
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

FString BuildBehaviorRecommendedNextStepDetail(EWAYBehaviorExecutionMode ExecutionMode, const FWanaMovementReadiness& MovementReadiness)
{
    if (ExecutionMode == EWAYBehaviorExecutionMode::MovementAllowed)
    {
        return TEXT("Visually verify the behavior distance and repeat Test against a different target relationship.");
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

void AppendBehaviorExecutionLines(
    FWanaCommandResponse& Response,
    EWAYBehaviorExecutionMode ExecutionMode,
    const FWanaMovementReadiness& MovementReadiness)
{
    Response.OutputLines.Add(FString::Printf(TEXT("Movement Compatibility: %s"), *BuildBehaviorMovementCompatibilityDetail(MovementReadiness)));
    Response.OutputLines.Add(FString::Printf(TEXT("Fallback Reason: %s"), *BuildBehaviorFallbackReasonDetail(ExecutionMode, MovementReadiness)));
    Response.OutputLines.Add(FString::Printf(TEXT("Recommended Next Step: %s"), *BuildBehaviorRecommendedNextStepDetail(ExecutionMode, MovementReadiness)));
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
    if (!IsValid(ObserverActor) || !IsValid(TargetActor))
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Observer or target is missing."),
            TEXT("Select or assign both an observer and target actor before evaluating the pair."));
    }

    if (ObserverActor->GetWorld() && TargetActor->GetWorld() && ObserverActor->GetWorld() != TargetActor->GetWorld())
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Observer and target are in different world contexts."),
            TEXT("Choose actors from the same editor or PIE world before evaluating the pair."));
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
    const bool bStarterHookApplied = Evaluation.RecommendedBehavior != EWAYBehaviorPreset::None
        && ProfileComponent->ApplyBehaviorPresetHook(TargetActor, Evaluation.RecommendedBehavior);

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

    FWanaEnvironmentReadinessSnapshot ReadinessSnapshot;
    WanaWorksUIEditorActions::GetEnvironmentReadinessSnapshotForActorPair(
        ObserverActor,
        TargetActor,
        bTargetFallsBackToObserver,
        bTargetFallsBackToObserver ? TEXT("Current selection (observer fallback)") : TEXT("Current selection"),
        ReadinessSnapshot);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = bStarterHookApplied
        ? TEXT("Status: Character Intelligence behavior test complete.")
        : TEXT("Status: Character Intelligence behavior test limited.");
    Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), *ObserverActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), *TargetActor->GetActorNameOrLabel()));
    AppendEnvironmentReadinessLines(Response, ReadinessSnapshot);
    Response.OutputLines.Add(FString::Printf(TEXT("Identity Seed: %s"), *MakeIdentitySeedSummary(TargetActor)));
    Response.OutputLines.Add(FString::Printf(TEXT("Relationship State: %s"), *GetRelationshipStateDisplayLabel(Evaluation.RelationshipProfile.RelationshipState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Reaction State: %s"), *GetReactionStateDisplayLabel(Evaluation.ReactionState)));
    Response.OutputLines.Add(FString::Printf(TEXT("Recommended Behavior: %s"), *GetBehaviorPresetDisplayLabel(Evaluation.RecommendedBehavior)));
    Response.OutputLines.Add(FString::Printf(TEXT("Requested Behavior: %s"), *GetBehaviorPresetDisplayLabel(Evaluation.RecommendedBehavior)));
    Response.OutputLines.Add(FString::Printf(TEXT("Behavior Execution: %s"), bStarterHookApplied ? TEXT("Applied") : TEXT("Limited")));
    Response.OutputLines.Add(FString::Printf(TEXT("Starter Hook: %s"), Evaluation.RecommendedBehavior != EWAYBehaviorPreset::None ? TEXT("Available") : TEXT("Not Available")));
    {
        const FString VisibleBehaviorLabel = ProfileComponent->GetVisibleBehaviorLabelForTarget(TargetActor).TrimStartAndEnd();
        Response.OutputLines.Add(FString::Printf(TEXT("Visible Behavior: %s"), VisibleBehaviorLabel.IsEmpty() ? TEXT("(not applied yet)") : *VisibleBehaviorLabel));
    }
    Response.OutputLines.Add(FString::Printf(TEXT("Last Applied Hook: %s"), *GetBehaviorPresetDisplayLabel(ProfileComponent->GetLastAppliedBehaviorHookForTarget(TargetActor))));
    const EWAYBehaviorExecutionMode ExecutionMode = ProfileComponent->GetBehaviorExecutionModeForTarget(TargetActor);
    Response.OutputLines.Add(FString::Printf(TEXT("Execution Mode: %s"), *GetBehaviorExecutionModeDisplayLabel(ExecutionMode)));
    AppendBehaviorExecutionLines(Response, ExecutionMode, ReadinessSnapshot.MovementReadiness);
    Response.OutputLines.Add(FString::Printf(TEXT("Environment Shaping: %s"), *WanaWorksUIFormattingUtils::GetEnvironmentShapingSummaryLabel(ReadinessSnapshot.MovementReadiness)));
    {
        const FString BehaviorExecutionDetail = ProfileComponent->GetBehaviorExecutionDetailForTarget(TargetActor);
        Response.OutputLines.Add(FString::Printf(TEXT("Result Notes: %s"), BehaviorExecutionDetail.IsEmpty() ? TEXT("No visible behavior has been applied to this target yet.") : *BehaviorExecutionDetail));
    }
    const FWAYAnimationHookState AnimationHookState = ProfileComponent->GetCurrentAnimationHookState();
    const bool bAnimationHookMatchesTarget = AnimationHookState.TargetActor == TargetActor;
    Response.OutputLines.Add(FString::Printf(
        TEXT("Animation Hook Application: %s"),
        *GetAnimationHookApplicationStatusDisplayLabel(
            bAnimationHookMatchesTarget
                ? AnimationHookState.ApplicationStatus
                : EWAYAnimationHookApplicationStatus::Limited)));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Facing Hook: %s"),
        GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bFacingHookRequested)));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Turn-To-Target Hook: %s"),
        GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bTurnToTargetRequested)));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Animation Behavior Intent: %s"),
        bAnimationHookMatchesTarget ? *AnimationHookState.BehaviorIntent : TEXT("(not driven yet)")));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Animation Posture Hint: %s"),
        bAnimationHookMatchesTarget ? *AnimationHookState.PostureHint : TEXT("(not driven yet)")));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Animation Posture Category: %s"),
        bAnimationHookMatchesTarget ? *AnimationHookState.PostureCategory : TEXT("(not driven yet)")));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Animation Identity Role Hint: %s"),
        bAnimationHookMatchesTarget ? *AnimationHookState.IdentityRoleHint : TEXT("(limited context)")));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Animation Fallback Hint: %s"),
        bAnimationHookMatchesTarget ? *AnimationHookState.FallbackHint : TEXT("(not driven yet)")));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Movement-Limited Animation Hook: %s"),
        GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bMovementLimitedFallbackHint)));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Outward Guard Hook: %s"),
        GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bOutwardGuardHintRequested)));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Physical Reaction Hook: %s"),
        GetAnimationHookRequestDisplayLabel(bAnimationHookMatchesTarget && AnimationHookState.bPhysicalReactionStateAvailable)));
    Response.OutputLines.Add(FString::Printf(
        TEXT("Animation Hook Notes: %s"),
        bAnimationHookMatchesTarget
            ? (AnimationHookState.Detail.IsEmpty() ? TEXT("No animation hook notes available.") : *AnimationHookState.Detail)
            : TEXT("Animation hook application is ready, but the current target has not driven it yet.")));

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

bool GetCharacterEnhancementSnapshotForActor(const AActor* Actor, FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot)
{
    return BuildCharacterEnhancementSnapshot(Actor, OutSnapshot);
}

bool GetCharacterEnhancementSnapshotForSubjectObject(const UObject* SubjectObject, FWanaSelectedCharacterEnhancementSnapshot& OutSnapshot)
{
    return BuildCharacterEnhancementSnapshotFromSubjectObject(SubjectObject, OutSnapshot);
}

bool GetSelectedRelationshipContextSnapshot(FWanaSelectedRelationshipContextSnapshot& OutSnapshot)
{
    OutSnapshot = FWanaSelectedRelationshipContextSnapshot();

    const AActor* ObserverActor = nullptr;
    const AActor* TargetActor = nullptr;
    bool bTargetFallsBackToObserver = false;

    if (!GetSelectedActorPair(ObserverActor, TargetActor, bTargetFallsBackToObserver) || !IsValid(ObserverActor))
    {
        return false;
    }

    OutSnapshot.bHasObserverActor = true;
    OutSnapshot.ObserverActor = const_cast<AActor*>(ObserverActor);
    OutSnapshot.ObserverActorLabel = ObserverActor->GetActorNameOrLabel();
    OutSnapshot.bObserverHasRelationshipComponent = ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>() != nullptr;

    if (IsValid(TargetActor))
    {
        OutSnapshot.bHasTargetActor = true;
        OutSnapshot.TargetActor = const_cast<AActor*>(TargetActor);
        OutSnapshot.TargetActorLabel = TargetActor->GetActorNameOrLabel();
    }

    OutSnapshot.bTargetFallsBackToObserver = bTargetFallsBackToObserver;
    return true;
}

bool GetEnvironmentReadinessSnapshotForActorPair(const AActor* ObserverActor, const AActor* TargetActor, bool bTargetFallsBackToObserver, const FString& PairSourceLabel, FWanaEnvironmentReadinessSnapshot& OutSnapshot)
{
    OutSnapshot = FWanaEnvironmentReadinessSnapshot();
    OutSnapshot.PairSourceLabel = PairSourceLabel;
    OutSnapshot.bTargetFallsBackToObserver = bTargetFallsBackToObserver;
    const bool bObserverValid = IsValid(ObserverActor);
    const bool bTargetValid = IsValid(TargetActor);

    if (bObserverValid)
    {
        OutSnapshot.bHasObserverActor = true;
        OutSnapshot.ObserverActor = const_cast<AActor*>(ObserverActor);
        OutSnapshot.ObserverActorLabel = ObserverActor->GetActorNameOrLabel();
    }

    if (bTargetValid)
    {
        OutSnapshot.bHasTargetActor = true;
        OutSnapshot.TargetActor = const_cast<AActor*>(TargetActor);
        OutSnapshot.TargetActorLabel = TargetActor->GetActorNameOrLabel();
    }

    if (bObserverValid && bTargetValid && ObserverActor->GetWorld() && TargetActor->GetWorld() && ObserverActor->GetWorld() != TargetActor->GetWorld())
    {
        OutSnapshot.MovementReadiness.Detail = TEXT("Movement blocked because observer and target are in different world contexts.");
    }
    else if (bObserverValid || bTargetValid)
    {
        OutSnapshot.MovementReadiness = UWITBlueprintLibrary::GetMovementReadinessForObserverTarget(
            bObserverValid ? const_cast<AActor*>(ObserverActor) : nullptr,
            bTargetValid ? const_cast<AActor*>(TargetActor) : nullptr);
    }
    else
    {
        OutSnapshot.MovementReadiness.Detail = TEXT("Movement blocked because no observer or target is assigned for readiness scanning.");
    }

    return OutSnapshot.bHasObserverActor || OutSnapshot.bHasTargetActor;
}

bool GetBehaviorResultsSnapshotForActorPair(const AActor* ObserverActor, const AActor* TargetActor, bool bTargetFallsBackToObserver, const FString& PairSourceLabel, FWanaBehaviorResultsSnapshot& OutSnapshot)
{
    OutSnapshot = FWanaBehaviorResultsSnapshot();
    OutSnapshot.PairSourceLabel = PairSourceLabel;
    OutSnapshot.bTargetFallsBackToObserver = bTargetFallsBackToObserver;
    const bool bObserverValid = IsValid(ObserverActor);
    const bool bTargetValid = IsValid(TargetActor);

    if (bObserverValid)
    {
        OutSnapshot.bHasObserverActor = true;
        OutSnapshot.ObserverActor = const_cast<AActor*>(ObserverActor);
        OutSnapshot.ObserverActorLabel = ObserverActor->GetActorNameOrLabel();
    }

    if (bTargetValid)
    {
        OutSnapshot.bHasTargetActor = true;
        OutSnapshot.TargetActor = const_cast<AActor*>(TargetActor);
        OutSnapshot.TargetActorLabel = TargetActor->GetActorNameOrLabel();
    }

    if (bObserverValid && bTargetValid && ObserverActor->GetWorld() && TargetActor->GetWorld() && ObserverActor->GetWorld() != TargetActor->GetWorld())
    {
        OutSnapshot.MovementReadiness.Detail = TEXT("Movement blocked because observer and target are in different world contexts.");
    }
    else if (bObserverValid || bTargetValid)
    {
        OutSnapshot.MovementReadiness = UWITBlueprintLibrary::GetMovementReadinessForObserverTarget(
            bObserverValid ? const_cast<AActor*>(ObserverActor) : nullptr,
            bTargetValid ? const_cast<AActor*>(TargetActor) : nullptr);
    }

    const UWAYPlayerProfileComponent* ProfileComponent = bObserverValid ? ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>() : nullptr;

    if (!ProfileComponent || !bTargetValid)
    {
        return OutSnapshot.bHasObserverActor || OutSnapshot.bHasTargetActor;
    }

    OutSnapshot.bHasWAYComponent = true;
    OutSnapshot.ReactionState = ProfileComponent->GetReactionForTarget(const_cast<AActor*>(TargetActor));
    OutSnapshot.RecommendedBehavior = ProfileComponent->GetRecommendedBehaviorForTarget(const_cast<AActor*>(TargetActor));
    OutSnapshot.bStarterHookAvailable = ProfileComponent->HasStarterBehaviorHookForTarget(const_cast<AActor*>(TargetActor));
    OutSnapshot.LastAppliedHook = ProfileComponent->GetLastAppliedBehaviorHookForTarget(const_cast<AActor*>(TargetActor));
    OutSnapshot.ExecutionMode = ProfileComponent->GetBehaviorExecutionModeForTarget(const_cast<AActor*>(TargetActor));
    OutSnapshot.VisibleBehaviorLabel = ProfileComponent->GetVisibleBehaviorLabelForTarget(const_cast<AActor*>(TargetActor));
    OutSnapshot.BehaviorExecutionDetail = ProfileComponent->GetBehaviorExecutionDetailForTarget(const_cast<AActor*>(TargetActor));
    OutSnapshot.MovementReadiness = UWITBlueprintLibrary::GetMovementReadinessForObserverTarget(const_cast<AActor*>(ObserverActor), const_cast<AActor*>(TargetActor));
    const FWAYAnimationHookState AnimationHookState = ProfileComponent->GetCurrentAnimationHookState();
    const bool bAnimationHookMatchesTarget = AnimationHookState.TargetActor == TargetActor;
    OutSnapshot.bAnimationFacingHookRequested = bAnimationHookMatchesTarget && AnimationHookState.bFacingHookRequested;
    OutSnapshot.bAnimationTurnToTargetRequested = bAnimationHookMatchesTarget && AnimationHookState.bTurnToTargetRequested;
    OutSnapshot.bAnimationLocomotionHintSafe = bAnimationHookMatchesTarget && AnimationHookState.bLocomotionSafeExecutionHint;
    OutSnapshot.bAnimationMovementLimitedFallbackHint = bAnimationHookMatchesTarget && AnimationHookState.bMovementLimitedFallbackHint;
    OutSnapshot.bAnimationOutwardGuardHintRequested = bAnimationHookMatchesTarget && AnimationHookState.bOutwardGuardHintRequested;
    OutSnapshot.bAnimationPhysicalReactionStateAvailable = bAnimationHookMatchesTarget && AnimationHookState.bPhysicalReactionStateAvailable;
    OutSnapshot.AnimationHookApplicationStatus = bAnimationHookMatchesTarget
        ? AnimationHookState.ApplicationStatus
        : EWAYAnimationHookApplicationStatus::Limited;
    OutSnapshot.AnimationReactionState = bAnimationHookMatchesTarget
        ? AnimationHookState.ReactionState
        : OutSnapshot.ReactionState;
    OutSnapshot.AnimationRecommendedBehavior = bAnimationHookMatchesTarget
        ? AnimationHookState.RecommendedBehavior
        : OutSnapshot.RecommendedBehavior;
    OutSnapshot.AnimationExecutionMode = bAnimationHookMatchesTarget
        ? AnimationHookState.ExecutionMode
        : OutSnapshot.ExecutionMode;
    OutSnapshot.AnimationRelationshipState = bAnimationHookMatchesTarget
        ? AnimationHookState.RelationshipState
        : OutSnapshot.RelationshipState;
    OutSnapshot.AnimationPhysicalState = AnimationHookState.PhysicalState;
    OutSnapshot.AnimationPhysicalStabilityScore = AnimationHookState.PhysicalStabilityScore;
    OutSnapshot.AnimationPhysicalRecoveryProgress = AnimationHookState.PhysicalRecoveryProgress;
    OutSnapshot.AnimationPhysicalInstabilityAlpha = AnimationHookState.PhysicalInstabilityAlpha;
    OutSnapshot.AnimationPhysicalImpactDirection = AnimationHookState.PhysicalImpactDirection;
    OutSnapshot.AnimationPhysicalImpactStrength = AnimationHookState.PhysicalImpactStrength;
    OutSnapshot.AnimationBehaviorIntent = bAnimationHookMatchesTarget
        ? AnimationHookState.BehaviorIntent
        : FString();
    OutSnapshot.AnimationVisibleBehaviorLabel = bAnimationHookMatchesTarget
        ? AnimationHookState.VisibleBehaviorLabel
        : FString();
    OutSnapshot.AnimationIdentityRoleHint = bAnimationHookMatchesTarget
        ? AnimationHookState.IdentityRoleHint
        : FString();
    OutSnapshot.AnimationPostureHint = bAnimationHookMatchesTarget
        ? AnimationHookState.PostureHint
        : FString();
    OutSnapshot.AnimationPostureCategory = bAnimationHookMatchesTarget
        ? AnimationHookState.PostureCategory
        : FString();
    OutSnapshot.AnimationFallbackHint = bAnimationHookMatchesTarget
        ? AnimationHookState.FallbackHint
        : FString();
    OutSnapshot.AnimationHookDetail = bAnimationHookMatchesTarget
        ? AnimationHookState.Detail
        : TEXT("Animation hook application is ready, but the current target has not driven it yet.");

    FWAYRelationshipProfile RelationshipProfile;

    if (ProfileComponent->GetRelationshipProfileForTarget(const_cast<AActor*>(TargetActor), RelationshipProfile))
    {
        OutSnapshot.bHasRelationshipProfile = true;
        OutSnapshot.RelationshipState = RelationshipProfile.RelationshipState;
    }

    if (OutSnapshot.VisibleBehaviorLabel.IsEmpty() && OutSnapshot.RecommendedBehavior != EWAYBehaviorPreset::None)
    {
        OutSnapshot.VisibleBehaviorLabel = FString::Printf(
            TEXT("%s (not applied yet)"),
            *GetBehaviorPresetDisplayLabel(OutSnapshot.RecommendedBehavior));
    }

    if (OutSnapshot.BehaviorExecutionDetail.IsEmpty())
    {
        OutSnapshot.BehaviorExecutionDetail = TEXT("No visible starter or reaction behavior has been applied to this target yet.");
    }

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

    if (!IsValid(ObserverActor) || !IsValid(TargetActor))
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Observer or target is missing."),
            TEXT("Assign both an observer and relationship target before preparing WAY-lite relationship data."));
    }

    if (ObserverActor->GetWorld() && TargetActor->GetWorld() && ObserverActor->GetWorld() != TargetActor->GetWorld())
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Observer and target are in different world contexts."),
            TEXT("Choose actors from the same editor or PIE world before preparing WAY-lite relationship data."));
    }

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

    if (!IsValid(ObserverActor) || !IsValid(TargetActor))
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Observer or target is missing."),
            TEXT("Assign both an observer and relationship target before applying a WAY-lite relationship state."));
    }

    if (ObserverActor->GetWorld() && TargetActor->GetWorld() && ObserverActor->GetWorld() != TargetActor->GetWorld())
    {
        return MakeEditorFailureResponse(
            TEXT("Status: Observer and target are in different world contexts."),
            TEXT("Choose actors from the same editor or PIE world before applying a WAY-lite relationship state."));
    }

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
    const bool bShouldPrepareAutomaticAnimationIntegration = IsSandboxOrFinalizedWanaWorksActor(SelectedActor);
    bool bHasAutomaticAnimationPreparation = false;
    FWanaAutomaticAnimationPreparationResult AutomaticAnimationPreparation;

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
        if (bShouldPrepareAutomaticAnimationIntegration)
        {
            AutomaticAnimationPreparation = PrepareAutomaticAnimationIntegrationForActor(SelectedActor);
            bHasAutomaticAnimationPreparation = true;
            SelectedActor->MarkPackageDirty();
        }

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

        if (bHasAutomaticAnimationPreparation)
        {
            AppendAutomaticAnimationIntegrationLines(Response, AutomaticAnimationPreparation);
    Response.OutputLines.Add(TEXT("Compatibility Notes: Automatic Anim BP integration stays on the working or finalized subject and does not overwrite the original source asset."));
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

    if (bShouldPrepareAutomaticAnimationIntegration)
    {
        AutomaticAnimationPreparation = PrepareAutomaticAnimationIntegrationForActor(SelectedActor);
        bHasAutomaticAnimationPreparation = true;

        if (AutomaticAnimationPreparation.bAddedPhysicalStateComponent)
        {
            AddedComponents.Add(TEXT("UWanaPhysicalStateComponent"));
        }

        if (AutomaticAnimationPreparation.bAddedAutomaticIntegrationComponent)
        {
            AddedComponents.Add(TEXT("UWanaAutoAnimationIntegrationComponent"));
        }
    }

    SelectedActor->MarkPackageDirty();

    CompatibilityNotes.Add(TEXT("Existing setup preserved. No existing components were replaced."));
    CompatibilityNotes.Add(TEXT("This enhancement only adds missing starter components for the selected actor."));

    if (bNeedsWAI)
    {
        CompatibilityNotes.Add(TEXT("Full WanaAI Starter uses UWAIPersonalityComponent as the current WAI starter component."));
    }

    if (bHasAutomaticAnimationPreparation)
    {
    CompatibilityNotes.Add(TEXT("Automatic Anim BP integration was prepared on the working or finalized subject only, so the original source pawn and Anim BP stay untouched."));
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

    if (bHasAutomaticAnimationPreparation)
    {
        AppendAutomaticAnimationIntegrationLines(Response, AutomaticAnimationPreparation);
    }

    return Response;
}

FWanaCommandResponse ExecuteCreateSandboxDuplicateCommand()
{
    AActor* SelectedActor = GetFirstSelectedActorMutable();

    if (!SelectedActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No actors selected."), TEXT("Select a character or pawn before creating a working copy."));
    }

    UWorld* EditorWorld = SelectedActor->GetWorld();

    if (!GEditor || !EditorWorld || !SelectedActor->GetLevel())
    {
        return MakeEditorFailureResponse(TEXT("Status: Working copy failed."), TEXT("Could not access the editor world for working-copy creation."));
    }

    const FTransform SourceTransform = SelectedActor->GetActorTransform();
    FTransform DuplicateTransform = SourceTransform;
    DuplicateTransform.AddToTranslation(FVector(180.0f, 0.0f, 0.0f));

    FScopedTransaction Transaction(LOCTEXT("WanaWorksCreateSandboxDuplicateTransaction", "Create WanaWorks Working Copy"));
    AActor* DuplicateActor = GEditor->AddActor(SelectedActor->GetLevel(), SelectedActor->GetClass(), DuplicateTransform);

    if (!DuplicateActor)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Working copy failed."), TEXT("Could not create a working copy for the selected actor."));
    }

    DuplicateActor->SetFlags(RF_Transactional);
    DuplicateActor->Modify();
    DuplicateActor->SetActorLabel(FString::Printf(TEXT("WW_Sandbox_%s"), *SelectedActor->GetActorNameOrLabel()), true);
    DuplicateActor->SetFolderPath(FName(TEXT("WanaWorks/Sandbox")));

    const FWanaAutomaticAnimationPreparationResult AutomaticAnimationPreparation = PrepareAutomaticAnimationIntegrationForActor(DuplicateActor);
    DuplicateActor->MarkPackageDirty();

    GEditor->SelectNone(false, true, false);
    GEditor->SelectActor(DuplicateActor, true, true, true);
    GEditor->NoteSelectionChange();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Created working copy.");
    Response.OutputLines.Add(FString::Printf(TEXT("Selected Actor: %s"), *SelectedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(TEXT("Workflow Path: Create Working Copy"));
    Response.OutputLines.Add(TEXT("Subject Mode: Working copy"));
    Response.OutputLines.Add(FString::Printf(TEXT("Active Subject: %s"), *DuplicateActor->GetActorNameOrLabel()));
    AppendAutomaticAnimationIntegrationLines(Response, AutomaticAnimationPreparation);
    Response.OutputLines.Add(TEXT("Compatibility Notes: The original actor was left untouched."));
    Response.OutputLines.Add(TEXT("Compatibility Notes: The working copy was placed in the WanaWorks workspace area and selected for continued setup."));
    Response.OutputLines.Add(TEXT("Compatibility Notes: Automatic Anim BP integration, when supported, was prepared on the working copy only."));
    return Response;
}

FWanaCommandResponse ExecuteCreateSandboxSubjectFromAssetCommand(const UObject* SubjectObject, const UAnimBlueprint* AnimationBlueprintOverride)
{
    if (!SubjectObject)
    {
        return MakeEditorFailureResponse(TEXT("Status: No subject asset selected."), TEXT("Choose a Character Pawn or AI Pawn asset in Wana Works before creating a working subject."));
    }

    const UBlueprint* SubjectBlueprint = Cast<UBlueprint>(SubjectObject);
    const UClass* SubjectClass = SubjectBlueprint ? SubjectBlueprint->GeneratedClass : Cast<UClass>(SubjectObject);

    if (!SubjectClass || !SubjectClass->IsChildOf(AActor::StaticClass()))
    {
        return MakeEditorFailureResponse(TEXT("Status: Invalid subject asset."), TEXT("The selected asset is not a Character Pawn or AI Pawn that can be spawned into the editor world."));
    }

    if (!GEditor)
    {
        return MakeEditorFailureResponse(TEXT("Status: Editor world is unavailable."), TEXT("Could not create a working subject because GEditor is unavailable."));
    }

    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();

    if (!EditorWorld)
    {
        return MakeEditorFailureResponse(TEXT("Status: Editor world is unavailable."), TEXT("Could not resolve the current editor world for working-subject creation."));
    }

    ULevel* TargetLevel = EditorWorld->GetCurrentLevel();

    if (!TargetLevel)
    {
        TargetLevel = EditorWorld->PersistentLevel.Get();
    }

    if (!TargetLevel)
    {
        return MakeEditorFailureResponse(TEXT("Status: Working subject failed."), TEXT("Could not resolve a target editor level for working-subject creation."));
    }

    FTransform SpawnTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 120.0f));

    if (const AActor* FirstSelectedActor = GetFirstSelectedActor())
    {
        SpawnTransform = FirstSelectedActor->GetActorTransform();
        SpawnTransform.AddToTranslation(FVector(180.0f, 0.0f, 0.0f));
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksCreateSandboxSubjectFromAssetTransaction", "Create WanaWorks Working Subject From Asset"));
    AActor* SpawnedActor = GEditor->AddActor(TargetLevel, const_cast<UClass*>(SubjectClass), SpawnTransform);

    if (!SpawnedActor)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Working subject failed."), TEXT("Could not spawn a working subject from the selected project asset."));
    }

    SpawnedActor->SetFlags(RF_Transactional);
    SpawnedActor->Modify();
    SpawnedActor->SetActorLabel(FString::Printf(TEXT("WW_Sandbox_%s"), *SubjectObject->GetName()), true);
    SpawnedActor->SetFolderPath(FName(TEXT("WanaWorks/Sandbox")));

    bool bAppliedAnimationOverride = false;

    if (AnimationBlueprintOverride && AnimationBlueprintOverride->GeneratedClass)
    {
        TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
        SpawnedActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);

        for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
        {
            if (!SkeletalMeshComponent)
            {
                continue;
            }

            SkeletalMeshComponent->Modify();
            SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
            SkeletalMeshComponent->SetAnimInstanceClass(AnimationBlueprintOverride->GeneratedClass);
            bAppliedAnimationOverride = true;
        }
    }

    const FWanaAutomaticAnimationPreparationResult AutomaticAnimationPreparation = PrepareAutomaticAnimationIntegrationForActor(SpawnedActor);

    SpawnedActor->MarkPackageDirty();

    GEditor->SelectNone(false, true, false);
    GEditor->SelectActor(SpawnedActor, true, true, true);
    GEditor->NoteSelectionChange();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Created working subject from project asset.");
    Response.OutputLines.Add(FString::Printf(TEXT("Picked Subject Asset: %s"), *SubjectObject->GetName()));
    Response.OutputLines.Add(TEXT("Workflow Path: Project Asset Picker -> Working Subject"));
    Response.OutputLines.Add(FString::Printf(TEXT("Active Subject: %s"), *SpawnedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(TEXT("Subject Mode: Working subject"));
    AppendAutomaticAnimationIntegrationLines(Response, AutomaticAnimationPreparation);
    Response.OutputLines.Add(TEXT("Compatibility Notes: WanaWorks created a working subject from the picked project asset without changing the original asset."));
    Response.OutputLines.Add(TEXT("Compatibility Notes: Automatic Anim BP integration, when supported, was applied to the workspace path only."));

    if (AnimationBlueprintOverride)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Picked Animation Blueprint: %s"), *AnimationBlueprintOverride->GetName()));
        Response.OutputLines.Add(FString::Printf(
            TEXT("Animation Override Applied: %s"),
            bAppliedAnimationOverride ? TEXT("Yes") : TEXT("Not applied (no compatible skeletal mesh component found)")));
    }
    else
    {
        Response.OutputLines.Add(TEXT("Picked Animation Blueprint: Auto-detect from subject"));
    }

    return Response;
}

FWanaCommandResponse ExecuteCreateFinalizedBuildAssetFromActorCommand(AActor* SourceActor)
{
    if (!SourceActor)
    {
        return MakeEditorFailureResponse(TEXT("Status: No working subject available for build."), TEXT("Create or select a working copy in WanaWorks before building the final output asset."));
    }

    if (!GEditor)
    {
        return MakeEditorFailureResponse(TEXT("Status: Editor tools are unavailable."), TEXT("Could not build a finalized asset because GEditor is unavailable."));
    }

    const FString FinalAssetPath = BuildUniqueFinalizedAssetPath(SourceActor->GetActorNameOrLabel());

    FKismetEditorUtilities::FCreateBlueprintFromActorParams CreateParams;
    CreateParams.bReplaceActor = false;
    CreateParams.bKeepMobility = true;
    CreateParams.bOpenBlueprint = false;

    FScopedTransaction Transaction(LOCTEXT("WanaWorksCreateFinalizedBuildAssetTransaction", "Create WanaWorks Finalized Build Asset"));
    UBlueprint* FinalizedBlueprint = FKismetEditorUtilities::CreateBlueprintFromActor(FinalAssetPath, SourceActor, CreateParams);

    if (!FinalizedBlueprint)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Final build failed."), TEXT("Could not create a finalized WanaWorks Blueprint asset from the current working subject."));
    }

    FinalizedBlueprint->MarkPackageDirty();

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Final build completed quietly.");
    Response.OutputLines.Add(FString::Printf(TEXT("Source Working Subject: %s"), *SourceActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(TEXT("Workflow Path: Working Copy -> Final Asset"));
    Response.OutputLines.Add(FString::Printf(TEXT("Finalized Output: %s"), *FinalizedBlueprint->GetName()));
    Response.OutputLines.Add(FString::Printf(TEXT("Finalized Output Path: %s"), *FinalAssetPath));
    Response.OutputLines.Add(TEXT("Build Mode: Finalized Blueprint asset"));
    Response.OutputLines.Add(TEXT("Workspace Notice: The final asset was saved in the background so WanaWorks could keep the workspace flow focused."));
    Response.OutputLines.Add(TEXT("Workspace Notice: You can find the result later in /Game/WanaWorks/Builds without the Content Browser interrupting the current session."));
    Response.OutputLines.Add(TEXT("Compatibility Notes: The original source asset was left untouched."));
    Response.OutputLines.Add(TEXT("Compatibility Notes: The working copy remains available in WanaWorks for additional testing or revisions."));
    Response.OutputLines.Add(TEXT("Compatibility Notes: Build Final now creates a clean asset result in /Game/WanaWorks/Builds instead of spawning another world actor as the primary output."));
    return Response;
}

FWanaCommandResponse ExecuteCreateFinalizedBuildSubjectFromAssetCommand(const UObject* SubjectObject, const UAnimBlueprint* AnimationBlueprintOverride)
{
    if (!SubjectObject)
    {
        return MakeEditorFailureResponse(TEXT("Status: No subject available for build."), TEXT("Choose a Character Pawn or AI Pawn in Wana Works, or prepare a working subject first, before building a final version."));
    }

    const UBlueprint* SubjectBlueprint = Cast<UBlueprint>(SubjectObject);
    const UClass* SubjectClass = SubjectBlueprint ? SubjectBlueprint->GeneratedClass : Cast<UClass>(SubjectObject);

    if (!SubjectClass || !SubjectClass->IsChildOf(AActor::StaticClass()))
    {
        return MakeEditorFailureResponse(TEXT("Status: Invalid build source."), TEXT("The current workspace source is not a Character Pawn or AI Pawn that can be built into a final asset."));
    }

    if (!GEditor)
    {
        return MakeEditorFailureResponse(TEXT("Status: Editor world is unavailable."), TEXT("Could not build a finalized subject because GEditor is unavailable."));
    }

    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();

    if (!EditorWorld)
    {
        return MakeEditorFailureResponse(TEXT("Status: Editor world is unavailable."), TEXT("Could not resolve the current editor world for final build creation."));
    }

    ULevel* TargetLevel = EditorWorld->GetCurrentLevel();

    if (!TargetLevel)
    {
        TargetLevel = EditorWorld->PersistentLevel.Get();
    }

    if (!TargetLevel)
    {
        return MakeEditorFailureResponse(TEXT("Status: Final build failed."), TEXT("Could not resolve a target editor level for the finalized build output."));
    }

    FTransform SpawnTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 120.0f));

    if (const AActor* FirstSelectedActor = GetFirstSelectedActor())
    {
        SpawnTransform = FirstSelectedActor->GetActorTransform();
        SpawnTransform.AddToTranslation(FVector(360.0f, 0.0f, 0.0f));
    }

    FScopedTransaction Transaction(LOCTEXT("WanaWorksCreateFinalizedBuildTransaction", "Create WanaWorks Finalized Build"));
    AActor* SpawnedActor = GEditor->AddActor(TargetLevel, const_cast<UClass*>(SubjectClass), SpawnTransform);

    if (!SpawnedActor)
    {
        Transaction.Cancel();
        return MakeEditorFailureResponse(TEXT("Status: Final build failed."), TEXT("Could not create the finalized build output from the current workspace source."));
    }

    SpawnedActor->SetFlags(RF_Transactional);
    SpawnedActor->Modify();
    SpawnedActor->SetActorLabel(BuildFinalizedActorLabel(SubjectObject->GetName()), true);
    SpawnedActor->SetFolderPath(FName(TEXT("WanaWorks/Builds")));

    bool bAppliedAnimationOverride = false;

    if (AnimationBlueprintOverride && AnimationBlueprintOverride->GeneratedClass)
    {
        TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
        SpawnedActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);

        for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
        {
            if (!SkeletalMeshComponent)
            {
                continue;
            }

            SkeletalMeshComponent->Modify();
            SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
            SkeletalMeshComponent->SetAnimInstanceClass(AnimationBlueprintOverride->GeneratedClass);
            bAppliedAnimationOverride = true;
        }
    }

    const FWanaAutomaticAnimationPreparationResult AutomaticAnimationPreparation = PrepareAutomaticAnimationIntegrationForActor(SpawnedActor);

    SpawnedActor->MarkPackageDirty();

    GEditor->SelectNone(false, true, false);
    GEditor->SelectActor(SpawnedActor, true, true, true);
    GEditor->NoteSelectionChange();

    const FString OutputFolder = SpawnedActor->GetFolderPath().ToString();
    const FString OutputPath = FString::Printf(
        TEXT("%s/%s"),
        OutputFolder.IsEmpty() ? TEXT("WanaWorks/Builds") : *OutputFolder,
        *SpawnedActor->GetActorNameOrLabel());

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = TEXT("Status: Created finalized build output.");
    Response.OutputLines.Add(FString::Printf(TEXT("Source Asset: %s"), *SubjectObject->GetName()));
    Response.OutputLines.Add(TEXT("Workflow Path: Working Subject -> Final Build"));
    Response.OutputLines.Add(FString::Printf(TEXT("Finalized Output: %s"), *SpawnedActor->GetActorNameOrLabel()));
    Response.OutputLines.Add(FString::Printf(TEXT("Finalized Output Path: %s"), *OutputPath));
    Response.OutputLines.Add(TEXT("Build Mode: Finalized upgraded subject"));
    AppendAutomaticAnimationIntegrationLines(Response, AutomaticAnimationPreparation);
    Response.OutputLines.Add(TEXT("Compatibility Notes: The original source was left untouched."));
    Response.OutputLines.Add(TEXT("Compatibility Notes: The working subject remains available for continued testing."));
    Response.OutputLines.Add(TEXT("Compatibility Notes: Automatic Anim BP integration, when supported, was prepared on the finalized output only."));

    if (AnimationBlueprintOverride)
    {
        Response.OutputLines.Add(FString::Printf(TEXT("Picked Animation Blueprint: %s"), *AnimationBlueprintOverride->GetName()));
        Response.OutputLines.Add(FString::Printf(
            TEXT("Animation Override Applied: %s"),
            bAppliedAnimationOverride ? TEXT("Yes") : TEXT("Not applied (no compatible skeletal mesh component found)")));
    }
    else
    {
        Response.OutputLines.Add(TEXT("Picked Animation Blueprint: Auto-detect from subject"));
    }

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

FWanaCommandResponse ExecuteScanEnvironmentReadinessCommand(AActor* ObserverActor, AActor* TargetActor, const FString& PairSourceLabel, bool bTargetFallsBackToObserver)
{
    FWanaEnvironmentReadinessSnapshot Snapshot;
    GetEnvironmentReadinessSnapshotForActorPair(ObserverActor, TargetActor, bTargetFallsBackToObserver, PairSourceLabel, Snapshot);

    FWanaCommandResponse Response;
    Response.bSucceeded = Snapshot.bHasObserverActor && Snapshot.bHasTargetActor;
    Response.StatusMessage = Response.bSucceeded
        ? (Snapshot.MovementReadiness.bCanAttemptMovement
            ? TEXT("Status: Environment readiness scan allows movement-aware reactions.")
            : TEXT("Status: Environment readiness scan recommends facing-only fallback."))
        : TEXT("Status: Environment readiness scan is incomplete.");
    Response.OutputLines.Add(FString::Printf(TEXT("Observer: %s"), Snapshot.bHasObserverActor ? *Snapshot.ObserverActorLabel : TEXT("(not assigned)")));
    Response.OutputLines.Add(FString::Printf(TEXT("Target: %s"), Snapshot.bHasTargetActor ? *Snapshot.TargetActorLabel : TEXT("(not assigned)")));
    AppendEnvironmentReadinessLines(Response, Snapshot);
    return Response;
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
        FString::Printf(TEXT("Assign a %s in the workspace before focusing it."), *RoleLabel.ToLower()));
    }

    GEditor->MoveViewportCamerasToActor(*Actor, false);

    FWanaCommandResponse Response;
    Response.bSucceeded = true;
    Response.StatusMessage = FString::Printf(TEXT("Status: Focused %s."), *RoleLabel.ToLower());
    Response.OutputLines.Add(FString::Printf(TEXT("%s: %s"), *RoleLabel, *Actor->GetActorNameOrLabel()));
    Response.OutputLines.Add(TEXT("Readiness Notes: Viewport camera moved to the assigned workspace actor."));
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
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Movement Confidence:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Movement Capability:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Navigation Context:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Reachability:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Obstacle Pressure:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Movement Space:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Fallback Mode:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Identity Seed:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Relationship State:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Reaction State:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Recommended Behavior:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Starter Hook:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Visible Behavior:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Last Applied Hook:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Execution Mode:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Movement Compatibility:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Fallback Reason:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Environment Shaping:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Result Notes:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Recommended Next Step:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Animation Hook Application:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Animation Behavior Intent:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Animation Posture Hint:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Animation Posture Category:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Animation Identity Role Hint:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Animation Fallback Hint:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Facing Hook:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Turn-To-Target Hook:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Movement-Limited Animation Hook:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Outward Guard Hook:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Physical Reaction Hook:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Animation Hook Notes:"));
    AppendLinesWithPrefix(Response, EnhancementResponse, TEXT("Compatibility Notes:"));
    AppendLinesWithPrefix(Response, EvaluationResponse, TEXT("Readiness Notes:"));
    return Response;
}
}

#undef LOCTEXT_NAMESPACE

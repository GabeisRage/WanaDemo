#include "WITBlueprintLibrary.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "WanaWorksEnvironmentReadiness.h"
#include "WAYPlayerProfileComponent.h"

FWanaPredictionProfile UWITBlueprintLibrary::BuildPredictionProfile(FName ClassificationTag, EWanaBehaviorDomain Domain, float Confidence)
{
    FWanaPredictionProfile Profile;
    Profile.ClassificationTag = ClassificationTag;
    Profile.Domain = Domain;
    Profile.Confidence = FMath::Clamp(Confidence, 0.0f, 1.0f);
    return Profile;
}

FWanaPredictionProfile UWITBlueprintLibrary::EvaluateActorForObserver(AActor* ObserverActor, AActor* TargetActor)
{
    if (!TargetActor)
    {
        return BuildPredictionProfile(TEXT("unknown"), EWanaBehaviorDomain::Unknown, 0.0f);
    }

    if (ObserverActor && ObserverActor != TargetActor)
    {
        if (UWAYPlayerProfileComponent* RelationshipComponent = ObserverActor->FindComponentByClass<UWAYPlayerProfileComponent>())
        {
            RelationshipComponent->EvaluateTarget(TargetActor);
        }
    }

    TArray<UActorComponent*> Components;
    TargetActor->GetComponents(Components);

    TArray<FName> ComponentClassNames;
    ComponentClassNames.Reserve(Components.Num());

    for (const UActorComponent* Component : Components)
    {
        if (Component)
        {
            ComponentClassNames.Add(Component->GetClass()->GetFName());
        }
    }

    return ClassifyActorHeuristically(
        TargetActor->GetFName(),
        TargetActor->GetClass()->GetFName(),
        TargetActor->Tags,
        ComponentClassNames);
}

FWanaMovementReadiness UWITBlueprintLibrary::GetMovementReadinessForObserverTarget(AActor* ObserverActor, AActor* TargetActor)
{
    return FWanaWorksEnvironmentReadiness::EvaluateMovementReadiness(ObserverActor, TargetActor);
}

FWanaPredictionProfile UWITBlueprintLibrary::ClassifyActorHeuristically(
    FName ActorName,
    FName ClassName,
    const TArray<FName>& ActorTags,
    const TArray<FName>& ComponentClassNames)
{
    const FString ActorNameString = ActorName.ToString().ToLower();
    const FString ClassNameString = ClassName.ToString().ToLower();
    FString Combined = ActorNameString + TEXT(" ") + ClassNameString;

    for (const FName& ActorTag : ActorTags)
    {
        Combined += TEXT(" ");
        Combined += ActorTag.ToString().ToLower();
    }

    for (const FName& ComponentClassName : ComponentClassNames)
    {
        Combined += TEXT(" ");
        Combined += ComponentClassName.ToString().ToLower();
    }

    if (Combined.Contains(TEXT("character")) || Combined.Contains(TEXT("pawn")) || Combined.Contains(TEXT("player")) || Combined.Contains(TEXT("npc")))
    {
        return BuildPredictionProfile(TEXT("character"), EWanaBehaviorDomain::Character, 0.90f);
    }

    if (Combined.Contains(TEXT("vehicle")) || Combined.Contains(TEXT("car")) || Combined.Contains(TEXT("truck")) || Combined.Contains(TEXT("bike")))
    {
        return BuildPredictionProfile(TEXT("vehicle"), EWanaBehaviorDomain::Vehicle, 0.90f);
    }

    if (Combined.Contains(TEXT("tree")) || Combined.Contains(TEXT("rock")) || Combined.Contains(TEXT("landscape")) || Combined.Contains(TEXT("terrain")) || Combined.Contains(TEXT("sky")) || Combined.Contains(TEXT("light")) || Combined.Contains(TEXT("foliage")) || Combined.Contains(TEXT("instancedstaticmesh")))
    {
        return BuildPredictionProfile(TEXT("environment"), EWanaBehaviorDomain::Environment, 0.80f);
    }

    if (Combined.Contains(TEXT("cube")) || Combined.Contains(TEXT("mesh")) || Combined.Contains(TEXT("prop")) || Combined.Contains(TEXT("crate")) || Combined.Contains(TEXT("chair")) || Combined.Contains(TEXT("table")) || Combined.Contains(TEXT("door")) || Combined.Contains(TEXT("staticmeshcomponent")))
    {
        return BuildPredictionProfile(TEXT("prop"), EWanaBehaviorDomain::Prop, 0.75f);
    }

    return BuildPredictionProfile(TEXT("unknown"), EWanaBehaviorDomain::Unknown, 0.25f);
}

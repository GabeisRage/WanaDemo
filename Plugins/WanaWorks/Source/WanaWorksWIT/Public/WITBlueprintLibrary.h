#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "WanaWorksTypes.h"
#include "WITBlueprintLibrary.generated.h"

class AActor;

UCLASS()
class WANAWORKSWIT_API UWITBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Wana Works|WIT")
    static FWanaPredictionProfile BuildPredictionProfile(FName ClassificationTag, EWanaBehaviorDomain Domain, float Confidence);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WIT", meta = (DefaultToSelf = "ObserverActor"))
    static FWanaPredictionProfile EvaluateActorForObserver(AActor* ObserverActor, AActor* TargetActor);

    static FWanaPredictionProfile ClassifyActorHeuristically(
        FName ActorName,
        FName ClassName,
        const TArray<FName>& ActorTags,
        const TArray<FName>& ComponentClassNames);
};

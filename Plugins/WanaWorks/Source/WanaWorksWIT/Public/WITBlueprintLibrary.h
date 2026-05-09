#pragma once

#include "CoreMinimal.h"
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

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WIT")
    static FWITObjectProfile ClassifyActor(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WIT")
    static FString GetObjectSummary(const FWITObjectProfile& Profile);
};

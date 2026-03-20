#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "WanaWorksTypes.h"
#include "WITBlueprintLibrary.generated.h"

UCLASS()
class WANAWORKSWIT_API UWITBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Wana Works|WIT")
    static FWanaPredictionProfile BuildPredictionProfile(FName ClassificationTag, EWanaBehaviorDomain Domain, float Confidence);
};

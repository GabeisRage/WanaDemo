#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WanaProjectScanner.generated.h"

class AActor;

UCLASS(BlueprintType)
class WANAWORKSCORE_API UWanaProjectScanner : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Wana Works|Core", meta = (WorldContext = "WorldContext"))
    static TArray<AActor*> ScanForCharacterBlueprints(UObject* WorldContext);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Core", meta = (WorldContext = "WorldContext"))
    static TArray<AActor*> ScanForAIPawns(UObject* WorldContext);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Core", meta = (WorldContext = "WorldContext"))
    static TArray<AActor*> ScanForAIControllers(UObject* WorldContext);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Core", meta = (WorldContext = "WorldContext"))
    static FString GetScanSummary(UObject* WorldContext);
};

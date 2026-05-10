#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WanaProjectScanner.generated.h"

UCLASS()
class WANAWORKSCORE_API UWanaProjectScanner : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Wana Works|Scanner", meta=(WorldContext="WorldContextObject"))
    static TArray<AActor*> ScanForCharacterBlueprints(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="Wana Works|Scanner", meta=(WorldContext="WorldContextObject"))
    static TArray<AActor*> ScanForAIPawns(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="Wana Works|Scanner", meta=(WorldContext="WorldContextObject"))
    static TArray<AActor*> ScanForAIControllers(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="Wana Works|Scanner", meta=(WorldContext="WorldContextObject"))
    static FString GetScanSummary(UObject* WorldContextObject);
};

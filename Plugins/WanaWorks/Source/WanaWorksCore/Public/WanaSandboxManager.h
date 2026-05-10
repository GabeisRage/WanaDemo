#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WanaSandboxManager.generated.h"

UCLASS()
class WANAWORKSCORE_API UWanaSandboxManager : public UObject
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, Category="Wana Works|Sandbox")
    static FString BackupActorState(AActor* Target);

    UFUNCTION(BlueprintCallable, Category="Wana Works|Sandbox")
    static TArray<FString> ListBackups();

    UFUNCTION(BlueprintCallable, Category="Wana Works|Sandbox")
    static bool RestoreFromBackup(const FString& BackupId, AActor* Target);

private:
    static TMap<FString, TArray<uint8>> BackupStore;
#endif
};

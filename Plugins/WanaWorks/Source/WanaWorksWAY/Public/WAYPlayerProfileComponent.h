#pragma once

#include "Components/ActorComponent.h"
#include "WAYPlayerProfileComponent.generated.h"

USTRUCT(BlueprintType)
struct WANAWORKSWAY_API FWAYPreferenceSignal
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    FName SignalName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Weight = 0.0f;
};

UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSWAY_API UWAYPlayerProfileComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY")
    void RecordPreferenceSignal(FName SignalName, float Weight);

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAY")
    const TArray<FWAYPreferenceSignal>& GetSignals() const { return Signals; }

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|WAY", meta = (AllowPrivateAccess = "true"))
    TArray<FWAYPreferenceSignal> Signals;
};

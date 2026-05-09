#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WAYPlayerProfileComponent.generated.h"

UENUM(BlueprintType)
enum class EWAYBehaviorCategory : uint8
{
    Combat,
    Stealth,
    Mercy,
    Betrayal,
    Loyalty,
    Destruction,
    Exploration,
    Dialogue
};

USTRUCT(BlueprintType)
struct WANAWORKSWAY_API FWAYBehaviorRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    EWAYBehaviorCategory Category = EWAYBehaviorCategory::Exploration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    float Weight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAY")
    FString Context;
};

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
    UWAYPlayerProfileComponent();

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY")
    void RecordBehavior(EWAYBehaviorCategory Category, float Weight, const FString& Context);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY")
    void RecordPreferenceSignal(FName SignalName, float Weight);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAY")
    void ResetProfile();

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAY")
    EWAYBehaviorCategory GetDominantPlayStyle() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAY")
    float GetBehaviorScore(EWAYBehaviorCategory Category) const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAY")
    const TArray<FWAYBehaviorRecord>& GetBehaviorHistory() const { return BehaviorHistory; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAY")
    const TArray<FWAYPreferenceSignal>& GetSignals() const { return Signals; }

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|WAY", meta = (AllowPrivateAccess = "true"))
    TArray<FWAYBehaviorRecord> BehaviorHistory;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|WAY", meta = (AllowPrivateAccess = "true"))
    TArray<FWAYPreferenceSignal> Signals;
};

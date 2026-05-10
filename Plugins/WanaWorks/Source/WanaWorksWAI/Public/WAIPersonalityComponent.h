#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WAIPersonalityComponent.generated.h"

USTRUCT(BlueprintType)
struct WANAWORKSWAI_API FWAIMemoryEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAI")
    FString Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAI")
    float EmotionalImpact = 0.0f;
};

USTRUCT(BlueprintType)
struct WANAWORKSWAI_API FWAITraitSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Aggression = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Fear = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Friendliness = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Intelligence = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Curiosity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Loyalty = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Stress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Confidence = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Suspicion = 0.0f;
};

UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSWAI_API UWAIPersonalityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWAIPersonalityComponent();

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAI")
    void RememberEvent(const FString& Summary, float EmotionalImpact);

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAI")
    const TArray<FWAIMemoryEvent>& GetMemories() const { return Memories; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|WAI")
    FWAITraitSet Traits;

    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAI")
    void AdjustTrait(FName TraitName, float Delta);

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAI")
    FName GetDominantTrait() const;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|WAI", meta = (AllowPrivateAccess = "true"))
    TArray<FWAIMemoryEvent> Memories;
};

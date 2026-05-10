#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WAIEmotionComponent.generated.h"

UENUM(BlueprintType)
enum class EWAIEmotionState : uint8
{
    Calm,
    Happy,
    Afraid,
    Angry,
    Stressed,
    Confused,
    Hostile,
    Trusting
};

UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSWAI_API UWAIEmotionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI")
    EWAIEmotionState CurrentEmotion = EWAIEmotionState::Calm;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    float EmotionIntensity = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Wana Works|WAI")
    void SetEmotion(EWAIEmotionState NewEmotion, float Intensity);

    UFUNCTION(BlueprintCallable, Category="Wana Works|WAI")
    void BlendEmotion(EWAIEmotionState TargetEmotion, float Alpha);

    UFUNCTION(BlueprintPure, Category="Wana Works|WAI")
    EWAIEmotionState GetCurrentEmotion() const { return CurrentEmotion; }

    UFUNCTION(BlueprintPure, Category="Wana Works|WAI")
    float GetEmotionIntensity() const { return EmotionIntensity; }
};

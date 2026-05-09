#include "WAIEmotionComponent.h"

#include "WanaWorksCoreModule.h"

UWAIEmotionComponent::UWAIEmotionComponent()
    : CurrentEmotion(EWAIEmotionState::Calm)
    , EmotionIntensity(0.0f)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWAIEmotionComponent::SetEmotion(EWAIEmotionState NewEmotion, float Intensity)
{
    CurrentEmotion = NewEmotion;
    EmotionIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
    FWanaLogger::Log(FString::Printf(TEXT("WAI Emotion set: state=%d intensity=%.2f"),
        static_cast<int32>(CurrentEmotion), EmotionIntensity));
}

void UWAIEmotionComponent::BlendEmotion(EWAIEmotionState TargetEmotion, float Alpha)
{
    const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
    if (TargetEmotion == CurrentEmotion)
    {
        EmotionIntensity = FMath::Clamp(FMath::Lerp(EmotionIntensity, 1.0f, ClampedAlpha), 0.0f, 1.0f);
        return;
    }

    EmotionIntensity = FMath::Clamp(FMath::Lerp(EmotionIntensity, 0.0f, ClampedAlpha), 0.0f, 1.0f);
    if (EmotionIntensity <= KINDA_SMALL_NUMBER)
    {
        CurrentEmotion = TargetEmotion;
        EmotionIntensity = ClampedAlpha;
    }
}

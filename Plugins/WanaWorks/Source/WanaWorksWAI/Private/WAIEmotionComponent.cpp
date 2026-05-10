#include "WAIEmotionComponent.h"

void UWAIEmotionComponent::SetEmotion(EWAIEmotionState NewEmotion, float Intensity)
{
    CurrentEmotion = NewEmotion;
    EmotionIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
}

void UWAIEmotionComponent::BlendEmotion(EWAIEmotionState TargetEmotion, float Alpha)
{
    const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
    if (TargetEmotion != CurrentEmotion && ClampedAlpha > 0.5f)
    {
        CurrentEmotion = TargetEmotion;
    }
    EmotionIntensity = FMath::Clamp(FMath::Lerp(EmotionIntensity, 1.0f, ClampedAlpha), 0.0f, 1.0f);
}

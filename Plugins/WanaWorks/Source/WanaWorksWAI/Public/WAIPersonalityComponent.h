#pragma once

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

UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSWAI_API UWAIPersonalityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Wana Works|WAI")
    void RememberEvent(const FString& Summary, float EmotionalImpact);

    UFUNCTION(BlueprintPure, Category = "Wana Works|WAI")
    const TArray<FWAIMemoryEvent>& GetMemories() const { return Memories; }

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|WAI", meta = (AllowPrivateAccess = "true"))
    TArray<FWAIMemoryEvent> Memories;
};

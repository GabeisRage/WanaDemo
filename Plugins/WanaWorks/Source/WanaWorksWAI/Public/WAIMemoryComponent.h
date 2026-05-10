#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WAIMemoryComponent.generated.h"

UENUM(BlueprintType)
enum class EWAIMemoryType : uint8
{
    ShortTerm,
    LongTerm,
    Combat,
    Location,
    Event,
    PlayerRelationship
};

USTRUCT(BlueprintType)
struct WANAWORKSWAI_API FWAIMemoryRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI")
    FString Content;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI")
    EWAIMemoryType MemoryType = EWAIMemoryType::ShortTerm;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI")
    float EmotionalWeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wana Works|WAI")
    float Timestamp = 0.0f;
};

UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSWAI_API UWAIMemoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Wana Works|WAI")
    void StoreMemory(FString Content, EWAIMemoryType Type, float EmotionalWeight);

    UFUNCTION(BlueprintCallable, Category="Wana Works|WAI")
    void ForgetOldMemories(int32 MaxShortTermCount);

    UFUNCTION(BlueprintPure, Category="Wana Works|WAI")
    TArray<FWAIMemoryRecord> GetMemoriesByType(EWAIMemoryType Type) const;

    UFUNCTION(BlueprintPure, Category="Wana Works|WAI")
    FWAIMemoryRecord GetMostRecentMemory() const;

private:
    UPROPERTY()
    TArray<FWAIMemoryRecord> MemoryBank;
};

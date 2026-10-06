#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WAIEmotionComponent.h"
#include "WAIMemoryComponent.h"
#include "WAIPersonalityComponent.h"
#include "WAYPlayerProfileComponent.h"
#include "WanaIdentityComponent.h"
#include "WanaMemorySubsystem.h"
#include "WanaLegacyStateAdapter.generated.h"

class AActor;

/* Explicit save/load for the in-memory WAI, WAY, and identity components.
   Nothing here runs unless a designer calls it. The components themselves
   keep their original behavior. */
UCLASS()
class WANAWORKSMEMORY_API UWanaLegacyStateAdapter : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    static bool SaveLegacySnapshot(
        UWanaMemorySubsystem* Memory,
        FString CharacterId,
        UWAIMemoryComponent* MemoryComponent,
        UWAIEmotionComponent* EmotionComponent,
        UWAIPersonalityComponent* PersonalityComponent,
        UWAYPlayerProfileComponent* WayComponent,
        UWanaIdentityComponent* IdentityComponent,
        FString& OutReport);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    static bool LoadLegacySnapshot(
        UWanaMemorySubsystem* Memory,
        FString CharacterId,
        UWAIMemoryComponent* MemoryComponent,
        UWAIEmotionComponent* EmotionComponent,
        UWAIPersonalityComponent* PersonalityComponent,
        UWAYPlayerProfileComponent* WayComponent,
        AActor* WayTargetActor,
        UWanaIdentityComponent* IdentityComponent,
        FString& OutReport);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    static bool ImportRelationshipFromWAY(
        UWanaMemorySubsystem* Memory,
        FString CharacterId,
        FString PlayerId,
        UWAYPlayerProfileComponent* WayComponent,
        AActor* PlayerActor,
        FString& OutReport);
};

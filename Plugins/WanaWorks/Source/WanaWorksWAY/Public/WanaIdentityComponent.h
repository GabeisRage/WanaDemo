#pragma once

#include "Components/ActorComponent.h"
#include "WAYRelationshipTypes.h"
#include "WanaIdentityComponent.generated.h"

UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSWAY_API UWanaIdentityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWanaIdentityComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Identity")
    FName FactionTag = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Identity")
    FWAYRelationshipSeed DefaultRelationshipSeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Identity")
    TArray<FName> ReputationTags;
};

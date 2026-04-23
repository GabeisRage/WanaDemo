#pragma once

#include "Components/ActorComponent.h"
#include "WAYRelationshipTypes.h"
#include "WanaAutoAnimationIntegrationComponent.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;

UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSWAY_API UWanaAutoAnimationIntegrationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWanaAutoAnimationIntegrationComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Animation", meta = (DisplayName = "Refresh Automatic Animation Integration"))
    void RefreshAutomaticAnimationIntegration();

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation")
    EWAYAutomaticAnimationIntegrationStatus GetAutomaticIntegrationStatus() const { return AutomaticIntegrationStatus; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation")
    bool HasAutomaticAnimationIntegrationSupport() const { return SupportedFieldCount > 0; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation")
    EWAYAutomaticAnimationIntegrationStatus AutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::NotSupported;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation")
    bool bAutoAttachSucceeded = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation")
    bool bAutoWireSucceeded = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation")
    int32 SupportedFieldCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation")
    int32 LastAppliedFieldCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation")
    FString SourceAnimationBlueprintLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation")
    FString IntegrationTargetLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation")
    FString Detail;

private:
    USkeletalMeshComponent* ResolveIntegrationMesh() const;
    UAnimInstance* ResolveLiveAnimInstance(USkeletalMeshComponent*& OutMeshComponent, UClass*& OutAnimClass) const;
};

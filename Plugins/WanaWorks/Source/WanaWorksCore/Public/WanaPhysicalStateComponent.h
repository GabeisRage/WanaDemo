#pragma once

#include "Components/ActorComponent.h"
#include "WanaWorksTypes.h"
#include "WanaPhysicalStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWanaPhysicalStateChangedSignature, EWanaPhysicalState, PreviousState, EWanaPhysicalState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWanaRecoveryStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWanaRecoveryFinishedSignature);

UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSCORE_API UWanaPhysicalStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWanaPhysicalStateComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Physical State", meta = (DisplayName = "Set Physical State"))
    void SetPhysicalState(EWanaPhysicalState NewPhysicalState);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Physical State", meta = (DisplayName = "Enter Alert State"))
    void EnterAlertState(float AlertFearInfluence = 0.25f);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Physical State", meta = (DisplayName = "Set Bracing"))
    void SetBracing(bool bInBracing, float StabilityLift = 0.15f);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Physical State", meta = (DisplayName = "Start Recovery"))
    void StartRecovery(float RecoveryDurationOverride = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Physical State", meta = (DisplayName = "Finish Recovery"))
    void FinishRecovery();

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Physical State", meta = (DisplayName = "Apply Impact Hint"))
    void ApplyImpactHint(FVector ImpactDirection, float ImpactStrength, bool bAutoStartRecovery = true);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Physical State", meta = (DisplayName = "Set Fear Influence"))
    void SetFearInfluence(float NewFearInfluence);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Physical State", meta = (DisplayName = "Refresh Physical Readiness"))
    void RefreshPhysicalReadiness();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Physical State")
    EWanaPhysicalState PhysicalState = EWanaPhysicalState::Stable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Physical State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StabilityScore = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Physical State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RecoveryProgress = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Physical State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FearInfluence = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Physical State")
    FVector LastImpactDirection = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Physical State", meta = (ClampMin = "0.0"))
    float LastImpactStrength = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Physical State")
    bool bBracing = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Physical State")
    bool bCanCommitToMovement = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Physical State")
    bool bCanCommitToAttack = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Physical State")
    bool bNeedsRecovery = false;

    UPROPERTY(BlueprintAssignable, Category = "Wana Works|Physical State")
    FWanaPhysicalStateChangedSignature OnPhysicalStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Wana Works|Physical State")
    FWanaRecoveryStartedSignature OnRecoveryStarted;

    UPROPERTY(BlueprintAssignable, Category = "Wana Works|Physical State")
    FWanaRecoveryFinishedSignature OnRecoveryFinished;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Physical State", meta = (ClampMin = "0.1"))
    float RecoveryDuration = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Physical State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MovementCommitThreshold = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Physical State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AttackCommitThreshold = 0.60f;

private:
    void BroadcastPhysicalStateChanged(EWanaPhysicalState PreviousState, EWanaPhysicalState NewState);
    void UpdateDerivedFlags();
};

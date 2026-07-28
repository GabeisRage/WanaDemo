#pragma once

#include "Components/ActorComponent.h"
#include "WanaWorksTypes.h"
#include "WanaProceduralReactionComponent.generated.h"

class UWanaPhysicalStateComponent;
class USkeletalMeshComponent;

/**
 * Phase 6 foundation: turns WanaWorks physical state into a visible, plugin-owned
 * procedural reaction on the subject's skeletal mesh component (lean away from the
 * last impact, sway while staggered/off-balance, settle during recovery).
 *
 * Non-destructive by design: only the mesh component's relative transform is offset
 * at runtime/editor time, layered on top of whatever the Anim BP already does. No
 * animation assets or Anim BP graphs are read or modified, and the captured base
 * transform is restored when the reaction settles or the component shuts down.
 */
UCLASS(ClassGroup=(WanaWorks), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WANAWORKSCORE_API UWanaProceduralReactionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWanaProceduralReactionComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnUnregister() override;

    /** Re-resolve the physical state component and target mesh on the owner. */
    UFUNCTION(BlueprintCallable, Category = "Wana Works|Procedural Reaction", meta = (DisplayName = "Refresh Reaction Bindings"))
    void RefreshReactionBindings();

    /** Demo/test helper: forwards an impact hint to the owner's physical state component. */
    UFUNCTION(BlueprintCallable, Category = "Wana Works|Procedural Reaction", meta = (DisplayName = "Trigger Test Impact"))
    void TriggerTestImpact(FVector ImpactDirection, float ImpactStrength);

    /** Maximum lean applied at full instability, in degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Procedural Reaction", meta = (ClampMin = "0.0", ClampMax = "45.0"))
    float MaxLeanAngleDegrees = 14.0f;

    /** Additional sway amplitude while Staggered / Off Balance / Panicked, in degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Procedural Reaction", meta = (ClampMin = "0.0", ClampMax = "30.0"))
    float StaggerSwayDegrees = 6.0f;

    /** Sway oscillation speed while unstable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Procedural Reaction", meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float SwayFrequency = 5.5f;

    /** Interp speed toward a stronger reaction (impact response feels sharp). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Procedural Reaction", meta = (ClampMin = "0.1", ClampMax = "30.0"))
    float ReactionInterpSpeed = 8.0f;

    /** Interp speed while settling back toward the base pose (recovery feels gradual). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Procedural Reaction", meta = (ClampMin = "0.1", ClampMax = "30.0"))
    float RecoveryInterpSpeed = 3.0f;

    /** Vertical dip applied at full instability (world units, applied downward). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Procedural Reaction", meta = (ClampMin = "0.0", ClampMax = "30.0"))
    float MaxInstabilityDip = 5.0f;

    /** True while a non-trivial reaction offset is being applied to the mesh. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Procedural Reaction")
    bool bReactionActive = false;

    /** Current rotation offset applied on top of the mesh's base relative rotation. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Procedural Reaction")
    FRotator CurrentReactionOffset = FRotator::ZeroRotator;

private:
    void ResolveBindings();
    void CaptureBaseTransform();
    void RestoreBaseTransform();
    float ComputeStateLeanScale(EWanaPhysicalState State, float RecoveryProgress) const;

    UPROPERTY()
    TWeakObjectPtr<UWanaPhysicalStateComponent> PhysicalState;

    UPROPERTY()
    TWeakObjectPtr<USkeletalMeshComponent> TargetMesh;

    FRotator BaseRelativeRotation = FRotator::ZeroRotator;
    FVector BaseRelativeLocation = FVector::ZeroVector;
    bool bBaseTransformCaptured = false;
    bool bOffsetApplied = false;
    float CurrentDip = 0.0f;
    float SwayTime = 0.0f;
};

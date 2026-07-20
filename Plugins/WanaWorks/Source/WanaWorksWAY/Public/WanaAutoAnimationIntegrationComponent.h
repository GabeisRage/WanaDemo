#pragma once

#include "Components/ActorComponent.h"
#include "WAYRelationshipTypes.h"
#include "WanaWorksTypes.h"
#include "WanaAutoAnimationIntegrationComponent.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;
class UWanaAnimationAdapterReportAsset;

USTRUCT(BlueprintType)
struct FWanaRuntimeAnimationAdapterState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bHookReadable = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bReportReadable = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString ReadinessState = TEXT("Not Supported");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RecommendedStrategy = TEXT("Needs Enhance");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString DirectGraphEditSafety = TEXT("Limited");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString SharedAnimBPRisk = TEXT("Unknown");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString PersistentReportPath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString BehaviorIntent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString VisibleBehaviorLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString PostureHint = TEXT("observant");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString PostureCategory = TEXT("Observe");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString FallbackHint = TEXT("stable observe stance");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString IdentityRole = TEXT("neutral");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWAYReactionState ReactionState = EWAYReactionState::Observational;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWAYRelationshipState RelationshipState = EWAYRelationshipState::Neutral;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWAYBehaviorPreset RecommendedBehavior = EWAYBehaviorPreset::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWAYBehaviorExecutionMode ExecutionMode = EWAYBehaviorExecutionMode::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bFacingHookRequested = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bTurnToTargetRequested = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bOutwardGuardHintRequested = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bLocomotionSafeExecutionHint = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bMovementLimitedFallbackHint = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bPhysicalReactionAvailable = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWanaPhysicalState PhysicalState = EWanaPhysicalState::Stable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    float StabilityScore = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    float InstabilityAlpha = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    float RecoveryProgress = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FVector ImpactDirection = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    float ImpactStrength = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bBracing = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bNeedsRecovery = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bCanCommitToMovement = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bCanCommitToAttack = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    int32 SupportedFieldCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    int32 AppliedFieldCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString Detail;
};

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

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Animation", meta = (DisplayName = "Refresh Runtime WanaAnimation Adapter"))
    void RefreshRuntimeAnimationAdapter();

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Animation", meta = (DisplayName = "Set Persistent Adapter Report Asset"))
    void SetPersistentAdapterReportAsset(UWanaAnimationAdapterReportAsset* InReportAsset);

    UFUNCTION(BlueprintCallable, Category = "Wana Works|Animation", meta = (DisplayName = "Set Persistent Adapter Report Path"))
    void SetPersistentAdapterReportPath(const FString& InReportPath);

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation")
    EWAYAutomaticAnimationIntegrationStatus GetAutomaticIntegrationStatus() const { return AutomaticIntegrationStatus; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation")
    bool HasAutomaticAnimationIntegrationSupport() const { return SupportedFieldCount > 0; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Adapter Animation Hook State"))
    FWAYAnimationHookState GetAdapterAnimationHookState() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Adapter Posture Hint"))
    FString GetAdapterPostureHint() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Adapter Reaction State"))
    EWAYReactionState GetAdapterReactionState() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Adapter Recommended Behavior"))
    EWAYBehaviorPreset GetAdapterRecommendedBehavior() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Adapter Visible Behavior Label"))
    FString GetAdapterVisibleBehaviorLabel() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is Adapter Locomotion Safe Hint Active"))
    bool IsAdapterLocomotionSafeHintActive() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is Adapter Movement Limited Fallback Active"))
    bool IsAdapterMovementLimitedFallbackActive() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Adapter Instability Alpha"))
    float GetAdapterInstabilityAlpha() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Adapter Recovery Progress"))
    float GetAdapterRecoveryProgress() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Adapter Impact Direction"))
    FVector GetAdapterImpactDirection() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Posture Hint"))
    FString GetWanaAnimationPostureHint() const { return RuntimeAdapterPostureHint; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Runtime Adapter State"))
    FWanaRuntimeAnimationAdapterState GetWanaAnimationRuntimeAdapterState() const;

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Posture Category"))
    FString GetWanaAnimationPostureCategory() const { return RuntimeAdapterPostureCategory; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Reaction State"))
    EWAYReactionState GetWanaAnimationReactionState() const { return RuntimeAdapterReactionState; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Recommended Behavior"))
    EWAYBehaviorPreset GetWanaAnimationRecommendedBehavior() const { return RuntimeAdapterRecommendedBehavior; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Visible Behavior Label"))
    FString GetWanaAnimationVisibleBehaviorLabel() const { return RuntimeAdapterVisibleBehaviorLabel; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Behavior Intent"))
    FString GetWanaAnimationBehaviorIntent() const { return RuntimeAdapterBehaviorIntent; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Fallback Hint"))
    FString GetWanaAnimationFallbackHint() const { return RuntimeAdapterFallbackHint; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Identity Role"))
    FString GetWanaAnimationIdentityRole() const { return RuntimeAdapterIdentityRole; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Relationship State"))
    EWAYRelationshipState GetWanaAnimationRelationshipState() const { return RuntimeAdapterRelationshipState; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Execution Mode"))
    EWAYBehaviorExecutionMode GetWanaAnimationExecutionMode() const { return RuntimeAdapterExecutionMode; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is WanaAnimation Facing Hook Requested"))
    bool IsWanaAnimationFacingHookRequested() const { return bRuntimeAdapterFacingHookRequested; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is WanaAnimation Turn To Target Requested"))
    bool IsWanaAnimationTurnToTargetRequested() const { return bRuntimeAdapterTurnToTargetRequested; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is WanaAnimation Outward Guard Hint Requested"))
    bool IsWanaAnimationOutwardGuardHintRequested() const { return bRuntimeAdapterOutwardGuardHintRequested; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is WanaAnimation Locomotion Safe Execution Hint Active"))
    bool IsWanaAnimationLocomotionSafeExecutionHintActive() const { return bRuntimeAdapterLocomotionSafeExecutionHint; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is WanaAnimation Movement Limited Fallback Hint Active"))
    bool IsWanaAnimationMovementLimitedFallbackHintActive() const { return bRuntimeAdapterMovementLimitedFallbackHint; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Instability Alpha"))
    float GetWanaAnimationInstabilityAlpha() const { return RuntimeAdapterInstabilityAlpha; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Physical State"))
    EWanaPhysicalState GetWanaAnimationPhysicalState() const { return RuntimeAdapterPhysicalState; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Stability Score"))
    float GetWanaAnimationStabilityScore() const { return RuntimeAdapterStabilityScore; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Recovery Progress"))
    float GetWanaAnimationRecoveryProgress() const { return RuntimeAdapterRecoveryProgress; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Impact Direction"))
    FVector GetWanaAnimationImpactDirection() const { return RuntimeAdapterImpactDirection; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Impact Strength"))
    float GetWanaAnimationImpactStrength() const { return RuntimeAdapterImpactStrength; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is WanaAnimation Bracing"))
    bool IsWanaAnimationBracing() const { return bRuntimeAdapterBracing; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Does WanaAnimation Need Recovery"))
    bool DoesWanaAnimationNeedRecovery() const { return bRuntimeAdapterNeedsRecovery; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Can WanaAnimation Commit To Movement"))
    bool CanWanaAnimationCommitToMovement() const { return bRuntimeAdapterCanCommitToMovement; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Can WanaAnimation Commit To Attack"))
    bool CanWanaAnimationCommitToAttack() const { return bRuntimeAdapterCanCommitToAttack; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Adapter Readiness"))
    FString GetWanaAnimationAdapterReadiness() const { return RuntimeAdapterReadinessState; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get WanaAnimation Recommended Strategy"))
    FString GetWanaAnimationRecommendedStrategy() const { return RuntimeAdapterRecommendedStrategy; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Persistent Adapter Report Path"))
    FString GetPersistentAdapterReportPath() const { return PersistentAdapterReportPath; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Has Readable WanaAnimation Hook State"))
    bool HasReadableWanaAnimationHookState() const { return bRuntimeAdapterHookReadable; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is Persistent Adapter Report Readable"))
    bool IsPersistentAdapterReportReadable() const { return bRuntimeAdapterReportReadable; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is Direct Anim Graph Edit Safe"))
    bool IsDirectAnimGraphEditSafe() const { return bRuntimeAdapterDirectGraphEditSafe; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Is Shared Anim BP Detected"))
    bool IsSharedAnimBPDetected() const { return bRuntimeAdapterSharedAnimBPDetected; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Animation", meta = (DisplayName = "Get Runtime Adapter Detail"))
    FString GetRuntimeAdapterDetail() const { return RuntimeAdapterDetail; }

    UFUNCTION(BlueprintPure, Category = "Wana Works|Visible Reaction", meta = (DisplayName = "Is Procedural Visible Reaction Active"))
    bool IsProceduralVisibleReactionActive() const;

    // Procedural visible reaction: plugin-owned lean/stagger/recovery motion applied to the
    // resolved skeletal mesh transform so physical state is observable without Anim BP edits.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Visible Reaction")
    bool bEnableProceduralVisibleReaction = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Visible Reaction", meta = (ClampMin = "0.0", ClampMax = "45.0"))
    float MaxVisibleLeanDegrees = 16.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Visible Reaction", meta = (ClampMin = "0.0", ClampMax = "30.0"))
    float VisibleWobbleDegrees = 7.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Visible Reaction", meta = (ClampMin = "0.0", ClampMax = "40.0"))
    float BracingCrouchOffset = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Visible Reaction", meta = (ClampMin = "0.5", ClampMax = "30.0"))
    float VisibleReactionInterpSpeed = 9.0f;

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    TSoftObjectPtr<UWanaAnimationAdapterReportAsset> PersistentAdapterReport;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString PersistentAdapterReportPath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterReadinessState = TEXT("Not Supported");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterRecommendedStrategy = TEXT("Needs Enhance");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterDirectGraphEditSafety = TEXT("Limited");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterSharedAnimBPRisk = TEXT("Unknown");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterBehaviorIntent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterVisibleBehaviorLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterPostureHint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterPostureCategory;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterFallbackHint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterIdentityRole;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWAYReactionState RuntimeAdapterReactionState = EWAYReactionState::Observational;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWAYRelationshipState RuntimeAdapterRelationshipState = EWAYRelationshipState::Neutral;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWAYBehaviorPreset RuntimeAdapterRecommendedBehavior = EWAYBehaviorPreset::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWAYBehaviorExecutionMode RuntimeAdapterExecutionMode = EWAYBehaviorExecutionMode::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterHookReadable = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterReportReadable = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterSharedAnimBPDetected = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterDirectGraphEditSafe = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterFacingHookRequested = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterTurnToTargetRequested = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterOutwardGuardHintRequested = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterLocomotionSafeExecutionHint = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterMovementLimitedFallbackHint = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterPhysicalReactionAvailable = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    EWanaPhysicalState RuntimeAdapterPhysicalState = EWanaPhysicalState::Stable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    float RuntimeAdapterStabilityScore = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    float RuntimeAdapterInstabilityAlpha = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    float RuntimeAdapterRecoveryProgress = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FVector RuntimeAdapterImpactDirection = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    float RuntimeAdapterImpactStrength = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterBracing = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterNeedsRecovery = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterCanCommitToMovement = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bRuntimeAdapterCanCommitToAttack = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RuntimeAdapterDetail;

private:
    UWanaAnimationAdapterReportAsset* ResolvePersistentAdapterReportAsset() const;
    USkeletalMeshComponent* ResolveIntegrationMesh() const;
    UAnimInstance* ResolveLiveAnimInstance(USkeletalMeshComponent*& OutMeshComponent, UClass*& OutAnimClass) const;
    void ApplyProceduralVisibleReaction(float DeltaTime);
    void RestoreVisibleReactionBaseTransform();

    TWeakObjectPtr<USkeletalMeshComponent> VisibleReactionMesh;
    bool bVisibleReactionBaseCaptured = false;
    FQuat VisibleReactionBaseRelativeQuat = FQuat::Identity;
    FVector VisibleReactionBaseRelativeLocation = FVector::ZeroVector;
    FQuat CurrentVisibleReactionOffsetQuat = FQuat::Identity;
    float CurrentVisibleCrouchOffset = 0.0f;
    float VisibleReactionTimeSeconds = 0.0f;
};

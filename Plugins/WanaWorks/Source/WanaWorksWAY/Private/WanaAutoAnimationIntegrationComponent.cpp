#include "WanaAutoAnimationIntegrationComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Misc/PackageName.h"
#include "UObject/UnrealType.h"
#include "WAYPlayerProfileComponent.h"
#include "WanaAnimationAdapterReportAsset.h"
#include "WanaPhysicalStateComponent.h"

namespace
{
template <typename PredicateType>
FProperty* FindMatchingPropertyByAliases(const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases, PredicateType&& Predicate)
{
    if (!OwnerClass)
    {
        return nullptr;
    }

    for (const TCHAR* Alias : Aliases)
    {
        if (!Alias)
        {
            continue;
        }

        if (FProperty* Property = FindFProperty<FProperty>(OwnerClass, FName(Alias)))
        {
            if (Predicate(Property))
            {
                return Property;
            }
        }
    }

    return nullptr;
}

bool HasBoolPropertyAlias(const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases)
{
    return FindMatchingPropertyByAliases(
        OwnerClass,
        Aliases,
        [](FProperty* Property)
        {
            return CastField<FBoolProperty>(Property) != nullptr;
        }) != nullptr;
}

bool HasFloatPropertyAlias(const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases)
{
    return FindMatchingPropertyByAliases(
        OwnerClass,
        Aliases,
        [](FProperty* Property)
        {
            return CastField<FFloatProperty>(Property) != nullptr || CastField<FDoubleProperty>(Property) != nullptr;
        }) != nullptr;
}

bool HasStringPropertyAlias(const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases)
{
    return FindMatchingPropertyByAliases(
        OwnerClass,
        Aliases,
        [](FProperty* Property)
        {
            return CastField<FStrProperty>(Property) != nullptr;
        }) != nullptr;
}

bool HasStructPropertyAlias(const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases, const UScriptStruct* ExpectedStruct)
{
    return FindMatchingPropertyByAliases(
        OwnerClass,
        Aliases,
        [ExpectedStruct](FProperty* Property)
        {
            const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
            return StructProperty && StructProperty->Struct == ExpectedStruct;
        }) != nullptr;
}

bool HasObjectPropertyAlias(const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases, const UClass* ExpectedClass)
{
    return FindMatchingPropertyByAliases(
        OwnerClass,
        Aliases,
        [ExpectedClass](FProperty* Property)
        {
            const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
            return ObjectProperty && ObjectProperty->PropertyClass && ObjectProperty->PropertyClass->IsChildOf(ExpectedClass);
        }) != nullptr;
}

bool HasEnumPropertyAlias(const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases)
{
    return FindMatchingPropertyByAliases(
        OwnerClass,
        Aliases,
        [](FProperty* Property)
        {
            return CastField<FEnumProperty>(Property) != nullptr || CastField<FByteProperty>(Property) != nullptr;
        }) != nullptr;
}

bool TrySetBoolPropertyAlias(UObject* Object, const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases, bool bValue)
{
    if (!Object)
    {
        return false;
    }

    if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(FindMatchingPropertyByAliases(
            OwnerClass,
            Aliases,
            [](FProperty* Property)
            {
                return CastField<FBoolProperty>(Property) != nullptr;
            })))
    {
        BoolProperty->SetPropertyValue_InContainer(Object, bValue);
        return true;
    }

    return false;
}

bool TrySetFloatPropertyAlias(UObject* Object, const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases, float Value)
{
    if (!Object)
    {
        return false;
    }

    if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(FindMatchingPropertyByAliases(
            OwnerClass,
            Aliases,
            [](FProperty* Property)
            {
                return CastField<FFloatProperty>(Property) != nullptr;
            })))
    {
        FloatProperty->SetPropertyValue_InContainer(Object, Value);
        return true;
    }

    if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(FindMatchingPropertyByAliases(
            OwnerClass,
            Aliases,
            [](FProperty* Property)
            {
                return CastField<FDoubleProperty>(Property) != nullptr;
            })))
    {
        DoubleProperty->SetPropertyValue_InContainer(Object, static_cast<double>(Value));
        return true;
    }

    return false;
}

bool TrySetStringPropertyAlias(UObject* Object, const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases, const FString& Value)
{
    if (!Object)
    {
        return false;
    }

    if (FStrProperty* StringProperty = CastField<FStrProperty>(FindMatchingPropertyByAliases(
            OwnerClass,
            Aliases,
            [](FProperty* Property)
            {
                return CastField<FStrProperty>(Property) != nullptr;
            })))
    {
        StringProperty->SetPropertyValue_InContainer(Object, Value);
        return true;
    }

    return false;
}

template <typename StructType>
bool TrySetStructPropertyAlias(
    UObject* Object,
    const UClass* OwnerClass,
    std::initializer_list<const TCHAR*> Aliases,
    const UScriptStruct* ExpectedStruct,
    const StructType& Value)
{
    if (!Object)
    {
        return false;
    }

    if (FStructProperty* StructProperty = CastField<FStructProperty>(FindMatchingPropertyByAliases(
            OwnerClass,
            Aliases,
            [ExpectedStruct](FProperty* Property)
            {
                const FStructProperty* TypedProperty = CastField<FStructProperty>(Property);
                return TypedProperty && TypedProperty->Struct == ExpectedStruct;
            })))
    {
        *StructProperty->ContainerPtrToValuePtr<StructType>(Object) = Value;
        return true;
    }

    return false;
}

bool TrySetObjectPropertyAlias(
    UObject* Object,
    const UClass* OwnerClass,
    std::initializer_list<const TCHAR*> Aliases,
    const UClass* ExpectedClass,
    UObject* Value)
{
    if (!Object)
    {
        return false;
    }

    if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(FindMatchingPropertyByAliases(
            OwnerClass,
            Aliases,
            [ExpectedClass](FProperty* Property)
            {
                const FObjectPropertyBase* TypedProperty = CastField<FObjectPropertyBase>(Property);
                return TypedProperty && TypedProperty->PropertyClass && TypedProperty->PropertyClass->IsChildOf(ExpectedClass);
            })))
    {
        ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
        return true;
    }

    return false;
}

bool TrySetEnumPropertyAlias(UObject* Object, const UClass* OwnerClass, std::initializer_list<const TCHAR*> Aliases, int64 Value)
{
    if (!Object)
    {
        return false;
    }

    if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(FindMatchingPropertyByAliases(
            OwnerClass,
            Aliases,
            [](FProperty* Property)
            {
                return CastField<FEnumProperty>(Property) != nullptr;
            })))
    {
        if (FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
        {
            void* ValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(Object);
            UnderlyingProperty->SetIntPropertyValue(ValuePtr, Value);
            return true;
        }
    }

    if (FByteProperty* ByteProperty = CastField<FByteProperty>(FindMatchingPropertyByAliases(
            OwnerClass,
            Aliases,
            [](FProperty* Property)
            {
                return CastField<FByteProperty>(Property) != nullptr;
            })))
    {
        ByteProperty->SetPropertyValue_InContainer(Object, static_cast<uint8>(Value));
        return true;
    }

    return false;
}

int32 CountSupportedAutomaticIntegrationFields(const UClass* AnimClass)
{
    if (!AnimClass)
    {
        return 0;
    }

    int32 SupportedFieldCount = 0;
    SupportedFieldCount += HasStructPropertyAlias(AnimClass, { TEXT("WanaAnimationHookState"), TEXT("AnimationHookState") }, FWAYAnimationHookState::StaticStruct()) ? 1 : 0;
    SupportedFieldCount += HasObjectPropertyAlias(AnimClass, { TEXT("WanaRuntimeAnimationAdapter"), TEXT("WanaAnimationAdapterComponent"), TEXT("WanaAutoAnimationIntegrationComponent") }, UWanaAutoAnimationIntegrationComponent::StaticClass()) ? 1 : 0;
    SupportedFieldCount += HasObjectPropertyAlias(AnimClass, { TEXT("WanaWAYComponent"), TEXT("WanaProfileComponent"), TEXT("WAYPlayerProfileComponent") }, UWAYPlayerProfileComponent::StaticClass()) ? 1 : 0;
    SupportedFieldCount += HasObjectPropertyAlias(AnimClass, { TEXT("WanaPhysicalStateComponent"), TEXT("PhysicalStateComponent") }, UWanaPhysicalStateComponent::StaticClass()) ? 1 : 0;
    SupportedFieldCount += HasBoolPropertyAlias(AnimClass, { TEXT("bWanaFacingHookRequested"), TEXT("bFacingHookRequested") }) ? 1 : 0;
    SupportedFieldCount += HasBoolPropertyAlias(AnimClass, { TEXT("bWanaTurnToTargetRequested"), TEXT("bTurnToTargetRequested") }) ? 1 : 0;
    SupportedFieldCount += HasEnumPropertyAlias(AnimClass, { TEXT("WanaReactionState"), TEXT("ReactionState") }) ? 1 : 0;
    SupportedFieldCount += HasEnumPropertyAlias(AnimClass, { TEXT("WanaRecommendedBehavior"), TEXT("RecommendedBehavior") }) ? 1 : 0;
    SupportedFieldCount += HasStringPropertyAlias(AnimClass, { TEXT("WanaBehaviorIntent"), TEXT("BehaviorIntent") }) ? 1 : 0;
    SupportedFieldCount += HasStringPropertyAlias(AnimClass, { TEXT("WanaVisibleBehaviorLabel"), TEXT("VisibleBehaviorLabel") }) ? 1 : 0;
    SupportedFieldCount += HasStringPropertyAlias(AnimClass, { TEXT("WanaPostureHint"), TEXT("PostureHint") }) ? 1 : 0;
    SupportedFieldCount += HasStringPropertyAlias(AnimClass, { TEXT("WanaPostureCategory"), TEXT("PostureCategory") }) ? 1 : 0;
    SupportedFieldCount += HasStringPropertyAlias(AnimClass, { TEXT("WanaFallbackHint"), TEXT("FallbackHint") }) ? 1 : 0;
    SupportedFieldCount += HasStringPropertyAlias(AnimClass, { TEXT("WanaAdapterReadinessState"), TEXT("AdapterReadinessState") }) ? 1 : 0;
    SupportedFieldCount += HasStringPropertyAlias(AnimClass, { TEXT("WanaAdapterRecommendedStrategy"), TEXT("AdapterRecommendedStrategy") }) ? 1 : 0;
    SupportedFieldCount += HasBoolPropertyAlias(AnimClass, { TEXT("bWanaLocomotionSafeExecutionHint"), TEXT("bLocomotionSafeExecutionHint") }) ? 1 : 0;
    SupportedFieldCount += HasEnumPropertyAlias(AnimClass, { TEXT("WanaPhysicalState"), TEXT("PhysicalState") }) ? 1 : 0;
    SupportedFieldCount += HasFloatPropertyAlias(AnimClass, { TEXT("WanaStabilityScore"), TEXT("StabilityScore") }) ? 1 : 0;
    SupportedFieldCount += HasFloatPropertyAlias(AnimClass, { TEXT("WanaRecoveryProgress"), TEXT("RecoveryProgress") }) ? 1 : 0;
    SupportedFieldCount += HasFloatPropertyAlias(AnimClass, { TEXT("WanaInstabilityAlpha"), TEXT("InstabilityAlpha") }) ? 1 : 0;
    SupportedFieldCount += HasStructPropertyAlias(AnimClass, { TEXT("WanaLastImpactDirection"), TEXT("LastImpactDirection") }, TBaseStructure<FVector>::Get()) ? 1 : 0;
    SupportedFieldCount += HasFloatPropertyAlias(AnimClass, { TEXT("WanaLastImpactStrength"), TEXT("LastImpactStrength") }) ? 1 : 0;
    SupportedFieldCount += HasBoolPropertyAlias(AnimClass, { TEXT("bWanaBracing"), TEXT("bBracing") }) ? 1 : 0;
    SupportedFieldCount += HasBoolPropertyAlias(AnimClass, { TEXT("bWanaNeedsRecovery"), TEXT("bNeedsRecovery") }) ? 1 : 0;
    SupportedFieldCount += HasBoolPropertyAlias(AnimClass, { TEXT("bWanaCanCommitToMovement"), TEXT("bCanCommitToMovement") }) ? 1 : 0;
    SupportedFieldCount += HasBoolPropertyAlias(AnimClass, { TEXT("bWanaCanCommitToAttack"), TEXT("bCanCommitToAttack") }) ? 1 : 0;
    return SupportedFieldCount;
}

int32 ApplyAutomaticIntegrationToAnimInstance(
    UAnimInstance* AnimInstance,
    const UClass* AnimClass,
    UWanaAutoAnimationIntegrationComponent* AdapterComponent,
    UWAYPlayerProfileComponent* WAYComponent,
    UWanaPhysicalStateComponent* PhysicalStateComponent)
{
    if (!AnimInstance || !AnimClass)
    {
        return 0;
    }

    const FWAYAnimationHookState HookState = WAYComponent ? WAYComponent->GetCurrentAnimationHookState() : FWAYAnimationHookState();
    const EWanaPhysicalState PhysicalState = PhysicalStateComponent ? PhysicalStateComponent->PhysicalState : EWanaPhysicalState::Stable;
    const float StabilityScore = PhysicalStateComponent ? PhysicalStateComponent->StabilityScore : 1.0f;
    const float RecoveryProgress = PhysicalStateComponent ? PhysicalStateComponent->RecoveryProgress : 1.0f;
    const float InstabilityAlpha = PhysicalStateComponent ? PhysicalStateComponent->InstabilityAlpha : 0.0f;
    const FVector LastImpactDirection = PhysicalStateComponent ? PhysicalStateComponent->LastImpactDirection : FVector::ZeroVector;
    const float LastImpactStrength = PhysicalStateComponent ? PhysicalStateComponent->LastImpactStrength : 0.0f;
    const bool bBracing = PhysicalStateComponent ? PhysicalStateComponent->bBracing : false;
    const bool bNeedsRecovery = PhysicalStateComponent ? PhysicalStateComponent->bNeedsRecovery : false;
    const bool bCanCommitToMovement = PhysicalStateComponent ? PhysicalStateComponent->bCanCommitToMovement : true;
    const bool bCanCommitToAttack = PhysicalStateComponent ? PhysicalStateComponent->bCanCommitToAttack : true;

    int32 AppliedFieldCount = 0;
    AppliedFieldCount += TrySetStructPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaAnimationHookState"), TEXT("AnimationHookState") }, FWAYAnimationHookState::StaticStruct(), HookState) ? 1 : 0;
    AppliedFieldCount += TrySetObjectPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaRuntimeAnimationAdapter"), TEXT("WanaAnimationAdapterComponent"), TEXT("WanaAutoAnimationIntegrationComponent") }, UWanaAutoAnimationIntegrationComponent::StaticClass(), AdapterComponent) ? 1 : 0;
    AppliedFieldCount += TrySetObjectPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaWAYComponent"), TEXT("WanaProfileComponent"), TEXT("WAYPlayerProfileComponent") }, UWAYPlayerProfileComponent::StaticClass(), WAYComponent) ? 1 : 0;
    AppliedFieldCount += TrySetObjectPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaPhysicalStateComponent"), TEXT("PhysicalStateComponent") }, UWanaPhysicalStateComponent::StaticClass(), PhysicalStateComponent) ? 1 : 0;
    AppliedFieldCount += TrySetBoolPropertyAlias(AnimInstance, AnimClass, { TEXT("bWanaFacingHookRequested"), TEXT("bFacingHookRequested") }, HookState.bFacingHookRequested) ? 1 : 0;
    AppliedFieldCount += TrySetBoolPropertyAlias(AnimInstance, AnimClass, { TEXT("bWanaTurnToTargetRequested"), TEXT("bTurnToTargetRequested") }, HookState.bTurnToTargetRequested) ? 1 : 0;
    AppliedFieldCount += TrySetEnumPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaReactionState"), TEXT("ReactionState") }, static_cast<int64>(HookState.ReactionState)) ? 1 : 0;
    AppliedFieldCount += TrySetEnumPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaRecommendedBehavior"), TEXT("RecommendedBehavior") }, static_cast<int64>(HookState.RecommendedBehavior)) ? 1 : 0;
    AppliedFieldCount += TrySetStringPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaBehaviorIntent"), TEXT("BehaviorIntent") }, HookState.BehaviorIntent) ? 1 : 0;
    AppliedFieldCount += TrySetStringPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaVisibleBehaviorLabel"), TEXT("VisibleBehaviorLabel") }, HookState.VisibleBehaviorLabel) ? 1 : 0;
    AppliedFieldCount += TrySetStringPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaPostureHint"), TEXT("PostureHint") }, HookState.PostureHint) ? 1 : 0;
    AppliedFieldCount += TrySetStringPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaPostureCategory"), TEXT("PostureCategory") }, HookState.PostureCategory) ? 1 : 0;
    AppliedFieldCount += TrySetStringPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaFallbackHint"), TEXT("FallbackHint") }, HookState.FallbackHint) ? 1 : 0;
    AppliedFieldCount += TrySetStringPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaAdapterReadinessState"), TEXT("AdapterReadinessState") }, AdapterComponent ? AdapterComponent->RuntimeAdapterReadinessState : FString(TEXT("Limited"))) ? 1 : 0;
    AppliedFieldCount += TrySetStringPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaAdapterRecommendedStrategy"), TEXT("AdapterRecommendedStrategy") }, AdapterComponent ? AdapterComponent->RuntimeAdapterRecommendedStrategy : FString(TEXT("Needs Enhance"))) ? 1 : 0;
    AppliedFieldCount += TrySetBoolPropertyAlias(AnimInstance, AnimClass, { TEXT("bWanaLocomotionSafeExecutionHint"), TEXT("bLocomotionSafeExecutionHint") }, HookState.bLocomotionSafeExecutionHint) ? 1 : 0;
    AppliedFieldCount += TrySetEnumPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaPhysicalState"), TEXT("PhysicalState") }, static_cast<int64>(PhysicalState)) ? 1 : 0;
    AppliedFieldCount += TrySetFloatPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaStabilityScore"), TEXT("StabilityScore") }, StabilityScore) ? 1 : 0;
    AppliedFieldCount += TrySetFloatPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaRecoveryProgress"), TEXT("RecoveryProgress") }, RecoveryProgress) ? 1 : 0;
    AppliedFieldCount += TrySetFloatPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaInstabilityAlpha"), TEXT("InstabilityAlpha") }, InstabilityAlpha) ? 1 : 0;
    AppliedFieldCount += TrySetStructPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaLastImpactDirection"), TEXT("LastImpactDirection") }, TBaseStructure<FVector>::Get(), LastImpactDirection) ? 1 : 0;
    AppliedFieldCount += TrySetFloatPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaLastImpactStrength"), TEXT("LastImpactStrength") }, LastImpactStrength) ? 1 : 0;
    AppliedFieldCount += TrySetBoolPropertyAlias(AnimInstance, AnimClass, { TEXT("bWanaBracing"), TEXT("bBracing") }, bBracing) ? 1 : 0;
    AppliedFieldCount += TrySetBoolPropertyAlias(AnimInstance, AnimClass, { TEXT("bWanaNeedsRecovery"), TEXT("bNeedsRecovery") }, bNeedsRecovery) ? 1 : 0;
    AppliedFieldCount += TrySetBoolPropertyAlias(AnimInstance, AnimClass, { TEXT("bWanaCanCommitToMovement"), TEXT("bCanCommitToMovement") }, bCanCommitToMovement) ? 1 : 0;
    AppliedFieldCount += TrySetBoolPropertyAlias(AnimInstance, AnimClass, { TEXT("bWanaCanCommitToAttack"), TEXT("bCanCommitToAttack") }, bCanCommitToAttack) ? 1 : 0;
    return AppliedFieldCount;
}

FString BuildAdapterReportObjectPath(const FString& ReportPath)
{
    if (ReportPath.IsEmpty())
    {
        return FString();
    }

    if (ReportPath.Contains(TEXT(".")))
    {
        return ReportPath;
    }

    FString AssetName = FPackageName::GetLongPackageAssetName(ReportPath);
    if (AssetName.IsEmpty())
    {
        int32 LastSlashIndex = INDEX_NONE;
        ReportPath.FindLastChar(TEXT('/'), LastSlashIndex);
        AssetName = LastSlashIndex == INDEX_NONE ? ReportPath : ReportPath.RightChop(LastSlashIndex + 1);
    }

    return FString::Printf(TEXT("%s.%s"), *ReportPath, *AssetName);
}
}

UWanaAutoAnimationIntegrationComponent::UWanaAutoAnimationIntegrationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

FWAYAnimationHookState UWanaAutoAnimationIntegrationComponent::GetAdapterAnimationHookState() const
{
    const AActor* OwnerActor = GetOwner();
    const UWAYPlayerProfileComponent* WAYComponent = OwnerActor
        ? OwnerActor->FindComponentByClass<UWAYPlayerProfileComponent>()
        : nullptr;

    if (WAYComponent)
    {
        return WAYComponent->GetCurrentAnimationHookState();
    }

    FWAYAnimationHookState EmptyState;
    EmptyState.Detail = TEXT("WanaAnimation adapter hook state is limited because no WAY hook provider was found on the owner.");
    return EmptyState;
}

FString UWanaAutoAnimationIntegrationComponent::GetAdapterPostureHint() const
{
    return GetAdapterAnimationHookState().PostureHint;
}

EWAYReactionState UWanaAutoAnimationIntegrationComponent::GetAdapterReactionState() const
{
    return GetAdapterAnimationHookState().ReactionState;
}

EWAYBehaviorPreset UWanaAutoAnimationIntegrationComponent::GetAdapterRecommendedBehavior() const
{
    return GetAdapterAnimationHookState().RecommendedBehavior;
}

FString UWanaAutoAnimationIntegrationComponent::GetAdapterVisibleBehaviorLabel() const
{
    return GetAdapterAnimationHookState().VisibleBehaviorLabel;
}

bool UWanaAutoAnimationIntegrationComponent::IsAdapterLocomotionSafeHintActive() const
{
    return GetAdapterAnimationHookState().bLocomotionSafeExecutionHint;
}

bool UWanaAutoAnimationIntegrationComponent::IsAdapterMovementLimitedFallbackActive() const
{
    return GetAdapterAnimationHookState().bMovementLimitedFallbackHint;
}

float UWanaAutoAnimationIntegrationComponent::GetAdapterInstabilityAlpha() const
{
    const FWAYAnimationHookState HookState = GetAdapterAnimationHookState();

    if (HookState.bPhysicalReactionStateAvailable)
    {
        return HookState.PhysicalInstabilityAlpha;
    }

    const AActor* OwnerActor = GetOwner();
    const UWanaPhysicalStateComponent* PhysicalStateComponent = OwnerActor
        ? OwnerActor->FindComponentByClass<UWanaPhysicalStateComponent>()
        : nullptr;
    return PhysicalStateComponent ? PhysicalStateComponent->InstabilityAlpha : 0.0f;
}

float UWanaAutoAnimationIntegrationComponent::GetAdapterRecoveryProgress() const
{
    const FWAYAnimationHookState HookState = GetAdapterAnimationHookState();

    if (HookState.bPhysicalReactionStateAvailable)
    {
        return HookState.PhysicalRecoveryProgress;
    }

    const AActor* OwnerActor = GetOwner();
    const UWanaPhysicalStateComponent* PhysicalStateComponent = OwnerActor
        ? OwnerActor->FindComponentByClass<UWanaPhysicalStateComponent>()
        : nullptr;
    return PhysicalStateComponent ? PhysicalStateComponent->RecoveryProgress : 1.0f;
}

FVector UWanaAutoAnimationIntegrationComponent::GetAdapterImpactDirection() const
{
    const FWAYAnimationHookState HookState = GetAdapterAnimationHookState();

    if (HookState.bPhysicalReactionStateAvailable)
    {
        return HookState.PhysicalImpactDirection;
    }

    const AActor* OwnerActor = GetOwner();
    const UWanaPhysicalStateComponent* PhysicalStateComponent = OwnerActor
        ? OwnerActor->FindComponentByClass<UWanaPhysicalStateComponent>()
        : nullptr;
    return PhysicalStateComponent ? PhysicalStateComponent->LastImpactDirection : FVector::ZeroVector;
}

void UWanaAutoAnimationIntegrationComponent::SetPersistentAdapterReportAsset(UWanaAnimationAdapterReportAsset* InReportAsset)
{
    PersistentAdapterReport = InReportAsset;
    PersistentAdapterReportPath = InReportAsset
        ? (InReportAsset->AdapterPackagePath.IsEmpty() ? InReportAsset->GetPathName() : InReportAsset->AdapterPackagePath)
        : FString();
    RefreshRuntimeAnimationAdapter();
}

void UWanaAutoAnimationIntegrationComponent::SetPersistentAdapterReportPath(const FString& InReportPath)
{
    PersistentAdapterReportPath = InReportPath;
    RefreshRuntimeAnimationAdapter();
}

UWanaAnimationAdapterReportAsset* UWanaAutoAnimationIntegrationComponent::ResolvePersistentAdapterReportAsset() const
{
    if (UWanaAnimationAdapterReportAsset* LoadedReport = PersistentAdapterReport.Get())
    {
        return LoadedReport;
    }

    if (!PersistentAdapterReport.IsNull())
    {
        if (UWanaAnimationAdapterReportAsset* LoadedReport = PersistentAdapterReport.LoadSynchronous())
        {
            return LoadedReport;
        }
    }

    const FString ObjectPath = BuildAdapterReportObjectPath(PersistentAdapterReportPath);
    if (ObjectPath.IsEmpty())
    {
        return nullptr;
    }

    return Cast<UWanaAnimationAdapterReportAsset>(
        StaticLoadObject(
            UWanaAnimationAdapterReportAsset::StaticClass(),
            nullptr,
            *ObjectPath));
}

void UWanaAutoAnimationIntegrationComponent::RefreshRuntimeAnimationAdapter()
{
    bRuntimeAdapterHookReadable = false;
    bRuntimeAdapterReportReadable = false;
    bRuntimeAdapterSharedAnimBPDetected = false;
    bRuntimeAdapterDirectGraphEditSafe = false;
    bRuntimeAdapterFacingHookRequested = false;
    bRuntimeAdapterTurnToTargetRequested = false;
    bRuntimeAdapterLocomotionSafeExecutionHint = false;
    bRuntimeAdapterMovementLimitedFallbackHint = false;
    RuntimeAdapterBehaviorIntent = FString();
    RuntimeAdapterVisibleBehaviorLabel = FString();
    RuntimeAdapterPostureHint = TEXT("observant");
    RuntimeAdapterFallbackHint = TEXT("stable observe stance");
    RuntimeAdapterIdentityRole = TEXT("neutral");
    RuntimeAdapterReactionState = EWAYReactionState::Observational;
    RuntimeAdapterRelationshipState = EWAYRelationshipState::Neutral;
    RuntimeAdapterRecommendedBehavior = EWAYBehaviorPreset::None;
    RuntimeAdapterInstabilityAlpha = 0.0f;
    RuntimeAdapterRecoveryProgress = 1.0f;
    RuntimeAdapterImpactDirection = FVector::ZeroVector;
    RuntimeAdapterDirectGraphEditSafety = TEXT("Limited");
    RuntimeAdapterSharedAnimBPRisk = TEXT("Unknown");
    RuntimeAdapterRecommendedStrategy = TEXT("Needs Enhance");
    RuntimeAdapterReadinessState = TEXT("Not Supported");
    RuntimeAdapterDetail = TEXT("Runtime WanaAnimation adapter is limited because no valid owner was found.");

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    USkeletalMeshComponent* MeshComponent = ResolveIntegrationMesh();
    UClass* AnimClass = MeshComponent ? MeshComponent->GetAnimClass() : nullptr;
    if (!AnimClass && MeshComponent && MeshComponent->GetAnimInstance())
    {
        AnimClass = MeshComponent->GetAnimInstance()->GetClass();
    }

    const UWAYPlayerProfileComponent* WAYComponent = OwnerActor->FindComponentByClass<UWAYPlayerProfileComponent>();
    const UWanaPhysicalStateComponent* PhysicalStateComponent = OwnerActor->FindComponentByClass<UWanaPhysicalStateComponent>();
    const FWAYAnimationHookState HookState = WAYComponent ? WAYComponent->GetCurrentAnimationHookState() : FWAYAnimationHookState();

    bRuntimeAdapterHookReadable = WAYComponent != nullptr;
    bRuntimeAdapterFacingHookRequested = HookState.bFacingHookRequested;
    bRuntimeAdapterTurnToTargetRequested = HookState.bTurnToTargetRequested;
    bRuntimeAdapterLocomotionSafeExecutionHint = HookState.bLocomotionSafeExecutionHint;
    bRuntimeAdapterMovementLimitedFallbackHint = HookState.bMovementLimitedFallbackHint;
    RuntimeAdapterBehaviorIntent = HookState.BehaviorIntent;
    RuntimeAdapterVisibleBehaviorLabel = HookState.VisibleBehaviorLabel;
    RuntimeAdapterPostureHint = HookState.PostureHint.IsEmpty() ? RuntimeAdapterPostureHint : HookState.PostureHint;
    RuntimeAdapterFallbackHint = HookState.FallbackHint.IsEmpty() ? RuntimeAdapterFallbackHint : HookState.FallbackHint;
    RuntimeAdapterIdentityRole = HookState.IdentityRoleHint.IsEmpty() ? RuntimeAdapterIdentityRole : HookState.IdentityRoleHint;
    RuntimeAdapterReactionState = HookState.ReactionState;
    RuntimeAdapterRelationshipState = HookState.RelationshipState;
    RuntimeAdapterRecommendedBehavior = HookState.RecommendedBehavior;

    if (HookState.bPhysicalReactionStateAvailable)
    {
        RuntimeAdapterInstabilityAlpha = HookState.PhysicalInstabilityAlpha;
        RuntimeAdapterRecoveryProgress = HookState.PhysicalRecoveryProgress;
        RuntimeAdapterImpactDirection = HookState.PhysicalImpactDirection;
    }
    else if (PhysicalStateComponent)
    {
        RuntimeAdapterInstabilityAlpha = PhysicalStateComponent->InstabilityAlpha;
        RuntimeAdapterRecoveryProgress = PhysicalStateComponent->RecoveryProgress;
        RuntimeAdapterImpactDirection = PhysicalStateComponent->LastImpactDirection;
    }

    if (UWanaAnimationAdapterReportAsset* ReportAsset = ResolvePersistentAdapterReportAsset())
    {
        bRuntimeAdapterReportReadable = true;
        if (PersistentAdapterReportPath.IsEmpty())
        {
            PersistentAdapterReportPath = ReportAsset->AdapterPackagePath.IsEmpty()
                ? ReportAsset->GetPathName()
                : ReportAsset->AdapterPackagePath;
        }

        RuntimeAdapterReadinessState = ReportAsset->AdapterReadinessState.IsEmpty()
            ? ReportAsset->AdapterStatus
            : ReportAsset->AdapterReadinessState;
        RuntimeAdapterRecommendedStrategy = ReportAsset->RecommendedIntegrationStrategy.IsEmpty()
            ? TEXT("Generated Adapter")
            : ReportAsset->RecommendedIntegrationStrategy;
        RuntimeAdapterDirectGraphEditSafety = ReportAsset->DirectGraphEditSafety.IsEmpty()
            ? TEXT("Limited")
            : ReportAsset->DirectGraphEditSafety;
        RuntimeAdapterSharedAnimBPRisk = ReportAsset->SharedAnimBlueprintStatus.IsEmpty()
            ? TEXT("Unknown")
            : ReportAsset->SharedAnimBlueprintStatus;
    }
    else if (!MeshComponent)
    {
        RuntimeAdapterReadinessState = TEXT("Not Supported");
        RuntimeAdapterRecommendedStrategy = TEXT("Not Supported");
    }
    else if (!AnimClass)
    {
        RuntimeAdapterReadinessState = TEXT("Limited");
        RuntimeAdapterRecommendedStrategy = TEXT("Needs Anim BP");
    }
    else if (!WAYComponent)
    {
        RuntimeAdapterReadinessState = TEXT("Needs Enhance");
        RuntimeAdapterRecommendedStrategy = TEXT("Needs Enhance");
    }
    else
    {
        RuntimeAdapterReadinessState = SupportedFieldCount > 0 ? TEXT("Ready") : TEXT("Adapter Recommended");
        RuntimeAdapterRecommendedStrategy = SupportedFieldCount > 0 ? TEXT("Runtime Adapter / Existing Fields") : TEXT("Generated Adapter / Manual Review");
    }

    bRuntimeAdapterSharedAnimBPDetected = RuntimeAdapterSharedAnimBPRisk.Equals(TEXT("Yes"), ESearchCase::IgnoreCase)
        || RuntimeAdapterDirectGraphEditSafety.Equals(TEXT("Unsafe"), ESearchCase::IgnoreCase);
    bRuntimeAdapterDirectGraphEditSafe = RuntimeAdapterDirectGraphEditSafety.Equals(TEXT("Safe"), ESearchCase::IgnoreCase);

    RuntimeAdapterDetail = FString::Printf(
        TEXT("Runtime WanaAnimation adapter: %s. Hook Readable: %s. Persistent Report: %s. Posture: %s. Reaction: %s. Facing Hook: %s. Turn-To-Target: %s. Locomotion: %s. Movement Fallback: %s. Report Path: %s. Original Anim BP remains untouched."),
        RuntimeAdapterReadinessState.IsEmpty() ? TEXT("Limited") : *RuntimeAdapterReadinessState,
        bRuntimeAdapterHookReadable ? TEXT("Ready") : TEXT("Needs Enhance"),
        bRuntimeAdapterReportReadable ? TEXT("Readable") : TEXT("Not Found"),
        RuntimeAdapterPostureHint.IsEmpty() ? TEXT("(pending)") : *RuntimeAdapterPostureHint,
        *UEnum::GetValueAsString(RuntimeAdapterReactionState),
        bRuntimeAdapterFacingHookRequested ? TEXT("Requested") : TEXT("Not Requested"),
        bRuntimeAdapterTurnToTargetRequested ? TEXT("Requested") : TEXT("Not Requested"),
        bRuntimeAdapterLocomotionSafeExecutionHint ? TEXT("Safe") : TEXT("Limited"),
        bRuntimeAdapterMovementLimitedFallbackHint ? TEXT("Active") : TEXT("Inactive"),
        PersistentAdapterReportPath.IsEmpty() ? TEXT("(not linked)") : *PersistentAdapterReportPath);
}

void UWanaAutoAnimationIntegrationComponent::BeginPlay()
{
    Super::BeginPlay();
    RefreshAutomaticAnimationIntegration();
}

void UWanaAutoAnimationIntegrationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    RefreshAutomaticAnimationIntegration();
}

void UWanaAutoAnimationIntegrationComponent::RefreshAutomaticAnimationIntegration()
{
    bAutoAttachSucceeded = GetOwner() != nullptr;
    bAutoWireSucceeded = false;
    SupportedFieldCount = 0;
    LastAppliedFieldCount = 0;
    SourceAnimationBlueprintLabel = TEXT("(not detected)");
    IntegrationTargetLabel = TEXT("(not attached)");
    Detail = TEXT("Automatic sandbox animation integration has not been prepared yet.");
    AutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::NotSupported;

    USkeletalMeshComponent* MeshComponent = nullptr;
    UClass* AnimClass = nullptr;
    UAnimInstance* AnimInstance = ResolveLiveAnimInstance(MeshComponent, AnimClass);

    if (!MeshComponent)
    {
        Detail = TEXT("Automatic Anim BP integration is not supported because this subject has no skeletal animation stack.");
        RefreshRuntimeAnimationAdapter();
        return;
    }

    if (AnimClass)
    {
        SourceAnimationBlueprintLabel = AnimClass->GetName();
    }

    IntegrationTargetLabel = FString::Printf(
        TEXT("%s -> %s"),
        *MeshComponent->GetName(),
        AnimClass ? *AnimClass->GetName() : TEXT("(no Animation Blueprint detected)"));

    if (!AnimClass)
    {
        Detail = TEXT("Automatic Anim BP integration is not supported because no linked Animation Blueprint or live AnimInstance class was detected on the sandbox subject.");
        RefreshRuntimeAnimationAdapter();
        return;
    }

    SupportedFieldCount = CountSupportedAutomaticIntegrationFields(AnimClass);
    RefreshRuntimeAnimationAdapter();

    if (SupportedFieldCount <= 0)
    {
        AutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Limited;
        Detail = TEXT("Automatic Anim BP integration is limited because the detected Animation Blueprint does not expose the supported WanaWorks auto-wire fields or component references in this first pass.");
        RefreshRuntimeAnimationAdapter();
        return;
    }

    UWAYPlayerProfileComponent* WAYComponent = GetOwner()->FindComponentByClass<UWAYPlayerProfileComponent>();
    UWanaPhysicalStateComponent* PhysicalStateComponent = GetOwner()->FindComponentByClass<UWanaPhysicalStateComponent>();
    const FString WAYSourceLabel = WAYComponent ? TEXT("Present") : TEXT("Missing");
    const FString PhysicalSourceLabel = PhysicalStateComponent ? TEXT("Present") : TEXT("Missing");

    if (!AnimInstance)
    {
        AutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Ready;
        Detail = FString::Printf(
            TEXT("Automatic Anim BP integration is ready for %d supported field(s). WAY source: %s. Physical source: %s. It will auto-wire when a live AnimInstance is active on the sandbox or finalized subject."),
            SupportedFieldCount,
            *WAYSourceLabel,
            *PhysicalSourceLabel);
        RefreshRuntimeAnimationAdapter();
        return;
    }

    LastAppliedFieldCount = ApplyAutomaticIntegrationToAnimInstance(AnimInstance, AnimClass, this, WAYComponent, PhysicalStateComponent);

    if (LastAppliedFieldCount > 0)
    {
        bAutoWireSucceeded = true;
        AutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Applied;
        Detail = FString::Printf(
            TEXT("Automatic Anim BP integration applied %d supported field(s). WAY source: %s. Physical source: %s. The original Animation Blueprint stays preserved while the sandbox subject receives WanaWorks hook data automatically."),
            LastAppliedFieldCount,
            *WAYSourceLabel,
            *PhysicalSourceLabel);
        RefreshRuntimeAnimationAdapter();
        return;
    }

    AutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Ready;
    Detail = FString::Printf(
        TEXT("Automatic Anim BP integration found %d supported field(s), but no live data was written yet. WAY source: %s. Physical source: %s."),
        SupportedFieldCount,
        *WAYSourceLabel,
        *PhysicalSourceLabel);
    RefreshRuntimeAnimationAdapter();
}

USkeletalMeshComponent* UWanaAutoAnimationIntegrationComponent::ResolveIntegrationMesh() const
{
    AActor* OwnerActor = GetOwner();

    if (!OwnerActor)
    {
        return nullptr;
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
    OwnerActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);

    for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
    {
        if (!SkeletalMeshComponent)
        {
            continue;
        }

        if (SkeletalMeshComponent->GetAnimInstance() || SkeletalMeshComponent->GetAnimClass())
        {
            return SkeletalMeshComponent;
        }
    }

    return SkeletalMeshComponents.Num() > 0 ? SkeletalMeshComponents[0] : nullptr;
}

UAnimInstance* UWanaAutoAnimationIntegrationComponent::ResolveLiveAnimInstance(USkeletalMeshComponent*& OutMeshComponent, UClass*& OutAnimClass) const
{
    OutMeshComponent = ResolveIntegrationMesh();
    OutAnimClass = nullptr;

    if (!OutMeshComponent)
    {
        return nullptr;
    }

    if (UAnimInstance* AnimInstance = OutMeshComponent->GetAnimInstance())
    {
        OutAnimClass = AnimInstance->GetClass();
        return AnimInstance;
    }

    if (UClass* AnimClass = OutMeshComponent->GetAnimClass())
    {
        OutAnimClass = AnimClass;
    }

    return nullptr;
}

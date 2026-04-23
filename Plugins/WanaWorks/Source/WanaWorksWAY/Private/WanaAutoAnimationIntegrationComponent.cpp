#include "WanaAutoAnimationIntegrationComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UnrealType.h"
#include "WAYPlayerProfileComponent.h"
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
    SupportedFieldCount += HasObjectPropertyAlias(AnimClass, { TEXT("WanaWAYComponent"), TEXT("WanaProfileComponent"), TEXT("WAYPlayerProfileComponent") }, UWAYPlayerProfileComponent::StaticClass()) ? 1 : 0;
    SupportedFieldCount += HasObjectPropertyAlias(AnimClass, { TEXT("WanaPhysicalStateComponent"), TEXT("PhysicalStateComponent") }, UWanaPhysicalStateComponent::StaticClass()) ? 1 : 0;
    SupportedFieldCount += HasBoolPropertyAlias(AnimClass, { TEXT("bWanaFacingHookRequested"), TEXT("bFacingHookRequested") }) ? 1 : 0;
    SupportedFieldCount += HasBoolPropertyAlias(AnimClass, { TEXT("bWanaTurnToTargetRequested"), TEXT("bTurnToTargetRequested") }) ? 1 : 0;
    SupportedFieldCount += HasEnumPropertyAlias(AnimClass, { TEXT("WanaReactionState"), TEXT("ReactionState") }) ? 1 : 0;
    SupportedFieldCount += HasEnumPropertyAlias(AnimClass, { TEXT("WanaRecommendedBehavior"), TEXT("RecommendedBehavior") }) ? 1 : 0;
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
    AppliedFieldCount += TrySetObjectPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaWAYComponent"), TEXT("WanaProfileComponent"), TEXT("WAYPlayerProfileComponent") }, UWAYPlayerProfileComponent::StaticClass(), WAYComponent) ? 1 : 0;
    AppliedFieldCount += TrySetObjectPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaPhysicalStateComponent"), TEXT("PhysicalStateComponent") }, UWanaPhysicalStateComponent::StaticClass(), PhysicalStateComponent) ? 1 : 0;
    AppliedFieldCount += TrySetBoolPropertyAlias(AnimInstance, AnimClass, { TEXT("bWanaFacingHookRequested"), TEXT("bFacingHookRequested") }, HookState.bFacingHookRequested) ? 1 : 0;
    AppliedFieldCount += TrySetBoolPropertyAlias(AnimInstance, AnimClass, { TEXT("bWanaTurnToTargetRequested"), TEXT("bTurnToTargetRequested") }, HookState.bTurnToTargetRequested) ? 1 : 0;
    AppliedFieldCount += TrySetEnumPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaReactionState"), TEXT("ReactionState") }, static_cast<int64>(HookState.ReactionState)) ? 1 : 0;
    AppliedFieldCount += TrySetEnumPropertyAlias(AnimInstance, AnimClass, { TEXT("WanaRecommendedBehavior"), TEXT("RecommendedBehavior") }, static_cast<int64>(HookState.RecommendedBehavior)) ? 1 : 0;
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
}

UWanaAutoAnimationIntegrationComponent::UWanaAutoAnimationIntegrationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
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
        return;
    }

    SupportedFieldCount = CountSupportedAutomaticIntegrationFields(AnimClass);

    if (SupportedFieldCount <= 0)
    {
        AutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Limited;
        Detail = TEXT("Automatic Anim BP integration is limited because the detected Animation Blueprint does not expose the supported WanaWorks auto-wire fields or component references in this first pass.");
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
        return;
    }

    LastAppliedFieldCount = ApplyAutomaticIntegrationToAnimInstance(AnimInstance, AnimClass, WAYComponent, PhysicalStateComponent);

    if (LastAppliedFieldCount > 0)
    {
        bAutoWireSucceeded = true;
        AutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Applied;
        Detail = FString::Printf(
            TEXT("Automatic Anim BP integration applied %d supported field(s). WAY source: %s. Physical source: %s. The original Animation Blueprint stays preserved while the sandbox subject receives WanaWorks hook data automatically."),
            LastAppliedFieldCount,
            *WAYSourceLabel,
            *PhysicalSourceLabel);
        return;
    }

    AutomaticIntegrationStatus = EWAYAutomaticAnimationIntegrationStatus::Ready;
    Detail = FString::Printf(
        TEXT("Automatic Anim BP integration found %d supported field(s), but no live data was written yet. WAY source: %s. Physical source: %s."),
        SupportedFieldCount,
        *WAYSourceLabel,
        *PhysicalSourceLabel);
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

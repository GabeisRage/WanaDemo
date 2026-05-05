#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WanaAnimationAdapterReportAsset.generated.h"

UCLASS(BlueprintType)
class WANAWORKSWAY_API UWanaAnimationAdapterReportAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UWanaAnimationAdapterReportAsset();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString GeneratedLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString GeneratedAtUtc;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString WorkspaceLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString SourceSubjectName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString SourceSubjectPath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString SkeletalMeshLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString SkeletonLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString AssignedAnimClassLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString GeneratedAnimBPClassLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString ParentAnimInstanceClassLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString AnimBlueprintAssetLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString AnimBlueprintAssetPath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString HookProviderStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString HookDataReadableStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString PhysicalStateProviderStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString SharedCharacterBlueprintStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString SharedSkeletalMeshStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString SharedSkeletonStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString SharedAnimBlueprintStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString DirectGraphEditSafety;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString RecommendedIntegrationStrategy;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString AdapterReadinessState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString AdapterStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString AdapterPackagePath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString AdapterObjectPath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    TArray<FString> ReadableHookFields;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString PostureReactionMappingSummary;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    FString Limitations;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter", meta = (MultiLine = "true"))
    FString FullReportText;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bOriginalAnimBlueprintPreserved = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bAssignedAnimClassReplaced = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bDirectGraphEditPerformed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|Animation Adapter")
    bool bPersistentReportSaved = false;
};

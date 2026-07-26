#pragma once

#include "CoreMinimal.h"
#include "WanaProjectBlueprintTypes.generated.h"

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaProjectVisionBrief
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString VisionBrief;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString TeamSize = TEXT("Solo");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString ExperienceLevel = TEXT("Intermediate");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Timeline = TEXT("6 Months");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Budget = TEXT("Indie");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString TargetPlatform = TEXT("PC");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString IntendedDeliverable = TEXT("Vertical Slice");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString DesiredPlaytime = TEXT("15 Minutes");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString ProjectIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString EngineAdapterId = TEXT("Unreal");
};

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaProjectBlueprintReadinessContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString ProjectHealthStatus = TEXT("Unknown");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString CharacterBuildingStatus = TEXT("Unknown");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString CharacterIntelligenceStatus = TEXT("Unknown");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString EnvironmentStatus = TEXT("Unknown");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString AnimationAdapterStatus = TEXT("Unknown");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString CharacterEvidence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString IntelligenceEvidence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString EnvironmentEvidence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString AdapterEvidence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    bool bProjectHealthScanAvailable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    bool bCharacterSubjectSelected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    bool bIntelligenceSubjectSelected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    bool bEnvironmentReportAvailable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    bool bAnimationAdapterAvailable = false;
};

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaProjectBlueprintPillar
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Definition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString PlayerValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FString> RequiredSystems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString WorkspaceRoute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Readiness = TEXT("Needs Validation");
};

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaProjectBlueprintSystemRequirement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FString> Dependencies;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Readiness = TEXT("Unknown");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Evidence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString WorkspaceRoute;
};

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaProjectBlueprintMilestone
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString StableId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Objective;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FString> RequiredSystems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FString> DependsOn;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString CompletionEvidence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString WorkspaceRoute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Complexity = TEXT("Medium");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Readiness = TEXT("Needs Validation");
};

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaProjectBlueprintGap
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Capability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Status = TEXT("Unknown");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Evidence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString WorkspaceRoute;
};

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaProjectBlueprintRoute
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Reason;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Workspace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString WorkflowStep;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Priority = TEXT("Medium");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Readiness = TEXT("Needs Review");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString Risk = TEXT("Low");
};

USTRUCT(BlueprintType)
struct WANAWORKSCORE_API FWanaProjectBlueprintPlan
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString PlanId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString VisionSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString InterpretedGameType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString CameraPerspective;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString PlayerFantasy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString ProductionComplexity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString ScopeReality;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString ScopeReason;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString RecommendedDeliverable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString RecommendedVerticalSlice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString VerticalSliceProof;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FWanaProjectBlueprintPillar> GameplayPillars;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FWanaProjectBlueprintSystemRequirement> RequiredSystems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FWanaProjectBlueprintMilestone> Milestones;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FWanaProjectBlueprintGap> Gaps;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FWanaProjectBlueprintRoute> RecommendedRoutes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FString> Assumptions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    TArray<FString> Limitations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString EngineAdapterId = TEXT("Unreal");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString PlannerMode = TEXT("Rule-Based V1");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    FString GeneratedLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wana Works|Project Blueprint")
    bool bHasUsableVision = false;
};

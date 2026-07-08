#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WanaWITEnvironmentReportAsset.generated.h"

UENUM(BlueprintType)
enum class EWanaWITEnvironmentCandidateType : uint8
{
    Unknown UMETA(DisplayName = "Unknown"),
    Cover UMETA(DisplayName = "Cover"),
    Obstacle UMETA(DisplayName = "Obstacle"),
    MovementSpace UMETA(DisplayName = "Movement Space"),
    Boundary UMETA(DisplayName = "Boundary"),
    NavigationRelevant UMETA(DisplayName = "Navigation Relevant")
};

USTRUCT(BlueprintType)
struct WANAWORKSWIT_API FWanaWITEnvironmentReportCandidate
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString ActorName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString ActorPath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    EWanaWITEnvironmentCandidateType CandidateType = EWanaWITEnvironmentCandidateType::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    float Confidence = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString Reason;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FVector ApproximateBoundsExtent = FVector::ZeroVector;
};

UCLASS(BlueprintType)
class WANAWORKSWIT_API UWanaWITEnvironmentReportAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UWanaWITEnvironmentReportAsset();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString ReportVersion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString GeneratedLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString GeneratedAtUtc;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString WorkspaceLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString SourceWorldName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString SourceWorldPath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString ScanMode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString ScanStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString ReadinessStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 SelectedActorCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 ScannedActorCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 StaticMeshActorCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 SkeletalMeshActorCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 PrimitiveComponentCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 CollisionCandidateCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 CoverCandidateCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 ObstacleCandidateCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 MovementSpaceCandidateCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 BoundaryCandidateCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 NavigationRelevantCandidateCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    float Confidence = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString NavigationContextStatus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString SemanticSummary;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString SceneInterpretation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString Limitations;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString RecommendedNextAction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    int32 CandidateDetailCap = 100;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    bool bCandidateDetailsTruncated = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    TArray<FWanaWITEnvironmentReportCandidate> CandidateDetails;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString ReportPackagePath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    FString ReportObjectPath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report", meta = (MultiLine = "true"))
    FString FullReportText;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    bool bLevelGeometryModified = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    bool bLevelActorsSpawned = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    bool bCollisionSettingsChanged = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wana Works|WIT Report")
    bool bNavMeshModified = false;
};

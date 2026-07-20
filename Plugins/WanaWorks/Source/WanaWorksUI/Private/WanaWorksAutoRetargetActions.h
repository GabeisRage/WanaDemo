#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;

// Per-skeleton auto-characterization result (one side of a source/target retarget pair).
struct FWanaAutoRetargetSideResult
{
    FString SkeletalMeshLabel;
    FString IKRigAssetPath;
    bool bUsedKnownTemplate = false;
    FString MatchedTemplateName;
    float TemplateMatchScore = 0.0f;
    int32 DetectedChainCount = 0;
    int32 MissingBoneCount = 0;
};

struct FWanaAutoRetargetResult
{
    bool bSucceeded = false;
    FString StatusLabel;
    FString Detail;
    FWanaAutoRetargetSideResult Source;
    FWanaAutoRetargetSideResult Target;
    int32 MappedChainCount = 0;
    int32 TotalTargetChainCount = 0;
    FString RetargeterAssetPath;
    FString ReadinessSummaryText;
};

namespace WanaWorksAutoRetargetActions
{
    // Perceive + Prepare + Prove: auto-characterizes both skeletons via UE's IK Rig system,
    // auto-generates retarget chain definitions, creates a WanaWorks-owned IK Retargeter asset,
    // and auto-maps chains between them. Original meshes are never modified - all generated
    // assets are written under /Game/WanaWorks/Retarget.
    FWanaAutoRetargetResult ExecuteAutoRetargetAnalysis(
        USkeletalMesh* SourceMesh,
        USkeletalMesh* TargetMesh,
        const FString& SourceLabel,
        const FString& TargetLabel);
}

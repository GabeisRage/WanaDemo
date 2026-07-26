#pragma once

#include "CoreMinimal.h"

// A single project-health finding. Read-only diagnosis - WanaWorks never acts on these
// automatically in V1.
struct FWanaProjectHealthFinding
{
    FString FindingId;
    FString Title;
    FString Category;
    FString Severity;
    FString DiagnosisClassification;
    FString ProblemDescription;
    FString RootCause;
    FString AffectedSystem;
    FString PotentialImpact;
    FString RecommendedRoute;
    FString RecommendedWorkflowStep = TEXT("Analyze");
    FString PreventionGuidance;
    FString EngineAdapterId = TEXT("Unreal");
    FString Confidence;
    FString Evidence;
    bool bCanWanaWorksActNow = false;
    bool bRequiresManualEngineAction = false;
    FString RelatedPath;
    FString CurrentStateStatus;
};

// Read-only, approval-ready guidance derived from one evidence-backed finding. This is a
// proposal model only: it deliberately contains no execution callback, mutation request, or
// approval state transition.
struct FWanaProjectHealthCorrectionPlan
{
    FString PlanId;
    FString SourceFindingId;
    FString Title;
    FString Summary;
    FString SourceSeverity;
    FString EngineAdapterId = TEXT("Unreal");
    FString DiagnosisCategory;
    FString DiagnosisConfidence;
    FString SourceEvidence;
    FString DetectedCondition;
    FString ProposedCorrection;
    FString CorrectionRationale;
    FString AffectedSystems;
    FString RelatedFilesOrProjectAreas;
    FString EstimatedImplementationRisk = TEXT("Unknown");
    FString EstimatedValidationRisk = TEXT("Unknown");
    FString CorrectionType = TEXT("Manual Validation");
    FString ExecutionCapability = TEXT("Guidance Only");
    bool bRequiresDeveloperApproval = false;
    bool bRequiresManualEngineAction = false;
    bool bFutureAutomationCandidate = false;
    FString RollbackGuidance;
    FString ValidationSteps;
    FString PreventionGuidance;
    FString Limitations;
    FString CurrentPlanState = TEXT("Proposed");
};

// Project-aware output evidence resolved by the UI module before the bounded metadata scan.
// Keeping this context data-only lets the established scanner own all Project Health findings
// without coupling it to editor worlds or WanaWorks report asset classes.
struct FWanaProjectHealthScanContext
{
    bool bHasOutputContext = false;
    bool bHasEditorWorld = false;
    FString CurrentWorldLabel;
    FString CurrentWorldPath;
    bool bHasAnyWITReport = false;
    bool bCurrentWorldWITReportReadable = false;
    bool bCurrentWorldWITReportMatches = false;
    FString CurrentWorldWITReportPath;
    bool bHasAnyAnimationAdapterReport = false;
};

struct FWanaProjectHealthScanResult
{
    bool bScanCompleted = false;
    FString OverallStatusLabel = TEXT("Not Scanned");
    FString ProjectName;
    FString EngineVersionLabel;
    FString EngineCompatibilityStatus;
    FString WanaWorksPluginVersionLabel;
    int32 ProjectModuleCount = 0;
    int32 DeclaredPluginCount = 0;
    int32 EnabledPluginCount = 0;
    int32 DeclaredWanaWorksModuleCount = 0;
    int32 AvailableWanaWorksModuleCount = 0;
    int32 LoadedWanaWorksModuleCount = 0;
    bool bEditorTargetRecognized = false;
    FString BuildReadinessStatus = TEXT("Unknown");
    TArray<FWanaProjectHealthFinding> Findings;
    int32 CriticalCount = 0;
    int32 HighCount = 0;
    int32 MediumCount = 0;
    int32 LowCount = 0;
    int32 InformationalCount = 0;
    FString ScanLimitationsText;
    FString ScanTimestampUtc;
};

namespace WanaWorksProjectHealthActions
{
    // Cheap, safe overview: engine version, project name, plugin counts. No file scanning beyond
    // descriptors UE already has loaded in memory. Safe to call on every workspace open.
    FWanaProjectHealthScanResult RunLightweightOverview();

    // Deeper read-only scan: everything in the overview plus module runtime/editor
    // classification, declared-plugin enabled-state cross-check, and a bounded text scan of each
    // project module's Build.cs for known editor-only dependency names. Never modifies any file.
    FWanaProjectHealthScanResult RunFullScan(const FWanaProjectHealthScanContext& ScanContext = FWanaProjectHealthScanContext());

    // Converts eligible findings into read-only, approval-ready proposals. Informational
    // readiness findings are omitted unless they have a concrete validation or unsupported route.
    TArray<FWanaProjectHealthCorrectionPlan> BuildCorrectionPlans(const FWanaProjectHealthScanResult& ScanResult);
}

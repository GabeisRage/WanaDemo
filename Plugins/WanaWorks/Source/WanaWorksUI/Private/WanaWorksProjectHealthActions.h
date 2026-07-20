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
    FString ProblemDescription;
    FString RootCause;
    FString AffectedSystem;
    FString RecommendedRoute;
    FString PreventionGuidance;
    FString EngineAdapterId = TEXT("Unreal");
    FString Confidence;
    FString Evidence;
    bool bCanWanaWorksActNow = false;
    bool bRequiresManualEngineAction = false;
    FString RelatedPath;
    FString CurrentStateStatus;
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
    FWanaProjectHealthScanResult RunFullScan();
}

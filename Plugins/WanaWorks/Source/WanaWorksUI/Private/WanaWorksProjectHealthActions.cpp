#include "WanaWorksProjectHealthActions.h"

#include "HAL/PlatformFilemanager.h"
#include "Interfaces/IPluginManager.h"
#include "Interfaces/IProjectManager.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ModuleDescriptor.h"
#include "PluginDescriptor.h"
#include "ProjectDescriptor.h"

namespace
{
constexpr int32 MaxModulesToTextScan = 20;

// Names that only belong in Editor-type modules. Presence in a Runtime-type module's Build.cs
// dependency list is a reliable, non-speculative signal - these modules are not packaged into
// runtime builds, so referencing them there cannot link/ship correctly.
const TCHAR* EditorOnlyDependencyNames[] =
{
    TEXT("UnrealEd"),
    TEXT("EditorStyle"),
    TEXT("EditorSubsystem"),
    TEXT("LevelEditor"),
    TEXT("PropertyEditor"),
    TEXT("EditorWidgets"),
    TEXT("WorkspaceMenuStructure"),
};

bool IsRuntimeHostType(EHostType::Type Type)
{
    return Type == EHostType::Runtime
        || Type == EHostType::RuntimeNoCommandlet
        || Type == EHostType::RuntimeAndProgram
        || Type == EHostType::CookedOnly
        || Type == EHostType::UncookedOnly
        || Type == EHostType::Developer
        || Type == EHostType::ServerOnly
        || Type == EHostType::ClientOnly;
}

void AddFinding(
    TArray<FWanaProjectHealthFinding>& Findings,
    const FString& FindingId,
    const FString& Title,
    const FString& Category,
    const FString& Severity,
    const FString& ProblemDescription,
    const FString& RootCause,
    const FString& AffectedSystem,
    const FString& RecommendedRoute,
    const FString& PreventionGuidance,
    const FString& Confidence,
    const FString& Evidence,
    bool bCanWanaWorksActNow,
    bool bRequiresManualEngineAction,
    const FString& CurrentStateStatus,
    const FString& RelatedPath = FString())
{
    FWanaProjectHealthFinding Finding;
    Finding.FindingId = FindingId;
    Finding.Title = Title;
    Finding.Category = Category;
    Finding.Severity = Severity;
    Finding.ProblemDescription = ProblemDescription;
    Finding.RootCause = RootCause;
    Finding.AffectedSystem = AffectedSystem;
    Finding.RecommendedRoute = RecommendedRoute;
    Finding.PreventionGuidance = PreventionGuidance;
    Finding.Confidence = Confidence;
    Finding.Evidence = Evidence;
    Finding.bCanWanaWorksActNow = bCanWanaWorksActNow;
    Finding.bRequiresManualEngineAction = bRequiresManualEngineAction;
    Finding.CurrentStateStatus = CurrentStateStatus;
    Finding.RelatedPath = RelatedPath;
    Findings.Add(Finding);
}

FString FormatEngineVersionLabel()
{
    const FEngineVersion& Version = FEngineVersion::Current();
    return FString::Printf(TEXT("%d.%d.%d"), Version.GetMajor(), Version.GetMinor(), Version.GetPatch());
}

FString ResolveWanaWorksPluginVersionLabel()
{
    const TSharedPtr<IPlugin> WanaWorksPlugin = IPluginManager::Get().FindPlugin(TEXT("WanaWorks"));

    if (!WanaWorksPlugin.IsValid())
    {
        return TEXT("(WanaWorks plugin descriptor unavailable)");
    }

    const FPluginDescriptor& Descriptor = WanaWorksPlugin->GetDescriptor();
    return Descriptor.VersionName.IsEmpty() ? FString::Printf(TEXT("v%d"), Descriptor.Version) : Descriptor.VersionName;
}

void ScanModuleBuildCsForEditorOnlyDependencies(
    const FModuleDescriptor& ModuleDescriptor,
    TArray<FWanaProjectHealthFinding>& Findings,
    TArray<FString>& OutLimitations)
{
    if (!IsRuntimeHostType(ModuleDescriptor.Type))
    {
        return;
    }

    const FString ModuleName = ModuleDescriptor.Name.ToString();
    const FString BuildCsPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), ModuleName, ModuleName + TEXT(".Build.cs"));

    if (!FPaths::FileExists(BuildCsPath))
    {
        // Not every declared module follows the conventional Source/<Name>/<Name>.Build.cs
        // layout (plugin modules, nested folders). Skipping is safe - this is a best-effort,
        // reliably-detectable check, not a full build-graph parser.
        return;
    }

    FString BuildCsContents;

    if (!FFileHelper::LoadFileToString(BuildCsContents, *BuildCsPath))
    {
        OutLimitations.Add(FString::Printf(TEXT("Could not read %s for dependency review."), *BuildCsPath));
        return;
    }

    for (const TCHAR* EditorOnlyName : EditorOnlyDependencyNames)
    {
        if (BuildCsContents.Contains(EditorOnlyName))
        {
            AddFinding(
                Findings,
                FString::Printf(TEXT("ModuleEditorDependency_%s_%s"), *ModuleName, EditorOnlyName),
                FString::Printf(TEXT("Review Runtime/Editor Dependency Boundary in %s"), *ModuleName),
                TEXT("Modules and Dependencies"),
                TEXT("High"),
                FString::Printf(TEXT("Runtime module \"%s\" references \"%s\", which is an editor-only dependency."), *ModuleName, EditorOnlyName),
                TEXT("Unreal cannot safely load editor-only systems inside packaged runtime code, so this dependency will not link or ship correctly in a cooked build."),
                ModuleName,
                FString::Printf(TEXT("Move the functionality that needs \"%s\" into an editor module, or remove the dependency, before packaging."), EditorOnlyName),
                TEXT("Keep editor-only dependencies out of Runtime-type modules from the start; use a dedicated Editor module for tooling and editor UI."),
                TEXT("Confirmed from project metadata"),
                FString::Printf(TEXT("%s declares \"%s\" as a dependency and is a Runtime-type module in the project descriptor."), *BuildCsPath, EditorOnlyName),
                false,
                true,
                TEXT("Detected"),
                BuildCsPath);
        }
    }
}

FWanaProjectHealthScanResult BuildBaseResult()
{
    FWanaProjectHealthScanResult Result;
    Result.ProjectName = FApp::GetProjectName();
    Result.EngineVersionLabel = FormatEngineVersionLabel();
    Result.WanaWorksPluginVersionLabel = ResolveWanaWorksPluginVersionLabel();

    const FEngineVersion& Version = FEngineVersion::Current();
    if (Version.GetMajor() == 5)
    {
        Result.EngineCompatibilityStatus = TEXT("Recognized (UE5.x) - not exhaustively validated against every 5.x point release");
    }
    else
    {
        Result.EngineCompatibilityStatus = TEXT("Not yet validated - this WanaWorks build has only been verified against UE5.x");
    }

    const FProjectDescriptor* ProjectDescriptor = IProjectManager::Get().GetCurrentProject();
    Result.ProjectModuleCount = ProjectDescriptor ? ProjectDescriptor->Modules.Num() : 0;
    Result.DeclaredPluginCount = ProjectDescriptor ? ProjectDescriptor->Plugins.Num() : 0;
    Result.EnabledPluginCount = IPluginManager::Get().GetEnabledPlugins().Num();
    Result.ScanTimestampUtc = FDateTime::UtcNow().ToString(TEXT("%Y-%m-%dT%H:%M:%SZ"));
    return Result;
}

void TallySeverity(FWanaProjectHealthScanResult& Result)
{
    Result.CriticalCount = 0;
    Result.HighCount = 0;
    Result.MediumCount = 0;
    Result.LowCount = 0;
    Result.InformationalCount = 0;

    for (const FWanaProjectHealthFinding& Finding : Result.Findings)
    {
        if (Finding.Severity.Equals(TEXT("Critical"), ESearchCase::IgnoreCase)) { ++Result.CriticalCount; }
        else if (Finding.Severity.Equals(TEXT("High"), ESearchCase::IgnoreCase)) { ++Result.HighCount; }
        else if (Finding.Severity.Equals(TEXT("Medium"), ESearchCase::IgnoreCase)) { ++Result.MediumCount; }
        else if (Finding.Severity.Equals(TEXT("Low"), ESearchCase::IgnoreCase)) { ++Result.LowCount; }
        else { ++Result.InformationalCount; }
    }

    if (Result.CriticalCount + Result.HighCount + Result.MediumCount + Result.LowCount == 0)
    {
        Result.OverallStatusLabel = TEXT("Healthy");
    }
    else
    {
        Result.OverallStatusLabel = TEXT("Issues Found");
    }
}
}

FWanaProjectHealthScanResult WanaWorksProjectHealthActions::RunLightweightOverview()
{
    FWanaProjectHealthScanResult Result = BuildBaseResult();
    Result.bScanCompleted = false;
    Result.OverallStatusLabel = TEXT("Initial Check Ready");
    Result.ScanLimitationsText = TEXT("Lightweight overview only - engine version and plugin/module counts. Run Analyze for a full read-only scan.");
    return Result;
}

FWanaProjectHealthScanResult WanaWorksProjectHealthActions::RunFullScan()
{
    FWanaProjectHealthScanResult Result = BuildBaseResult();
    const FProjectDescriptor* ProjectDescriptor = IProjectManager::Get().GetCurrentProject();

    if (!ProjectDescriptor)
    {
        Result.bScanCompleted = false;
        Result.OverallStatusLabel = TEXT("Scan Failed Safely");
        Result.ScanLimitationsText = TEXT("The current project descriptor was unavailable, so module and plugin checks could not run. Engine version and WanaWorks plugin version are still reported above where available.");
        return Result;
    }

    TArray<FString> Limitations;

    // Engine compatibility finding.
    if (FEngineVersion::Current().GetMajor() != 5)
    {
        AddFinding(
            Result.Findings,
            TEXT("EngineVersionUnvalidated"),
            TEXT("Validate Engine Compatibility"),
            TEXT("Engine Compatibility"),
            TEXT("Medium"),
            FString::Printf(TEXT("The current engine version (%s) has not been validated by this WanaWorks build."), *Result.EngineVersionLabel),
            TEXT("WanaWorks V1 has only been exercised against UE5.x; behavior on other major versions is unconfirmed."),
            TEXT("WanaWorks plugin"),
            TEXT("Treat WanaWorks features as unverified on this engine version until tested; prefer UE5.x for now."),
            TEXT("Track WanaWorks release notes for supported engine versions before upgrading."),
            TEXT("Confirmed from project metadata"),
            FString::Printf(TEXT("FEngineVersion::Current() reports major version %d."), FEngineVersion::Current().GetMajor()),
            false,
            true,
            TEXT("Detected"));
    }
    else
    {
        AddFinding(
            Result.Findings,
            TEXT("EngineVersionRecognized"),
            TEXT("Engine Version Recognized"),
            TEXT("Engine Compatibility"),
            TEXT("Informational"),
            FString::Printf(TEXT("Detected Unreal Engine %s, within the UE5.x range this WanaWorks build targets."), *Result.EngineVersionLabel),
            FString(),
            TEXT("WanaWorks plugin"),
            FString(),
            FString(),
            TEXT("Confirmed from project metadata"),
            TEXT("FEngineVersion::Current()"),
            false,
            false,
            TEXT("Ready"));
    }

    // Plugin dependency / enabled-state cross-check.
    int32 DisabledOrMissingRequiredPlugins = 0;

    for (const FPluginReferenceDescriptor& PluginReference : ProjectDescriptor->Plugins)
    {
        if (!PluginReference.bEnabled)
        {
            continue;
        }

        const TSharedPtr<IPlugin> ResolvedPlugin = IPluginManager::Get().FindPlugin(PluginReference.Name);
        const bool bActuallyEnabled = ResolvedPlugin.IsValid() && ResolvedPlugin->IsEnabled();

        if (!bActuallyEnabled)
        {
            ++DisabledOrMissingRequiredPlugins;
            AddFinding(
                Result.Findings,
                FString::Printf(TEXT("PluginUnavailable_%s"), *PluginReference.Name),
                FString::Printf(TEXT("Validate Required Plugin: %s"), *PluginReference.Name),
                TEXT("Modules and Dependencies"),
                TEXT("High"),
                FString::Printf(TEXT("The project declares \"%s\" as a required plugin, but it is not currently enabled or could not be found."), *PluginReference.Name),
                TEXT("A plugin the project depends on is disabled, missing, or failed to load, which can break features that rely on it and may cause missing-module errors at startup or packaging."),
                PluginReference.Name,
                TEXT("Enable the plugin from the Plugins browser, or confirm it is installed, before relying on features that depend on it. WanaWorks does not enable plugins automatically."),
                TEXT("Keep plugin dependencies enabled and verify them after engine or plugin updates."),
                TEXT("Confirmed from project metadata"),
                FString::Printf(TEXT(".uproject declares \"%s\" with Enabled=true; IPluginManager reports %s."), *PluginReference.Name, ResolvedPlugin.IsValid() ? TEXT("it as disabled") : TEXT("it as not found")),
                false,
                true,
                TEXT("Detected"));
        }
    }

    if (DisabledOrMissingRequiredPlugins == 0 && ProjectDescriptor->Plugins.Num() > 0)
    {
        AddFinding(
            Result.Findings,
            TEXT("AllRequiredPluginsReady"),
            TEXT("Required Plugins Ready"),
            TEXT("Modules and Dependencies"),
            TEXT("Informational"),
            FString::Printf(TEXT("All %d project-declared required plugin(s) are enabled and available."), ProjectDescriptor->Plugins.Num()),
            FString(),
            TEXT("Project plugins"),
            FString(),
            FString(),
            TEXT("Confirmed from project metadata"),
            TEXT("IPluginManager cross-check against .uproject Plugins list."),
            false,
            false,
            TEXT("Ready"));
    }

    // Module runtime/editor classification + bounded Build.cs text scan.
    const int32 ModulesToScan = FMath::Min(ProjectDescriptor->Modules.Num(), MaxModulesToTextScan);

    if (ProjectDescriptor->Modules.Num() > MaxModulesToTextScan)
    {
        Limitations.Add(FString::Printf(TEXT("Project declares %d modules; dependency review was capped at the first %d for this pass."), ProjectDescriptor->Modules.Num(), MaxModulesToTextScan));
    }

    for (int32 Index = 0; Index < ModulesToScan; ++Index)
    {
        ScanModuleBuildCsForEditorOnlyDependencies(ProjectDescriptor->Modules[Index], Result.Findings, Limitations);
    }

    const bool bFoundModuleDependencyIssue = Result.Findings.ContainsByPredicate([](const FWanaProjectHealthFinding& Finding)
    {
        return Finding.Category.Equals(TEXT("Modules and Dependencies")) && Finding.FindingId.StartsWith(TEXT("ModuleEditorDependency_"));
    });

    if (!bFoundModuleDependencyIssue && ProjectDescriptor->Modules.Num() > 0)
    {
        AddFinding(
            Result.Findings,
            TEXT("ModuleBoundariesClean"),
            TEXT("Project Structure Ready"),
            TEXT("Modules and Dependencies"),
            TEXT("Informational"),
            TEXT("WanaWorks found no confirmed runtime/editor dependency boundary issues in the current scan."),
            FString(),
            TEXT("Project modules"),
            FString(),
            FString(),
            TEXT("Confirmed from project metadata"),
            TEXT("Build.cs text review of Runtime-type modules against a known editor-only dependency list."),
            false,
            false,
            TEXT("Ready"));
    }

    Result.bScanCompleted = true;
    Result.ScanLimitationsText = Limitations.Num() > 0
        ? FString::Join(Limitations, TEXT(" "))
        : TEXT("Scan covers engine version, declared plugin availability, and a text-based Runtime/Editor dependency review. It does not parse C++ source, validate compile correctness, or inspect asset content.");

    TallySeverity(Result);
    return Result;
}

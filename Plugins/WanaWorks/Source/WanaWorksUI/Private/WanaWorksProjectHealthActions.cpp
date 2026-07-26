#include "WanaWorksProjectHealthActions.h"

#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Interfaces/IProjectManager.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ModuleDescriptor.h"
#include "Modules/ModuleManager.h"
#include "PluginDescriptor.h"
#include "ProjectDescriptor.h"

namespace
{
constexpr int32 MaxModulesToTextScan = 20;
constexpr int64 MaxBuildCsTextScanBytes = 256 * 1024;
constexpr int32 MaxCorrectionPlans = 64;

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
    const FString& DiagnosisClassification,
    const FString& ProblemDescription,
    const FString& RootCause,
    const FString& AffectedSystem,
    const FString& PotentialImpact,
    const FString& RecommendedRoute,
    const FString& RecommendedWorkflowStep,
    const FString& PreventionGuidance,
    const FString& Confidence,
    const FString& Evidence,
    bool bCanWanaWorksActNow,
    bool bRequiresManualEngineAction,
    const FString& CurrentStateStatus,
    const FString& RelatedPath = FString())
{
    if (FindingId.IsEmpty() || Findings.ContainsByPredicate([&FindingId](const FWanaProjectHealthFinding& ExistingFinding)
    {
        return ExistingFinding.FindingId.Equals(FindingId, ESearchCase::IgnoreCase);
    }))
    {
        return;
    }

    FWanaProjectHealthFinding Finding;
    Finding.FindingId = FindingId;
    Finding.Title = Title;
    Finding.Category = Category;
    Finding.Severity = Severity;
    Finding.DiagnosisClassification = DiagnosisClassification.IsEmpty() ? TEXT("Requires Manual Validation") : DiagnosisClassification;
    Finding.ProblemDescription = ProblemDescription;
    Finding.RootCause = RootCause;
    Finding.AffectedSystem = AffectedSystem;
    Finding.PotentialImpact = PotentialImpact;
    Finding.RecommendedRoute = RecommendedRoute;
    Finding.RecommendedWorkflowStep = RecommendedWorkflowStep.IsEmpty() ? TEXT("Analyze") : RecommendedWorkflowStep;
    Finding.PreventionGuidance = PreventionGuidance;
    Finding.Confidence = Confidence.IsEmpty() ? TEXT("Unknown") : Confidence;
    Finding.Evidence = Evidence.IsEmpty() ? TEXT("No additional evidence was captured by this bounded check.") : Evidence;
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

FString StripBuildCsCommentsPreservingLayout(const FString& Contents)
{
    FString Stripped;
    Stripped.Reserve(Contents.Len());
    bool bInLineComment = false;
    bool bInBlockComment = false;
    bool bInString = false;
    bool bEscaped = false;

    for (int32 Index = 0; Index < Contents.Len(); ++Index)
    {
        const TCHAR Character = Contents[Index];
        const TCHAR NextCharacter = Index + 1 < Contents.Len() ? Contents[Index + 1] : TEXT('\0');

        if (bInLineComment)
        {
            if (Character == TEXT('\n'))
            {
                bInLineComment = false;
                Stripped.AppendChar(Character);
            }
            else
            {
                Stripped.AppendChar(TEXT(' '));
            }
            continue;
        }

        if (bInBlockComment)
        {
            if (Character == TEXT('*') && NextCharacter == TEXT('/'))
            {
                Stripped.Append(TEXT("  "));
                ++Index;
                bInBlockComment = false;
            }
            else
            {
                Stripped.AppendChar(Character == TEXT('\n') ? Character : TEXT(' '));
            }
            continue;
        }

        if (bInString)
        {
            Stripped.AppendChar(Character);
            if (bEscaped)
            {
                bEscaped = false;
            }
            else if (Character == TEXT('\\'))
            {
                bEscaped = true;
            }
            else if (Character == TEXT('"'))
            {
                bInString = false;
            }
            continue;
        }

        if (Character == TEXT('/') && NextCharacter == TEXT('/'))
        {
            Stripped.Append(TEXT("  "));
            ++Index;
            bInLineComment = true;
        }
        else if (Character == TEXT('/') && NextCharacter == TEXT('*'))
        {
            Stripped.Append(TEXT("  "));
            ++Index;
            bInBlockComment = true;
        }
        else
        {
            Stripped.AppendChar(Character);
            bInString = Character == TEXT('"');
        }
    }

    return Stripped;
}

void FindExactDependencyTokenMatches(const FString& Contents, const FString& DependencyName, TArray<int32>& OutMatchIndexes)
{
    const FString QuotedDependency = FString::Printf(TEXT("\"%s\""), *DependencyName);
    int32 SearchStart = 0;

    while (SearchStart < Contents.Len())
    {
        const int32 MatchIndex = Contents.Find(QuotedDependency, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
        if (MatchIndex == INDEX_NONE)
        {
            break;
        }

        OutMatchIndexes.Add(MatchIndex);
        SearchStart = MatchIndex + QuotedDependency.Len();
    }
}

bool IsDependencyDeclarationContext(const FString& Contents, int32 MatchIndex)
{
    const FString Prefix = Contents.Left(MatchIndex);
    const int32 DependencyListIndex = Prefix.Find(TEXT("DependencyModuleNames"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    const int32 DynamicListIndex = Prefix.Find(TEXT("DynamicallyLoadedModuleNames"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    const int32 DeclarationIndex = FMath::Max(DependencyListIndex, DynamicListIndex);
    if (DeclarationIndex == INDEX_NONE)
    {
        return false;
    }

    // A completed statement between the nearest dependency-list name and this token means the
    // token belongs to another expression. Custom helpers remain an uncertain outcome.
    return !Contents.Mid(DeclarationIndex, MatchIndex - DeclarationIndex).Contains(TEXT(";"));
}

bool IsLikelyInsideEditorConditional(const FString& Contents, int32 MatchIndex)
{
    const FString Prefix = Contents.Left(MatchIndex);
    const int32 BuildEditorGuardIndex = Prefix.Find(TEXT("Target.bBuildEditor"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    const int32 TargetTypeGuardIndex = Prefix.Find(TEXT("TargetType.Editor"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    const int32 GuardIndex = FMath::Max(BuildEditorGuardIndex, TargetTypeGuardIndex);

    if (GuardIndex == INDEX_NONE)
    {
        return false;
    }

    const int32 OpeningBraceIndex = Contents.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, GuardIndex);
    if (OpeningBraceIndex == INDEX_NONE || OpeningBraceIndex >= MatchIndex)
    {
        return false;
    }

    const int32 ConditionStart = FMath::Max(0, GuardIndex - 96);
    const FString ConditionContext = Contents.Mid(ConditionStart, OpeningBraceIndex - ConditionStart);
    if (!ConditionContext.Contains(TEXT("if"), ESearchCase::CaseSensitive)
        || ConditionContext.Contains(TEXT("!Target.bBuildEditor"), ESearchCase::CaseSensitive)
        || ConditionContext.Contains(TEXT("Target.bBuildEditor == false"), ESearchCase::CaseSensitive)
        || ConditionContext.Contains(TEXT("Target.Type != TargetType.Editor"), ESearchCase::CaseSensitive))
    {
        return false;
    }

    int32 BraceDepth = 0;
    for (int32 Index = OpeningBraceIndex; Index < MatchIndex; ++Index)
    {
        if (Contents[Index] == TEXT('{'))
        {
            ++BraceDepth;
        }
        else if (Contents[Index] == TEXT('}'))
        {
            --BraceDepth;
        }
    }

    return BraceDepth > 0;
}

FString BuildDependencyEvidence(const FString& OriginalContents, int32 MatchIndex, const FString& BuildCsPath)
{
    int32 LineStart = MatchIndex;
    while (LineStart > 0 && OriginalContents[LineStart - 1] != TEXT('\n'))
    {
        --LineStart;
    }

    int32 LineEnd = MatchIndex;
    while (LineEnd < OriginalContents.Len() && OriginalContents[LineEnd] != TEXT('\n'))
    {
        ++LineEnd;
    }

    int32 LineNumber = 1;
    for (int32 Index = 0; Index < LineStart; ++Index)
    {
        if (OriginalContents[Index] == TEXT('\n'))
        {
            ++LineNumber;
        }
    }

    FString EvidenceLine = OriginalContents.Mid(LineStart, LineEnd - LineStart).TrimStartAndEnd();
    EvidenceLine.ReplaceInline(TEXT("\t"), TEXT(" "));
    if (EvidenceLine.Len() > 180)
    {
        EvidenceLine = EvidenceLine.Left(177) + TEXT("...");
    }

    return FString::Printf(TEXT("%s line %d: %s"), *BuildCsPath, LineNumber, EvidenceLine.IsEmpty() ? TEXT("(empty line)") : *EvidenceLine);
}

void ScanModuleBuildCsForEditorOnlyDependencies(
    const FModuleDescriptor& ModuleDescriptor,
    const FString& BuildCsPath,
    TArray<FWanaProjectHealthFinding>& Findings,
    TArray<FString>& OutLimitations)
{
    if (!IsRuntimeHostType(ModuleDescriptor.Type))
    {
        return;
    }

    const FString ModuleName = ModuleDescriptor.Name.ToString();
    const int64 BuildCsSize = IFileManager::Get().FileSize(*BuildCsPath);
    if (BuildCsSize < 0 || BuildCsSize > MaxBuildCsTextScanBytes)
    {
        OutLimitations.Add(FString::Printf(
            TEXT("Skipped dependency text review for %s because its Build.cs size was unavailable or exceeded the %lld-byte safety cap."),
            *ModuleName,
            MaxBuildCsTextScanBytes));
        return;
    }

    FString BuildCsContents;
    if (!FFileHelper::LoadFileToString(BuildCsContents, *BuildCsPath))
    {
        OutLimitations.Add(FString::Printf(TEXT("Could not read %s for dependency review."), *BuildCsPath));
        return;
    }

    const FString CommentStrippedContents = StripBuildCsCommentsPreservingLayout(BuildCsContents);
    bool bIgnoredCommentOnlyMatch = false;

    for (const TCHAR* EditorOnlyName : EditorOnlyDependencyNames)
    {
        const FString DependencyName(EditorOnlyName);
        TArray<int32> OriginalMatches;
        TArray<int32> ActiveMatches;
        FindExactDependencyTokenMatches(BuildCsContents, DependencyName, OriginalMatches);
        FindExactDependencyTokenMatches(CommentStrippedContents, DependencyName, ActiveMatches);

        if (ActiveMatches.IsEmpty())
        {
            bIgnoredCommentOnlyMatch |= !OriginalMatches.IsEmpty();
            continue;
        }

        int32 UnguardedDependencyMatch = INDEX_NONE;
        int32 UncertainMatch = INDEX_NONE;
        int32 GuardedDependencyMatch = INDEX_NONE;

        for (const int32 MatchIndex : ActiveMatches)
        {
            const bool bDependencyDeclaration = IsDependencyDeclarationContext(CommentStrippedContents, MatchIndex);
            const bool bEditorGuarded = IsLikelyInsideEditorConditional(CommentStrippedContents, MatchIndex);

            if (bDependencyDeclaration && !bEditorGuarded)
            {
                UnguardedDependencyMatch = MatchIndex;
                break;
            }
            if (!bDependencyDeclaration && UncertainMatch == INDEX_NONE)
            {
                UncertainMatch = MatchIndex;
            }
            if (bDependencyDeclaration && bEditorGuarded && GuardedDependencyMatch == INDEX_NONE)
            {
                GuardedDependencyMatch = MatchIndex;
            }
        }

        if (UnguardedDependencyMatch != INDEX_NONE)
        {
            AddFinding(
                Findings,
                FString::Printf(TEXT("ModuleEditorDependency_%s_%s"), *ModuleName, EditorOnlyName),
                FString::Printf(TEXT("Potential Runtime/Editor Dependency Boundary in %s"), *ModuleName),
                TEXT("Modules and Dependencies"),
                TEXT("High"),
                TEXT("Possible Risk"),
                FString::Printf(TEXT("Runtime module \"%s\" appears to declare editor-only dependency \"%s\" outside an obvious editor-only conditional."), *ModuleName, EditorOnlyName),
                TEXT("Editor tooling and packaged gameplay code are sharing a runtime module dependency boundary."),
                ModuleName,
                TEXT("UnrealBuildTool packaging, linking, or packaged runtime module loading may fail because editor modules are unavailable in cooked runtime targets."),
                FString::Printf(TEXT("Move the functionality that needs \"%s\" into an Editor module, or remove the dependency from the Runtime module after manual Build.cs review."), EditorOnlyName),
                TEXT("Manual Unreal Step"),
                TEXT("Keep editor tooling and runtime gameplay code in separate modules, and guard editor-only dependencies with an editor target condition when shared source is unavoidable."),
                TEXT("High - quoted dependency token found in a dependency declaration; bounded C# interpretation is not a complete parser."),
                BuildDependencyEvidence(BuildCsContents, UnguardedDependencyMatch, BuildCsPath),
                false,
                true,
                TEXT("Detected"),
                BuildCsPath);
        }
        else if (UncertainMatch != INDEX_NONE)
        {
            AddFinding(
                Findings,
                FString::Printf(TEXT("ModuleEditorDependencyContext_%s_%s"), *ModuleName, EditorOnlyName),
                FString::Printf(TEXT("Validate Editor Dependency Context in %s"), *ModuleName),
                TEXT("Modules and Dependencies"),
                TEXT("Medium"),
                TEXT("Requires Manual Validation"),
                FString::Printf(TEXT("Editor-only module name \"%s\" appears in active Build.cs text for Runtime module \"%s\", but the bounded scan could not prove it is a dependency declaration."), EditorOnlyName, *ModuleName),
                TEXT("The token may be used by custom or conditional Build.cs logic that requires human interpretation."),
                ModuleName,
                TEXT("If the token resolves to a runtime dependency, packaging may fail; if it is only metadata or guarded logic, there may be no packaged-build issue."),
                TEXT("Review the cited Build.cs line and its surrounding conditional scope before changing module structure."),
                TEXT("Manual Unreal Step"),
                TEXT("Keep dependency declarations explicit and place editor-only logic in clearly scoped editor target conditions."),
                TEXT("Medium - active token match, uncertain declaration context."),
                BuildDependencyEvidence(BuildCsContents, UncertainMatch, BuildCsPath),
                false,
                true,
                TEXT("Needs Manual Validation"),
                BuildCsPath);
        }
        else if (GuardedDependencyMatch != INDEX_NONE)
        {
            AddFinding(
                Findings,
                FString::Printf(TEXT("ModuleEditorDependencyGuarded_%s_%s"), *ModuleName, EditorOnlyName),
                FString::Printf(TEXT("Editor Dependency Guarded in %s"), *ModuleName),
                TEXT("Modules and Dependencies"),
                TEXT("Informational"),
                TEXT("Informational"),
                FString::Printf(TEXT("Runtime module \"%s\" references editor-only dependency \"%s\" inside an apparent editor-target conditional."), *ModuleName, EditorOnlyName),
                TEXT("The dependency appears intentionally limited to editor target builds."),
                ModuleName,
                TEXT("No packaged-runtime blocker is inferred from this bounded check, though custom C# branches still require normal build validation."),
                TEXT("Keep the editor guard explicit and confirm runtime packaging after changing the conditional."),
                TEXT("Test"),
                TEXT("Continue separating editor tooling from runtime code and retain the target guard."),
                TEXT("High - quoted dependency and enclosing editor guard detected by bounded context analysis."),
                BuildDependencyEvidence(BuildCsContents, GuardedDependencyMatch, BuildCsPath),
                false,
                false,
                TEXT("Ready"),
                BuildCsPath);
        }
    }

    if (bIgnoredCommentOnlyMatch)
    {
        OutLimitations.Add(FString::Printf(TEXT("Ignored comment-only editor dependency names in %s; comments are not treated as active dependencies."), *ModuleName));
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
        Result.EngineCompatibilityStatus = TEXT("Recognized With Limitations - recognized UE5 family; this exact release is not exhaustively validated");
    }
    else
    {
        Result.EngineCompatibilityStatus = TEXT("Not Yet Validated - this WanaWorks build has only been exercised against the UE5 family");
    }

    const FProjectDescriptor* ProjectDescriptor = IProjectManager::Get().GetCurrentProject();
    Result.ProjectModuleCount = ProjectDescriptor ? ProjectDescriptor->Modules.Num() : 0;
    Result.DeclaredPluginCount = ProjectDescriptor ? ProjectDescriptor->Plugins.Num() : 0;
    Result.EnabledPluginCount = IPluginManager::Get().GetEnabledPlugins().Num();
    Result.bEditorTargetRecognized = GIsEditor;
    Result.ScanTimestampUtc = FDateTime::UtcNow().ToString(TEXT("%Y-%m-%dT%H:%M:%SZ"));
    return Result;
}

void InspectWanaWorksPluginDescriptor(FWanaProjectHealthScanResult& Result)
{
    const TSharedPtr<IPlugin> WanaWorksPlugin = IPluginManager::Get().FindPlugin(TEXT("WanaWorks"));
    if (!WanaWorksPlugin.IsValid())
    {
        AddFinding(
            Result.Findings,
            TEXT("WanaWorksPluginDescriptorUnavailable"),
            TEXT("WanaWorks Plugin Descriptor Unavailable"),
            TEXT("Modules and Dependencies"),
            TEXT("High"),
            TEXT("Confirmed Risk"),
            TEXT("IPluginManager could not resolve the WanaWorks plugin descriptor during Project Health analysis."),
            TEXT("The plugin may be incompletely installed, located outside the current plugin search paths, or represented by stale metadata."),
            TEXT("WanaWorks plugin"),
            TEXT("WanaWorks module, version, and compatibility checks are incomplete; editor startup or packaging may report missing plugin modules."),
            TEXT("Confirm the WanaWorks plugin directory and descriptor are present, then restart Unreal so plugin metadata can refresh."),
            TEXT("Manual Unreal Step"),
            TEXT("Keep the plugin descriptor and module folders together, and validate discovery after moving the project or upgrading Unreal."),
            TEXT("Confirmed - IPluginManager lookup returned no descriptor."),
            TEXT("IPluginManager::FindPlugin(\"WanaWorks\") returned an invalid plugin reference."),
            false,
            true,
            TEXT("Detected"));
        return;
    }

    const FPluginDescriptor& Descriptor = WanaWorksPlugin->GetDescriptor();
    const FString DescriptorPath = WanaWorksPlugin->GetDescriptorFileName();
    Result.DeclaredWanaWorksModuleCount = Descriptor.Modules.Num();
    TSet<FName> SeenModuleNames;

    for (const FModuleDescriptor& ModuleDescriptor : Descriptor.Modules)
    {
        const FName ModuleName = ModuleDescriptor.Name;
        if (ModuleName.IsNone())
        {
            continue;
        }

        if (SeenModuleNames.Contains(ModuleName))
        {
            AddFinding(
                Result.Findings,
                FString::Printf(TEXT("DuplicateWanaWorksModule_%s"), *ModuleName.ToString()),
                FString::Printf(TEXT("Duplicate WanaWorks Module: %s"), *ModuleName.ToString()),
                TEXT("Modules and Dependencies"),
                TEXT("High"),
                TEXT("Confirmed Risk"),
                FString::Printf(TEXT("The WanaWorks descriptor lists module \"%s\" more than once."), *ModuleName.ToString()),
                TEXT("Duplicate descriptor entries create ambiguous module loading and build metadata."),
                ModuleName.ToString(),
                TEXT("UnrealBuildTool or plugin loading may reject or inconsistently interpret the duplicate declaration."),
                TEXT("Remove the duplicate descriptor entry after manual review; Project Health does not edit descriptors."),
                TEXT("Manual Unreal Step"),
                TEXT("Keep every plugin module name unique and review descriptor merge conflicts when adding modules."),
                TEXT("Confirmed - duplicate module FName in the loaded descriptor."),
                FString::Printf(TEXT("%s contains repeated module name %s."), *DescriptorPath, *ModuleName.ToString()),
                false,
                true,
                TEXT("Detected"),
                DescriptorPath);
            continue;
        }
        SeenModuleNames.Add(ModuleName);

        if (FModuleManager::Get().ModuleExists(*ModuleName.ToString()))
        {
            ++Result.AvailableWanaWorksModuleCount;
        }
        else
        {
            AddFinding(
                Result.Findings,
                FString::Printf(TEXT("WanaWorksModuleUnavailable_%s"), *ModuleName.ToString()),
                FString::Printf(TEXT("WanaWorks Module Unavailable: %s"), *ModuleName.ToString()),
                TEXT("Modules and Dependencies"),
                TEXT("High"),
                TEXT("Confirmed Risk"),
                FString::Printf(TEXT("The WanaWorks descriptor declares module \"%s\", but the current module manager cannot resolve it for this editor target."), *ModuleName.ToString()),
                TEXT("The declared module binary or build metadata is unavailable to the active editor target."),
                ModuleName.ToString(),
                TEXT("Features owned by the module may be unavailable, and editor build or packaging validation may fail with a missing-module error."),
                TEXT("Confirm the module source and Build.cs are present, then rebuild the editor target manually."),
                TEXT("Manual Unreal Step"),
                TEXT("Keep descriptor declarations synchronized with module folders and Build.cs files."),
                TEXT("High - descriptor entry exists, but FModuleManager reports no matching module."),
                FString::Printf(TEXT("%s declares %s; FModuleManager::ModuleExists returned false."), *DescriptorPath, *ModuleName.ToString()),
                false,
                true,
                TEXT("Detected"),
                DescriptorPath);
        }

        if (FModuleManager::Get().IsModuleLoaded(ModuleName))
        {
            ++Result.LoadedWanaWorksModuleCount;
        }
    }
}

void AppendOutputReadinessFindings(FWanaProjectHealthScanResult& Result, const FWanaProjectHealthScanContext& ScanContext)
{
    if (!ScanContext.bHasOutputContext)
    {
        return;
    }

    const FString WorldEvidence = FString::Printf(
        TEXT("Current world=%s (%s); WIT lookup=%s; project-level WIT reports=%s."),
        ScanContext.CurrentWorldLabel.IsEmpty() ? TEXT("unavailable") : *ScanContext.CurrentWorldLabel,
        ScanContext.CurrentWorldPath.IsEmpty() ? TEXT("path unavailable") : *ScanContext.CurrentWorldPath,
        ScanContext.CurrentWorldWITReportPath.IsEmpty() ? TEXT("none") : *ScanContext.CurrentWorldWITReportPath,
        ScanContext.bHasAnyWITReport ? TEXT("present") : TEXT("none"));

    if (!ScanContext.bHasEditorWorld)
    {
        AddFinding(
            Result.Findings,
            TEXT("CurrentWorldContextUnavailable"),
            TEXT("Current Map WIT Check Requires an Editor World"),
            TEXT("WanaWorks Outputs"),
            TEXT("Informational"),
            TEXT("Unsupported Check"),
            TEXT("No active editor world was available, so Project Health could not associate a persistent WIT report with the current map."),
            TEXT("Current-map report matching requires a valid editor world and package path."),
            TEXT("Level Design / Character Intelligence"),
            TEXT("Environment-context readiness remains unknown; this does not mean the project or existing reports are broken."),
            TEXT("Open the intended level and run Analyze again."),
            TEXT("Analyze"),
            TEXT("Run Project Health with the intended map open when validating cross-workspace output readiness."),
            TEXT("Confirmed - no editor world context was available."),
            WorldEvidence,
            false,
            false,
            TEXT("Not Supported"));
    }
    else if (ScanContext.bCurrentWorldWITReportReadable && ScanContext.bCurrentWorldWITReportMatches)
    {
        AddFinding(
            Result.Findings,
            TEXT("CurrentWorldWITReportReady"),
            TEXT("Current Map WIT Context Ready"),
            TEXT("WanaWorks Outputs"),
            TEXT("Informational"),
            TEXT("Informational"),
            TEXT("A readable persistent WIT report matches the current editor map."),
            TEXT("Level Design has produced map-specific environment context."),
            TEXT("Level Design / Character Intelligence"),
            TEXT("Character Intelligence can consume the available context, subject to the report's own confidence and limitations."),
            TEXT("Rebuild the WIT report after significant level-layout or navigation changes."),
            TEXT("Test"),
            TEXT("Regenerate WIT context after substantial geometry, collision, or navigation changes."),
            TEXT("Confirmed from the loaded WIT report source-world metadata."),
            WorldEvidence,
            false,
            false,
            TEXT("Ready"),
            ScanContext.CurrentWorldWITReportPath);
    }
    else
    {
        const bool bMismatchedReport = ScanContext.bCurrentWorldWITReportReadable && !ScanContext.bCurrentWorldWITReportMatches;
        AddFinding(
            Result.Findings,
            bMismatchedReport ? TEXT("CurrentWorldWITReportMismatch") : TEXT("CurrentWorldWITReportMissing"),
            bMismatchedReport ? TEXT("WIT Report Does Not Match Current Map") : TEXT("Generate WIT Context for Current Map"),
            TEXT("WanaWorks Outputs"),
            TEXT("Medium"),
            bMismatchedReport ? TEXT("Possible Risk") : TEXT("Confirmed Gap"),
            bMismatchedReport ? TEXT("A readable WIT report was found, but its source-world metadata does not clearly match the current map.") : TEXT("No readable persistent WIT report was found for the current map."),
            bMismatchedReport ? TEXT("The available report belongs to another map or its world association is stale.") : TEXT("Level Design Build has not produced a readable report for this context."),
            TEXT("Level Design / Character Intelligence"),
            TEXT("Character Intelligence can operate, but environment-aware behavior confidence is limited because current cover, obstacle, movement-space, and navigation meaning are unavailable."),
            TEXT("Open Level Design for the current map and run Build to generate a fresh persistent WIT report."),
            TEXT("Build"),
            TEXT("Regenerate WIT context after significant level-layout, collision, or navigation changes."),
            bMismatchedReport ? TEXT("Medium - report readable, map association uncertain.") : TEXT("High - no readable report resolved for the current map."),
            WorldEvidence,
            true,
            false,
            TEXT("Missing"),
            ScanContext.CurrentWorldWITReportPath);
    }

    AddFinding(
        Result.Findings,
        ScanContext.bHasAnyAnimationAdapterReport ? TEXT("WanaAnimationAdapterReportsPresent") : TEXT("WanaAnimationAdapterReportsMissing"),
        ScanContext.bHasAnyAnimationAdapterReport ? TEXT("WanaAnimation Adapter Reports Present") : TEXT("Generate a WanaAnimation Adapter Report"),
        TEXT("WanaWorks Outputs"),
        ScanContext.bHasAnyAnimationAdapterReport ? TEXT("Informational") : TEXT("Low"),
        ScanContext.bHasAnyAnimationAdapterReport ? TEXT("Informational") : TEXT("Confirmed Gap"),
        ScanContext.bHasAnyAnimationAdapterReport
            ? TEXT("At least one persistent WanaAnimation adapter report exists under the WanaWorks output path.")
            : TEXT("No persistent WanaAnimation adapter report was found under the WanaWorks output path."),
        ScanContext.bHasAnyAnimationAdapterReport
            ? TEXT("A subject animation stack has produced a WanaWorks-owned readiness record.")
            : TEXT("No subject animation stack has yet produced a persistent adapter-readiness record at the expected location."),
        TEXT("Character Intelligence / Character Building"),
        ScanContext.bHasAnyAnimationAdapterReport
            ? TEXT("Project-level adapter readiness is present, but each selected subject still requires its own matching report.")
            : TEXT("Animation hook consumption readiness is not proven at the project level; existing gameplay animation remains unchanged."),
        ScanContext.bHasAnyAnimationAdapterReport
            ? TEXT("Use the character workspace cards to confirm a report belongs to the intended subject.")
            : TEXT("Select the intended subject and run Build in Character Building or Character Intelligence."),
        ScanContext.bHasAnyAnimationAdapterReport ? TEXT("Test") : TEXT("Build"),
        TEXT("Regenerate adapter reports after changing a subject's skeletal mesh, skeleton, or Anim BP."),
        TEXT("High - Asset Registry output-path check; subject-specific matching was not performed here."),
        ScanContext.bHasAnyAnimationAdapterReport
            ? TEXT("Asset Registry found an asset under /Game/WanaWorks/Adapters.")
            : TEXT("Asset Registry found no assets under /Game/WanaWorks/Adapters."),
        !ScanContext.bHasAnyAnimationAdapterReport,
        false,
        ScanContext.bHasAnyAnimationAdapterReport ? TEXT("Ready") : TEXT("Missing"),
        TEXT("/Game/WanaWorks/Adapters"));
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

    Result.BuildReadinessStatus = Result.CriticalCount + Result.HighCount > 0
        ? TEXT("Packaging Confidence Limited")
        : TEXT("No Confirmed Metadata Blockers Detected");
}

bool IsCorrectionPlanEligible(const FWanaProjectHealthFinding& Finding)
{
    if (Finding.FindingId.IsEmpty())
    {
        return false;
    }

    if (Finding.Severity.Equals(TEXT("Critical"), ESearchCase::IgnoreCase)
        || Finding.Severity.Equals(TEXT("High"), ESearchCase::IgnoreCase))
    {
        return true;
    }

    if (Finding.Severity.Equals(TEXT("Medium"), ESearchCase::IgnoreCase))
    {
        return Finding.bCanWanaWorksActNow
            || Finding.bRequiresManualEngineAction
            || Finding.DiagnosisClassification.Equals(TEXT("Possible Risk"), ESearchCase::IgnoreCase)
            || Finding.DiagnosisClassification.Equals(TEXT("Requires Manual Validation"), ESearchCase::IgnoreCase)
            || Finding.DiagnosisClassification.Equals(TEXT("Confirmed Gap"), ESearchCase::IgnoreCase);
    }

    if (Finding.Severity.Equals(TEXT("Low"), ESearchCase::IgnoreCase))
    {
        return Finding.bCanWanaWorksActNow
            || Finding.bRequiresManualEngineAction
            || Finding.DiagnosisClassification.Equals(TEXT("Confirmed Gap"), ESearchCase::IgnoreCase);
    }

    // Informational readiness does not become a repair task. These two classes are retained only
    // when they describe a concrete validation route or an honestly unsupported check.
    return Finding.DiagnosisClassification.Equals(TEXT("Requires Manual Validation"), ESearchCase::IgnoreCase)
        || Finding.DiagnosisClassification.Equals(TEXT("Unsupported Check"), ESearchCase::IgnoreCase);
}

FString ResolveCorrectionPlanTitle(const FWanaProjectHealthFinding& Finding)
{
    if (Finding.FindingId.StartsWith(TEXT("ModuleEditorDependencyContext_")))
    {
        return TEXT("Validate Module Dependency Context");
    }
    if (Finding.FindingId.StartsWith(TEXT("ModuleEditorDependency_")))
    {
        return TEXT("Correct Runtime/Editor Module Boundary");
    }
    if (Finding.FindingId.StartsWith(TEXT("PluginUnavailable_")))
    {
        return TEXT("Restore Required Plugin Availability");
    }
    if (Finding.FindingId.StartsWith(TEXT("DuplicateProjectModule_"))
        || Finding.FindingId.StartsWith(TEXT("DuplicateWanaWorksModule_")))
    {
        return TEXT("Correct Duplicate Module Declaration");
    }
    if (Finding.FindingId.StartsWith(TEXT("WanaWorksModuleUnavailable_"))
        || Finding.FindingId.StartsWith(TEXT("ProjectModuleMetadataUnavailable_")))
    {
        return TEXT("Validate Declared Module Availability");
    }
    if (Finding.FindingId.Equals(TEXT("ProjectDescriptorUnavailable"), ESearchCase::IgnoreCase))
    {
        return TEXT("Restore Project Descriptor Context");
    }
    if (Finding.FindingId.Equals(TEXT("WanaWorksPluginDescriptorUnavailable"), ESearchCase::IgnoreCase))
    {
        return TEXT("Restore WanaWorks Plugin Descriptor Context");
    }
    if (Finding.FindingId.StartsWith(TEXT("EngineVersion")))
    {
        return TEXT("Validate Unreal Version Compatibility");
    }
    if (Finding.FindingId.Equals(TEXT("CurrentWorldWITReportMismatch"), ESearchCase::IgnoreCase))
    {
        return TEXT("Regenerate WIT Context for the Current Map");
    }
    if (Finding.FindingId.Equals(TEXT("CurrentWorldWITReportMissing"), ESearchCase::IgnoreCase))
    {
        return TEXT("Generate WIT Context for the Current Map");
    }
    if (Finding.FindingId.Equals(TEXT("WanaAnimationAdapterReportsMissing"), ESearchCase::IgnoreCase))
    {
        return TEXT("Generate a WanaAnimation Adapter Report");
    }
    if (Finding.FindingId.Equals(TEXT("CurrentWorldContextUnavailable"), ESearchCase::IgnoreCase))
    {
        return TEXT("Open a Map for Output Validation");
    }

    return Finding.Title.IsEmpty() ? TEXT("Review Project Health Finding") : Finding.Title;
}

void ApplyCorrectionPlanClassification(
    const FWanaProjectHealthFinding& Finding,
    FWanaProjectHealthCorrectionPlan& Plan)
{
    const bool bModuleDependency = Finding.FindingId.StartsWith(TEXT("ModuleEditorDependency_"));
    const bool bUncertainModuleDependency = Finding.FindingId.StartsWith(TEXT("ModuleEditorDependencyContext_"));
    const bool bPluginState = Finding.FindingId.StartsWith(TEXT("PluginUnavailable_"));
    const bool bDescriptorOrModuleMetadata = Finding.FindingId.Contains(TEXT("Descriptor"))
        || Finding.FindingId.StartsWith(TEXT("DuplicateProjectModule_"))
        || Finding.FindingId.StartsWith(TEXT("DuplicateWanaWorksModule_"))
        || Finding.FindingId.StartsWith(TEXT("WanaWorksModuleUnavailable_"))
        || Finding.FindingId.StartsWith(TEXT("ProjectModuleMetadataUnavailable_"));
    const bool bWITOutput = Finding.FindingId.StartsWith(TEXT("CurrentWorldWITReport"));
    const bool bAnimationAdapterOutput = Finding.FindingId.Equals(TEXT("WanaAnimationAdapterReportsMissing"), ESearchCase::IgnoreCase);
    const bool bEngineCompatibility = Finding.FindingId.StartsWith(TEXT("EngineVersion"));
    const bool bUnsupportedCheck = Finding.DiagnosisClassification.Equals(TEXT("Unsupported Check"), ESearchCase::IgnoreCase);
    const bool bManualValidation = Finding.DiagnosisClassification.Equals(TEXT("Requires Manual Validation"), ESearchCase::IgnoreCase)
        || bUncertainModuleDependency;

    if (bUncertainModuleDependency || bEngineCompatibility)
    {
        Plan.CorrectionType = TEXT("Manual Validation");
    }
    else if (bModuleDependency)
    {
        Plan.CorrectionType = TEXT("Module Boundary Change");
    }
    else if (bPluginState)
    {
        Plan.CorrectionType = TEXT("Plugin State Change");
    }
    else if (bDescriptorOrModuleMetadata)
    {
        Plan.CorrectionType = TEXT("Configuration Change");
    }
    else if (bWITOutput || bAnimationAdapterOutput)
    {
        Plan.CorrectionType = TEXT("Asset or Output Regeneration");
    }
    else if (bUnsupportedCheck)
    {
        Plan.CorrectionType = TEXT("Unsupported");
    }
    else
    {
        Plan.CorrectionType = TEXT("Workflow Change");
    }

    Plan.bRequiresManualEngineAction = Finding.bRequiresManualEngineAction
        || Finding.RecommendedWorkflowStep.Equals(TEXT("Manual Unreal Step"), ESearchCase::IgnoreCase);
    Plan.bRequiresDeveloperApproval = Plan.CorrectionType.Equals(TEXT("Configuration Change"), ESearchCase::IgnoreCase)
        || Plan.CorrectionType.Equals(TEXT("Module Boundary Change"), ESearchCase::IgnoreCase)
        || Plan.CorrectionType.Equals(TEXT("Dependency Change"), ESearchCase::IgnoreCase)
        || Plan.CorrectionType.Equals(TEXT("Plugin State Change"), ESearchCase::IgnoreCase);
    Plan.bFutureAutomationCandidate = !bUnsupportedCheck
        && !bManualValidation
        && !Finding.bCanWanaWorksActNow
        && (Plan.CorrectionType.Equals(TEXT("Configuration Change"), ESearchCase::IgnoreCase)
            || Plan.CorrectionType.Equals(TEXT("Module Boundary Change"), ESearchCase::IgnoreCase));

    if (bUnsupportedCheck)
    {
        Plan.ExecutionCapability = TEXT("Unsupported");
        Plan.CurrentPlanState = TEXT("Unsupported");
    }
    else if (Finding.bCanWanaWorksActNow && !Plan.bRequiresManualEngineAction)
    {
        Plan.ExecutionCapability = TEXT("Existing WanaWorks Workflow");
        Plan.CurrentPlanState = TEXT("Proposed");
    }
    else if (Plan.bRequiresManualEngineAction)
    {
        Plan.ExecutionCapability = TEXT("Manual Engine Step");
        Plan.CurrentPlanState = Plan.bRequiresDeveloperApproval ? TEXT("Requires Approval") : TEXT("Manual Only");
    }
    else if (Plan.bFutureAutomationCandidate)
    {
        Plan.ExecutionCapability = TEXT("Future WanaWorks Automation");
        Plan.CurrentPlanState = Plan.bRequiresDeveloperApproval ? TEXT("Requires Approval") : TEXT("Future Automation Candidate");
    }
    else
    {
        Plan.ExecutionCapability = TEXT("Guidance Only");
        Plan.CurrentPlanState = bManualValidation ? TEXT("Manual Only") : TEXT("Proposed");
    }

    Plan.EstimatedImplementationRisk = Finding.Severity.Equals(TEXT("Critical"), ESearchCase::IgnoreCase) ? TEXT("Critical")
        : Finding.Severity.Equals(TEXT("High"), ESearchCase::IgnoreCase) ? TEXT("High")
        : Finding.Severity.Equals(TEXT("Medium"), ESearchCase::IgnoreCase) ? TEXT("Medium")
        : Finding.Severity.Equals(TEXT("Low"), ESearchCase::IgnoreCase) ? TEXT("Low")
        : TEXT("Minimal");
    Plan.EstimatedValidationRisk = bModuleDependency || bPluginState || bEngineCompatibility
        ? TEXT("High")
        : (bDescriptorOrModuleMetadata ? TEXT("Medium") : TEXT("Low"));

    if (bUnsupportedCheck || bEngineCompatibility && Finding.Severity.Equals(TEXT("High"), ESearchCase::IgnoreCase))
    {
        Plan.EstimatedImplementationRisk = TEXT("Unknown");
    }
}

void ApplyCorrectionPlanGuidance(
    const FWanaProjectHealthFinding& Finding,
    const FWanaProjectHealthScanResult& ScanResult,
    FWanaProjectHealthCorrectionPlan& Plan)
{
    const bool bUncertainModuleDependency = Finding.FindingId.StartsWith(TEXT("ModuleEditorDependencyContext_"));
    const bool bModuleDependency = Finding.FindingId.StartsWith(TEXT("ModuleEditorDependency_"));
    const bool bPluginState = Finding.FindingId.StartsWith(TEXT("PluginUnavailable_"));
    const bool bDescriptorOrModuleMetadata = Finding.FindingId.Contains(TEXT("Descriptor"))
        || Finding.FindingId.StartsWith(TEXT("DuplicateProjectModule_"))
        || Finding.FindingId.StartsWith(TEXT("DuplicateWanaWorksModule_"))
        || Finding.FindingId.StartsWith(TEXT("WanaWorksModuleUnavailable_"))
        || Finding.FindingId.StartsWith(TEXT("ProjectModuleMetadataUnavailable_"));
    const bool bWITOutput = Finding.FindingId.StartsWith(TEXT("CurrentWorldWITReport"));
    const bool bAnimationAdapterOutput = Finding.FindingId.Equals(TEXT("WanaAnimationAdapterReportsMissing"), ESearchCase::IgnoreCase);
    const bool bEngineCompatibility = Finding.FindingId.StartsWith(TEXT("EngineVersion"));

    Plan.ProposedCorrection = Finding.RecommendedRoute;
    Plan.CorrectionRationale = Finding.RootCause.IsEmpty()
        ? TEXT("The proposed route addresses the detected condition while keeping all changes approval-gated and independently verifiable.")
        : FString::Printf(TEXT("%s The proposed route limits change to the affected project area and requires validation before broader adoption."), *Finding.RootCause);
    Plan.ValidationSteps = TEXT("Re-run Project Health Test to confirm the condition still exists, then perform the validation required by the affected Unreal system.");
    Plan.RollbackGuidance = TEXT("No change was made by this plan. If a future approved correction is performed, restore the prior source-controlled project state if validation fails.");

    if (bUncertainModuleDependency)
    {
        Plan.ProposedCorrection = TEXT("Manually review the cited Build.cs declaration and enclosing target conditions. Do not propose source changes until the dependency context is confirmed.");
        Plan.CorrectionRationale = TEXT("A bounded text match cannot prove how custom C# Build.cs logic evaluates, so validation must precede any source or module-layout proposal.");
        Plan.ValidationSteps = TEXT("Inspect the cited declaration and conditional scope; confirm the module host type; only if the dependency is active for runtime targets, compile Editor and runtime/game targets before packaging.");
        Plan.RollbackGuidance = TEXT("Validation-only plan; no rollback is required because no source change is proposed.");
        Plan.Limitations = TEXT("Manual validation required before proposing source changes. Build.cs interpretation is bounded and is not a complete C# parse.");
    }
    else if (bModuleDependency)
    {
        Plan.ValidationSteps = TEXT("Compile the Editor target; compile the runtime/game target where available; verify plugin loading and editor tools; run packaging validation separately.");
        Plan.RollbackGuidance = TEXT("If a future approved module-boundary change fails validation, restore the previous dependency declaration and code placement from source control.");
        Plan.Limitations = TEXT("The dependency declaration is high-confidence evidence, but code-level usage and the safest destination Editor module still require manual review.");
    }
    else if (bPluginState)
    {
        Plan.ProposedCorrection = TEXT("Confirm the project still requires the declared plugin, then install or enable the matching plugin version manually. Restart Unreal only if the editor requests it.");
        Plan.CorrectionRationale = TEXT("The project explicitly declares the plugin as enabled; validating that requirement before changing plugin state avoids enabling an incorrect or obsolete dependency.");
        Plan.ValidationSteps = TEXT("Confirm the plugin resolves in Unreal; restart if requested; compile the Editor target; open representative dependent assets; run packaging validation separately.");
        Plan.RollbackGuidance = TEXT("If a future approved plugin-state change causes regressions, restore the prior project/plugin configuration from source control and reopen the editor.");
    }
    else if (bEngineCompatibility)
    {
        Plan.ProposedCorrection = Finding.Severity.Equals(TEXT("High"), ESearchCase::IgnoreCase)
            ? TEXT("Identify the exact Unreal release and compare it with documented WanaWorks support. Use a compatible WanaWorks build or adapter only after manual review; do not migrate automatically.")
            : TEXT("Preserve the current project state, compile WanaWorks against the detected release, test each active workspace, and validate packaging separately before adopting or shipping with this engine version.");
        Plan.CorrectionRationale = TEXT("Engine family recognition is not proof that every version-sensitive editor API, module, serialization path, or target configuration is compatible.");
        Plan.ValidationSteps = TEXT("Compile the Editor target; exercise Character Intelligence, Character Building, Level Design, and Project Health; validate representative assets; run packaging validation separately.");
        Plan.RollbackGuidance = TEXT("Validation-only plan; keep production pinned to its previously validated engine/WanaWorks pairing until the detected release passes review.");
        Plan.Limitations = TEXT("No exhaustive compatibility table is available, so this plan cannot recommend an upgrade or downgrade.");
    }
    else if (bWITOutput)
    {
        Plan.ProposedCorrection = TEXT("Open Level Design for the current map, run Test, then Build to create a fresh persistent WIT report for that map.");
        Plan.CorrectionRationale = TEXT("Using the established WanaWorks-owned report workflow restores environment context without modifying map geometry, collision, actors, or NavMesh.");
        Plan.ValidationSteps = TEXT("Return to Character Intelligence; run Analyze or Test; confirm the WIT report is readable, matches the current map, and exposes environment influence.");
        Plan.RollbackGuidance = TEXT("If the report is not valid, regenerate or remove only the WanaWorks-owned report; the original map remains unchanged.");
    }
    else if (bAnimationAdapterOutput)
    {
        Plan.ProposedCorrection = TEXT("Select the intended subject and run Build in Character Building or Character Intelligence to create or refresh its WanaWorks-owned adapter report. Do not edit the original Anim BP.");
        Plan.CorrectionRationale = TEXT("The existing adapter-report workflow records animation integration readiness without replacing Anim Classes or changing user animation graphs.");
        Plan.ValidationSteps = TEXT("Run Test in the subject's character workspace; confirm the report matches the subject and hook/runtime-adapter readiness is readable.");
        Plan.RollbackGuidance = TEXT("Remove or regenerate only the WanaWorks-owned adapter report if it is stale; original character and animation assets remain untouched.");
    }
    else if (Finding.FindingId.Equals(TEXT("CurrentWorldContextUnavailable"), ESearchCase::IgnoreCase))
    {
        Plan.ProposedCorrection = TEXT("Open the intended editor level and run Project Health Analyze again before proposing any current-map output correction.");
        Plan.CorrectionRationale = TEXT("Current-map WIT readiness cannot be evaluated safely without an editor world and package context.");
        Plan.ValidationSteps = TEXT("Confirm the intended level is open, rerun Analyze, and review the refreshed current-map WIT diagnosis.");
        Plan.RollbackGuidance = TEXT("No project change is proposed, so no rollback is required.");
        Plan.Limitations = TEXT("Unsupported until a valid editor world is available.");
    }
    else if (bDescriptorOrModuleMetadata)
    {
        Plan.ValidationSteps = TEXT("Review the cited descriptor/module path; compile the Editor target; verify the declared plugin and module load; run packaging validation separately if metadata changes are later approved.");
        Plan.RollbackGuidance = TEXT("If a future approved metadata change fails validation, restore the prior descriptor or module layout from source control.");
    }

    if (Plan.ProposedCorrection.IsEmpty())
    {
        Plan.ProposedCorrection = TEXT("Review the source evidence manually and define a correction only after the affected Unreal system and project area are confirmed.");
    }
    if (Plan.Limitations.IsEmpty())
    {
        Plan.Limitations = ScanResult.ScanLimitationsText.IsEmpty()
            ? TEXT("Read-only proposal based on current project state; no correction was executed and no historical resolution state is available.")
            : FString::Printf(TEXT("Read-only proposal; no correction was executed. Scan limitation: %s"), *ScanResult.ScanLimitationsText);
    }
}

FWanaProjectHealthCorrectionPlan BuildCorrectionPlan(
    const FWanaProjectHealthFinding& Finding,
    const FWanaProjectHealthScanResult& ScanResult)
{
    FWanaProjectHealthCorrectionPlan Plan;
    Plan.PlanId = FString::Printf(TEXT("Correction_%s"), *Finding.FindingId);
    Plan.SourceFindingId = Finding.FindingId;
    Plan.Title = ResolveCorrectionPlanTitle(Finding);
    Plan.Summary = Finding.RecommendedRoute.IsEmpty() ? Finding.ProblemDescription : Finding.RecommendedRoute;
    Plan.SourceSeverity = Finding.Severity.IsEmpty() ? TEXT("Unknown") : Finding.Severity;
    const bool bSupportedEngineAdapter = Finding.EngineAdapterId.IsEmpty()
        || Finding.EngineAdapterId.Equals(TEXT("Unreal"), ESearchCase::IgnoreCase);
    Plan.EngineAdapterId = TEXT("Unreal");
    Plan.DiagnosisCategory = Finding.Category;
    Plan.DiagnosisConfidence = Finding.Confidence.IsEmpty() ? TEXT("Unknown") : Finding.Confidence;
    Plan.SourceEvidence = Finding.Evidence.IsEmpty() ? TEXT("No additional evidence was captured by this bounded check.") : Finding.Evidence;
    Plan.DetectedCondition = Finding.ProblemDescription;
    Plan.AffectedSystems = Finding.AffectedSystem;
    Plan.RelatedFilesOrProjectAreas = Finding.RelatedPath.IsEmpty() ? Finding.AffectedSystem : Finding.RelatedPath;
    Plan.PreventionGuidance = Finding.PreventionGuidance;

    ApplyCorrectionPlanClassification(Finding, Plan);
    ApplyCorrectionPlanGuidance(Finding, ScanResult, Plan);
    if (!bSupportedEngineAdapter)
    {
        Plan.Limitations += FString::Printf(
            TEXT(" Source engine adapter '%s' is not supported by this implementation; the proposal remains Unreal guidance only."),
            *Finding.EngineAdapterId);
    }
    return Plan;
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

FWanaProjectHealthScanResult WanaWorksProjectHealthActions::RunFullScan(const FWanaProjectHealthScanContext& ScanContext)
{
    FWanaProjectHealthScanResult Result = BuildBaseResult();
    const FProjectDescriptor* ProjectDescriptor = IProjectManager::Get().GetCurrentProject();
    InspectWanaWorksPluginDescriptor(Result);

    if (!ProjectDescriptor)
    {
        AddFinding(
            Result.Findings,
            TEXT("ProjectDescriptorUnavailable"),
            TEXT("Project Descriptor Unavailable"),
            TEXT("Modules and Dependencies"),
            TEXT("High"),
            TEXT("Confirmed Risk"),
            TEXT("IProjectManager did not provide the current project descriptor, so Project Health could not inspect project modules or declared plugins."),
            TEXT("The editor has no readable current-project descriptor context or project metadata is not fully initialized."),
            TEXT("Project metadata scan"),
            TEXT("Module and plugin consistency checks are incomplete; Project Health cannot estimate metadata readiness."),
            TEXT("Confirm the project is opened from a valid .uproject and restart the editor."),
            TEXT("Manual Unreal Step"),
            TEXT("Keep the .uproject valid and open the project through its descriptor after moving it."),
            TEXT("Confirmed - IProjectManager::GetCurrentProject returned null."),
            TEXT("No current FProjectDescriptor was available in memory."),
            false,
            true,
            TEXT("Detected"));
        AppendOutputReadinessFindings(Result, ScanContext);
        TallySeverity(Result);
        Result.bScanCompleted = false;
        Result.OverallStatusLabel = TEXT("Scan Failed Safely");
        Result.BuildReadinessStatus = TEXT("Packaging Confidence Limited");
        Result.ScanLimitationsText = TEXT("The current project descriptor was unavailable, so module and plugin checks could not run. Engine version and WanaWorks plugin version are still reported above where available.");
        return Result;
    }

    TArray<FString> Limitations;
    Limitations.Add(TEXT("Build.cs review is bounded heuristic analysis, not a complete C# parser."));
    Limitations.Add(TEXT("Editor-type modules are excluded from runtime-boundary warnings because editor-only dependencies are normally valid there."));
    Limitations.Add(TEXT("Project Health does not compile or package the project."));

    // Engine compatibility finding.
    if (FEngineVersion::Current().GetMajor() != 5)
    {
        AddFinding(
            Result.Findings,
            TEXT("EngineVersionUnvalidated"),
            TEXT("Validate Engine Compatibility"),
            TEXT("Engine Compatibility"),
            TEXT("High"),
            TEXT("Unsupported Check"),
            FString::Printf(TEXT("The current engine version (%s) is outside the UE5 family recognized by this WanaWorks build."), *Result.EngineVersionLabel),
            TEXT("WanaWorks has no compatibility table or validation evidence for this engine family."),
            TEXT("WanaWorks plugin"),
            TEXT("WanaWorks editor integration, module loading, or generated outputs may be incompatible; no packaging conclusion can be made."),
            TEXT("Use a documented supported engine version or manually validate editor startup, compilation, and packaging before production use."),
            TEXT("Manual Unreal Step"),
            TEXT("Review compatibility notes and test upgrades on a branch or project copy before changing the production engine version."),
            TEXT("High for exact version detection; compatibility is unsupported by current evidence."),
            FString::Printf(TEXT("FEngineVersion::Current() reports %s; WanaWorks plugin version is %s."), *Result.EngineVersionLabel, *Result.WanaWorksPluginVersionLabel),
            false,
            true,
            TEXT("Not Supported"));
    }
    else
    {
        AddFinding(
            Result.Findings,
            TEXT("EngineVersionRecognized"),
            TEXT("Engine Version Recognized"),
            TEXT("Engine Compatibility"),
            TEXT("Informational"),
            TEXT("Requires Manual Validation"),
            FString::Printf(TEXT("Detected Unreal Engine %s in the recognized UE5 family; this exact point release is not claimed as exhaustively validated."), *Result.EngineVersionLabel),
            TEXT("No exhaustive exact-release compatibility table exists in current WanaWorks project documentation."),
            TEXT("WanaWorks plugin"),
            TEXT("Version-sensitive editor APIs, module loading, asset serialization, and packaging still require normal validation after engine upgrades."),
            TEXT("Run the normal editor build and a representative package smoke test before shipping or upgrading production."),
            TEXT("Manual Unreal Step"),
            TEXT("Upgrade Unreal on a branch or project copy, rebuild plugins, open representative assets, and validate packaging before adopting the release."),
            TEXT("High for exact version detection; conservative for full compatibility."),
            FString::Printf(TEXT("FEngineVersion::Current() reports %s; WanaWorks plugin version is %s."), *Result.EngineVersionLabel, *Result.WanaWorksPluginVersionLabel),
            false,
            false,
            TEXT("Ready"));
    }

    // Plugin dependency / enabled-state cross-check.
    int32 DisabledOrMissingRequiredPlugins = 0;
    int32 EnabledProjectPluginCount = 0;

    for (const FPluginReferenceDescriptor& PluginReference : ProjectDescriptor->Plugins)
    {
        if (!PluginReference.bEnabled)
        {
            continue;
        }
        ++EnabledProjectPluginCount;

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
                TEXT("Confirmed Risk"),
                FString::Printf(TEXT("The project declares \"%s\" with Enabled=true, but it is not currently enabled or could not be resolved."), *PluginReference.Name),
                TEXT("A plugin required by the current project descriptor is missing, disabled, or unavailable to this editor session."),
                PluginReference.Name,
                TEXT("Features, assets, modules, editor startup, or packaging that depend on the plugin may fail or load incompletely."),
                TEXT("Confirm the plugin is installed and enable it through Unreal's Plugins browser; Project Health does not change plugin state."),
                TEXT("Manual Unreal Step"),
                TEXT("Verify required plugins after engine upgrades, project transfers, and source-control merges."),
                TEXT("Confirmed from loaded project and plugin-manager metadata."),
                FString::Printf(TEXT(".uproject declares \"%s\" Enabled=true; IPluginManager reports %s."), *PluginReference.Name, ResolvedPlugin.IsValid() ? TEXT("disabled") : TEXT("not found")),
                false,
                true,
                TEXT("Detected"));
        }
    }

    if (DisabledOrMissingRequiredPlugins == 0 && EnabledProjectPluginCount > 0)
    {
        AddFinding(
            Result.Findings,
            TEXT("AllRequiredPluginsReady"),
            TEXT("Required Project Plugins Ready"),
            TEXT("Modules and Dependencies"),
            TEXT("Informational"),
            TEXT("Informational"),
            FString::Printf(TEXT("All %d project plugin(s) declared with Enabled=true are available and enabled."), EnabledProjectPluginCount),
            TEXT("Loaded project plugin requirements and current plugin-manager state agree."),
            TEXT("Project plugins"),
            TEXT("No missing required-plugin blocker was detected from current metadata; target-specific behavior still requires normal validation."),
            TEXT("Re-check Project Health after engine or plugin upgrades."),
            TEXT("Test"),
            TEXT("Keep required plugin declarations synchronized with installed plugin versions."),
            TEXT("Confirmed from the loaded .uproject plugin list and IPluginManager."),
            TEXT("Every Enabled=true project plugin reference resolved to an enabled IPlugin."),
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

    TSet<FName> SeenProjectModuleNames;
    for (int32 Index = 0; Index < ModulesToScan; ++Index)
    {
        const FModuleDescriptor& ModuleDescriptor = ProjectDescriptor->Modules[Index];
        const FString ModuleName = ModuleDescriptor.Name.ToString();
        if (ModuleName.IsEmpty())
        {
            continue;
        }

        if (SeenProjectModuleNames.Contains(ModuleDescriptor.Name))
        {
            AddFinding(
                Result.Findings,
                FString::Printf(TEXT("DuplicateProjectModule_%s"), *ModuleName),
                FString::Printf(TEXT("Duplicate Project Module: %s"), *ModuleName),
                TEXT("Modules and Dependencies"),
                TEXT("High"),
                TEXT("Confirmed Risk"),
                FString::Printf(TEXT("The project descriptor lists module \"%s\" more than once."), *ModuleName),
                TEXT("Duplicate project module declarations create ambiguous build metadata."),
                ModuleName,
                TEXT("UnrealBuildTool target generation or module loading may fail or behave inconsistently."),
                TEXT("Remove the duplicate module entry after manual descriptor review."),
                TEXT("Manual Unreal Step"),
                TEXT("Keep module names unique and review .uproject merge conflicts when adding modules."),
                TEXT("Confirmed from the loaded project descriptor."),
                FString::Printf(TEXT(".uproject contains repeated module name %s."), *ModuleName),
                false,
                true,
                TEXT("Detected"));
            continue;
        }
        SeenProjectModuleNames.Add(ModuleDescriptor.Name);

        const FString BuildCsPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), ModuleName, ModuleName + TEXT(".Build.cs")));
        const bool bBuildCsExists = FPaths::FileExists(BuildCsPath);
        const bool bModuleAvailable = FModuleManager::Get().ModuleExists(*ModuleName);

        if (!bBuildCsExists || !bModuleAvailable)
        {
            AddFinding(
                Result.Findings,
                FString::Printf(TEXT("ProjectModuleMetadataUnavailable_%s"), *ModuleName),
                FString::Printf(TEXT("Validate Project Module Metadata: %s"), *ModuleName),
                TEXT("Modules and Dependencies"),
                bBuildCsExists ? TEXT("Medium") : TEXT("Low"),
                TEXT("Requires Manual Validation"),
                FString::Printf(TEXT("Project module \"%s\" is declared, but %s."), *ModuleName, !bBuildCsExists ? TEXT("the conventional Build.cs path was not found") : TEXT("the current module manager cannot resolve it for this editor target")),
                TEXT("The module may use a nonstandard source layout, be excluded from the active target, or have incomplete build metadata."),
                ModuleName,
                TEXT("Project Health cannot validate its dependency boundary; a genuinely unavailable module may cause compile, startup, or packaging errors."),
                TEXT("Review the module descriptor entry, target inclusion, and actual Build.cs location manually."),
                TEXT("Manual Unreal Step"),
                TEXT("Keep project module names, Source folders, Build.cs files, and target rules synchronized."),
                TEXT("Medium - descriptor state is confirmed, but nonstandard layouts and target filters are valid possibilities."),
                FString::Printf(TEXT(".uproject declares %s; expected Build.cs=%s (%s); ModuleExists=%s."), *ModuleName, *BuildCsPath, bBuildCsExists ? TEXT("present") : TEXT("missing"), bModuleAvailable ? TEXT("true") : TEXT("false")),
                false,
                true,
                TEXT("Needs Manual Validation"),
                BuildCsPath);
        }

        if (bBuildCsExists)
        {
            ScanModuleBuildCsForEditorOnlyDependencies(ModuleDescriptor, BuildCsPath, Result.Findings, Limitations);
        }
    }

    const bool bFoundModuleDependencyIssue = Result.Findings.ContainsByPredicate([](const FWanaProjectHealthFinding& Finding)
    {
        return Finding.Category.Equals(TEXT("Modules and Dependencies"), ESearchCase::IgnoreCase)
            && !Finding.Severity.Equals(TEXT("Informational"), ESearchCase::IgnoreCase);
    });

    if (!bFoundModuleDependencyIssue && ProjectDescriptor->Modules.Num() > 0)
    {
        AddFinding(
            Result.Findings,
            TEXT("ModuleBoundariesClean"),
            TEXT("No Confirmed Module Blockers"),
            TEXT("Modules and Dependencies"),
            TEXT("Informational"),
            TEXT("Informational"),
            TEXT("The bounded scan found no active, unguarded editor-only dependency declaration or confirmed module metadata blocker in the reviewed project modules."),
            TEXT("Project descriptor metadata and reviewed Build.cs contexts did not expose a confirmed runtime/editor boundary violation."),
            TEXT("Project modules"),
            TEXT("No metadata blocker is confirmed; custom Build.cs logic, C++ usage, target rules, and packaging remain outside this check."),
            TEXT("Continue with the normal editor build and a package smoke test for deeper validation."),
            TEXT("Test"),
            TEXT("Keep editor code in Editor modules and maintain explicit target guards around unavoidable editor-only dependencies."),
            TEXT("High for the bounded checks performed; not proof of complete build correctness."),
            FString::Printf(TEXT("Reviewed up to %d module descriptor(s); comments were excluded and Editor modules were not treated as runtime violations."), ModulesToScan),
            false,
            false,
            TEXT("Ready"));
    }

    AppendOutputReadinessFindings(Result, ScanContext);
    Result.bScanCompleted = true;
    Result.ScanLimitationsText = FString::Join(Limitations, TEXT(" "));

    TallySeverity(Result);
    return Result;
}

TArray<FWanaProjectHealthCorrectionPlan> WanaWorksProjectHealthActions::BuildCorrectionPlans(const FWanaProjectHealthScanResult& ScanResult)
{
    TArray<FWanaProjectHealthCorrectionPlan> Plans;
    Plans.Reserve(FMath::Min(ScanResult.Findings.Num(), MaxCorrectionPlans));

    for (const FWanaProjectHealthFinding& Finding : ScanResult.Findings)
    {
        if (Plans.Num() >= MaxCorrectionPlans || !IsCorrectionPlanEligible(Finding))
        {
            continue;
        }

        FWanaProjectHealthCorrectionPlan Plan = BuildCorrectionPlan(Finding, ScanResult);
        if (Plan.PlanId.IsEmpty() || Plans.ContainsByPredicate([&Plan](const FWanaProjectHealthCorrectionPlan& ExistingPlan)
        {
            return ExistingPlan.PlanId.Equals(Plan.PlanId, ESearchCase::IgnoreCase);
        }))
        {
            continue;
        }

        Plans.Add(MoveTemp(Plan));
    }

    return Plans;
}

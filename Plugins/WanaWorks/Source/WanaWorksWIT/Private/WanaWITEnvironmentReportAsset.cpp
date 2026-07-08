#include "WanaWITEnvironmentReportAsset.h"

UWanaWITEnvironmentReportAsset::UWanaWITEnvironmentReportAsset()
{
    ReportVersion = TEXT("WIT Environment Report V1");
    GeneratedLabel = TEXT("WIT Environment Report");
    WorkspaceLabel = TEXT("Level Design");
    ScanMode = TEXT("No Context");
    ScanStatus = TEXT("Report Not Generated");
    ReadinessStatus = TEXT("Limited");
    NavigationContextStatus = TEXT("Needs NavMesh");
    SemanticSummary = TEXT("No WIT environment scan has been captured yet.");
    SceneInterpretation = TEXT("WanaWorks has not generated a semantic world report yet.");
    Limitations = TEXT("Generated report only. Original level geometry, actors, collision, and NavMesh are preserved.");
    RecommendedNextAction = TEXT("Run Level Design Build after a readable WIT scan.");
}

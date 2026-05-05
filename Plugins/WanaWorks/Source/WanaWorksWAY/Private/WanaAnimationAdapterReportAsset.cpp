#include "WanaAnimationAdapterReportAsset.h"

UWanaAnimationAdapterReportAsset::UWanaAnimationAdapterReportAsset()
{
    GeneratedLabel = TEXT("WanaAnimation Adapter Report");
    AdapterStatus = TEXT("Adapter Not Generated");
    AdapterReadinessState = TEXT("Limited");
    DirectGraphEditSafety = TEXT("Unsafe by policy");
    RecommendedIntegrationStrategy = TEXT("Generated Adapter");
    HookProviderStatus = TEXT("Needs Enhance");
    HookDataReadableStatus = TEXT("Needs Enhance");
    PhysicalStateProviderStatus = TEXT("Limited");
    SharedCharacterBlueprintStatus = TEXT("Unknown");
    SharedSkeletalMeshStatus = TEXT("Unknown");
    SharedSkeletonStatus = TEXT("Unknown");
    SharedAnimBlueprintStatus = TEXT("Unknown");
    Limitations = TEXT("Generated report only. Original animation assets are preserved.");
}

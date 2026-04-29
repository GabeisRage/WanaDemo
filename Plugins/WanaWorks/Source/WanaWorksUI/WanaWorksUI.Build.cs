using UnrealBuildTool;

public class WanaWorksUI : ModuleRules
{
    public WanaWorksUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "AssetRegistry",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Projects",
            "ToolMenus",
            "UnrealEd",
            "WorkspaceMenuStructure",
            "WanaWorksCore",
            "WanaWorksRender",
            "WanaWorksWAI",
            "WanaWorksWIT",
            "WanaWorksWAY",
            "Slate",
            "SlateCore"
        });
    }
}

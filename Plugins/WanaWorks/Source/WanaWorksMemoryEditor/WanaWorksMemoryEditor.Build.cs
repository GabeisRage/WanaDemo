using UnrealBuildTool;

public class WanaWorksMemoryEditor : ModuleRules
{
    public WanaWorksMemoryEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Editor module only. Shipping game targets do not load it.
        // Register it in WanaWorks.uplugin with Type "Editor" after a local compile.
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Projects",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "UnrealEd",
            "WorkspaceMenuStructure",
            "WanaWorksMemory"
        });
    }
}

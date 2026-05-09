using UnrealBuildTool;

public class WanaWorksUI : ModuleRules
{
    public WanaWorksUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "EditorStyle",
            "UnrealEd",
            "ToolMenus",
            "WanaWorksCore"
        });
    }
}

using UnrealBuildTool;

public class WanaWorksWIT : ModuleRules
{
    public WanaWorksWIT(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "WanaWorksCore"
        });
    }
}

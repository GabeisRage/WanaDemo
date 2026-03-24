using UnrealBuildTool;

public class WanaWorksWAI : ModuleRules
{
    public WanaWorksWAI(ReadOnlyTargetRules Target) : base(Target)
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

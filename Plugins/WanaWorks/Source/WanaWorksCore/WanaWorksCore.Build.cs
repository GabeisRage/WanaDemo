using UnrealBuildTool;

public class WanaWorksCore : ModuleRules
{
    public WanaWorksCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "GameplayTasks"
        });
    }
}

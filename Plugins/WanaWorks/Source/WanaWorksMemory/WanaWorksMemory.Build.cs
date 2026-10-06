using UnrealBuildTool;
using System.IO;

public class WanaWorksMemory : ModuleRules
{
    public WanaWorksMemory(ReadOnlyTargetRules Target) : base(Target)
    {
        // No PCH and no unity: this module mixes portable C++ with the vendored
        // SQLite amalgamation, and neither should inherit Unreal's check() macro.
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "HTTP",
            "WanaWorksCore",
            "WanaWorksWAI",
            "WanaWorksWAY"
        });

        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Portable"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "sqlite"));
    }
}

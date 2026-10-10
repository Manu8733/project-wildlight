using UnrealBuildTool;
using System.IO;
public class Wildlight : ModuleRules
{
    public Wildlight(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = false;
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "ApplicationCore" });
        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Core/include")));
    }
}

using UnrealBuildTool;
public class WildlightTarget : TargetRules
{
    public WildlightTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Wildlight");
    }
}

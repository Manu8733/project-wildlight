using UnrealBuildTool;
public class WildlightEditorTarget : TargetRules
{
    public WildlightEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Wildlight");
    }
}

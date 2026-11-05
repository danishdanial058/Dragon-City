using UnrealBuildTool;

public class WyrmhavenEditorTarget : TargetRules
{
	public WyrmhavenEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Wyrmhaven");
	}
}

using UnrealBuildTool;

public class WyrmhavenTarget : TargetRules
{
	public WyrmhavenTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Wyrmhaven");
	}
}

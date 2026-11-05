using UnrealBuildTool;

public class Wyrmhaven : ModuleRules
{
	public Wyrmhaven(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets code include "Rules/...", "Game/...", "World/..." and "UI/..." from the module root.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Json",
			"UMG",
			"Slate",
			"SlateCore"
		});
	}
}

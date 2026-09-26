using UnrealBuildTool;

public class CineCamToolsEditor : ModuleRules
{
	public CineCamToolsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"AssetTools",
			"DeveloperSettings",
			"PropertyEditor",
			"Settings",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd",
			"CineCamTools"
		});
	}
}

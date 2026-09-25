using UnrealBuildTool;

public class CineCamToolsShaders : ModuleRules
{
	public CineCamToolsShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"RenderCore",
			"RHI",
			"Renderer",
			"Projects"
		});
	}
}

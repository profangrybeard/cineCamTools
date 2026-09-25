using System.IO;
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

		// ScreenPass.h has lived in Renderer Private, Internal and Public across 5.x.
		// Remove these once the 5.8 location is confirmed.
		PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"));
		PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Internal"));
	}
}

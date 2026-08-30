using UnrealBuildTool;

public class KDataForge : ModuleRules
{
	public KDataForge(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core", "CoreUObject",
				"Engine",
				"Projects",
				"GameplayTags",
				"Json",
				"NetCore",
				"AssetRegistry",
				"ImageWrapper",
				"RenderCore", "RHI"
			});

		// Header stubs
		PublicDependencyModuleNames.AddRange(new[]
		{
			"DummyHeaders"
		});

		PublicDependencyModuleNames.AddRange(new[] { "FactoryGame", "SML" });

		PublicDependencyModuleNames.AddRange(new[] { "KDataForgeApi", "KDataForgeYaml" });
	}
}

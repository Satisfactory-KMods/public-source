using UnrealBuildTool;

public class KDataForgeEditorUI : ModuleRules
{
	public KDataForgeEditorUI(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core", "CoreUObject",
				"Engine",
				"InputCore",
				"UMG", "Slate", "SlateCore",
				"AppFramework", // SColorPicker (runtime module)
				"AssetRegistry",
				"FactoryGame", "SML",
				"KDataForgeApi", "KDataForgeYaml", "KDataForge"
			});
	}
}

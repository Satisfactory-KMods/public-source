using UnrealBuildTool;

public class KDataForgeApi : ModuleRules
{
	public KDataForgeApi(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core", "CoreUObject",
				"Engine"
			});
	}
}

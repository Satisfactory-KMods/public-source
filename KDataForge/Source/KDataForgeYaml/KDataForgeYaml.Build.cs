using System.IO;
using UnrealBuildTool;

public class KDataForgeYaml : ModuleRules
{
	public KDataForgeYaml(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;

		// rapidyaml is amalgamated into this module (ThirdParty/ryml/rapidyaml.hpp) and reports parse
		// errors through a callback that throws; exceptions stay contained inside this wrapper module —
		// the public surface only ever returns FKDFNode trees and error strings.
		bEnableExceptions = true;

		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty"));

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core", "CoreUObject",
				"KDataForgeApi"
			});
	}
}

using UnrealBuildTool;

public class SBS : ModuleRules
{
	public SBS(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "HTTP", "Json", "UMG",
			"FactoryGame", "SML", "KBFL", "KPrivateCodeLib"
		});
		PrivateDependencyModuleNames.AddRange(new[] { "NetCore", "DummyHeaders", "InputCore", "ApplicationCore" });
		if (Target.Platform == UnrealTargetPlatform.Win64 && Target.Type != TargetType.Server)
		{
			PublicSystemLibraries.Add("Advapi32.lib");
		}
	}
}

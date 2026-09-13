using UnrealBuildTool;

public class SBSEditor : ModuleRules
{
	public SBSEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "Kismet", "KismetCompiler", "BlueprintGraph", "Json", "HTTP", "SBS", "Projects" });
	}
}

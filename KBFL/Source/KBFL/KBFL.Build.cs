using UnrealBuildTool;

public class KBFL : ModuleRules
{
	public KBFL(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core", "CoreUObject",
				"Engine",
				"DeveloperSettings",
				"PhysicsCore",
				"InputCore",
				//"OnlineSubsystem", "OnlineSubsystemUtils", "OnlineSubsystemNull",
				//"SignificanceManager",
				"GeometryCollectionEngine",
				//"ChaosVehiclesCore", "ChaosVehicles", "ChaosSolverEngine",
				"AnimGraphRuntime",
				//"AkAudio",
				"AssetRegistry",
				"NavigationSystem",
				"ReplicationGraph",
				"AIModule",
				"GameplayTasks",
				"SlateCore", "Slate", "UMG",
				//"InstancedSplines",
				"RenderCore",
				"CinematicCamera",
				"Foliage",
				//"Niagara",
				"EnhancedInput",
				//"GameplayCameras",
				//"TemplateSequence",
				"Projects",
				"NetCore",
				"GameplayTags",
				"Json"
			});

		// FactoryGame plugins
		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"AbstractInstance"
				//"SignificanceISPC"
			});

		// Header stubs
		PublicDependencyModuleNames.AddRange(new[]
		{
			"DummyHeaders"
		});

		if (Target.Type == TargetRules.TargetType.Editor)
			PublicDependencyModuleNames.AddRange(new[]
			{
				/*"OnlineBlueprintSupport",*/ "AnimGraph"
			});

		// Editor-only: Content Browser right-click actions (ToolMenus) for KBFL module assets.
		if (Target.bBuildEditor)
			PrivateDependencyModuleNames.AddRange(new[]
			{
				"ToolMenus", "UnrealEd", "ContentBrowser"
			});

		PublicDependencyModuleNames.AddRange(new[] { "FactoryGame", "SML" });
	}
}
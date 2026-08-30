using UnrealBuildTool;

public class RSS : ModuleRules
{
	public RSS(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Json",
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"PhysicsCore",
				"InputCore",
				"OnlineSubsystem", "OnlineSubsystemNull", "OnlineSubsystemUtils",
				"SignificanceManager",
				"GeometryCollectionEngine",
				"ChaosVehiclesCore", "ChaosVehicles", "ChaosSolverEngine",
				"AnimGraphRuntime",
				"AkAudio",
				"AssetRegistry",
				"NavigationSystem",
				"ReplicationGraph",
				"AIModule",
				"GameplayTasks",
				"SlateCore", "Slate", "UMG",
				"RenderCore", "RHI",
				"ImageWrapper",
				"CinematicCamera",
				"Foliage",
				"Niagara",
				"EnhancedInput",
				"GameplayCameras",
				"TemplateSequence",
				"NetCore", "HTTP", "Sockets", "Networking",
				"GameplayTags"
			});

		// FactoryGame plugins
		PublicDependencyModuleNames.AddRange(new[]
		{
			"AbstractInstance",
			"SignificanceISPC"
		});

		// Header stubs
		PublicDependencyModuleNames.AddRange(new[]
		{
			"DummyHeaders"
		});

		if (Target.Type == TargetRules.TargetType.Editor)
			PublicDependencyModuleNames.AddRange(new[] { "OnlineBlueprintSupport", "AnimGraph" });
		PublicDependencyModuleNames.AddRange(new[] { "FactoryGame", "SML", "KBFL", "KPrivateCodeLib", "KAPI", "KUI" });
	}
}
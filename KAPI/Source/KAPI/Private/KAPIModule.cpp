#include "KAPIModule.h"

#include "Buildables/FGBuildableManufacturer.h"
#include "Patching/NativeHookManager.h"
#include "Subsystems/KAPIDataAssetSubsystem.h"

DEFINE_LOG_CATEGORY(LogKApi);
DEFINE_LOG_CATEGORY(LogKApiCDOHelper);

#pragma warning(disable : 4702) // intentional early-return dead code

//~ Begin FDefaultGameModuleImpl Interface
void FKAPIModule::StartupModule()
{
#if WITH_EDITOR
	return;
#endif
	GameFix();
}
//~ End FDefaultGameModuleImpl Interface

void FKAPIModule::GameFix()
{
	auto ManuHandler = [](AFGBuildableManufacturer* Manufacturer)
	{
		if (!IsValid(Manufacturer))
		{
			return;
		}
		UKAPIDataAssetSubsystem* Subsystem = UKAPIDataAssetSubsystem::Get(Manufacturer->GetWorld());
		if (!IsValid(Subsystem))
		{
			return;
		}
		Subsystem->ApplyManufacturerModifications(Manufacturer, nullptr);
	};

	SUBSCRIBE_METHOD_VIRTUAL(
		AFGBuildableManufacturer::BeginPlay, GetMutableDefault<AFGBuildableManufacturer>(),
		[](auto& Scope, AFGBuildableManufacturer* Manufacturer)
		{
			// mFluidStackSizeMultiplier has no SaveGame flag – instances may carry the pre-patch default (1).
			// Always read from the CDO to get the post-KBFL-patch value.
			const int32 FluidMultiplier =
				Manufacturer->GetClass()->GetDefaultObject<AFGBuildableManufacturer>()->mFluidStackSizeMultiplier;
			Manufacturer->mFluidStackSizeMultiplier = FluidMultiplier;
		});

	SUBSCRIBE_METHOD_VIRTUAL(AFGBuildableManufacturer::AssignInputAccessIndices,
							 GetMutableDefault<AFGBuildableManufacturer>(),
							 [](auto& Scope, AFGBuildableManufacturer* self, TSubclassOf<class UFGRecipe> recipe)
							 {
								 UKAPIDataAssetSubsystem* Subsystem = UKAPIDataAssetSubsystem::Get(self->GetWorld());
								 if (!IsValid(Subsystem))
								 {
									 return;
								 }

								 if (Subsystem->ApplyManufacturerAssign(self, recipe))
								 {
									 Scope.Override(true);
								 }
							 });

	SUBSCRIBE_METHOD_VIRTUAL_AFTER(AFGBuildableManufacturer::SetRecipe, GetMutableDefault<AFGBuildableManufacturer>(),
								   [](AFGBuildableManufacturer* self, TSubclassOf<class UFGRecipe> recipe)
								   {
									   UKAPIDataAssetSubsystem* Subsystem =
										   UKAPIDataAssetSubsystem::Get(self->GetWorld());
									   if (!IsValid(Subsystem))
									   {
										   return;
									   }
									   Subsystem->ApplyManufacturerModifications(self, recipe);
								   });

	SUBSCRIBE_METHOD_VIRTUAL_AFTER(AFGBuildableManufacturer::BeginPlay, GetMutableDefault<AFGBuildableManufacturer>(),
								   ManuHandler);
}

IMPLEMENT_GAME_MODULE(FKAPIModule, KAPI);

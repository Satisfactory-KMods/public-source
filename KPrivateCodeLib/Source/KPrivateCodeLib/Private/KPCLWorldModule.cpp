// Fill out your copyright notice in the Description page of Project Settings.


#include "KPCLWorldModule.h"

#include "FGGameState.h"
#include "Description/KPCLRegistryObject.h"
#include "Registry/ModContentRegistry.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(KBFLWorldModuleV2Log, Log, All);

DEFINE_LOG_CATEGORY(KBFLWorldModuleV2Log);

UKPCLWorldModule::UKPCLWorldModule()
	: Super()
{
}

void UKPCLWorldModule::InitPhase_Implementation()
{
	Super::InitPhase_Implementation();

	if (IsEasyNodesEnabled())
	{
		ApplyEasyNodes(mResearchTrees);
	}

	UKBFLAssetDataSubsystem* Subsystem = UKBFLAssetDataSubsystem::Get(GetWorld());
	UModContentRegistry* ModContentRegistry = UModContentRegistry::Get(GetWorld());

	if (IsValid(Subsystem) && IsValid(ModContentRegistry))
	{
		//Register default content
		TArray<TSubclassOf<UKPCLRegistryObject>> Objects;
		Subsystem->GetObjectsOfChilds_Internal({UKPCLRegistryObject::StaticClass()}, Objects);

		for (TSubclassOf<UKPCLRegistryObject> RegistryObjectClass : Objects)
		{
			if (UKPCLRegistryObject::ShouldRegister(RegistryObjectClass))
			{
				UKPCLRegistryObject* RegistryObject = RegistryObjectClass.GetDefaultObject();

				//Register Schematics
				for (TSubclassOf<UFGSchematic> Schematic : RegistryObject->GetSchematicsToRegister())
				{
					if (!IsValid(Schematic))
					{
						continue;
					}

					ModContentRegistry->RegisterSchematic(GetOwnerModReference(), Schematic);
				}

				//Register Recipes
				for (TSubclassOf<UFGRecipe> Recipe : RegistryObject->GetRecipesToRegister())
				{
					if (!IsValid(Recipe))
					{
						continue;
					}

					ModContentRegistry->RegisterRecipe(GetOwnerModReference(), Recipe);
				}
			}
		}
	}
}

bool UKPCLWorldModule::IsEasyNodesEnabled_Implementation()
{
	return mUseEasyNodes;
}

void UKPCLWorldModule::ApplyEasyNodes_Implementation(const TArray<TSubclassOf<UFGResearchTree>>& Nodes)
{
}

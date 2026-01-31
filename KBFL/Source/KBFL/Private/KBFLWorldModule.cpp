// Fill out your copyright notice in the Description page of Project Settings.


#include "KBFLWorldModule.h"

#include "KBFLLogging.h"
#include "Registry/ModContentRegistry.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLCustomizerSubsystem.h"
#include "Subsystems/KBFLResourceNodeSubsystem.h"

UKBFLWorldModule::UKBFLWorldModule()
{
	bRootModule = false;

	mCDOInformationMap.Add(ELifecyclePhase::CONSTRUCTION, FKBFLCDOInformation());
	mCDOInformationMap.Add(ELifecyclePhase::INITIALIZATION, FKBFLCDOInformation());
	mCDOInformationMap.Add(ELifecyclePhase::POST_INITIALIZATION, FKBFLCDOInformation());
}

FKBFLCDOInformation UKBFLWorldModule::GetCDOInformationFromPhase_Implementation(ELifecyclePhase Phase, bool& bHasPhase)
{
	if (mCDOInformationMap.Contains(Phase))
	{
		return mCDOInformationMap[Phase];
	}
	return FKBFLCDOInformation();
}

void UKBFLWorldModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	if (Phase == ELifecyclePhase::CONSTRUCTION)
	{
		ConstructionPhase();
	}

	if (Phase == ELifecyclePhase::INITIALIZATION)
	{
		RegisterKBFLLogicContent();
		InitPhase();
	}

	if (Phase == ELifecyclePhase::POST_INITIALIZATION)
	{
		PostInitPhase();
	}

	Super::DispatchLifecycleEvent(Phase);
}

void UKBFLWorldModule::InitPhase_Implementation() {}

void UKBFLWorldModule::ConstructionPhase_Implementation() {}

void UKBFLWorldModule::PostInitPhase_Implementation() {}

void UKBFLWorldModule::RegisterKBFLLogicContent()
{
	if (!mUseAssetRegistry)
	{
		bScanForCDOsDone = true;
		return;
	}

	UWorld* WorldObject = GetWorld();
	UModContentRegistry* ModContentRegistry = UModContentRegistry::Get(WorldObject);
	fgcheck(ModContentRegistry);

	UKBFLAssetDataSubsystem* AssetDataSub = UKBFLAssetDataSubsystem::Get(WorldObject);
	fgcheck(AssetDataSub);

	FKBFLAssetData Datas = AssetDataSub->GetModRelatedData(this);

	if (mRegisterSchematics)
	{
		for (UClass* Class : Datas.mAllFoundedSchematics)
		{
			if (TSubclassOf<UFGSchematic> SchematicClass = Class)
			{
				if (IsAllowedToRegister(SchematicClass))
				{
					// UE_LOG(KBFLWorldModuleLog, Warning, TEXT("Register Schematic (%s) in ModContentRegistry"),
					// *SchematicClass->GetName());
					// ModContentRegistry->RegisterSchematic(GetOwnerModReference(), SchematicClass);

					// is a fix for SF+ Content remover!
					int32 AddedAt = mSchematics.AddUnique(SchematicClass);
					// ModContentRegistry->RegisterSchematic(GetOwnerModReference(), SchematicClass);
#
					if (AddedAt != INDEX_NONE)
					{
						UE_LOG(KBFLWorldModuleLog, Warning, TEXT("Register Schematic (%s) in ModContentRegistry"),
							   *SchematicClass->GetName());
					}
				}
				else
				{
					UE_LOG(KBFLWorldModuleLog, Warning,
						   TEXT("Cancle Register Schematic (%s) in ModContentRegistry because it not allowed!"),
						   *SchematicClass->GetName());
				}
			}
		}
	}

	if (mRegisterResearchTrees)
	{
		for (UClass* Class : Datas.mAllFoundResearchTrees)
		{
			if (TSubclassOf<UFGResearchTree> ResearchTreeClass = Class)
			{
				if (IsAllowedToRegister(ResearchTreeClass))
				{
					// UE_LOG( KBFLWorldModuleLog, Warning, TEXT("Register ResearchTrees (%s) in ModContentRegistry"),
					// *ResearchTreeClass->GetName() );
					// ModContentRegistry->RegisterResearchTree(GetOwnerModReference(), ResearchTreeClass);

					// is a fix for SF+ Content remover!
					mResearchTrees.AddUnique(ResearchTreeClass);
				}
				else
				{
					UE_LOG(KBFLWorldModuleLog, Warning,
						   TEXT("Cancle Register ResearchTree (%s) in ModContentRegistry because it not allowed!"),
						   *ResearchTreeClass->GetName());
				}
			}
		}
	}

	if (mRegisterRecipes)
	{
		for (UClass* Class : Datas.mAllFoundedRecipes)
		{
			if (TSubclassOf<UFGRecipe> RecipeClass = Class)
			{

				if (IsAllowedToRegister(RecipeClass))
				{
					// UE_LOG(KBFLWorldModuleLog, Warning, TEXT("Register Schematic (%s) in ModContentRegistry"),
					// *SchematicClass->GetName());
					ModContentRegistry->RegisterRecipe(GetOwnerModReference(), RecipeClass);
				}
				else
				{
					UE_LOG(KBFLWorldModuleLog, Warning,
						   TEXT("Cancle Register Recipe (%s) in ModContentRegistry because it not allowed!"),
						   *RecipeClass->GetName());
				}
			}
		}
	}
}

bool UKBFLWorldModule::IsAllowedToRegister(TSubclassOf<UObject> Object) const
{
	if (!IsValid(Object))
	{
		return false;
	}

	UModContentRegistry* ModContentRegistry = UModContentRegistry::Get(GetWorld());
	fgcheck(ModContentRegistry);

	TSubclassOf<UFGSchematic> AsSchematic = TSubclassOf<UFGSchematic>(Object);
	TSubclassOf<UFGResearchTree> AsTree = TSubclassOf<UFGResearchTree>(Object);
	TSubclassOf<UFGRecipe> AsRecipe = TSubclassOf<UFGRecipe>(Object);

	if (AsSchematic)
	{
		if (const FGameObjectRegistration* State =
				ModContentRegistry->RecipeRegistryState.FindObjectRegistration(AsSchematic))
		{
			UE_LOG(KBFLWorldModuleLog, Warning,
				   TEXT("Schematic (%s) already registered in ModContentRegistry, skip register! (%s)"),
				   *AsSchematic->GetName(), *State->OwnedByModReference.ToString());
			return false;
		}
	}
	else if (AsRecipe)
	{
		if (const FGameObjectRegistration* State =
				ModContentRegistry->SchematicRegistryState.FindObjectRegistration(AsRecipe))
		{
			UE_LOG(KBFLWorldModuleLog, Warning,
				   TEXT("Recipe (%s) already registered in ModContentRegistry, skip register! (%s)"),
				   *AsRecipe->GetName(), *State->OwnedByModReference.ToString());
			return false;
		}
	}
	else if (AsTree)
	{
		if (const FGameObjectRegistration* State =
				ModContentRegistry->ResearchTreeRegistryState.FindObjectRegistration(AsTree))
		{
			UE_LOG(KBFLWorldModuleLog, Warning,
				   TEXT("Tree (%s) already registered in ModContentRegistry, skip register! (%s)"), *AsTree->GetName(),
				   *State->OwnedByModReference.ToString());
			return false;
		}
	}
	return !mBlacklistedClasses.Contains(Object);
}

void UKBFLWorldModule::FindAllCDOs()
{
	if (!mUseAssetRegistry || !mRegisterCDOs)
	{
		bScanForCDOsDone = true;
		return;
	}
	UKBFLAssetDataSubsystem* AssetDataSub = UKBFLAssetDataSubsystem::Get(GetWorld());
	fgcheck(AssetDataSub);

	constexpr ELifecyclePhase CdoPhase = ELifecyclePhase::CONSTRUCTION;
	FKBFLAssetData Datas = AssetDataSub->GetModRelatedData(this);
	if (!mCDOInformationMap.Find(CdoPhase))
	{
		mCDOInformationMap.Add(CdoPhase, FKBFLCDOInformation());
	}
	FKBFLCDOInformation* CDOInfo = mCDOInformationMap.Find(CdoPhase);
	fgcheck(CDOInfo);

	for (UClass* CDOHelper : Datas.mAllFoundedCDOHelpers)
	{
		if (TSubclassOf<UKBFL_CDOHelperClass_Base> CDOHelperClass = CDOHelper)
		{
			if (mBlacklistedCDOClasses.Contains(CDOHelperClass))
			{
				continue;
			}
			UE_LOG(KBFLWorldModuleLog, Warning, TEXT("Found CDO helper (%s) and add to map"),
				   *CDOHelperClass->GetName());
			CDOInfo->mCDOHelperClasses.AddUnique(CDOHelperClass);
		}
	}
	bScanForCDOsDone = true;
}

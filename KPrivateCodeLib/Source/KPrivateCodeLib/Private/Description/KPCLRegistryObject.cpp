// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Description/KPCLRegistryObject.h"

TArray<TSubclassOf<UFGSchematic>> UKPCLRegistryObject::GetSchematicsToRegister()
{
	return TArray<TSubclassOf<UFGSchematic>>();
}

TArray<TSubclassOf<UFGRecipe>> UKPCLRegistryObject::GetRecipesToRegister()
{
	return TArray<TSubclassOf<UFGRecipe>>();
}

bool UKPCLRegistryObject::ShouldRegister(TSubclassOf<UKPCLRegistryObject> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject()->mShouldRegister;
	}
	return true;
}

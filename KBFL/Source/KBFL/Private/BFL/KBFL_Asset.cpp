#include "BFL/KBFL_Asset.h"

#include "FGRecipe.h"

void UKBFL_Asset::MarkDefaultDirty(UClass* Class)
{
	if (IsValid(Class))
	{
		Class->MarkPackageDirty();
		Class->GetDefaultObject()->MarkPackageDirty();
	}
}

bool UKBFL_Asset::HasProducer(TSubclassOf<UFGRecipe> Recipe, TSubclassOf<UObject> Producer)
{
	TArray<TSubclassOf<UObject>> OutClasses = UFGRecipe::GetProducedIn(Recipe);
	return OutClasses.Contains(Producer);
}

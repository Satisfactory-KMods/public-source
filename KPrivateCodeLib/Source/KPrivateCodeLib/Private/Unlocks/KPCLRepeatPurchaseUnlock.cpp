// ILikeBanas

#include "Unlocks/KPCLRepeatPurchaseUnlock.h"

#include "FGSchematic.h"
#include "Unlocks/FGUnlock.h"

IKPCLRepeatPurchaseUnlock*
IKPCLRepeatPurchaseUnlock::FindOnSchematic(TSubclassOf<UFGSchematic> SchematicClass)
{
	if (!SchematicClass || !IsValid(SchematicClass.GetDefaultObject()))
	{
		return nullptr;
	}

	for (UFGUnlock* Unlock : UFGSchematic::GetUnlocks(SchematicClass))
	{
		if (IKPCLRepeatPurchaseUnlock* RepeatUnlock = Cast<IKPCLRepeatPurchaseUnlock>(Unlock))
		{
			return RepeatUnlock;
		}
	}
	return nullptr;
}

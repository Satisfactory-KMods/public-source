#include "Schematic/KPCLSchematic.h"

#include "FGSchematicManager.h"

UKPCLSchematic::UKPCLSchematic() {}

void UKPCLSchematic::GetUnlockedSchematicsByType(UObject* WorldContext, EKPCLSchematicType SchematicType,
												 TArray<TSubclassOf<UKPCLSchematic>>& OutSchematics)
{
	OutSchematics.Empty();

	AFGSchematicManager* Manager = AFGSchematicManager::Get(WorldContext);
	if (!IsValid(Manager))
	{
		return;
	}

	TArray<TSubclassOf<UFGSchematic>> PurchasedSchematics;
	Manager->GetAllPurchasedSchematics(PurchasedSchematics);

	for (TSubclassOf<UFGSchematic> PurchasedSchematic : PurchasedSchematics)
	{
		if (TSubclassOf<UKPCLSchematic> KPCLSchematic = TSubclassOf<UKPCLSchematic>(PurchasedSchematic))
		{
			if (KPCLSchematic.GetDefaultObject()->mSchematicType == SchematicType)
			{
				OutSchematics.Add(KPCLSchematic);
			}
		}
	}
}

void UKPCLSchematic::Faxit_GetLevel(UObject* WorldContext, EKPCLNetworkLevelType LevelType, int32& OutLevel)
{
	TArray<TSubclassOf<UKPCLSchematic>> Schematics;
	GetUnlockedSchematicsByType(WorldContext, EKPCLSchematicType::Faxit, Schematics);

	OutLevel = 1;
	for (TSubclassOf<UKPCLSchematic> Schematic : Schematics)
	{
		if (!IsValid(Schematic) || Schematic.GetDefaultObject()->mFaxitLevel != LevelType)
		{
			continue;
		}
		int32 Level = Schematic.GetDefaultObject()->mLevel;
		OutLevel += Level;
	}
}

void UKPCLSchematic::Faxit_IsUnlocked(UObject* WorldContext, EKPCLNetworkLevelType LevelType, bool& OutIsUnlocked)
{
	TArray<TSubclassOf<UKPCLSchematic>> Schematics;
	GetUnlockedSchematicsByType(WorldContext, EKPCLSchematicType::Faxit, Schematics);

	OutIsUnlocked = false;
	for (TSubclassOf<UKPCLSchematic> Schematic : Schematics)
	{
		if (!IsValid(Schematic) || Schematic.GetDefaultObject()->mFaxitLevel != LevelType)
		{
			continue;
		}
		OutIsUnlocked = true;
		return;
	}
}

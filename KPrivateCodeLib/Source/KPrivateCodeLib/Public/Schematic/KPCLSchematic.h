// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGSchematic.h"
#include "Subsystem/KPCLFaxitSubsystem.h"
#include "KPCLSchematic.generated.h"

UENUM(BlueprintType)
enum class EKPCLSchematicType : uint8
{
	Faxit UMETA(DisplayName = "Faxit") //,
	// Repeatable UMETA(DisplayName = "Repeatable"),
};

UCLASS(Blueprintable)
class KPRIVATECODELIB_API UKPCLSchematic : public UFGSchematic
{
	GENERATED_BODY()

	static TArray<TSubclassOf<UKPCLSchematic>> CachedSchematics;

public:
	UKPCLSchematic();

	static void GetUnlockedSchematicsByType(UObject* WorldContext,
	                                        EKPCLSchematicType SchematicType,
	                                        TArray<TSubclassOf<UKPCLSchematic>>& OutSchematics);

	static void Faxit_GetLevel(UObject* WorldContext, EKPCLNetworkLevelType LevelType,
	                           int32& OutLevel);

	static void Faxit_IsUnlocked(UObject* WorldContext,
	                             EKPCLNetworkLevelType LevelType,
	                             bool& OutIsUnlocked);

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	TEnumAsByte<EKPCLSchematicType> mSchematicType = EKPCLSchematicType::Faxit;

	UPROPERTY(EditDefaultsOnly, Category="KMods", meta=(EditCondition="mSchematicType == EKPCLSchematicType::Faxit"))
	TEnumAsByte<EKPCLNetworkLevelType> mFaxitLevel;

	UPROPERTY(EditDefaultsOnly, Category="KMods", meta=(EditCondition="mSchematicType == EKPCLSchematicType::Faxit"))
	int32 mLevel = 1;
};

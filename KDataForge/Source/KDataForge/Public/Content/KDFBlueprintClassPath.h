// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct KDATAFORGE_API FKDFBlueprintClassPath
{
	inline static constexpr const TCHAR* UnlockArmEquipmentSlot =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockArmEquipmentSlot.BP_UnlockArmEquipmentSlot_C");
	inline static constexpr const TCHAR* UnlockBlueprints =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockBlueprints.BP_UnlockBlueprints_C");
	inline static constexpr const TCHAR* UnlockBuildEfficiency =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockBuildEfficiency.BP_UnlockBuildEfficiency_C");
	inline static constexpr const TCHAR* UnlockBuildOverclock =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockBuildOverclock.BP_UnlockBuildOverclock_C");
	inline static constexpr const TCHAR* UnlockBuildProductionBoost =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockBuildProductionBoost.BP_UnlockBuildProductionBoost_C");
	inline static constexpr const TCHAR* UnlockCentralStorageItemLimit =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockCentralStorageItemLimit.BP_UnlockCentralStorageItemLimit_C");
	inline static constexpr const TCHAR* UnlockCentralStorageUploadSlots =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockCentralStorageUploadSlots.BP_UnlockCentralStorageUploadSlots_C");
	inline static constexpr const TCHAR* UnlockCentralStorageUploadSpeed =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockCentralStorageUploadSpeed.BP_UnlockCentralStorageUploadSpeed_C");
	inline static constexpr const TCHAR* UnlockCheckmark =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockCheckmark.BP_UnlockCheckmark_C");
	inline static constexpr const TCHAR* UnlockCircuitDaisyChaining =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockCircuitDaisyChaining.BP_UnlockCircuitDaisyChaining_C");
	inline static constexpr const TCHAR* UnlockCustomizer =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockCustomizer.BP_UnlockCustomizer_C");
	inline static constexpr const TCHAR* UnlockEmote =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockEmote.BP_UnlockEmote_C");
	inline static constexpr const TCHAR* UnlockGiveItem =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockGiveItem.BP_UnlockGiveItem_C");
	inline static constexpr const TCHAR* UnlockInfoOnly =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockInfoOnly.BP_UnlockInfoOnly_C");
	inline static constexpr const TCHAR* UnlockInventorySlot =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockInventorySlot.BP_UnlockInventorySlot_C");
	inline static constexpr const TCHAR* UnlockItemDescriptor =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockItemDescriptor.BP_UnlockItemDescriptor_C");
	inline static constexpr const TCHAR* UnlockMap = TEXT("/Game/FactoryGame/Unlocks/BP_UnlockMap.BP_UnlockMap_C");
	inline static constexpr const TCHAR* UnlockRecipe =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockRecipe.BP_UnlockRecipe_C");
	inline static constexpr const TCHAR* UnlockSAMIntensity =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockSAMIntensity.BP_UnlockSAMIntensity_C");
	inline static constexpr const TCHAR* UnlockScannableObject =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockScannableObject.BP_UnlockScannableObject_C");
	inline static constexpr const TCHAR* UnlockScannableResource =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockScannableResource.BP_UnlockScannableResource_C");
	inline static constexpr const TCHAR* UnlockSchematic =
		TEXT("/Game/FactoryGame/Unlocks/BP_UnlockSchematic.BP_UnlockSchematic_C");

	inline static constexpr const TCHAR* DependencyActorsBuilt =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_ActorsBuiltDependency.BP_ActorsBuiltDependency_C");
	inline static constexpr const TCHAR* DependencyCurrentOnboardingStep =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_CurrentOnboardingStepDependency."
			 "BP_CurrentOnboardingStepDependency_C");
	inline static constexpr const TCHAR* DependencyGameCompleted =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_GameCompletedDependency.BP_GameCompletedDependency_C");
	inline static constexpr const TCHAR* DependencyGamePhaseReached = TEXT(
		"/Game/FactoryGame/AvailabilityDependencies/BP_GamePhaseReachedDependency.BP_GamePhaseReachedDependency_C");
	inline static constexpr const TCHAR* DependencyItemFailedToSink =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_ItemFailedToSinkDependency."
			 "BP_ItemFailedToSinkDependency_C");
	inline static constexpr const TCHAR* DependencyItemPickedUp =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_ItemPickedUpDependency.BP_ItemPickedUpDependency_C");
	inline static constexpr const TCHAR* DependencyLockSpaceElevatorShipment =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_LockSpaceElevatorShipmentDependency."
			 "BP_LockSpaceElevatorShipmentDependency_C");
	inline static constexpr const TCHAR* DependencyMessageInterrupted =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_MessageInterruptedDependency."
			 "BP_MessageInterruptedDependency_C");
	inline static constexpr const TCHAR* DependencyMessageNotPlayed = TEXT(
		"/Game/FactoryGame/AvailabilityDependencies/BP_MessageNotPlayedDependency.BP_MessageNotPlayedDependency_C");
	inline static constexpr const TCHAR* DependencyMessagePlayed =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_MessagePlayedDependency.BP_MessagePlayedDependency_C");
	inline static constexpr const TCHAR* DependencyRandomChance =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_RandomChanceDependency.BP_RandomChanceDependency_C");
	inline static constexpr const TCHAR* DependencyResearchTreeProgression =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_ResearchTreeProgressionDependency."
			 "BP_ResearchTreeProgressionDependency_C");
	inline static constexpr const TCHAR* DependencySchematicNotPurchased =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_SchematicNotPurchasedDependency."
			 "BP_SchematicNotPurchasedDependency_C");
	inline static constexpr const TCHAR* DependencySchematicPurchased =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_SchematicPurchasedDependency."
			 "BP_SchematicPurchasedDependency_C");
	inline static constexpr const TCHAR* DependencySchematicSpammed = TEXT(
		"/Game/FactoryGame/AvailabilityDependencies/BP_SchematicSpammedDependency.BP_SchematicSpammedDependency_C");
	inline static constexpr const TCHAR* DependencySchematicType =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_SchematicTypeDependency.BP_SchematicTypeDependency_C");
	inline static constexpr const TCHAR* DependencySpaceElevatorFullyBuilt =
		TEXT("/Game/FactoryGame/AvailabilityDependencies/BP_SpaceElevatorFullyBuiltDependency."
			 "BP_SpaceElevatorFullyBuiltDependency_C");

	static FString Remap(const FString& ClassPath)
	{
		struct FClassRemap
		{
			const TCHAR* NativePath;
			const TCHAR* BlueprintPath;
		};
		static const FClassRemap Remaps[] = {
			{TEXT("/Script/FactoryGame.FGUnlockArmEquipmentSlot"), UnlockArmEquipmentSlot},
			{TEXT("/Script/FactoryGame.FGUnlockBlueprints"), UnlockBlueprints},
			{TEXT("/Script/FactoryGame.FGUnlockBuildEfficiency"), UnlockBuildEfficiency},
			{TEXT("/Script/FactoryGame.FGUnlockBuildOverclock"), UnlockBuildOverclock},
			{TEXT("/Script/FactoryGame.FGUnlockBuildProductionBoost"), UnlockBuildProductionBoost},
			{TEXT("/Script/FactoryGame.FGUnlockCentralStorageItemLimit"), UnlockCentralStorageItemLimit},
			{TEXT("/Script/FactoryGame.FGUnlockCentralStorageUploadSlots"), UnlockCentralStorageUploadSlots},
			{TEXT("/Script/FactoryGame.FGUnlockCentralStorageUploadSpeed"), UnlockCentralStorageUploadSpeed},
			{TEXT("/Script/FactoryGame.FGUnlockCheckmark"), UnlockCheckmark},
			{TEXT("/Script/FactoryGame.FGUnlockCircuitDaisyChaining"), UnlockCircuitDaisyChaining},
			{TEXT("/Script/FactoryGame.FGUnlockCustomizer"), UnlockCustomizer},
			{TEXT("/Script/FactoryGame.FGUnlockEmote"), UnlockEmote},
			{TEXT("/Script/FactoryGame.FGUnlockGiveItem"), UnlockGiveItem},
			{TEXT("/Script/FactoryGame.FGUnlockInfoOnly"), UnlockInfoOnly},
			{TEXT("/Script/FactoryGame.FGUnlockInventorySlot"), UnlockInventorySlot},
			{TEXT("/Script/FactoryGame.FGUnlockItemDescriptor"), UnlockItemDescriptor},
			{TEXT("/Script/FactoryGame.FGUnlockMap"), UnlockMap},
			{TEXT("/Script/FactoryGame.FGUnlockRecipe"), UnlockRecipe},
			{TEXT("/Script/FactoryGame.FGUnlockSAMIntensity"), UnlockSAMIntensity},
			{TEXT("/Script/FactoryGame.FGUnlockScannableObject"), UnlockScannableObject},
			{TEXT("/Script/FactoryGame.FGUnlockScannableResource"), UnlockScannableResource},
			{TEXT("/Script/FactoryGame.FGUnlockSchematic"), UnlockSchematic},
			{TEXT("/Script/FactoryGame.FGActorsBuiltDependency"), DependencyActorsBuilt},
			{TEXT("/Script/FactoryGame.FGCurrentOnboardingStepDependency"), DependencyCurrentOnboardingStep},
			{TEXT("/Script/FactoryGame.FGGameCompletedDependency"), DependencyGameCompleted},
			{TEXT("/Script/FactoryGame.FGGamePhaseReachedDependency"), DependencyGamePhaseReached},
			{TEXT("/Script/FactoryGame.FGItemFailedToSinkDependency"), DependencyItemFailedToSink},
			{TEXT("/Script/FactoryGame.FGItemPickedUpDependency"), DependencyItemPickedUp},
			{TEXT("/Script/FactoryGame.FGLockSpaceElevatorShipmentDependency"), DependencyLockSpaceElevatorShipment},
			{TEXT("/Script/FactoryGame.FGMessageInterruptedDependency"), DependencyMessageInterrupted},
			{TEXT("/Script/FactoryGame.FGMessageNotPlayedDependency"), DependencyMessageNotPlayed},
			{TEXT("/Script/FactoryGame.FGMessagePlayedDependency"), DependencyMessagePlayed},
			{TEXT("/Script/FactoryGame.FGRandomChanceDependency"), DependencyRandomChance},
			{TEXT("/Script/FactoryGame.FGResearchTreeProgressionDependency"), DependencyResearchTreeProgression},
			{TEXT("/Script/FactoryGame.FGSchematicNotPurchasedDependency"), DependencySchematicNotPurchased},
			{TEXT("/Script/FactoryGame.FGSchematicPurchasedDependency"), DependencySchematicPurchased},
			{TEXT("/Script/FactoryGame.FGSchematicSpammedDependency"), DependencySchematicSpammed},
			{TEXT("/Script/FactoryGame.FGSchematicTypeDependency"), DependencySchematicType},
			{TEXT("/Script/FactoryGame.FGSpaceElevatorFullyBuiltDependency"), DependencySpaceElevatorFullyBuilt},
			{TEXT("/Script/KLib.KLGiveItemToAllPlayerUnlock"),
			 TEXT("/KLib/Unlocks/BP_GiveToPlayersUnlock.BP_GiveToPlayersUnlock_C")},
			{TEXT("/Script/KLib.KLUnlockMaxRepeatPurchases"),
			 TEXT("/KLib/Unlocks/BP_GiveChallangeUnlock.BP_GiveChallangeUnlock_C")},
			{TEXT("/Script/KLib.UKLUnlockCleanerItem"), TEXT("/KLib/Unlocks/BP_CleanerUnlock.BP_CleanerUnlock_C")},
			{TEXT("/Script/KPrivateCodeLib.KPCLDeliveryTaskInventorySlotUnlock"),
			 TEXT("/KPrivateCodeLib/Unlocks/BP_DeliverInventorySlotUnlock.BP_DeliverInventorySlotUnlock_C")},
			{TEXT("/Script/KPrivateCodeLib.KPCLDeliveryTaskUnlock"),
			 TEXT("/KPrivateCodeLib/Unlocks/BP_DeliveryTaskUnlock.BP_DeliveryTaskUnlock_C")},
			{TEXT("/Script/KPrivateCodeLib.KPCLFaxitCapacityUnlock"),
			 TEXT("/KPrivateCodeLib/Unlocks/BP_FaxitCapaUnlock.BP_FaxitCapaUnlock_C")},
			{TEXT("/Script/KPrivateCodeLib.KPCLFaxitFeatureUnlock"),
			 TEXT("/KPrivateCodeLib/Unlocks/BP_FaxitFeatureUnlock.BP_FaxitFeatureUnlock_C")},
			{TEXT("/Script/KPrivateCodeLib.KPCLFaxitSpeedUnlock"),
			 TEXT("/KPrivateCodeLib/Unlocks/BP_FaxitSpeedUnlock.BP_FaxitSpeedUnlock_C")},
		};
		for (const FClassRemap& Entry : Remaps)
		{
			if (ClassPath.Equals(Entry.NativePath, ESearchCase::IgnoreCase))
			{
				return Entry.BlueprintPath;
			}
		}
		return ClassPath;
	}
};

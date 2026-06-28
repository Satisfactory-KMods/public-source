// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildable/Modular/KPCLModularBuildingBase.h"
#include "Buildables/FGBuildableFactory.h"
#include "Cpp/KBFLCppInventoryHelper.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "KLMMBuildableModule.generated.h"

UCLASS(Abstract)
class KLIB_API AKLMMBuildableModule : public AKPCLModularBuildingBase
{
	GENERATED_BODY()

public:
	AKLMMBuildableModule();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin AFGBuildableFactory Interface
	virtual bool CanProduce_Implementation() const override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual bool Factory_HasPower() const override;
	virtual float GetPowerConsume() const override;
	//~ End AFGBuildableFactory Interface

	//~ Begin AKPCLModularBuildingBase Interface
	virtual void HandlePower(float dt) override;
	virtual void SetBelts() override;
	//~ End AKPCLModularBuildingBase Interface

	UFUNCTION(BlueprintNativeEvent)
	void OnModuleProductionCompleted();

	void TryToRegister();
	void StorageItem(TSubclassOf<UFGItemDescriptor> Item, int Count) const;
	bool HasPipeConnection() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	bool CanStorageOutput(TSubclassOf<UFGItemDescriptor> Item, int Count) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	bool IsWasteProducer() const { return mWasteProductionClass != nullptr; }

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TSubclassOf<UKAPIWasteProducerType> GetWasteClass() const { return mWasteProductionClass; }

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TSubclassOf<UFGItemDescriptor> GetAllowedItem() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	TSubclassOf<UKAPIModularAttachmentDescriptor> GetType() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	bool HasMiner() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	bool IsVariable() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	class AKLMMBuildableBase* GetMiner() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	int GetTier() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	float GetMalus() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Modular Extractor")
	float GetBonus() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	TSubclassOf<UKAPIWasteProducerType> mWasteProductionClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	TSubclassOf<UKAPIModularAttachmentDescriptor> mAttachmentClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	int mTier = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	float mMalus = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	float mBonus = 0.0f;

	UPROPERTY(EditDefaultsOnly, SaveGame, BlueprintReadOnly, Category = "KMods|Modular Extractor")
	TObjectPtr<UFGInventoryComponent> mInventory = nullptr;

	UPROPERTY(SaveGame)
	uint8 mSlotIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "KMods|Modular Extractor")
	bool mHasBeltOutput = false;

	UPROPERTY(Transient)
	TObjectPtr<UFGPipeConnectionFactory> mPipeConnection = nullptr;
};

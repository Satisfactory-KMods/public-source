// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableAttachmentSplitter.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "KPCLBuildableBalanceSplitter.generated.h"

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FKPCLSplitterTimer
{
	GENERATED_BODY()

	FKPCLSplitterTimer() {};

	UPROPERTY(SaveGame, NotReplicated, BlueprintReadOnly)
	float mCurrentTimeLeft = 0.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	float mTargetPerMin = -1.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TObjectPtr<UFGFactoryConnectionComponent> mConnection = nullptr;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<TSubclassOf<UFGItemDescriptor>> mFilteredItems;

	bool CanPushItem(TSubclassOf<UFGItemDescriptor> ItemClass, bool bOnlyOverflowing,
					 bool bIsItemDefinedElsewhere) const;

	void Tick(float dt);

	bool CanPush() const;
	void HasPushed();
	void Reset();

	bool HasOverrule() const;
	bool HasWildcard() const;
	bool HasNone() const;
	bool HasAnyUndefined() const;

	bool HasItemOfClass(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	FString ToString() const;
};

UCLASS()
class KPRIVATECODELIB_API UKPCLBalanceSplitterClipboardSettings : public UFGFactoryClipboardSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	TArray<FKPCLSplitterTimer> mRules;
};

UENUM(BlueprintType)
enum class EKPCLBalanceSplitterType : uint8
{
	NORMAL UMETA(DisplayName = "Normal"),
	SMART UMETA(DisplayName = "Smart"),
	PROGRAMMABLE UMETA(DisplayName = "Programmable"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKPCLSplitterRulesChanged, bool, bFilterChanged, FKPCLSplitterTimer,
											   NewValue, int32, Idx);

UCLASS()
class KPRIVATECODELIB_API AKPCLBuildableBalanceSplitter : public AFGBuildableConveyorAttachment
{
	GENERATED_BODY()

public:
	AKPCLBuildableBalanceSplitter();

	UFUNCTION(BlueprintPure)
	static float GetProgressForTimer(const FKPCLSplitterTimer& Timer);

	UFUNCTION(BlueprintPure)
	static bool CanPushForTimer(const FKPCLSplitterTimer& Timer);

	UFUNCTION(BlueprintPure)
	const TArray<FKPCLSplitterTimer>& GetOutputTimers() const { return mOutputTimers; }

	UFUNCTION(BlueprintPure)
	EKPCLBalanceSplitterType GetSplitterType() const { return mSplitterType; }

	UFUNCTION(BlueprintCallable)
	void SetFilteredItems(int32 Idx, TArray<TSubclassOf<UFGItemDescriptor>> Items);

	UFUNCTION(BlueprintCallable)
	void SetItemsPerMin(int32 Idx, float ItemsPerMin);

	UFUNCTION(BlueprintCallable)
	void RemoveFromFilter(int32 Idx, TSubclassOf<UFGItemDescriptor> Item);

	UFUNCTION(BlueprintCallable)
	void AddOrSetFilter(int32 Idx, TSubclassOf<UFGItemDescriptor> Item);

	UPROPERTY(BlueprintAssignable)
	FKPCLSplitterRulesChanged mOnSplitterRulesChanged;

	//~ Begin AFGBuildableFactory Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Factory_Tick(float dt) override;
	virtual void BeginPlay() override;
	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
											  class AFGPlayerController* player) override;
	virtual void GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund,
												   bool noBuildCostEnabled) const override;
	virtual bool Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection,
												   FInventoryItem& out_item, float& out_OffsetBeyond,
												   TSubclassOf<UFGItemDescriptor> type) override;
	virtual bool Factory_PeekOutput_Implementation(const class UFGFactoryConnectionComponent* connection,
												   TArray<FInventoryItem>& out_items,
												   TSubclassOf<UFGItemDescriptor> type) const override;
	//~ End AFGBuildableFactory Interface

protected:
	virtual void Factory_HandleBelts(float dt);

	/** Distributes items from slot 0 (input buffer) to output slots (1+). */
	virtual void DistributeItemsToOutputSlots(float dt);

	bool DistributeNormalMode(UFGInventoryComponent* BufferInventory, int32 InputSlotIndex, float dt);
	void DistributeSmartOrProgrammableMode(UFGInventoryComponent* BufferInventory, int32 InputSlotIndex, float dt);

	bool CanOutputAcceptItem(int32 OutputIndex, TSubclassOf<UFGItemDescriptor> ItemClass, bool bIsItemDefinedElsewhere,
							 bool bOverflowMode) const;

	bool TryMoveItemToOutput(UFGInventoryComponent* BufferInventory, int32 InputSlotIndex,
							 const FInventoryStack& InputStack, int32 OutputIndex);

	UFGFactoryConnectionComponent* GetOutputByName(FName ConnectionName) const;
	FKPCLSplitterTimer* GetTimerForConnection(UFGFactoryConnectionComponent* Connection);
	const FKPCLSplitterTimer* GetTimerForConnection(UFGFactoryConnectionComponent* Connection) const;


	bool IsItemDefinedOnAnyOutput(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	// AFGBuildableConveyorAttachment derives from AFGBuildable (not AFGBuildableFactory), so it has
	// no mPropertyReplicator / IFGConditionalReplicationInterface. Use native UE replication instead.
	UPROPERTY(SaveGame, Replicated)
	TArray<FKPCLSplitterTimer> mOutputTimers;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UFGFactoryConnectionComponent>, int32> mOutputConnectionToIndexMap;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|BalanceSplitter")
	EKPCLBalanceSplitterType mSplitterType = EKPCLBalanceSplitterType::NORMAL;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|BalanceSplitter")
	int32 mInputBufferSize = 3;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|BalanceSplitter")
	int32 mOutputBufferSize = 3;

	/** Cycles through the outputs, stores the output we want to put mItem on. Index is for the mOutputs array. */
	UPROPERTY(SaveGame, Meta = (NoAutoJson))
	int32 mCurrentOutputIndex;
};

// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/KPCLDeliveryTaskSubsystem.h"
#include "Widget/KPCLDeliveryWidgetBase.h"

#include "KPCLDeliveryQueueWidget.generated.h"

class UKPCLDeliveryTreeWidget;

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FKPCLDeliveryQueueEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DeliveryTree|Queue")
	int32 mQueueIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "DeliveryTree|Queue")
	TObjectPtr<UKAPIDeliveryTask> mTask;

	UPROPERTY(BlueprintReadOnly, Category = "DeliveryTree|Queue")
	FKPCLResearchTreeNode mNode;

	UPROPERTY(BlueprintReadOnly, Category = "DeliveryTree|Queue")
	bool bIsActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "DeliveryTree|Queue")
	bool bCanRemove = false;
};

UCLASS(Blueprintable, BlueprintType)
class KPRIVATECODELIB_API UKPCLDeliveryQueueWidget : public UKPCLDeliveryWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Queue")
	bool SetupDeliveryQueue(UKPCLDeliveryTreeWidget* DeliveryTreeWidget);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Queue")
	void TeardownDeliveryQueue();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Queue")
	bool RefreshDeliveryQueue();

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Queue")
	bool GetDeliveryQueueEntry(int32 QueueIndex, FKPCLDeliveryQueueEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Queue")
	int32 GetDeliveryQueueEntryCount() const { return mQueueEntries.Num(); }

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Queue")
	bool CanRemoveQueueEntry(int32 QueueIndex) const;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Queue")
	bool TryRemoveQueueEntry(int32 QueueIndex);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Queue")
	bool TryMoveQueueEntry(int32 FromIndex, int32 ToIndex);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Queue")
	TObjectPtr<UKPCLDeliveryTreeWidget> mDeliveryTreeWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Queue")
	TArray<FKPCLDeliveryQueueEntry> mQueueEntries;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Queue")
	bool bIsSetup = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Queue")
	void OnDeliveryQueueSetup(UKPCLDeliveryTreeWidget* DeliveryTreeWidget);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Queue")
	void OnDeliveryQueueChanged(const TArray<FKPCLDeliveryQueueEntry>& QueueEntries);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Queue")
	void OnDeliveryQueueDestroyed();

private:
	bool ContainsQueuedTask(UKAPIDeliveryTask* Task) const;

	virtual void HandleResearchQueueOrderChanged() override;

	virtual void HandleResearchNodeChanged(const FKPCLResearchTreeNode& Node) override;

	virtual void HandleActiveResearchChanged(UKAPIDeliveryTask* PreviousTask, UKAPIDeliveryTask* CurrentTask) override;

	virtual void HandleResearchProgressChanged(UKAPIDeliveryTask* Task, float Progress) override;

	virtual void HandleResearchAvailabilityChanged(UKAPIDeliveryTask* Task, bool bIsAvailable) override;
};

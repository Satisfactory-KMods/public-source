#pragma once

#include "CoreMinimal.h"
#include "Subsystem/KPCLDeliveryTaskSubsystem.h"
#include "Widget/KPCLDeliveryWidgetBase.h"

#include "KPCLDeliveryPreviewWidget.generated.h"

class UKPCLDeliveryTreeWidget;

UCLASS(Blueprintable, BlueprintType)
class KPRIVATECODELIB_API UKPCLDeliveryPreviewWidget : public UKPCLDeliveryWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Preview")
	bool SetupDeliveryPreview(UKPCLDeliveryTreeWidget* DeliveryTreeWidget);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Preview")
	void TeardownDeliveryPreview();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Preview")
	bool SetPreviewedDeliveryTask(UKAPIDeliveryTask* Task);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Preview")
	void ClearDeliveryPreview();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Preview")
	bool RefreshDeliveryPreview();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Preview")
	bool TryQueuePreviewedTask(TArray<FText>& ErrorMessages);

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Preview")
	UFGInventoryComponent* GetManualDeliveryInventory() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Preview")
	UFGInventoryComponent* GetRewardInventory() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Preview")
	TArray<UFGAvailabilityDependency*> GetTaskNotMetDependencies() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Preview")
	TArray<FKPCLParentTaskNotMetReason> GetNotMetParentTask() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Preview")
	bool ShouldShowUnlocks() const;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Preview")
	TObjectPtr<UKPCLDeliveryTreeWidget> mDeliveryTreeWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Preview")
	TObjectPtr<UKAPIDeliveryTask> mPreviewTask;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Preview")
	FKPCLResearchTreeNode mPreviewNode;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Preview")
	bool bHasPreview = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Preview")
	bool bIsSetup = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Preview")
	void OnDeliveryPreviewSetup(UKPCLDeliveryTreeWidget* DeliveryTreeWidget);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Preview")
	void OnDeliveryPreviewChanged(const FKPCLResearchTreeNode& PreviewNode);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Preview")
	void OnDeliveryPreviewCleared(UKAPIDeliveryTask* PreviousTask);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnSelectedNodeChange(const FKPCLResearchTreeNode& Node, bool bHasSelection);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Preview")
	void OnDeliveryPreviewQueueAttempted(UKAPIDeliveryTask* Task, bool bWasQueued, const TArray<FText>& ErrorMessages);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Preview")
	void OnDeliveryPreviewTaskCompleted(UKAPIDeliveryTask* Task, int32 CompletionCount);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Preview")
	void OnDeliveryPreviewDestroyed();

private:

	void ClearDeliveryPreviewInternal(bool bClearTreeSelection);

	virtual void HandleResearchQueueOrderChanged() override;

	virtual void HandleResearchNodeChanged(const FKPCLResearchTreeNode& Node) override;

	virtual void HandleDeliveryTaskCompleted(UKAPIDeliveryTask* Task, int32 CompletionCount) override;

	virtual void HandleActiveResearchChanged(UKAPIDeliveryTask* PreviousTask, UKAPIDeliveryTask* CurrentTask) override;

	virtual void HandleResearchProgressChanged(UKAPIDeliveryTask* Task, float Progress) override;

	virtual void HandleResearchAvailabilityChanged(UKAPIDeliveryTask* Task, bool bIsAvailable) override;
};

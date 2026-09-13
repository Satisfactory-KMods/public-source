// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/KPCLDeliveryTaskSubsystem.h"
#include "Widget/KPCLDeliveryWidgetBase.h"

#include "KPCLDeliverNodeWidget.generated.h"

class UButton;
class UKPCLDeliveryTreeWidget;

UCLASS(Blueprintable, BlueprintType)
class KPRIVATECODELIB_API UKPCLDeliverNodeWidget : public UKPCLDeliveryWidgetBase
{
	GENERATED_BODY()

public:
	UKPCLDeliverNodeWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry,
												  const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry,
												  const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	bool SetupDeliveryNode(UKPCLDeliveryTreeWidget* DeliveryTreeWidget, const FKPCLResearchTreeNode& Node);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	void TeardownDeliveryNode();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	bool RefreshDeliveryNode();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	bool UpdateDeliveryNode(const FKPCLResearchTreeNode& Node);

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	FVector2D GetDeliveryNodePosition(bool bRelativeToTreeBounds = true) const;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	bool TryQueueTask(TArray<FText>& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	bool TryQueueTaskFromDoubleClick();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	bool NotifyDeliveryNodeClicked();

	static bool ShouldQueueTaskFromDoubleClick(const FKey& MouseButton);
	static bool ShouldLockSelectionFromClick(const FKey& MouseButton);

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsLocked() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsUnlocked() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool CanBeUnlocked() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool ArePrerequisitesMet() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool CanBeSelected() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsAvailable() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsQueued() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsWaitingInQueue() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsActive() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsCompleted() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool HasBeenCompleted() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsRepeatable() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsMaxed() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	int32 GetQueueCount() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	int32 GetFirstQueueIndex() const;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	void SetDeliveryNodeHovered(bool bHovered);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	void SelectDeliveryNode();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	void UpdateDeliveryNodeInteractionState(bool bHovered, bool bSelected, bool bDependencyHighlighted);

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsDeliveryNodeHovered() const { return bIsHovered; }

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsDeliveryNodeSelected() const { return bIsSelected; }

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	bool IsDeliveryNodeDependencyHighlighted() const { return bIsDependencyHighlighted; }

	UPROPERTY(BlueprintReadOnly, Category = "DeliveryTree|Node", meta = (BindWidgetOptional))
	TObjectPtr<UButton> mButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DeliveryTree|Node")
	float mConsumedDoubleClickMaxDelay = 1.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Node")
	TObjectPtr<UKPCLDeliveryTreeWidget> mDeliveryTreeWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Node")
	FKPCLResearchTreeNode mNode;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Node")
	bool bIsSetup = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Node")
	bool bIsHovered = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Node")
	bool bIsSelected = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Node")
	bool bIsDependencyHighlighted = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeSetup(const FKPCLResearchTreeNode& Node);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeChanged(const FKPCLResearchTreeNode& Node);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeStateChanged(EKPCLResearchNodeState PreviousState, EKPCLResearchNodeState CurrentState);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeProgressChanged(float PreviousProgress, float CurrentProgress);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeAvailabilityChanged(bool bIsUnlocked, bool bCanBeUnlocked);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeQueueChanged(const TArray<int32>& QueueIndices);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeCompletionChanged(int32 CompletedCount, bool bIsMaxed);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeQueueAttempted(bool bWasQueued, const TArray<FText>& ErrorMessages);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeHoverChanged(bool bHovered);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeSelectionChanged(bool bSelected);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeSelectionAttempted(bool bWasSelected, bool bIsLocked);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeDependencyHighlightChanged(bool bDependencyHighlighted);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeDestroyed();

private:
	UFUNCTION()
	void HandleDeliveryNodeButtonPressed();

	bool bClickHasPreviewMouseDown = false;

	double mLastPreviewMouseDownTime = 0.0;
};

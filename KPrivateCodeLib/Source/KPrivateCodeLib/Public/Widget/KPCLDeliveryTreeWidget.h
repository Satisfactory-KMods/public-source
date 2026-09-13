// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Subsystem/KPCLDeliveryTaskSubsystem.h"
#include "UI/FGUserWidget.h"
#include "Widget/KPCLDeliveryWidgetBase.h"

#include "KPCLDeliveryTreeWidget.generated.h"

class UPanelSlot;
class UPanelWidget;
class UCanvasPanel;
class UCanvasPanelSlot;
class UKPCLDeliverNodeWidget;
class UKPCLDeliveryPreviewWidget;
class UKPCLDeliveryQueueWidget;
class UKPCLDeliveryTreeWidget;

UENUM(BlueprintType)
enum class EKPCLDeliveryTreeConnectionState : uint8
{
	Locked,
	Unlocked,
	Completed
};

UENUM(BlueprintType)
enum class EKPCLDeliveryTreeConnectionDrawMode : uint8
{
	Straight,
	Orthogonal,
	Spline
};

UCLASS(NotBlueprintable)
class KPRIVATECODELIB_API UKPCLDeliveryTreeConnectionLayer : public UFGUserWidget
{
	GENERATED_BODY()

public:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
							  const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
							  const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UPROPERTY(Transient)
	TObjectPtr<UKPCLDeliveryTreeWidget> mDeliveryTreeWidget;
};

UCLASS(Blueprintable, BlueprintType)
class KPRIVATECODELIB_API UKPCLDeliveryTreeWidget : public UKPCLDeliveryWidgetBase
{
	GENERATED_BODY()

public:
	UKPCLDeliveryTreeWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree")
	bool SetupDeliveryTree(AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem = nullptr);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree")
	void TeardownDeliveryTree();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree")
	bool RefreshDeliveryTree();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Performance")
	void RequestDeliveryTreeRefresh();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Performance")
	bool RequestDeliveryNodeRefresh(UKAPIDeliveryTask* Task);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Performance")
	int32 ProcessPendingDeliveryTreeUpdates(int32 MaxNodeOperations = -1, int32 MaxConnectionOperations = -1,
											float TimeBudgetMs = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Performance")
	void FlushPendingDeliveryTreeUpdates();

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Performance")
	bool HasPendingDeliveryTreeUpdates() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Performance")
	int32 GetPendingDeliveryTreeOperationCount() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Performance")
	float GetDeliveryTreeUpdateProgress() const;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Performance")
	int32 ValidateCachedDeliveryNodes(int32 MaxNodeValidations = -1);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Layout")
	void SetNodeCanvas(UCanvasPanel* NodeCanvas);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Layout")
	bool SynchronizeNodeCanvas();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Layout")
	bool AttachDeliveryNodeWidgetToCanvas(UKPCLDeliverNodeWidget* NodeWidget);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Layout")
	bool PositionDeliveryNodeWidget(UKPCLDeliverNodeWidget* NodeWidget);

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Layout")
	UCanvasPanelSlot* GetDeliveryNodeCanvasSlot(UKPCLDeliverNodeWidget* NodeWidget) const;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|AuxiliaryWidgets")
	void SetDeliveryQueueWidget(UKPCLDeliveryQueueWidget* DeliveryQueueWidget);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|AuxiliaryWidgets")
	void SetDeliveryPreviewWidget(UKPCLDeliveryPreviewWidget* DeliveryPreviewWidget);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|AuxiliaryWidgets")
	void SynchronizeDeliveryAuxiliaryWidgets();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Attachment")
	UPanelSlot* AttachToPanel(UPanelWidget* TargetPanel, FMargin SlotPadding, int32 Index = -1);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Attachment")
	void DetachFromPanel();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Navigation")
	bool BeginDeliveryTreeNavigationDrag(FVector2D LocalPointerPosition);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Navigation")
	bool UpdateDeliveryTreeNavigationDrag(FVector2D LocalPointerPosition);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Navigation")
	void EndDeliveryTreeNavigationDrag();

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Navigation")
	bool SetDeliveryTreeNavigationOffset(FVector2D NavigationOffset);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Navigation")
	void ResetDeliveryTreeNavigationOffset();

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Navigation")
	FVector2D ClampDeliveryTreeNavigationOffset(FVector2D NavigationOffset) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Navigation")
	FVector2D GetDeliveryTreeNavigationOffset() const { return mNavigationOffset; }

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Navigation")
	bool IsDeliveryTreeNavigationDragging() const { return bIsNavigationDragging; }

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Navigation")
	bool FocusDeliveryTreeCoordinate(FIntVector2 Coordinate);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Navigation")
	bool FocusDeliveryTask(UKAPIDeliveryTask* Task);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Navigation")
	bool FocusActiveDeliveryTask();

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Navigation")
	FVector2D GetDeliveryTreeFocusOffset(FIntVector2 Coordinate) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Layout")
	FVector2D GetDeliveryTreeViewSize() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Layout")
	FVector2D GetDeliveryNodeSize(FIntVector2 Coordinate) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Layout")
	FVector2D GetDeliveryNodeCenter(FIntVector2 Coordinate) const;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Zoom")
	bool SetDeliveryTreeZoom(float ZoomLevel);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Zoom")
	bool SetDeliveryTreeZoomAroundPosition(float ZoomLevel, FVector2D LocalPivot);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Zoom")
	bool StepDeliveryTreeZoom(int32 Steps);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Zoom")
	void ResetDeliveryTreeZoom();

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Zoom")
	float GetDeliveryTreeZoom() const { return mZoomLevel; }

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Zoom")
	float ClampDeliveryTreeZoom(float ZoomLevel) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree")
	bool GetCachedNode(UKAPIDeliveryTask* Task, FKPCLResearchTreeNode& OutNode) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree")
	bool GetCachedNodeAtCoordinate(FIntVector2 Coordinate, FKPCLResearchTreeNode& OutNode) const;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Interaction")
	void SetHoveredDeliveryTask(UKAPIDeliveryTask* Task);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Interaction")
	void SelectDeliveryTask(UKAPIDeliveryTask* Task);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Interaction")
	void ClearDeliveryTaskSelection();

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Interaction")
	UKAPIDeliveryTask* GetDependencyInteractionTask() const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Interaction")
	bool IsDeliveryTaskHovered(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Interaction")
	bool IsDeliveryTaskSelected(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Interaction")
	bool IsDeliveryTaskDependencyHighlighted(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Interaction")
	bool IsDeliveryTaskLocked(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Interaction")
	bool CanSelectDeliveryTask(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Interaction")
	bool IsResearchConnectionDependencyHighlighted(const FKPCLResearchTreeEdge& Edge) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Layout")
	static FVector2D CoordinateToPosition(FIntVector2 Coordinate, FVector2D NodeSpacing);

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Layout")
	FVector2D GetNodePosition(FIntVector2 Coordinate, bool bRelativeToTreeBounds = true) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Connections")
	int32 GetResearchConnectionCount() const { return mResearchEdges.Num(); }

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Connections")
	int32 GetVisibleResearchConnectionCount() const
	{
		return FMath::Min(mVisibleResearchEdgeCount, mResearchEdges.Num());
	}

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Connections")
	static EKPCLDeliveryTreeConnectionState GetResearchConnectionState(const FKPCLResearchTreeEdge& Edge);

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Connections")
	TArray<FVector2D> GetResearchConnectionPoints(const FKPCLResearchTreeEdge& Edge) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Connections")
	FLinearColor GetResearchConnectionColor(const FKPCLResearchTreeEdge& Edge) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Connections")
	float GetResearchConnectionThickness(const FKPCLResearchTreeEdge& Edge) const;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Connections")
	void DrawResearchConnections(UPARAM(ref) FPaintContext& Context) const;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Queue")
	bool TryQueueTask(UKAPIDeliveryTask* Task, TArray<FText>& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Queue")
	bool TryRemoveLastQueueEntry(int32 QueueIndex);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Queue")
	bool TryMoveQueueEntry(int32 FromIndex, int32 ToIndex);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	UKPCLDeliverNodeWidget* CreateDeliveryNodeWidget(const FKPCLResearchTreeNode& Node);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	void DestroyDeliveryNodeWidget(UKPCLDeliverNodeWidget* NodeWidget);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Node")
	void DestroyAllDeliveryNodeWidgets();

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Node")
	UKPCLDeliverNodeWidget* GetDeliveryNodeWidget(UKAPIDeliveryTask* Task) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DeliveryTree|Node")
	TSubclassOf<UKPCLDeliverNodeWidget> mDeliveryNodeWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Node")
	TArray<TObjectPtr<UKPCLDeliverNodeWidget>> mDeliveryNodeWidgets;

	UPROPERTY(BlueprintReadWrite, Category = "DeliveryTree|AuxiliaryWidgets", meta = (BindWidgetOptional))
	TObjectPtr<UKPCLDeliveryQueueWidget> mDeliveryQueueWidget;

	UPROPERTY(BlueprintReadWrite, Category = "DeliveryTree|AuxiliaryWidgets", meta = (BindWidgetOptional))
	TObjectPtr<UKPCLDeliveryPreviewWidget> mDeliveryPreviewWidget;

	UPROPERTY(BlueprintReadWrite, Category = "DeliveryTree|Layout", meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> mNodeCanvas;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Connections")
	TObjectPtr<UKPCLDeliveryTreeConnectionLayer> mConnectionLayerWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree")
	TArray<FKPCLResearchTreeNode> mResearchNodes;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree")
	TArray<FKPCLResearchTreeEdge> mResearchEdges;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree")
	TArray<TObjectPtr<UKAPIDeliveryTask>> mQueuedTasks;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree")
	TObjectPtr<UKAPIDeliveryTask> mActiveTask;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Interaction")
	TObjectPtr<UKAPIDeliveryTask> mHoveredTask;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Interaction")
	TObjectPtr<UKAPIDeliveryTask> mSelectedTask;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Interaction")
	TArray<TObjectPtr<UKAPIDeliveryTask>> mDependencyHighlightedTasks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Interaction")
	bool bQueueTaskOnNodeDoubleClick = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DeliveryTree|Interaction")
	bool bAllowSelectingLockedTasks = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Layout")
	FIntVector2 mTreeMinCoordinate = FIntVector2::ZeroValue;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Layout")
	FIntVector2 mTreeMaxCoordinate = FIntVector2::ZeroValue;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Layout")
	bool bHasTreeBounds = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree")
	bool bIsSetup = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Performance")
	bool bIncrementalTreeUpdates = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Performance")
	bool bProcessTreeUpdatesWhileDetached = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Performance", meta = (ClampMin = "0"))
	int32 mMaxNodeOperationsPerTick = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Performance", meta = (ClampMin = "0"))
	int32 mMaxConnectionOperationsPerTick = 128;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Performance", meta = (ClampMin = "0.0"))
	float mTreeUpdateTimeBudgetMs = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Performance")
	bool bValidateCachedNodesOnTick = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Performance", meta = (ClampMin = "0"))
	int32 mMaxNodeValidationsPerTick = 4;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Performance")
	bool bIsTreeUpdateInProgress = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Performance")
	int32 mCompletedTreeUpdateOperations = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Performance")
	int32 mTotalTreeUpdateOperations = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Connections")
	int32 mVisibleResearchEdgeCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Layout")
	FVector2D mNodeSpacing = FVector2D(256.0, 160.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Layout")
	FVector2D mNodePositionOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Layout")
	FVector2D mNodeAlignment = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Layout")
	bool bAutoSizeNodeSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Layout",
			  meta = (EditCondition = "!bAutoSizeNodeSlots"))
	FVector2D mNodeSlotSize = FVector2D(200.0, 100.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Layout")
	int32 mNodeZOrder = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Navigation")
	bool bEnableDragNavigation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Navigation")
	FKey mNavigationDragMouseButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Navigation", meta = (ClampMin = "0.0"))
	float mNavigationDragSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Navigation")
	FVector2D mInitialNavigationOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Navigation")
	bool bFocusInitialCoordinateOnSetup = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Navigation",
			  meta = (EditCondition = "bFocusInitialCoordinateOnSetup"))
	FIntVector2 mInitialFocusCoordinate = FIntVector2::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Navigation")
	bool bClampNavigationOffset = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Navigation",
			  meta = (EditCondition = "bClampNavigationOffset"))
	FVector2D mMinNavigationOffset = FVector2D(-4096.0, -4096.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Navigation",
			  meta = (EditCondition = "bClampNavigationOffset"))
	FVector2D mMaxNavigationOffset = FVector2D(4096.0, 4096.0);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Navigation")
	FVector2D mNavigationOffset = FVector2D::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Navigation")
	bool bIsNavigationDragging = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Zoom")
	bool bEnableZoomNavigation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Zoom", meta = (ClampMin = "0.01"))
	float mMinZoomLevel = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Zoom", meta = (ClampMin = "0.01"))
	float mMaxZoomLevel = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Zoom", meta = (ClampMin = "0.001"))
	float mZoomStep = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Zoom", meta = (ClampMin = "0.01"))
	float mInitialZoomLevel = 1.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree|Zoom")
	float mZoomLevel = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	bool bDrawResearchConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	bool bAutomaticallyDrawResearchConnections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	int32 mConnectionZOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	EKPCLDeliveryTreeConnectionDrawMode mConnectionDrawMode = EKPCLDeliveryTreeConnectionDrawMode::Straight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	FVector2D mConnectionParentOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	FVector2D mConnectionChildOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections",
			  meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float mOrthogonalBendRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections", meta = (ClampMin = "0.0"))
	float mSplineTangentScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	FLinearColor mLockedConnectionColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	FLinearColor mUnlockedConnectionColor = FLinearColor(0.85f, 0.55f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	FLinearColor mCompletedConnectionColor = FLinearColor(0.1f, 0.75f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	FLinearColor mHoveredDependencyConnectionColor = FLinearColor(1.0f, 0.75f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	FLinearColor mSelectedDependencyConnectionColor = FLinearColor(0.05f, 0.65f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections",
			  meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float mConnectionOpacity = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections", meta = (ClampMin = "0.1"))
	float mConnectionLineThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections", meta = (ClampMin = "0.1"))
	float mHoveredDependencyConnectionThickness = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections", meta = (ClampMin = "0.1"))
	float mSelectedDependencyConnectionThickness = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeliveryTree|Connections")
	bool bAntiAliasResearchConnections = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnDeliveryTreeSetup(AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnDeliveryTreeDestroyed();

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnDeliveryTreeRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Performance")
	void OnDeliveryTreeUpdateStarted(int32 TotalOperations);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Performance")
	void OnDeliveryTreeUpdateProgress(int32 CompletedOperations, int32 TotalOperations);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Performance")
	void OnDeliveryTreeUpdateFinished();

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnResearchNodeChanged(const FKPCLResearchTreeNode& Node);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnActiveResearchChanged(UKAPIDeliveryTask* PreviousTask, UKAPIDeliveryTask* CurrentTask);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnResearchProgressChanged(UKAPIDeliveryTask* Task, float Progress);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnResearchAvailabilityChanged(UKAPIDeliveryTask* Task, bool bIsAvailable);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnResearchQueueOrderChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Queue")
	void OnDeliveryTaskQueueAttempted(UKAPIDeliveryTask* Task, bool bWasQueued, const TArray<FText>& ErrorMessages);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Completion")
	void OnDeliveryTaskCompleted(UKAPIDeliveryTask* Task, int32 CompletionCount);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Interaction")
	void OnHoveredDeliveryTaskChanged(UKAPIDeliveryTask* PreviousTask, UKAPIDeliveryTask* CurrentTask);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Interaction")
	void OnSelectedDeliveryTaskChanged(UKAPIDeliveryTask* PreviousTask, UKAPIDeliveryTask* CurrentTask);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Interaction")
	void OnDeliveryTaskSelectionAttempted(UKAPIDeliveryTask* Task, bool bWasSelected, bool bIsLocked);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnSelectedNodeChange(const FKPCLResearchTreeNode& Node, bool bHasSelection);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Interaction")
	void OnDeliveryDependencyHighlightChanged(UKAPIDeliveryTask* InteractionTask);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Navigation")
	void OnDeliveryTreeNavigationDragStarted(FVector2D NavigationOffset);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Navigation")
	void OnDeliveryTreeNavigationOffsetChanged(FVector2D PreviousOffset, FVector2D CurrentOffset);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Navigation")
	void OnDeliveryTreeNavigationDragFinished(FVector2D NavigationOffset);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Events")
	void OnDeliveryTreeZoomChanged(float PreviousZoomLevel, float CurrentZoomLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeWidgetCreated(UKPCLDeliverNodeWidget* NodeWidget);

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Node")
	void OnDeliveryNodeWidgetDestroyed(UKPCLDeliverNodeWidget* NodeWidget);

private:
	friend class UKPCLDeliveryTreeConnectionLayer;

	virtual void BindSubsystemEvents() override;
	virtual void UnbindSubsystemEvents() override;

	bool GetCachedNodeByIndex(const int32* NodeIndex, FKPCLResearchTreeNode& OutNode) const;
	void RefreshQueuedTasks();
	bool PrepareDeliveryTreeRefresh();
	void ResetPendingDeliveryTreeWork();
	void QueueDeliveryNodeUpdate(UKAPIDeliveryTask* Task, bool bUseCachedNode);
	void QueueDeliveryNodeRemoval(UKAPIDeliveryTask* Task);
	bool ApplyPendingDeliveryNodeUpdate(UKAPIDeliveryTask* Task, bool bUseCachedNode);
	void FinalizePendingDeliveryTreeUpdates();
	void RebuildResearchLookupCaches();
	void RebuildDeliveryNodeWidgetLookup();
	void RebuildResearchEdges();
	void RecalculateTreeBounds();
	void RefreshAffectedResearchEdges(UKAPIDeliveryTask* Task, const FKPCLResearchTreeNode& Node);
	void InvalidateConnectionLayer();
	void ScheduleDetachedDeliveryTreeUpdate();
	void HandleDetachedDeliveryTreeUpdate();
	void ProcessIncrementalDeliveryTreeTick();
	void RebuildDependencyHighlight();
	void BroadcastSelectedNodeChange();
	void UpdateDeliveryNodeInteractionStates();
	void CollectDependencyParents(UKAPIDeliveryTask* Task, TSet<UKAPIDeliveryTask*>& OutParents) const;
	bool IsCachedTask(UKAPIDeliveryTask* Task) const;
	void TeardownDeliveryAuxiliaryWidgets();
	static bool HasSameNodeTopology(const FKPCLResearchTreeNode& Left, const FKPCLResearchTreeNode& Right);
	static bool HasSameNodeData(const FKPCLResearchTreeNode& Left, const FKPCLResearchTreeNode& Right);
	bool EnsureConnectionLayer();
	void DestroyConnectionLayer();
	bool IsDeliveryNodeMeasured(FIntVector2 Coordinate) const;
	void InitializeDeliveryTreeNavigationOffset();
	void ApplyDeliveryTreeNavigationOffset();
	TArray<FVector2D> GetResearchConnectionPointsInternal(const FKPCLResearchTreeEdge& Edge, FVector2D AdditionalOffset,
														  float AdditionalScale) const;
	void DrawResearchConnectionsInternal(FPaintContext& Context, FVector2D AdditionalOffset,
										 float AdditionalScale) const;

	bool bFullRefreshRequested = false;
	bool bRefreshCompletionPending = false;
	int32 mPendingNodeUpdateIndex = 0;
	int32 mPendingNodeRemovalIndex = 0;
	TArray<TWeakObjectPtr<UKAPIDeliveryTask>> mPendingNodeUpdates;
	TArray<TWeakObjectPtr<UKAPIDeliveryTask>> mPendingNodeRemovals;
	TSet<TWeakObjectPtr<UKAPIDeliveryTask>> mPendingNodeUpdateSet;
	TSet<TWeakObjectPtr<UKAPIDeliveryTask>> mPendingNodeRemovalSet;
	TSet<TWeakObjectPtr<UKAPIDeliveryTask>> mPendingCachedNodeTasks;
	TMap<UKAPIDeliveryTask*, int32> mResearchNodeIndexByTask;
	TMap<FIntVector2, int32> mResearchNodeIndexByCoordinate;
	TMap<UKAPIDeliveryTask*, UKPCLDeliverNodeWidget*> mDeliveryNodeWidgetByTask;
	TMap<UKAPIDeliveryTask*, TArray<UKAPIDeliveryTask*>> mResearchParentsByTask;
	TMap<UKAPIDeliveryTask*, TArray<int32>> mParentEdgeIndicesByTask;
	TMap<UKAPIDeliveryTask*, TArray<int32>> mChildEdgeIndicesByTask;
	TWeakObjectPtr<UKAPIDeliveryTask> mAppliedHoveredTask;
	TWeakObjectPtr<UKAPIDeliveryTask> mAppliedSelectedTask;
	TSet<TWeakObjectPtr<UKAPIDeliveryTask>> mAppliedDependencyHighlightedTasks;
	FTimerHandle mDetachedUpdateTimerHandle;
	uint64 mLastIncrementalUpdateFrame = MAX_uint64;
	int32 mNextNodeValidationIndex = 0;
	FVector2D mNavigationDragLastLocalPosition = FVector2D::ZeroVector;
	bool bNavigationOffsetInitialized = false;

	UFUNCTION()
	void HandleResearchTreeChanged();

	virtual void HandleResearchNodeChanged(const FKPCLResearchTreeNode& Node) override;

	virtual void HandleDeliveryTaskCompleted(UKAPIDeliveryTask* Task, int32 CompletionCount) override;

	virtual void HandleActiveResearchChanged(UKAPIDeliveryTask* PreviousTask, UKAPIDeliveryTask* CurrentTask) override;

	virtual void HandleResearchProgressChanged(UKAPIDeliveryTask* Task, float Progress) override;

	virtual void HandleResearchAvailabilityChanged(UKAPIDeliveryTask* Task, bool bIsAvailable) override;

	virtual void HandleResearchQueueOrderChanged() override;
};

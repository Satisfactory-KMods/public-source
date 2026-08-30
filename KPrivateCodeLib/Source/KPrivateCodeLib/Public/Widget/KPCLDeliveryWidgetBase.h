#pragma once

#include "CoreMinimal.h"
#include "Subsystem/KPCLDeliveryTaskSubsystem.h"
#include "UI/FGUserWidget.h"

#include "KPCLDeliveryWidgetBase.generated.h"

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EKPCLDeliveryWidgetDirty : uint8
{
	None = 0 UMETA(Hidden),

	Data = 1 << 0,

	State = 1 << 1,

	Progress = 1 << 2,

	Queue = 1 << 3,

	Selection = 1 << 4,

	Layout = 1 << 5,

	Connections = 1 << 6
};

ENUM_CLASS_FLAGS(EKPCLDeliveryWidgetDirty);

UCLASS(Abstract, Blueprintable, BlueprintType)
class KPRIVATECODELIB_API UKPCLDeliveryWidgetBase : public UFGUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Rerender")
	void
	MarkDeliveryWidgetDirty(UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/KPrivateCodeLib.EKPCLDeliveryWidgetDirty"))
								int32 DirtyFlags);

	UFUNCTION(BlueprintCallable, Category = "DeliveryTree|Rerender")
	bool FlushDeliveryWidgetRerender();

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Rerender")
	bool
	IsDeliveryWidgetDirty(UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/KPrivateCodeLib.EKPCLDeliveryWidgetDirty"))
							  int32 DirtyFlags) const;

	UFUNCTION(BlueprintPure, Category = "DeliveryTree|Rerender")
	int32 GetDeliveryWidgetDirtyFlags() const { return mDirtyFlags; }

	UFUNCTION(BlueprintImplementableEvent, Category = "DeliveryTree|Rerender")
	void
	OnDeliveryWidgetRerender(UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/KPrivateCodeLib.EKPCLDeliveryWidgetDirty"))
								 int32 DirtyFlags);

	void MarkDirty(EKPCLDeliveryWidgetDirty DirtyFlags) { MarkDeliveryWidgetDirty(static_cast<int32>(DirtyFlags)); }

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DeliveryTree")
	TObjectPtr<AKPCLDeliveryTaskSubsystem> mDeliveryTaskSubsystem;

protected:

	virtual void NativeOnDeliveryWidgetRerender(int32 DirtyFlags);

	virtual void BindSubsystemEvents();

	virtual void UnbindSubsystemEvents();

	UFUNCTION()
	virtual void HandleResearchQueueOrderChanged() {}

	UFUNCTION()
	virtual void HandleResearchNodeChanged(const FKPCLResearchTreeNode& Node) {}

	UFUNCTION()
	virtual void HandleDeliveryTaskCompleted(UKAPIDeliveryTask* Task, int32 CompletionCount) {}

	UFUNCTION()
	virtual void HandleActiveResearchChanged(UKAPIDeliveryTask* PreviousTask, UKAPIDeliveryTask* CurrentTask) {}

	UFUNCTION()
	virtual void HandleResearchProgressChanged(UKAPIDeliveryTask* Task, float Progress) {}

	UFUNCTION()
	virtual void HandleResearchAvailabilityChanged(UKAPIDeliveryTask* Task, bool bIsAvailable) {}

private:
	void ScheduleDeliveryWidgetRerender();

	int32 mDirtyFlags = 0;
	bool bRerenderScheduled = false;
};

// 

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/KPCLNetworkDataInterface.h"
#include "Subsystem/KPCLFaxitSubsystem.h"
#include "UI/FGInteractWidget.h"

#include "KPCLNetworkWidgetPanel.generated.h"

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLNetworkWidgetPanel : public UFGInteractWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void InitializeNetworkPanel(UObject* InteractObject);

	UFUNCTION(BlueprintCallable, Category="Faxit")
	void AddBuildingToCore(FKPCLFaxitNetwork Network);

	UFUNCTION(BlueprintCallable, Category="Faxit")
	bool HasNetwork();

	UFUNCTION(BlueprintCallable, Category="Faxit")
	FKPCLFaxitNetworkInfo GetNetworkInfo(bool& bSuccess, const FString& NetworkId = FString(""));

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void WidgetTick(const FGeometry& MyGeometry, float InDeltaTime);

	UFUNCTION(BlueprintCallable, Category="Faxit")
	AKPCLNetworkCore* GetNetworkCore();

	UFUNCTION(BlueprintCallable, Category="Faxit")
	FKPCLFaxitNetwork GetCurrentNetwork(bool& bSuccess);

	UPROPERTY(BlueprintReadOnly)
	AKPCLFaxitSubsystem* mFaxitSubsystem = nullptr;

	UPROPERTY(BlueprintReadWrite)
	class AKPCLNetworkCore* mNetworkCore = nullptr;

	UPROPERTY(BlueprintReadWrite)
	FNetworkUIData mUiData = FNetworkUIData();

	UFUNCTION(BlueprintImplementableEvent)
	void AfterInitializeNetworkPanel();

	UPROPERTY(BlueprintReadWrite)
	AKPCLNetworkBuildingBase* mNetworkBuilding = nullptr;
};

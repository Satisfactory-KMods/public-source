// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Subsystem/KPCLFaxitSubsystem.h"
#include "UObject/Interface.h"

#include "KPCLNetworkDataInterface.generated.h"

UENUM(BlueprintType)
enum class EKPCLWidgetType : uint8
{
	TW_PLAYER_REMOTE_ACCESS UMETA(DisplayName = "Player Remote Access"),
	TW_POLE UMETA(DisplayName = "Pole"),
	TW_TERMINAL UMETA(DisplayName = "Terminal"),
	TW_CORE UMETA(DisplayName = "Core"),
	TW_TRANSFER_SOLID UMETA(DisplayName = "Transfer Solid"),
	TW_TRANSFER_FLUID_GAS UMETA(DisplayName = "Transfer Fluid/Gas")
};

UENUM(BlueprintType)
enum class EKPCLNetworkError : uint8
{
	NoNetwork,
	NetworkOverloaded,
	NoPower,
	NetworkToManyAddresses,
	GlobalNetworkToManyAddresses,
	NoDrivesAvailable,
};

USTRUCT(BlueprintType)
struct FNetworkUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EKPCLWidgetType mWidgetType = EKPCLWidgetType::TW_POLE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<class UKPCLNetworkWidgetPanel> mNetworkPanelWidgetClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mCanChangeNetwork = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mCanBeWireless = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mCanAccessInventory = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool mShowRightPanel = true;

	UPROPERTY(BlueprintReadWrite)
	bool mIsWireless = false;

	UPROPERTY(BlueprintReadWrite)
	int32 mActiveDefaultTabIndex = 0;

	FNetworkUIData Copy() const
	{
		FNetworkUIData NewData;
		NewData.mWidgetType = mWidgetType;
		NewData.mNetworkPanelWidgetClass = mNetworkPanelWidgetClass;
		NewData.mCanChangeNetwork = mCanChangeNetwork;
		NewData.mCanBeWireless = mCanBeWireless;
		NewData.mCanAccessInventory = mCanAccessInventory;
		NewData.mShowRightPanel = mShowRightPanel;
		NewData.mIsWireless = mIsWireless;
		return NewData;
	}
};

UINTERFACE()
class UKPCLNetworkDataInterface : public UInterface
{
	GENERATED_BODY()
};

class KPRIVATECODELIB_API IKPCLNetworkDataInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KPCLCustomDataInterface")
	class AKPCLNetworkCore* GetCore();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KPCLCustomDataInterface")
	bool HasCore() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KPCLCustomDataInterface")
	bool HasCoreInNetwork() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KPCLCustomDataInterface")
	class UKPCLNetwork* GetNetwork() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KPCLCustomDataInterface")
	FNetworkUIData GetUIDData() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KPCLCustomDataInterface")
	FKPCLFaxitNetwork GetNetworkData() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KPCLCustomDataInterface")
	TArray<EKPCLNetworkError> GetNetworkErrors() const;
};

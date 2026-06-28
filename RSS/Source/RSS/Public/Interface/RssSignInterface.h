// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Components/WidgetComponent.h"

#include "EnumStruc/RssStruc.h"

#include "RssSignInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable)
class URssSignInterface : public UInterface
{
	// GENERATED_UINTERFACE_BODY()
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FRssMeshInfo
{
	GENERATED_BODY()

	FRssMeshInfo() : mMeshComponent(nullptr), mMaterialIndex(0), mDynamicMaterial(nullptr) {}

	FRssMeshInfo(UStaticMeshComponent* MeshComponent, uint8 MaterialIndex = 0,
				 UMaterialInstanceDynamic* DynamicMaterial = nullptr) :
		mMeshComponent(MeshComponent), mMaterialIndex(MaterialIndex), mDynamicMaterial(DynamicMaterial)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMeshComponent> mMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 mMaterialIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInstanceDynamic> mDynamicMaterial;

	bool TryToMakeDynMaterial()
	{
		if (mMeshComponent && !mDynamicMaterial)
		{
			mDynamicMaterial = mMeshComponent->CreateAndSetMaterialInstanceDynamic(mMaterialIndex);
		}
		return mDynamicMaterial != nullptr;
	}

	bool ResetMaterial(UMaterialInterface* Material)
	{
		if (mMeshComponent)
		{
			mMeshComponent->SetMaterial(mMaterialIndex, Material);
			mDynamicMaterial = nullptr;
		}
		return mDynamicMaterial == nullptr;
	}
};

class RSS_API IRssSignInterface
{
	// GENERATED_IINTERFACE_BODY()
	GENERATED_BODY()

public:
	/** ---------------------- REQUEST ------------------------ */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Request")
	void RequestCustomSign(FRssSignRequestData Request, bool ComesFromServer = false);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Request")
	void RequestInteractWidget(AFGPlayerController* PlayerController);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Request")
	void RequestCloseWidget(AFGPlayerController* PlayerController);

	/** ---------------------- UPDATE ------------------------ */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Update")
	void UpdateSign();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Update")
	void UpdateSignToCustomImage(FRssSignRequestData Request);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Update")
	void UpdateSignData(FRssSignData Data);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Update")
	void ApplySignData(FRssSignData Data);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Update")
	void PasteSignData(FRssSignData Data);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Update")
	void UpdateSignDisplayWidget();

	/** ---------------------- Sign Gun ------------------------ */

	UFUNCTION(BlueprintNativeEvent, Category = "RSS|Update")
	void SignGun_StartLookingAtSign(FRssSignData TempSignData);

	UFUNCTION(BlueprintNativeEvent, Category = "RSS|Update")
	void SignGun_EndLookingAtSign();

	UFUNCTION(BlueprintNativeEvent, Category = "RSS|Update")
	FRssSignData SignGun_GetRealSignData();

	/** ---------------------- GET ------------------------ */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Get")
	FRssSignData GetSignData() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Get")
	FRssUiData GetSignUiIData() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Get")
	TArray<FRssMeshInfo> GetRenderMeshInfos();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Get")
	TArray<FRssMeshInfo> GetScreenMeshInfos();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Get")
	UActorComponent* GetRenderComponent();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Get")
	bool IsBuildingSignificance();

	/** RSS U6 Update changes */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Get")
	TSubclassOf<class URssWidgetRenderComponent> GetWidgetRenderComponentClass();
};

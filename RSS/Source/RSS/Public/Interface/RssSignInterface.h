// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "EnumStruc/RssStruc.h"

#include "RssSignInterface.generated.h"

UINTERFACE(Blueprintable)
class URssSignInterface : public UInterface
{

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

	UPROPERTY()
	TObjectPtr<UMaterialInterface> mOriginalMaterial = nullptr;

	void CacheOriginalMaterial()
	{
		if (mMeshComponent && !mOriginalMaterial)
		{
			UMaterialInterface* Current = mMeshComponent->GetMaterial(mMaterialIndex);
			if (Current && !Current->IsA<UMaterialInstanceDynamic>())
			{
				mOriginalMaterial = Current;
			}
		}
	}

	bool TryToMakeDynMaterial()
	{
		if (mMeshComponent && !mDynamicMaterial)
		{
			CacheOriginalMaterial();
			mDynamicMaterial = mMeshComponent->CreateAndSetMaterialInstanceDynamic(mMaterialIndex);
		}
		return mDynamicMaterial != nullptr;
	}

	bool ResetMaterial(UMaterialInterface* Fallback = nullptr)
	{
		if (mMeshComponent)
		{
			if (UMaterialInterface* Restore = mOriginalMaterial ? mOriginalMaterial.Get() : Fallback)
			{
				mMeshComponent->SetMaterial(mMaterialIndex, Restore);
			}
			mDynamicMaterial = nullptr;
		}
		return mDynamicMaterial == nullptr;
	}
};

class RSS_API IRssSignInterface
{

	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Request")
	void RequestCustomSign(FRssSignRequestData Request, bool ComesFromServer = false);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Request")
	void RequestInteractWidget(AFGPlayerController* PlayerController);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Request")
	void RequestCloseWidget(AFGPlayerController* PlayerController);

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

	UFUNCTION(BlueprintNativeEvent, Category = "RSS|Update")
	void SignGun_StartLookingAtSign(FRssSignData TempSignData);

	UFUNCTION(BlueprintNativeEvent, Category = "RSS|Update")
	void SignGun_EndLookingAtSign();

	UFUNCTION(BlueprintNativeEvent, Category = "RSS|Update")
	FRssSignData SignGun_GetRealSignData();

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

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS|Get")
	TSubclassOf<class URssWidgetRenderComponent> GetWidgetRenderComponentClass();
};

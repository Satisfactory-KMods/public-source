// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGColoredInstanceMeshProxy.h"
#include "FGSaveInterface.h"
#include "FactoryGame.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "KPCLColoredStaticMesh.generated.h"

USTRUCT(BlueprintType)
struct FKPCLColorData
{
	GENERATED_BODY()

	FKPCLColorData() = default;

	FKPCLColorData(uint8 Index, float Data)
	{
		mColorIndex = Index;
		mIndexData = Data;
	}

	/** Begin with 0 (Index 20) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	uint8 mColorIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float mIndexData;
};

USTRUCT(BlueprintType)
struct FKPCLLinearColorData
{
	GENERATED_BODY()

	FKPCLLinearColorData() = default;

	FKPCLLinearColorData(uint8 Index, FLinearColor Color)
	{
		mStartColorIndex = Index;
		mColor = Color;
	}

	/** Begin with 0 (Index 20) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	uint8 mStartColorIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FLinearColor mColor;
};

UENUM(BlueprintType)
enum class EKPCLDefaultColorIndex : uint8
{
	Primary = 0,
	Secondary = 2,
	Light = 11,
};

/**
 * Proxy placed in buildings to be replaced with an instance on creation, supports coloring.
 * Modding version: can apply colours in thread (FactoryTick) and will only update if dirty.
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class KPRIVATECODELIB_API UKPCLColoredStaticMesh : public UFGColoredInstanceMeshProxy, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	UKPCLColoredStaticMesh();

	virtual bool ShouldSave_Implementation() const override;

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	//~ End UActorComponent Interface

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyNewColorDatas(TArray<FKPCLColorData> ColorData, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyNewColorData(FKPCLColorData ColorData, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyNewLinearColorDatas(TArray<FKPCLLinearColorData> ColorData, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyNewLinearColorData(FKPCLLinearColorData ColorData, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyNewFGLinearColorDatas(TArray<FKPCLLinearColorData> ColorData, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyNewFGLinearColorData(FKPCLLinearColorData ColorData, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyFGNewColorDatas(TArray<FKPCLColorData> ColorData, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyFgNewColorData(FKPCLColorData ColorData, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyFgNewColorToType(FLinearColor Color, EKPCLDefaultColorIndex Type, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void RemoveFGIndex(int32 Idx, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void GetAllFGOverwriteData(TArray<FKPCLColorData>& Datas);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void ApplyNewData();

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	bool CheckIndex(FKPCLColorData ColorData, bool IsFG = false);

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	void UpdateWorldTransform(FTransform Transform);
	void ApplyTransformToComponent();

	UFUNCTION(BlueprintCallable, Category = "KMods|ColoredStaticMesh")
	static void UpdateStaticMesh(UKPCLColoredStaticMesh* Proxy, AActor* Owner, UStaticMesh* Mesh);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, SaveGame, Category = "KMods")
	TArray<float> mCustomExtraData = {};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, SaveGame, Category = "KMods")
	TMap<int32, float> mFGOverwriteMap = {};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods")
	bool mStartWithDirtyState = true;

private:
	FSmartTimer mTimer = FSmartTimer(1.f);
	FTransform mLastWorldTransform;

	UPROPERTY(EditDefaultsOnly, Category = "KMods")
	bool mShouldSave = false;
};

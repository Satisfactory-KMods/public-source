// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_Util.h"
#include "Blueprint/AsyncTaskDownloadImage.h"
#include "Kismet/KismetStringLibrary.h"
#include "Subsystem/ModSubsystem.h"

#include "Cache/RssImageCache.h"
#include "EnumStruc/RssStruc.h"
#include "Widget/RssDownloadImage.h"

#include "RSSImageSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FInitRequests
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<ESignSize> mSignSizes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString mUrl;
};

UCLASS()
class RSS_API ARSSImageSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	ARSSImageSubsystem();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~ End AActor Interface

	UFUNCTION()
	void OnDownloadFailed(UTexture2DDynamic* Texture);

	UFUNCTION()
	void OnDownloadSuccess(UTexture2DDynamic* Texture);

	void SendRequestBack(FRssSignRequestData& Request, UTexture* Texture = nullptr);

	static bool IsUrlValid(FString Url);

	void GenerateInformation();
	TMap<FString, FRssCustomSignUrlData> GenerateMapFromInformation();
	void GenerateMapFromInformationInVar();
	bool ReadCustomDataFromFile(FString& FileContent);
	FString GetFilePath();
	bool SaveCustomDataToFile();

	static FString CustomInformationToString(UObject* WorldContext, FRssCustomDatas CustomInformation);
	static FRssCustomDatas CustomInformationStringToSignData(UObject* WorldContext, FString CustomInformation);

	UFUNCTION(BlueprintPure, Category = "Data Manager")
	static bool CheckString(FString String, TArray<FString> ExtraNameChecks);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS Subsystems")
	void onRequestImages(FRssSignRequestData Request);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS Subsystems")
	void OnCallToWidget(FRssSignRequestData Request);

	void OnCallToWidget_Implementation(FRssSignRequestData Request) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RSS Subsystems")
	void DoRemove(const FString& Url);

	UFUNCTION(BlueprintCallable, Category = "RSS Subsystems")
	void RemoveDataByUrl(FString Url);

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	FRssCustomSignUrlData GetDataFromUrl(FString Url);

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	TArray<FRssCustomSignUrlData> GetDataFromSignSize(ESignSize Size);

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	URssImageCacheManager* GetCacheManager() const { return CacheManager; }

	UFUNCTION(BlueprintCallable, Category = "RSS Subsystems")
	void ConfigureCache(const FRssImageCacheConfig& Config);

	UFUNCTION(BlueprintCallable, Category = "RSS Subsystems")
	void ClearUnusedCachedImages();

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	void GetCacheStatistics(int32& MemoryCached, int32& DiskCached, int64& MemoryUsage, int64& DiskUsage) const;

	UPROPERTY(BlueprintReadWrite, Category = "RSS Subsystems")
	TMap<FString, FRssCustomSignUrlData> mStoredImages;

	UPROPERTY(BlueprintReadWrite, Category = "RSS Subsystems")
	FRssCustomDatas mInformation;

	UPROPERTY(EditDefaultsOnly, Category = "RSS Subsystems")
	FString mFileName = "RssCustomData.txt";

	UPROPERTY(EditDefaultsOnly, Category = "RSS Subsystems")
	FString mAddFilePath = "";

	UPROPERTY(Transient)
	TObjectPtr<URssDownloadImage> mDownloadTask;

	bool bRequestIsRunning = false;
	bool bAllowToRunInitQuery = false;
	bool bInitIsRunning = false;
	bool bInitDone = false;

	TArray<FInitRequests> mInitRequests;
	TArray<FRssSignRequestData> mRuntimeRequests;

	float mInitTimer = 0.f;

	// the Tick coalesces rapid changes and dispatches the file write to a background thread
	bool bSaveDataDirty = false;
	float mSaveDirtyAccumulator = 0.f;
	static constexpr float mSaveDebounceTime = 2.0f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<URssImageCacheManager> CacheManager;
};

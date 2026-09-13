// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/Map.h"
#include "Engine/Texture2DDynamic.h"
#include "HAL/CriticalSection.h"

#include "RssImageCache.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(RSSImageCacheLog, Log, All);

USTRUCT(BlueprintType)
struct RSS_API FRssImageCacheEntry
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly)
	TObjectPtr<UTexture2DDynamic> Texture = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FString Url;

	UPROPERTY(BlueprintReadOnly)
	int32 RefCount = 0;

	UPROPERTY(BlueprintReadOnly)
	FDateTime LastAccessTime;

	UPROPERTY(BlueprintReadOnly)
	int64 SizeInBytes = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bIsPersistent = false;

	UPROPERTY()
	FString ContentHash;

	FRssImageCacheEntry() : LastAccessTime(FDateTime::Now()) {}

	bool IsValid() const { return ::IsValid(Texture); }
};

USTRUCT(BlueprintType)
struct RSS_API FRssImageCacheConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 MaxMemoryCacheSize = 512 * 1024 * 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 MaxDiskCacheSize = 1024 * 1024 * 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UnusedTimeoutSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableDiskCache = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableMemoryCache = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCompressDiskCache = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CacheDirectory = TEXT("RSSImageCache");
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnImageCacheLoaded, const FString&, Url, UTexture2DDynamic*, Texture);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnImageCacheFailed, const FString&, Url);

UCLASS(BlueprintType)
class RSS_API URssImageCacheManager : public UObject
{
	GENERATED_BODY()

public:
	URssImageCacheManager();

	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache", meta = (WorldContext = "WorldContextObject"))
	static URssImageCacheManager* Get(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void Initialize(const FRssImageCacheConfig& Config);

	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void Shutdown();

	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	bool RequestImage(const FString& Url, bool bPersistent = true);

	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	bool GetCachedImage(const FString& Url, UTexture2DDynamic*& OutTexture);

	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	bool IsImageCached(const FString& Url) const;

	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	bool IsImageLoading(const FString& Url) const;

	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void AddReference(const FString& Url);

	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void RemoveReference(const FString& Url);

	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	int32 GetReferenceCount(const FString& Url) const;

	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void ForceRemoveFromCache(const FString& Url);

	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void ClearUnreferencedImages();

	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void ClearAllCache();

	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	int64 GetCurrentMemoryUsage() const;

	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	int64 GetCurrentDiskUsage() const;

	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	void GetCacheStats(int32& OutMemoryCachedCount, int32& OutDiskCachedCount, int32& OutPendingDownloads) const;

	UPROPERTY(BlueprintAssignable, Category = "RSS|Image Cache")
	FOnImageCacheLoaded OnImageLoaded;

	UPROPERTY(BlueprintAssignable, Category = "RSS|Image Cache")
	FOnImageCacheFailed OnImageFailed;

	void Tick(float DeltaTime);

protected:
	bool LoadFromDiskCache(const FString& Url, TArray<uint8>& OutData);
	bool SaveToDiskCache(const FString& Url, const TArray<uint8>& Data);
	bool RemoveFromDiskCache(const FString& Url);
	FString GetCacheFilePath(const FString& Url) const;
	static FString GenerateUrlHash(const FString& Url);
	void CheckMemoryPressure();
	void EvictLRUEntries(int64 TargetSize);
	void EvictLRUEntriesLocked(int64 TargetSize);
	bool StartDownload(const FString& Url, bool bPersistent);
	void AddToMemoryCache(const FString& Url, UTexture2DDynamic* Texture, int64 SizeInBytes, bool bPersistent);

private:
	UPROPERTY()
	FRssImageCacheConfig CacheConfig;

	UPROPERTY(Transient)
	TMap<FString, FRssImageCacheEntry> MemoryCache;
	mutable FCriticalSection CacheLock;

	TSet<FString> PendingDownloads;
	mutable FCriticalSection DownloadLock;

	TMap<FString, FString> DiskCacheIndex;
	int64 CurrentDiskUsage = 0;
	mutable FCriticalSection DiskLock;
	int64 CurrentMemoryUsage = 0;

	float MaintenanceTimer = 0.0f;
	static constexpr float MaintenanceInterval = 30.0f;

	static TMap<TWeakObjectPtr<UWorld>, URssImageCacheManager*> Instances;

	bool bIsInitialized = false;
};

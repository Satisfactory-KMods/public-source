// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Containers/Map.h"
#include "Engine/Texture2DDynamic.h"
#include "HAL/CriticalSection.h"

#include "RssImageCache.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(RSSImageCacheLog, Log, All);

/**
 * Cache entry with reference counting and metadata
 */
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

	// Hash for cache file validation
	UPROPERTY()
	FString ContentHash;

	FRssImageCacheEntry() : LastAccessTime(FDateTime::Now()) {}

	bool IsValid() const { return ::IsValid(Texture); }
};

/**
 * Configuration for the image cache
 */
USTRUCT(BlueprintType)
struct RSS_API FRssImageCacheConfig
{
	GENERATED_BODY()

	// Maximum memory cache size in bytes
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 MaxMemoryCacheSize = 512 * 1024 * 1024;

	// Maximum disk cache size in bytes
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 MaxDiskCacheSize = 1024 * 1024 * 1024;

	// Time in seconds before an unused cached image is eligible for removal
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UnusedTimeoutSeconds = 5.0f;

	// Enable disk caching
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableDiskCache = true;

	// Enable memory caching
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableMemoryCache = true;

	// Compress images on disk
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCompressDiskCache = true;

	// Cache directory relative to saved folder
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CacheDirectory = TEXT("RSSImageCache");
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnImageCacheLoaded, const FString&, Url, UTexture2DDynamic*, Texture);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnImageCacheFailed, const FString&, Url);

/**
 * Singleton image cache manager for efficient texture caching with reference counting
 */
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

	/**
	 * Request an image from cache or download it
	 * @param Url - The URL to load
	 * @param bPersistent - If true, the image will be saved to disk
	 * @return True if the request was started
	 */
	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	bool RequestImage(const FString& Url, bool bPersistent = true);

	/**
	 * Get a cached image immediately if available
	 * @param Url - The URL of the cached image
	 * @param OutTexture - The cached texture if found
	 * @return True if the image was found in cache
	 */
	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	bool GetCachedImage(const FString& Url, UTexture2DDynamic*& OutTexture);

	/**
	 * Check if an image is in the cache
	 */
	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	bool IsImageCached(const FString& Url) const;

	/**
	 * Check if an image is currently being downloaded
	 */
	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	bool IsImageLoading(const FString& Url) const;

	/**
	 * Add a reference to a cached image (prevents removal)
	 */
	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void AddReference(const FString& Url);

	/**
	 * Remove a reference from a cached image (allows removal when count reaches 0)
	 */
	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void RemoveReference(const FString& Url);

	/**
	 * Get the current reference count for an image
	 */
	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	int32 GetReferenceCount(const FString& Url) const;

	/**
	 * Force remove an image from cache (ignores reference count)
	 */
	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void ForceRemoveFromCache(const FString& Url);

	/**
	 * Clear all unreferenced images from cache
	 */
	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void ClearUnreferencedImages();

	/**
	 * Clear the entire cache
	 */
	UFUNCTION(BlueprintCallable, Category = "RSS|Image Cache")
	void ClearAllCache();

	/**
	 * Get current memory usage
	 */
	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	int64 GetCurrentMemoryUsage() const;

	/**
	 * Get current disk usage
	 */
	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	int64 GetCurrentDiskUsage() const;

	/**
	 * Get cache statistics
	 */
	UFUNCTION(BlueprintPure, Category = "RSS|Image Cache")
	void GetCacheStats(int32& OutMemoryCachedCount, int32& OutDiskCachedCount, int32& OutPendingDownloads) const;

	UPROPERTY(BlueprintAssignable, Category = "RSS|Image Cache")
	FOnImageCacheLoaded OnImageLoaded;

	UPROPERTY(BlueprintAssignable, Category = "RSS|Image Cache")
	FOnImageCacheFailed OnImageFailed;

	// Tick function for cache maintenance
	void Tick(float DeltaTime);

protected:
	bool LoadFromDiskCache(const FString& Url, TArray<uint8>& OutData);
	bool SaveToDiskCache(const FString& Url, const TArray<uint8>& Data);
	bool RemoveFromDiskCache(const FString& Url);
	FString GetCacheFilePath(const FString& Url) const;
	static FString GenerateUrlHash(const FString& Url);
	void CheckMemoryPressure();
	void EvictLRUEntries(int64 TargetSize);
	bool StartDownload(const FString& Url, bool bPersistent);
	void AddToMemoryCache(const FString& Url, UTexture2DDynamic* Texture, int64 SizeInBytes, bool bPersistent);

private:
	UPROPERTY()
	FRssImageCacheConfig CacheConfig;

	TMap<FString, FRssImageCacheEntry> MemoryCache;
	mutable FCriticalSection CacheLock;

	TSet<FString> PendingDownloads;
	mutable FCriticalSection DownloadLock;

	TMap<FString, FString> DiskCacheIndex; // URL -> Hash
	int64 CurrentDiskUsage = 0;
	mutable FCriticalSection DiskLock;
	int64 CurrentMemoryUsage = 0;

	float MaintenanceTimer = 0.0f;
	static constexpr float MaintenanceInterval = 30.0f;

	static TMap<TWeakObjectPtr<UWorld>, URssImageCacheManager*> Instances;

	bool bIsInitialized = false;
};

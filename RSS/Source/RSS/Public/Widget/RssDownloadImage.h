// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Interfaces/IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UObject/ObjectMacros.h"

#include "RssDownloadImage.generated.h"

class UTexture2DDynamic;
class URssImageCacheManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDownloadImage, UTexture2DDynamic*, Texture);

/**
 * Async image downloader with integrated caching support.
 * Uses URssImageCacheManager for persistent disk caching and memory management.
 */
UCLASS()
class RSS_API URssDownloadImage : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

	/**
	 * Download an image from URL with optional caching
	 * @param URL - The URL to download from
	 * @param bUseCache - If true, use the cache manager for caching
	 * @param bPersistToCache - If true, save to disk cache for persistence
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static URssDownloadImage* DownloadImage(UObject* WorldContextObject, FString URL, bool bUseCache = true,
											bool bPersistToCache = true);

	/**
	 * Download an image bypassing the cache (legacy behavior)
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static URssDownloadImage* DownloadImageNoCache(FString URL);

	UPROPERTY(BlueprintAssignable)
	FOnDownloadImage OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnDownloadImage OnFail;

	UPROPERTY(Transient)
	TObjectPtr<class ARSSImageSubsystem> Subsystem;

	UPROPERTY(Transient)
	TObjectPtr<URssImageCacheManager> CacheManager;

	/** If true, the image size will be halved */
	bool bHalfImageSize = false;

	/** If true, use the cache manager */
	bool bUseCache = true;

	/** If true, persist to disk cache */
	bool bPersistToCache = true;

	UPROPERTY()
	FString CurrentUrl;

	void Start(FString URL);
	void StartWithCache(FString URL, UObject* WorldContextObject);

	static bool IsSafeRemoteImageUrl(const FString& Url);
	static bool ValidateImageDimensions(int64 CompressedBytes, int32 Width, int32 Height, int64& OutRawBytes);

	static constexpr int64 MaxCompressedImageBytes = 16 * 1024 * 1024;
	static constexpr int32 MaxImageDimension = 8192;
	static constexpr int64 MaxImagePixels = 32 * 1024 * 1024;

protected:
	void Cleanup();

private:
	static void WriteTexture_RenderThread(FTexture2DDynamicResource* TextureResource, TArray64<uint8>* RawData,
										  bool bUseSRGB = true);

	void HandleImageRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	UFUNCTION()
	void OnCacheImageLoaded(const FString& Url, UTexture2DDynamic* Texture);

	UFUNCTION()
	void OnCacheImageFailed(const FString& Url);

	bool bWaitingForCache = false;
};

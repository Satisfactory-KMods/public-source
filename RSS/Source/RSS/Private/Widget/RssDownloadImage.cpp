// Copyright Epic Games, Inc. All Rights Reserved.

#include "Widget/RssDownloadImage.h"
#include "Cache/RssImageCache.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture2DDynamic.h"
#include "HttpModule.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Modules/ModuleManager.h"
#include "Subsystem/RSSImageSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(RSSImageDownloaderLog, Log, All)

DEFINE_LOG_CATEGORY(RSSImageDownloaderLog)

//----------------------------------------------------------------------//
// URssDownloadImage
//----------------------------------------------------------------------//


URssDownloadImage::URssDownloadImage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		AddToRoot();
	}
}

URssDownloadImage* URssDownloadImage::DownloadImage(UObject* WorldContextObject, FString URL, bool bUseCache,
													bool bPersistToCache)
{
	URssDownloadImage* DownloadTask = NewObject<URssDownloadImage>();
	DownloadTask->bUseCache = bUseCache;
	DownloadTask->bPersistToCache = bPersistToCache;

	if (bUseCache && WorldContextObject)
	{
		DownloadTask->StartWithCache(URL, WorldContextObject);
	}
	else
	{
		DownloadTask->Start(URL);
	}

	return DownloadTask;
}

URssDownloadImage* URssDownloadImage::DownloadImageNoCache(FString URL)
{
	URssDownloadImage* DownloadTask = NewObject<URssDownloadImage>();
	DownloadTask->bUseCache = false;
	DownloadTask->Start(URL);

	return DownloadTask;
}

void URssDownloadImage::WriteTexture_RenderThread(FTexture2DDynamicResource* TextureResource, TArray64<uint8>* RawData,
												  bool bUseSRGB)
{
#if !UE_SERVER
	check(IsInRenderingThread());

	if (TextureResource)
	{
		FRHITexture* TextureRHI = TextureResource->GetTexture2DRHI();

		int32 Width = TextureRHI->GetSizeX();
		int32 Height = TextureRHI->GetSizeY();

		uint32 DestStride = 0;
		uint8* DestData =
			reinterpret_cast<uint8*>(RHILockTexture2D(TextureRHI, 0, RLM_WriteOnly, DestStride, false, false));

		for (int32 y = 0; y < Height; y++)
		{
			uint8* DestPtr = &DestData[(static_cast<int64>(Height) - 1 - y) * DestStride];

			const FColor* SrcPtr = &((FColor*)(RawData->GetData()))[(static_cast<int64>(Height) - 1 - y) * Width];
			for (int32 x = 0; x < Width; x++)
			{
				*DestPtr++ = SrcPtr->B;
				*DestPtr++ = SrcPtr->G;
				*DestPtr++ = SrcPtr->R;
				*DestPtr++ = SrcPtr->A;
				SrcPtr++;
			}
		}

		RHIUnlockTexture2D(TextureRHI, 0, false, false);
	}

	delete RawData;
#endif
}

void URssDownloadImage::Start(FString URL)
{
#if !UE_SERVER
	// Create the Http request and add to pending request list
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindUObject(this, &URssDownloadImage::HandleImageRequest);
	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->ProcessRequest();
#else
	// On the server we don't execute fail or success we just don't fire the request.
	RemoveFromRoot();
#endif
}

void URssDownloadImage::HandleImageRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
#if !UE_SERVER

	RemoveFromRoot();

	if (bSucceeded && HttpResponse.IsValid() && HttpResponse->GetContentLength() > 0)
	{
		IImageWrapperModule& ImageWrapperModule =
			FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrappers[3] = {
			ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG),
			ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG),
			ImageWrapperModule.CreateImageWrapper(EImageFormat::BMP),
		};

		for (auto ImageWrapper : ImageWrappers)
		{
			if (ImageWrapper.IsValid() &&
				ImageWrapper->SetCompressed(HttpResponse->GetContent().GetData(), HttpResponse->GetContentLength()))
			{
				TArray64<uint8>* RawData = new TArray64<uint8>();
				constexpr ERGBFormat InFormat = ERGBFormat::BGRA;
				if (ImageWrapper->GetRaw(InFormat, 8, *RawData))
				{
					int32 Width = bHalfImageSize ? ImageWrapper->GetWidth() / 2 : ImageWrapper->GetWidth();
					int32 Height = bHalfImageSize ? ImageWrapper->GetHeight() / 2 : ImageWrapper->GetHeight();

					if (UTexture2DDynamic* Texture = UTexture2DDynamic::Create(Width, Height))
					{
						Texture->SRGB = true;
						Texture->UpdateResource();

						if (FTexture2DDynamicResource* TextureResource =
								static_cast<FTexture2DDynamicResource*>(Texture->GetResource()))
						{
							ENQUEUE_RENDER_COMMAND(FWriteRawDataToTexture)(
								[TextureResource, RawData](FRHICommandListImmediate& RHICmdList)
								{ WriteTexture_RenderThread(TextureResource, RawData); });
						}
						else
						{
							delete RawData;
						}

						// (RSSImageSubsystem.cpp), so calling Subsystem->OnDownloadSuccess() directly
						// was a double-fire. The direct call also crashed on the non-cache Start() path
						// where Subsystem is null.
						OnSuccess.Broadcast(Texture);
						return;
					}
				}
			}
		}
	}

	OnFail.Broadcast(nullptr);

#endif
}

void URssDownloadImage::StartWithCache(FString URL, UObject* WorldContextObject)
{
#if !UE_SERVER
	CurrentUrl = URL;

	// Try to get from cache first
	CacheManager = URssImageCacheManager::Get(WorldContextObject);
	if (CacheManager)
	{
		// Check if already cached in memory
		UTexture2DDynamic* CachedTexture = nullptr;
		if (CacheManager->GetCachedImage(URL, CachedTexture))
		{
			UE_LOG(RSSImageDownloaderLog, Verbose, TEXT("Image found in memory cache: %s"), *URL);
			CacheManager->AddReference(URL);

			if (Subsystem)
			{
				Subsystem->OnDownloadSuccess(CachedTexture);
			}
			OnSuccess.Broadcast(CachedTexture);
			Cleanup();
			return;
		}

		// Bind to cache events
		bWaitingForCache = true;
		CacheManager->OnImageLoaded.AddDynamic(this, &URssDownloadImage::OnCacheImageLoaded);
		CacheManager->OnImageFailed.AddDynamic(this, &URssDownloadImage::OnCacheImageFailed);

		// Request from cache (will load from disk or download)
		CacheManager->RequestImage(URL, bPersistToCache);
		return;
	}

	// Fallback to direct download
	Start(URL);
#else
	RemoveFromRoot();
#endif
}

void URssDownloadImage::OnCacheImageLoaded(const FString& Url, UTexture2DDynamic* Texture)
{
	if (!bWaitingForCache || Url != CurrentUrl)
	{
		return;
	}

	bWaitingForCache = false;

	// Unbind from cache events
	if (CacheManager)
	{
		CacheManager->OnImageLoaded.RemoveDynamic(this, &URssDownloadImage::OnCacheImageLoaded);
		CacheManager->OnImageFailed.RemoveDynamic(this, &URssDownloadImage::OnCacheImageFailed);
	}

	UE_LOG(RSSImageDownloaderLog, Verbose, TEXT("Image loaded via cache: %s"), *Url);

	OnSuccess.Broadcast(Texture);
	Cleanup();
}

void URssDownloadImage::OnCacheImageFailed(const FString& Url)
{
	if (!bWaitingForCache || Url != CurrentUrl)
	{
		return;
	}

	bWaitingForCache = false;

	// Unbind from cache events
	if (CacheManager)
	{
		CacheManager->OnImageLoaded.RemoveDynamic(this, &URssDownloadImage::OnCacheImageLoaded);
		CacheManager->OnImageFailed.RemoveDynamic(this, &URssDownloadImage::OnCacheImageFailed);
	}

	UE_LOG(RSSImageDownloaderLog, Warning, TEXT("Cache image load failed: %s"), *Url);

	OnFail.Broadcast(nullptr);
	Cleanup();
}

void URssDownloadImage::Cleanup()
{
	RemoveFromRoot();
	CacheManager = nullptr;
	Subsystem = nullptr;
}

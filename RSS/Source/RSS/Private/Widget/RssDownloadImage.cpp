#include "Widget/RssDownloadImage.h"
#include "Cache/RssImageCache.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture2DDynamic.h"
#include "HttpModule.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IHttpResponse.h"
#include "Modules/ModuleManager.h"
#include "SocketSubsystem.h"
#include "Subsystem/RSSImageSubsystem.h"
#include "TextureResource.h"

DECLARE_LOG_CATEGORY_EXTERN(RSSImageDownloaderLog, Log, All)

DEFINE_LOG_CATEGORY(RSSImageDownloaderLog)

namespace
{
	bool IsPublicIPv4Address(const FIPv4Address& Address)
	{
		return Address.A != 0 && Address.A != 10 && Address.A != 127 &&
			!(Address.A == 100 && Address.B >= 64 && Address.B <= 127) &&
			!(Address.A == 169 && Address.B == 254) &&
			!(Address.A == 172 && Address.B >= 16 && Address.B <= 31) &&
			!(Address.A == 192 && Address.B == 168) && Address.A < 224;
	}

	bool IsGlobalUnicastIPv6Address(FString Address)
	{
		Address.RemoveFromStart(TEXT("["));
		Address.RemoveFromEnd(TEXT("]"));

		FString FirstHextet;
		if (!Address.Split(TEXT(":"), &FirstHextet, nullptr) || FirstHextet.IsEmpty())
		{
			return false;
		}

		const uint32 FirstValue = FParse::HexNumber(*FirstHextet);
		return FirstValue >= 0x2000 && FirstValue <= 0x3fff;
	}

	bool IsPublicAddress(const FString& AddressString)
	{
		FString Address = AddressString.ToLower();
		Address.RemoveFromStart(TEXT("["));
		Address.RemoveFromEnd(TEXT("]"));

		FIPv4Address IPv4;
		if (FIPv4Address::Parse(Address, IPv4))
		{
			return IsPublicIPv4Address(IPv4);
		}

		if (Address.StartsWith(TEXT("::ffff:")))
		{
			return IsPublicAddress(Address.RightChop(7));
		}

		return IsGlobalUnicastIPv6Address(Address);
	}
}

URssDownloadImage::URssDownloadImage(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
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
	if (!IsSafeRemoteImageUrl(URL))
	{
		OnFail.Broadcast(nullptr);
		Cleanup();
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindUObject(this, &URssDownloadImage::HandleImageRequest);
	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->ProcessRequest();
#else
	OnFail.Broadcast(nullptr);
	Cleanup();
#endif
}

void URssDownloadImage::HandleImageRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
#if !UE_SERVER

	RemoveFromRoot();

	if (bSucceeded && HttpResponse.IsValid() && EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()) &&
		IsSafeRemoteImageUrl(HttpResponse->GetURL()))
	{
		IImageWrapperModule& ImageWrapperModule =
			FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		const TArray<uint8>& CompressedData = HttpResponse->GetContent();
		if (CompressedData.IsEmpty() || CompressedData.Num() > MaxCompressedImageBytes)
		{
			OnFail.Broadcast(nullptr);
			return;
		}
		const EImageFormat ImageFormat =
			ImageWrapperModule.DetectImageFormat(CompressedData.GetData(), CompressedData.Num());
		TSharedPtr<IImageWrapper> ImageWrappers[1];
		if (ImageFormat != EImageFormat::Invalid)
		{
			ImageWrappers[0] = ImageWrapperModule.CreateImageWrapper(ImageFormat);
		}

		for (auto ImageWrapper : ImageWrappers)
		{
			if (ImageWrapper.IsValid() &&
				ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
			{
				const int32 SourceWidth = ImageWrapper->GetWidth();
				const int32 SourceHeight = ImageWrapper->GetHeight();
				int64 RawBytes = 0;
				if (!ValidateImageDimensions(CompressedData.Num(), SourceWidth, SourceHeight, RawBytes))
				{
					continue;
				}

				TArray64<uint8>* RawData = new TArray64<uint8>();
				constexpr ERGBFormat InFormat = ERGBFormat::BGRA;
				if (ImageWrapper->GetRaw(InFormat, 8, *RawData))
				{
					int32 Width = bHalfImageSize ? SourceWidth / 2 : SourceWidth;
					int32 Height = bHalfImageSize ? SourceHeight / 2 : SourceHeight;

					if (UTexture2DDynamic* Texture = UTexture2DDynamic::Create(Width, Height))
					{
						Texture->SRGB = true;
						Texture->UpdateResource();

						if (FTexture2DDynamicResource* TextureResource =
							static_cast<FTexture2DDynamicResource*>(Texture->GetResource()))
						{
							ENQUEUE_RENDER_COMMAND(FWriteRawDataToTexture)(
								[TextureResource, RawData](FRHICommandListImmediate& RHICmdList)
								{
									WriteTexture_RenderThread(TextureResource, RawData);
								});
						}
						else
						{
							delete RawData;
						}

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
	if (!IsSafeRemoteImageUrl(URL))
	{
		OnFail.Broadcast(nullptr);
		Cleanup();
		return;
	}

	CurrentUrl = URL;

	CacheManager = URssImageCacheManager::Get(WorldContextObject);
	if (CacheManager)
	{

		UTexture2DDynamic* CachedTexture = nullptr;
		if (CacheManager->GetCachedImage(URL, CachedTexture))
		{
			UE_LOG(RSSImageDownloaderLog, Verbose, TEXT("Image found in memory cache: %s"), *URL);
			CacheManager->AddReference(URL);

			OnSuccess.Broadcast(CachedTexture);
			Cleanup();
			return;
		}

		bWaitingForCache = true;
		CacheManager->OnImageLoaded.AddDynamic(this, &URssDownloadImage::OnCacheImageLoaded);
		CacheManager->OnImageFailed.AddDynamic(this, &URssDownloadImage::OnCacheImageFailed);

		CacheManager->RequestImage(URL, bPersistToCache);
		return;
	}

	Start(URL);
#else
	OnFail.Broadcast(nullptr);
	Cleanup();
#endif
}

bool URssDownloadImage::IsStructurallySafeRemoteImageUrl(const FString& Url)
{
	if (Url.Len() > 2048 || Url.Contains(TEXT("@")))
	{
		return false;
	}

	const FString TrimmedUrl = Url.TrimStartAndEnd();
	const FURL ParsedUrl(nullptr, *TrimmedUrl, TRAVEL_Absolute);
	FString Host = ParsedUrl.Host.ToLower();
	Host.RemoveFromStart(TEXT("["));
	Host.RemoveFromEnd(TEXT("]"));
	while (Host.RemoveFromEnd(TEXT(".")))
	{
	}

	if (!ParsedUrl.Valid ||
		(!ParsedUrl.Protocol.Equals(TEXT("http"), ESearchCase::IgnoreCase) &&
		 !ParsedUrl.Protocol.Equals(TEXT("https"), ESearchCase::IgnoreCase)) ||
		Host.IsEmpty() || Host == TEXT("localhost") || Host.EndsWith(TEXT(".localhost")) ||
		Host.EndsWith(TEXT(".local")) || Host.EndsWith(TEXT(".internal")))
	{
		return false;
	}

	FIPv4Address IPv4;
	if (FIPv4Address::Parse(Host, IPv4))
	{
		return IsPublicIPv4Address(IPv4);
	}

	if (Host.Contains(TEXT(":")))
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		const TSharedPtr<FInternetAddr> Address =
			SocketSubsystem ? SocketSubsystem->GetAddressFromString(Host) : nullptr;
		return Address.IsValid() && Address->IsValid() && IsGlobalUnicastIPv6Address(Address->ToString(false));
	}

	bool bLooksLikeIPv4 = !Host.IsEmpty();
	for (const TCHAR Character : Host)
	{
		bLooksLikeIPv4 &= FChar::IsDigit(Character) || Character == TEXT('.');
	}
	return !bLooksLikeIPv4;
}

bool URssDownloadImage::IsSafeRemoteImageUrl(const FString& Url)
{
	if (!IsStructurallySafeRemoteImageUrl(Url))
	{
		return false;
	}

	const FURL ParsedUrl(nullptr, *Url.TrimStartAndEnd(), TRAVEL_Absolute);
	FString Host = ParsedUrl.Host.ToLower();
	Host.RemoveFromStart(TEXT("["));
	Host.RemoveFromEnd(TEXT("]"));
	while (Host.RemoveFromEnd(TEXT(".")))
	{
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return false;
	}

	const FAddressInfoResult AddressInfo = SocketSubsystem->GetAddressInfo(
		*Host, nullptr, EAddressInfoFlags::Default, NAME_None);
	if (AddressInfo.ReturnCode != SE_NO_ERROR || AddressInfo.Results.IsEmpty())
	{
		return false;
	}

	for (const FAddressInfoResultData& Result : AddressInfo.Results)
	{
		if (!IsPublicAddress(Result.Address->ToString(false)))
		{
			return false;
		}
	}
	return true;
}

bool URssDownloadImage::ValidateImageDimensions(int64 CompressedBytes, int32 Width, int32 Height, int64& OutRawBytes)
{
	OutRawBytes = 0;
	if (CompressedBytes <= 0 || CompressedBytes > MaxCompressedImageBytes || Width <= 0 || Height <= 0 ||
		Width > MaxImageDimension || Height > MaxImageDimension || Width > MAX_int64 / Height)
	{
		return false;
	}

	const int64 Pixels = static_cast<int64>(Width) * Height;
	if (Pixels > MaxImagePixels || Pixels > MAX_int64 / 4)
	{
		return false;
	}

	OutRawBytes = Pixels * 4;
	return true;
}

void URssDownloadImage::OnCacheImageLoaded(const FString& Url, UTexture2DDynamic* Texture)
{
	if (!bWaitingForCache || Url != CurrentUrl)
	{
		return;
	}

	bWaitingForCache = false;

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

// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Subsystem/SBSApiSubsystem.h"

#include "Download/SBSDownloadIdProtocol.h"
#include "Http/SBSHttp.h"

FGuid USBSApiSubsystem::ResolveDownloadId(FString Input)
{
	check(IsInGameThread());
	FSBSResolvedDownload Download;
	Download.RequestId = FGuid::NewGuid();
	const FGuid RequestId = Download.RequestId;
	Download.Result = FSBSDownloadIdProtocol::ParseInput(Input, Download.Reference);
	if (!bInitialized || bChangingConfiguration)
	{
		Download.Result = FSBSOperationResult::Make(
			TEXT("resolve_unavailable"),
			NSLOCTEXT("SBS", "DownloadId.Unavailable", "SBS is not ready to resolve download IDs."));
	}
	else if (mResolveRequest)
	{
		Download.Result = FSBSOperationResult::Make(
			TEXT("resolve_busy"),
			NSLOCTEXT("SBS", "DownloadId.Busy", "Another download ID is being resolved. Wait or cancel it first."));
	}
	if (!Download.Result.bSuccess)
	{
		mOnDownloadIdResolved.Broadcast(Download);
		return RequestId;
	}
	BindConfiguration();
	mResolveId = RequestId;
	mResolveReference = Download.Reference;
	mResolveBase = FSBSStatics::MakeUrl(TEXT(""), this);
	mResolveWorld = GetWorld();
	mResolveRequest = FSBSRequest::Create(mResolveBase + TEXT("mod/resolveid"), TEXT("POST"),
										  FSBSDownloadIdProtocol::MakePayload(mResolveReference), this,
										  FSBSDownloadIdProtocol::MaxResponseBytes);
	mResolveRequest->mRequest->OnProcessRequestComplete().BindUObject(this, &USBSApiSubsystem::OnResolveDownloadIdDone);
	if (!mResolveRequest->Start())
	{
		OnResolveDownloadIdDone(mResolveRequest->mRequest, nullptr, false);
	}
	return RequestId;
}

void USBSApiSubsystem::CancelResolveDownloadId()
{
	check(IsInGameThread());
	if (!mResolveRequest)
	{
		return;
	}
	FSBSResolvedDownload Download;
	Download.Result = FSBSOperationResult::Make(
		TEXT("resolve_cancelled"), NSLOCTEXT("SBS", "DownloadId.Cancelled", "Download ID lookup cancelled."));
	FinishResolveDownloadId(MoveTemp(Download));
}

void USBSApiSubsystem::OnResolveDownloadIdDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!mResolveRequest || Request != mResolveRequest->mRequest || !bInitialized)
	{
		return;
	}
	FSBSResolvedDownload Download;
	const int32 Status = Response ? Response->GetResponseCode() : 0;
	TSharedPtr<FJsonObject> Json;
	if (mResolveWorld.IsStale() || mResolveWorld.Get() != GetWorld() ||
		mResolveBase != FSBSStatics::MakeUrl(TEXT(""), this))
	{
		Download.Result = FSBSOperationResult::Make(
			TEXT("resolve_context_changed"),
			NSLOCTEXT("SBS", "DownloadId.ContextChanged",
					  "The game session or SBS service changed. Enter the download ID again."));
	}
	else if (mResolveRequest->mBody->IsError())
	{
		Download.Result = FSBSOperationResult::Make(
			TEXT("resolve_response_too_large"),
			NSLOCTEXT("SBS", "DownloadId.ResponseTooLarge", "SBS returned too much data for this download ID."), false,
			Status);
	}
	else if (!bSuccess || Status != 200)
	{
		Download.Result = FSBSDownloadIdProtocol::HttpFailure(bSuccess ? Status : 0);
		if (Status == 429 && Response)
		{
			const FString Retry = Response->GetHeader(TEXT("Retry-After"));
			bool bDigits = !Retry.IsEmpty() && Retry.Len() <= 5;
			for (TCHAR Character : Retry)
			{
				bDigits &= Character >= '0' && Character <= '9';
			}
			if (bDigits)
			{
				Download.Result.RetryAfterSeconds = FMath::Clamp(FCString::Atoi(*Retry), 0, 86400);
			}
		}
	}
	else if (!mResolveRequest->ReadJson(Response, bSuccess, Json) ||
			 !FSBSDownloadIdProtocol::ParseResponse(Json, mResolveReference, Download, this))
	{
		Download.Result = FSBSOperationResult::Make(
			TEXT("invalid_resolve_response"),
			NSLOCTEXT("SBS", "DownloadId.InvalidResponse", "SBS returned invalid or incomplete download information."),
			false, Status);
	}
	FinishResolveDownloadId(MoveTemp(Download));
}

void USBSApiSubsystem::FinishResolveDownloadId(FSBSResolvedDownload Download)
{
	Download.RequestId = mResolveId;
	if (!Download.Result.bSuccess)
	{
		Download.Reference = mResolveReference;
	}
	mResolveRequest->Cancel();
	mResolveRequest.Reset();
	mResolveId.Invalidate();
	mResolveReference = FSBSDownloadReference();
	mResolveBase.Reset();
	mResolveWorld.Reset();
	if (bInitialized)
	{
		mOnDownloadIdResolved.Broadcast(Download);
	}
}

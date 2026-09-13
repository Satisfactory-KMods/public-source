// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "TimerManager.h"

RSS_SIGN_IMPL_CLASS::RSS_SIGN_IMPL_CLASS()
{
	bReplicates = true;

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	mScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	mScreenMesh->SetupAttachment(RootComponent);

	this->mFactoryTickFunction.bCanEverTick = false;
}

TArray<FRssMeshInfo> RSS_SIGN_IMPL_CLASS::GetRenderMeshInfos_Implementation()
{
	if (URssWidgetRenderComponent* Comp = FindComponentByClass<URssWidgetRenderComponent>())
	{
		return Comp->mMeshInfo;
	}
	return {};
}

TArray<FRssMeshInfo> RSS_SIGN_IMPL_CLASS::GetScreenMeshInfos_Implementation()
{
	if (mScreenMesh)
	{
		return {FRssMeshInfo(mScreenMesh, mScreenMaterialIndex)};
	}
	return {};
}

UActorComponent* RSS_SIGN_IMPL_CLASS::GetRenderComponent_Implementation()
{
	return FindComponentByClass<URssWidgetRenderComponent>();
}

void RSS_SIGN_IMPL_CLASS::PasteSignData_Implementation(FRssSignData Data)
{
	if (mSignData.mSignTypeSize != Data.mSignTypeSize || mSignData.mSignType != Data.mSignType)
	{
		return;
	}
	Execute_UpdateSignData(this, Data);
}

void RSS_SIGN_IMPL_CLASS::UpdateSign_Implementation() { URssBlueprintFunctionLibrary::UpdateSignGeneralFunction(this); }

void RSS_SIGN_IMPL_CLASS::UpdateSignData_Implementation(FRssSignData Data)
{
	if (mSignData.mSignTypeSize != Data.mSignTypeSize || mSignData.mSignType != Data.mSignType)
	{
		return;
	}
	SetSignData(Data);
}

void RSS_SIGN_IMPL_CLASS::ApplySignData_Implementation(FRssSignData Data)
{
	mSignData = Data;
	ValidateCustom();
	Execute_UpdateSign(this);
}

void RSS_SIGN_IMPL_CLASS::UpdateSignDisplayWidget_Implementation()
{
	if (mScreenRender)
	{
		mScreenRender->RequestUpdateComponent();
	}
}

void RSS_SIGN_IMPL_CLASS::UpdateSignToCustomImage_Implementation(FRssSignRequestData Request)
{
	URssBlueprintFunctionLibrary::RequestUpgradeSignToCustom(this, Request);
}

void RSS_SIGN_IMPL_CLASS::RequestCustomSign_Implementation(FRssSignRequestData Request, bool ComesFromServer)
{
	if (HasAuthority())
	{
		RequestCustomSignMulticast(Request);
	}
	else
	{

		if (URSSSignRCO* RCO = URSSSignRCO::Get(this))
		{
			RCO->RCO_Server_RequestSignData(this, Request);
		}
	}
}

void RSS_SIGN_IMPL_CLASS::OnRep_SignDataDirty()
{
	Execute_UpdateSign(this);

	ValidateCustom();
}

void RSS_SIGN_IMPL_CLASS::ValidateCustom() { URssBlueprintFunctionLibrary::ValidateCustomData(this); }

void RSS_SIGN_IMPL_CLASS::ValidateCustomDeferred()
{
	if (URssBlueprintFunctionLibrary::GetImageSubsystem(this))
	{
		ValidateCustom();
		return;
	}

	TWeakObjectPtr<RSS_SIGN_IMPL_CLASS> WeakThis(this);
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda(
		[WeakThis]()
		{
			if (RSS_SIGN_IMPL_CLASS* Self = WeakThis.Get())
			{
				Self->ValidateCustomDeferred();
			}
		}));
}

void RSS_SIGN_IMPL_CLASS::RequestCustomSignMulticast_Implementation(FRssSignRequestData Request)
{
	if (ARSSImageSubsystem* Subsystem = Cast<ARSSImageSubsystem>(UKBFL_Util::GetSubsystem(GetWorld(), mSubsystemClass)))
	{
		Subsystem->onRequestImages(Request);
	}
}

void RSS_SIGN_IMPL_CLASS::BeginPlay()
{
	Super::BeginPlay();

	mScreenRender = Cast<URssWidgetRenderComponent>(GetComponentByClass(URssWidgetRenderComponent::StaticClass()));

	Execute_UpdateSign(this);
	ValidateCustomDeferred();

	if (ARssDataManagerSubsystem* DataManager = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(GetWorld()))
	{
		DataManager->AcquirePooledComponent(this, mWidgetRenderClass);
	}
}

void RSS_SIGN_IMPL_CLASS::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	ReleaseCacheReferences();

	if (EndPlayReason == EEndPlayReason::Destroyed)
	{

		URssBlueprintFunctionLibrary::SignActorLostSignificant(this);
	}
	Super::EndPlay(EndPlayReason);
}

void RSS_SIGN_IMPL_CLASS::ReleaseCacheReferences()
{
	if (mSubsystem)
	{
		URssImageCacheManager* CacheManager = mSubsystem->GetCacheManager();
		if (CacheManager)
		{
			for (const FRssElement& Element : mSignData.mElements)
			{
				if (Element.mSharedData.mIsUsingCustom && !Element.mSharedData.mUrl.IsEmpty())
				{
					CacheManager->RemoveReference(Element.mSharedData.mUrl);
				}
			}
		}
	}
}

void RSS_SIGN_IMPL_CLASS::SetSignData(FRssSignData Data)
{
	mSignData = Data;

	if (!HasAuthority())
	{

		if (URSSSignRCO* RCO = URSSSignRCO::Get(this))
		{
			RCO->RCO_Server_UpdateSignData(this, Data);
		}
	}
	else
	{
		ForceNetUpdate();

		SignDataDirty++;
		if (SignDataDirty > 10)
		{
			SignDataDirty = 0;
		}
	}

	OnRep_SignDataDirty();
}

void RSS_SIGN_IMPL_CLASS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(RSS_SIGN_IMPL_CLASS, mSignData);
	DOREPLIFETIME(RSS_SIGN_IMPL_CLASS, SignDataDirty);
}

void RSS_SIGN_IMPL_CLASS::GainedSignificance_Implementation()
{
	URssBlueprintFunctionLibrary::SignActorGetSignificant(this);
}

void RSS_SIGN_IMPL_CLASS::LostSignificance_Implementation()
{
	URssBlueprintFunctionLibrary::SignActorLostSignificant(this);
}

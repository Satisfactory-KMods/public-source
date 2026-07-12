#include "OutlineSystem/KPCLOutlineSubsystem.h"

#include "EngineUtils.h"
#include "Kismet/KismetMathLibrary.h"
#include "Replication/KPCLDefaultRCO.h"

#include "BFL/KBFL_Util.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "OutlineSystem/KPCLOutlineActor.h"

AKPCLOutlineSubsystem* AKPCLOutlineSubsystem::Get(UObject* worldContext)
{
	return UKBFL_Util::GetSubsystem<AKPCLOutlineSubsystem>(worldContext);
}

AKPCLOutlineSubsystem::AKPCLOutlineSubsystem()
{
	mScene = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(mScene);

	mPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	mPostProcess->SetupAttachment(GetRootComponent());

	for (UMaterialInterface* PPMaterial : mPPMaterials)
	{
		if (PPMaterial)
		{
			FWeightedBlendable Blendable = FWeightedBlendable();
			Blendable.Object = PPMaterial;
			Blendable.Weight = 1.0f;
			mPostProcess->Settings.WeightedBlendables.Array.Add(Blendable);
		}
	}
}

void AKPCLOutlineSubsystem::BeginPlay()
{
	Super::BeginPlay();

	SetHidden(false);
	SetActorHiddenInGame(false);

	if (mMaterialCollection)
	{
		mMaterialCollectionInstance = GetWorld()->GetParameterCollectionInstance(mMaterialCollection);
	}

	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		if (It)
		{
			for (UMaterialInterface* PPMaterial : mPPMaterials)
			{
				if (PPMaterial)
				{
					FWeightedBlendable Blendable = FWeightedBlendable();
					Blendable.Object = PPMaterial;
					Blendable.Weight = 1.0f;
					It->Settings.WeightedBlendables.Array.Add(Blendable);
				}
			}
		}
	}
}

void AKPCLOutlineSubsystem::MultiCast_CreateOutlineForActor_Implementation(FOutlineData OutlineData)
{
	CreateOutline(OutlineData);
}

void AKPCLOutlineSubsystem::MultiCast_ClearOutlines_Implementation() { ClearOutlines(); }

void AKPCLOutlineSubsystem::MultiCast_SetOutlineColor_Implementation(FLinearColor Color, EOutlineColorSlot ColorSlot)
{
	SetOutlineColor(Color, ColorSlot);
}

void AKPCLOutlineSubsystem::MultiCast_ClearOutlinesForActor_Implementation(AActor* Actor)
{
	ClearOutlinesForActor(Actor);
}

void AKPCLOutlineSubsystem::CreateOutline(FOutlineData OutlineData, bool Multicast)
{
	if (OutlineData.mActorToOutline)
	{
		AActor* Actor = OutlineData.mActorToOutline;
		if (Multicast)
		{
			if (HasAuthority())
			{
				MultiCast_CreateOutlineForActor(OutlineData);
			}
			else
			{
				UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld());
				if (RCO)
				{
					RCO->Server_CreateOutlineForActor(this, OutlineData);
				}
			}
		}
		else
		{
			if (Actor)
			{
				if (mOutlineMap.Contains(Actor))
				{
					if (AKPCLOutlineActor* OutlineActor = GetOutlineActorForActor(Actor))
					{
						if (OutlineActor->mOutlineData.mOutlineType != OutlineData.mOutlineType ||
							OutlineActor->mOutlineData.mOutlineColorSlot != OutlineData.mOutlineColorSlot)
						{
							ClearOutlinesForActor(Actor);
						}
						else
						{
							return;
						}
					}
				}

				FTransform Transform = Actor->GetTransform();
				UWorld* SpawnWorld = GetWorld();
				if (SpawnWorld)
				{
					if (AKPCLOutlineActor* OutlineActor = SpawnWorld->SpawnActorDeferred<AKPCLOutlineActor>(
							AKPCLOutlineActor::StaticClass(), Transform, SpawnWorld->GetFirstPlayerController()))
					{
						if (OutlineActor->CreateOutlineFromActor(OutlineData))
						{
							OutlineActor->FinishSpawning(Transform, true);
							mOutlineMap.Add(Actor, OutlineActor);
						}
						else
						{
							OutlineActor->Destroy();
						}
					}
				}
			}
		}
	}
}

void AKPCLOutlineSubsystem::ClearOutlines(bool Multicast)
{
	if (Multicast)
	{
		if (HasAuthority())
		{
			MultiCast_ClearOutlines();
		}
		else
		{
			UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld());
			if (RCO)
			{
				RCO->Server_ClearOutlines(this);
			}
		}
	}
	else
	{
		if (mOutlineMap.Num() > 0)
		{
			for (auto OutlineActor : mOutlineMap)
			{
				if (OutlineActor.Value)
				{
					OutlineActor.Value->Destroy();
				}
			}
		}
		mOutlineMap.Empty();
	}
}

void AKPCLOutlineSubsystem::ClearOutlinesForActor(AActor* Actor, bool Multicast)
{
	if (Actor)
	{
		if (Multicast)
		{
			if (HasAuthority())
			{
				MultiCast_ClearOutlinesForActor(Actor);
			}
			else
			{
				UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld());
				if (RCO)
				{
					RCO->Server_ClearOutlineForActor(this, Actor);
				}
			}
		}
		else
		{
			if (mOutlineMap.Contains(Actor))
			{
				mOutlineMap[Actor]->Destroy();
				mOutlineMap.Remove(Actor);
			}
		}
	}
}

void AKPCLOutlineSubsystem::SetOutlineColor(FLinearColor Color, EOutlineColorSlot ColorSlot, bool Multicast)
{
	if (Multicast)
	{
		if (HasAuthority())
		{
			MultiCast_SetOutlineColor(Color, ColorSlot);
		}
		else
		{
			UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld());
			if (RCO)
			{
				RCO->Server_SetOutlineColor(this, Color, ColorSlot);
			}
		}
	}
	else
	{
		uint8 OutlineSlot =
			UKismetMathLibrary::FTrunc(UKismetMathLibrary::SafeDivide(static_cast<uint8>(ColorSlot), 3));

		if (mMaterialCollectionInstance && mMaterialCollectionTags.IsValidIndex(OutlineSlot))
		{
			mMaterialCollectionInstance->SetVectorParameterValue(mMaterialCollectionTags[OutlineSlot].mTag, Color);
		}
	}
}

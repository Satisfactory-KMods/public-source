// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/RSSTemplateSubsystem.h"

#include "FGPlayerController.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ARSSTemplateSubsystem::ARSSTemplateSubsystem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

void ARSSTemplateSubsystem::BeginPlay()
{
	Super::BeginPlay();

	if (!mWasWipeAfterFix && HasAuthority())
	{
		mTemplates.Empty();
		mWasWipeAfterFix = true;
	}
}

void ARSSTemplateSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARSSTemplateSubsystem, mTemplates);
}

TArray<FRssTemplateSignData> ARSSTemplateSubsystem::GetTemplates(ESignType SignType)
{
	TArray<FRssTemplateSignData> Array;
	for (int i = 0; i < mTemplates.Num(); ++i)
	{
		FRssTemplateSignData Info;
		Info.mIndex = i;
		Info.mSignData = mTemplates[i];
		if (mTemplates[i].mSignType == SignType)
		{
			Array.Add(Info);
		}
	}
	return Array;
}

TArray<FRssTemplateSignData> ARSSTemplateSubsystem::GetAllTemplates()
{
	TArray<FRssTemplateSignData> Array;
	for (int i = 0; i < mTemplates.Num(); ++i)
	{
		FRssTemplateSignData Info;
		Info.mIndex = i;
		Info.mSignData = mTemplates[i];
		Array.Add(Info);
	}

	return Array;
}

TArray<FRssTemplateSignData> ARSSTemplateSubsystem::GetTemplatesOfSignSize(ESignSize SignSize, ESignType SignType) const
{
	TArray<FRssTemplateSignData> Array;
	for (int i = 0; i < mTemplates.Num(); ++i)
	{
		FRssTemplateSignData Info;
		Info.mIndex = i;
		Info.mSignData = mTemplates[i];
		if (mTemplates[i].mSignTypeSize == SignSize && mTemplates[i].mSignType == SignType)
		{
			Array.Add(Info);
		}
	}
	return Array;
}

TArray<FRssTemplateSignData> ARSSTemplateSubsystem::GetAllTemplatesOfSignSize(ESignSize SignSize) const
{
	TArray<FRssTemplateSignData> Array;
	for (int i = 0; i < mTemplates.Num(); ++i)
	{
		FRssTemplateSignData Info;
		Info.mIndex = i;
		Info.mSignData = mTemplates[i];
		if (mTemplates[i].mSignTypeSize == SignSize)
		{
			Array.Add(Info);
		}
	}
	return Array;
}

void ARSSTemplateSubsystem::RenameTemplate(const FText& NewName, int TemplateIdx)
{
	if (mTemplates.IsValidIndex(TemplateIdx))
	{
		if (HasAuthority())
		{
			mTemplates[TemplateIdx].mTemplateData.mTemplateName = NewName;
		}
		else if (GetWorld())
		{
			if (AFGPlayerController* PlayerController =
					Cast<AFGPlayerController>(GetWorld()->GetFirstPlayerController()))
			{
				auto rco = Cast<URSSTemplateSubsystemRCO>(
					PlayerController->GetRemoteCallObjectOfClass(URSSTemplateSubsystemRCO::StaticClass()));

				if (rco)
				{
					rco->RCO_RenameTemplate(this, NewName, TemplateIdx);
				}
			}
		}
	}
	ForceNetUpdate();
}

void ARSSTemplateSubsystem::AddTemplate(FRssSignData Data)
{
	if (HasAuthority())
	{
		mTemplates.Add(Data);
	}
	else if (GetWorld())
	{
		if (AFGPlayerController* PlayerController = Cast<AFGPlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			auto rco = Cast<URSSTemplateSubsystemRCO>(
				PlayerController->GetRemoteCallObjectOfClass(URSSTemplateSubsystemRCO::StaticClass()));

			if (rco)
			{
				rco->RCO_AddTemplate(this, Data);
			}
		}
	}
	ForceNetUpdate();
}

void ARSSTemplateSubsystem::RemoveTemplate(int TemplateIdx)
{
	if (HasAuthority())
	{
		if (!mTemplates.IsValidIndex(TemplateIdx))
		{
			return;
		}
		mTemplates.RemoveAt(TemplateIdx);
	}
	else if (GetWorld())
	{
		if (AFGPlayerController* PlayerController = Cast<AFGPlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			auto rco = Cast<URSSTemplateSubsystemRCO>(
				PlayerController->GetRemoteCallObjectOfClass(URSSTemplateSubsystemRCO::StaticClass()));

			if (rco)
			{
				rco->RCO_RemoveTemplate(this, TemplateIdx);
			}
		}
	}
	ForceNetUpdate();
}

void URSSTemplateSubsystemRCO::RCO_AddTemplate_Implementation(ARSSTemplateSubsystem* Subsystem, FRssSignData Data)
{
	Subsystem->mTemplates.Add(Data);
	Subsystem->ForceNetUpdate();
}

bool URSSTemplateSubsystemRCO::RCO_AddTemplate_Validate(ARSSTemplateSubsystem* Subsystem, FRssSignData Data)
{
	return true;
}

// RCO
void URSSTemplateSubsystemRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URSSTemplateSubsystemRCO, bDummy);
}

void URSSTemplateSubsystemRCO::RCO_RemoveTemplate_Implementation(ARSSTemplateSubsystem* Subsystem, int TemplateIdx)
{
	if (!IsValid(Subsystem) || !Subsystem->mTemplates.IsValidIndex(TemplateIdx))
	{
		return;
	}
	Subsystem->mTemplates.RemoveAt(TemplateIdx);
	Subsystem->ForceNetUpdate();
}

bool URSSTemplateSubsystemRCO::RCO_RemoveTemplate_Validate(ARSSTemplateSubsystem* Subsystem, int TemplateIdx)
{
	return true;
}

void URSSTemplateSubsystemRCO::RCO_RenameTemplate_Implementation(ARSSTemplateSubsystem* Subsystem, const FText& NewName,
																 int TemplateIdx)
{
	if (!IsValid(Subsystem) || !Subsystem->mTemplates.IsValidIndex(TemplateIdx))
	{
		return;
	}
	Subsystem->mTemplates[TemplateIdx].mTemplateData.mTemplateName = NewName;
	Subsystem->ForceNetUpdate();
}

bool URSSTemplateSubsystemRCO::RCO_RenameTemplate_Validate(ARSSTemplateSubsystem* Subsystem, const FText& NewName,
														   int TemplateIdx)
{
	return true;
}

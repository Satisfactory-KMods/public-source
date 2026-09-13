// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structures/SBSDownloadIdTypes.h"
#include "SBSDownloadIdValidationReceiver.generated.h"

class USBSApiSubsystem;

UCLASS()
class USBSDownloadIdValidationReceiver : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void Receive(FSBSResolvedDownload Download);

	TArray<FSBSResolvedDownload> Results;
	UPROPERTY()
	TObjectPtr<USBSApiSubsystem> Api;
	bool bResolveAgain = false;
};

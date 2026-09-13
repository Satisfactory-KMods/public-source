// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"

#include "KDFChatCommand.generated.h"

UCLASS()
class KDATAFORGE_API AKDFChatCommand : public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AKDFChatCommand();

	virtual EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments,
														   const FString& Label) override;
};

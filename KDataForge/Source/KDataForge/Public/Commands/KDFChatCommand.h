#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"

#include "KDFChatCommand.generated.h"

/**
 * `/kdf <reload|report>` chat command.
 *  - reload: reverts applied patches, re-scans the DataForge directories, and re-applies live-safe
 *    stages (gameplay tags, localization, CDO changes, runtime patches). New content types still
 *    require a session restart — the SML content registry freezes at world begin play.
 *  - report: prints the current load report (packs, documents, ops, diagnostics).
 */
UCLASS()
class KDATAFORGE_API AKDFChatCommand : public AChatCommandInstance
{
	GENERATED_BODY()

public:
	AKDFChatCommand();

	virtual EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments,
														   const FString& Label) override;
};

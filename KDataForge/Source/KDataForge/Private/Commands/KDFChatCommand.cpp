#include "Commands/KDFChatCommand.h"

#include "Command/CommandSender.h"
#include "HAL/IConsoleManager.h"
#include "Subsystems/KDFSubsystem.h"

AKDFChatCommand::AKDFChatCommand()
{
	CommandName = TEXT("kdf");
	Usage = NSLOCTEXT("KDataForge", "KDFCommandUsage", "/kdf <report|editor|debug on|off> — KDataForge data editor");
	MinNumberOfArguments = 1;
	bOnlyUsableByPlayer = false;
}

EExecutionStatus AKDFChatCommand::ExecuteCommand_Implementation(UCommandSender* Sender,
																const TArray<FString>& Arguments, const FString& Label)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(this);
	if (Subsystem == nullptr)
	{
		Sender->SendChatMessage(TEXT("KDataForge subsystem is not available"), FLinearColor::Red);
		return EExecutionStatus::UNCOMPLETED;
	}

	const FString& SubCommand = Arguments[0];
	if (SubCommand.Equals(TEXT("editor"), ESearchCase::IgnoreCase))
	{
		// Host-local UI: the editor module resolves the local player controller itself.
		Subsystem->RequestToggleEditor(nullptr);
		Sender->SendChatMessage(TEXT("KDataForge editor toggled"));
		return EExecutionStatus::COMPLETED;
	}
	if (SubCommand.Equals(TEXT("debug"), ESearchCase::IgnoreCase))
	{
		IConsoleVariable* DebugCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("KDF.Debug"));
		if (DebugCVar == nullptr)
		{
			Sender->SendChatMessage(TEXT("KDF.Debug console variable not found"), FLinearColor::Red);
			return EExecutionStatus::UNCOMPLETED;
		}
		bool bEnable = !DebugCVar->GetBool(); // bare "/kdf debug" toggles
		if (Arguments.Num() >= 2)
		{
			const FString& Mode = Arguments[1];
			if (Mode.Equals(TEXT("on"), ESearchCase::IgnoreCase) || Mode == TEXT("1"))
			{
				bEnable = true;
			}
			else if (Mode.Equals(TEXT("off"), ESearchCase::IgnoreCase) || Mode == TEXT("0"))
			{
				bEnable = false;
			}
			else
			{
				PrintCommandUsage(Sender);
				return EExecutionStatus::BAD_ARGUMENTS;
			}
		}
		DebugCVar->Set(bEnable ? 1 : 0, ECVF_SetByConsole);
		Sender->SendChatMessage(FString::Printf(
			TEXT("KDataForge debug logging %s — all applied ops now %s to the log as [KDF DEBUG]"),
			bEnable ? TEXT("ENABLED") : TEXT("disabled"), bEnable ? TEXT("print") : TEXT("no longer print")));
		return EExecutionStatus::COMPLETED;
	}
	if (SubCommand.Equals(TEXT("report"), ESearchCase::IgnoreCase))
	{
		TArray<FString> Lines;
		Subsystem->BuildReportString().ParseIntoArrayLines(Lines);
		for (const FString& Line : Lines)
		{
			Sender->SendChatMessage(Line);
		}
		return EExecutionStatus::COMPLETED;
	}

	PrintCommandUsage(Sender);
	return EExecutionStatus::BAD_ARGUMENTS;
}

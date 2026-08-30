#include "Net/KDFRemoteCallObject.h"

#include "Engine/World.h"
#include "FGPlayerController.h"
#include "KDFLogging.h"
#include "KDFNode.h"
#include "Net/UnrealNetwork.h"
#include "Reflection/KDFOpEngine.h"
#include "Reflection/KDFValueCodec.h"
#include "Subsystems/KDFSubsystem.h"

UKDFRemoteCallObject* UKDFRemoteCallObject::Get(UWorld* World)
{
	if (World == nullptr)
	{
		return nullptr;
	}
	if (AFGPlayerController* Controller = Cast<AFGPlayerController>(World->GetFirstPlayerController()))
	{
		return Controller->GetRemoteCallObjectOfClass<UKDFRemoteCallObject>();
	}
	return nullptr;
}

void UKDFRemoteCallObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UKDFRemoteCallObject, mDummy);
}

bool UKDFRemoteCallObject::Server_ApplySetOp_Validate(const FString& TargetPath, const FString& PropertyPath,
													  const FString& ValueText)
{
	const AFGPlayerController* OwnerController = GetOwnerPlayerController();
	return OwnerController != nullptr && OwnerController->IsLocalController() && !TargetPath.IsEmpty() &&
		!PropertyPath.IsEmpty() && TargetPath.Len() <= 1024 && PropertyPath.Len() <= 2048 &&
		ValueText.Len() <= 1024 * 1024;
}

void UKDFRemoteCallObject::Server_ApplySetOp_Implementation(const FString& TargetPath, const FString& PropertyPath,
															const FString& ValueText)
{
	const AFGPlayerController* OwnerController = GetOwnerPlayerController();
	if (OwnerController == nullptr || !OwnerController->IsLocalController())
	{
		UE_LOG(LogKDataForge, Warning, TEXT("Server_ApplySetOp rejected: only the listen-server host may edit"));
		return;
	}
	UKDFSubsystem* KDF = UKDFSubsystem::Get(this);
	if (KDF == nullptr)
	{
		return;
	}
	UObject* Target = FSoftObjectPath(TargetPath).ResolveObject();
	if (Target == nullptr)
	{
		FString Error;
		if (UClass* Class = FKDFValueCodec::ResolveClass(TargetPath, Error))
		{
			Target = Class->GetDefaultObject();
		}
	}
	if (Target == nullptr)
	{
		UE_LOG(LogKDataForge, Warning, TEXT("Server_ApplySetOp: target '%s' not found"), *TargetPath);
		return;
	}

	const TSharedRef<FKDFNode> ValueNode = FKDFNode::MakeScalar(ValueText, true);
	FKDFOpArgs Args;
	Args.mValue = &ValueNode.Get();
	FString PreValue;
	FString PostValue;
	FString Error;
	if (!KDF->ApplyEditorOp(Target, PropertyPath, EKDFOp::Set, Args, PreValue, PostValue, Error))
	{
		UE_LOG(LogKDataForge, Warning, TEXT("Server_ApplySetOp failed: %s"), *Error);
	}
}

#include "Subsystems/KDFActorPatchSubsystem.h"

#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "KDFLogging.h"
#include "Reflection/KDFOpEngine.h"
#include "Reflection/KDFPropertyPath.h"
#include "Reflection/KDFValueCodec.h"
#include "Subsystems/KDFSubsystem.h"

bool UKDFActorPatchSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UKDFActorPatchSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(&InWorld);
	if (Subsystem == nullptr)
	{
		return;
	}

	mSpawnHandle = InWorld.AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &UKDFActorPatchSubsystem::OnActorSpawned));

	if (Subsystem->GetActorPatches().IsEmpty())
	{
		return;
	}
	int32 PatchedActorCount = 0;
	int32 AppliedOpCount = 0;
	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		const int32 Applied = ApplyPatchesToActor(*It);
		AppliedOpCount += Applied;
		PatchedActorCount += Applied > 0 ? 1 : 0;
	}
	if (PatchedActorCount > 0)
	{
		UE_LOG(LogKDataForge, Log, TEXT("Actor patches: applied %d op(s) to %d pre-existing actor(s)"), AppliedOpCount,
			   PatchedActorCount);
	}
}

void UKDFActorPatchSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld(); World != nullptr && mSpawnHandle.IsValid())
	{
		World->RemoveOnActorSpawnedHandler(mSpawnHandle);
	}
	Super::Deinitialize();
}

void UKDFActorPatchSubsystem::OnActorSpawned(AActor* Actor) { ApplyPatchesToActor(Actor); }

int32 UKDFActorPatchSubsystem::ApplyPatchesToActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return 0;
	}
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Actor);
	if (Subsystem == nullptr)
	{
		return 0;
	}

	int32 AppliedOpCount = 0;
	for (const FKDFActorPatch& Patch : Subsystem->GetActorPatches())
	{
		UClass* TargetClass = Patch.mTargetClass.Get();
		if (TargetClass == nullptr || !Patch.mPropertiesNode.IsValid())
		{
			continue;
		}
		const bool bMatches = Patch.bIncludeSubclasses ? Actor->IsA(TargetClass) : Actor->GetClass() == TargetClass;
		if (!bMatches)
		{
			continue;
		}
		// Bare generated-content ids in the patch's properties: resolve against its origin pack.
		const FKDFValueCodec::FPackScope PackScope(Patch.mPackRef);
		for (const TSharedRef<FKDFNode>& EntryRef : Patch.mPropertiesNode->Sequence)
		{
			FString PathString;
			EKDFOp Op = EKDFOp::Set;
			FKDFOpArgs Args;
			FString Error;
			if (!FKDFOpEngine::ParseOpEntry(EntryRef.Get(), PathString, Op, Args, Error))
			{
				continue; // already reported when the document applied to the CDO
			}
			FKDFPropertyPath Path;
			if (!FKDFPropertyPath::Parse(PathString, Path, Error))
			{
				continue;
			}
			if (FKDFOpEngine::ApplyOp(Actor, Path, Op, Args, Error))
			{
				++AppliedOpCount;
			}
			else
			{
				UE_LOG(LogKDataForge, Verbose, TEXT("Actor patch op failed on %s (%s): %s"), *Actor->GetName(),
					   *Patch.mSourceFile, *Error);
			}
		}
	}
	return AppliedOpCount;
}

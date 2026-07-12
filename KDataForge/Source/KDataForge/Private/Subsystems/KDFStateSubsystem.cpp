#include "Subsystems/KDFStateSubsystem.h"

#include "KDFLogging.h"
#include "Loader/KDFLoaderTypes.h"
#include "Subsystems/KDFSubsystem.h"

AKDFStateSubsystem::AKDFStateSubsystem() { ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer; }

void AKDFStateSubsystem::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		RecordAppliedPacks();
	}
}

void AKDFStateSubsystem::RecordAppliedPacks()
{
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(this);
	if (Subsystem == nullptr)
	{
		return;
	}
	int32 NewEntryCount = 0;
	for (const FKDFPack& Pack : Subsystem->GetPacks())
	{
		const bool bKnown = mPackHistory.ContainsByPredicate(
			[&Pack](const FKDFPackHistoryEntry& Entry)
			{ return Entry.mPackRef == Pack.mRef && Entry.mVersion == Pack.mVersion; });
		if (bKnown)
		{
			continue;
		}
		FKDFPackHistoryEntry& Entry = mPackHistory.AddDefaulted_GetRef();
		Entry.mPackRef = Pack.mRef;
		Entry.mVersion = Pack.mVersion;
		Entry.mFirstApplied = FDateTime::UtcNow().ToIso8601();
		++NewEntryCount;
	}
	if (NewEntryCount > 0)
	{
		UE_LOG(LogKDataForge, Log, TEXT("Pack history: %d new entr%s recorded for this save"), NewEntryCount,
			   NewEntryCount == 1 ? TEXT("y") : TEXT("ies"));
	}
}

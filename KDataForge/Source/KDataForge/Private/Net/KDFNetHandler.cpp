#include "Net/KDFNetHandler.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "KDFLogging.h"
#include "Misc/SecureHash.h"
#include "Network/NetworkHandler.h"
#include "Subsystems/KDFSubsystem.h"

namespace
{
	const FMessageType GKDFChecksumMessage{TEXT("KDataForge"), 1};

	UKDFSubsystem* FindKDFForConnection(UNetConnection* Connection)
	{
		UWorld* World = Connection != nullptr ? Connection->GetWorld() : nullptr;
		if (World == nullptr && GEngine != nullptr)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
				{
					World = Context.World();
					break;
				}
			}
		}
		return UKDFSubsystem::Get(World);
	}
} // namespace

FString FKDFNetHandler::ComputeChecksum(const UKDFSubsystem& Subsystem)
{
	// Canonical op stream: apply order is deterministic (stage → handler priority → pack priority
	// → pack ref → include order → path), so equal packs hash equal on every machine.
	FSHA1 Sha;
	for (const FKDFPatchRecord& Record : Subsystem.GetPatchRecords())
	{
		for (const FKDFOpRecord& Op : Record.mOps)
		{
			const FString Line = FString::Printf(TEXT("%s\n%s\n%d\n%s\n"), *Op.mTargetObjectPath, *Op.mPropertyPath,
												 static_cast<int32>(Op.mOp), *Op.mValueText);
			const FTCHARToUTF8 Utf8(*Line);
			Sha.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		}
	}
	Sha.Final();
	uint8 Hash[FSHA1::DigestSize];
	Sha.GetHash(Hash);
	return BytesToHex(Hash, FSHA1::DigestSize);
}

FString FKDFNetHandler::BuildPackManifest(const UKDFSubsystem& Subsystem)
{
	TArray<FString> Entries;
	for (const FKDFPack& Pack : Subsystem.GetPacks())
	{
		Entries.Add(FString::Printf(TEXT("%s@%s"), *Pack.mRef, *Pack.mVersion));
	}
	Entries.Sort();
	return Entries.IsEmpty() ? TEXT("<no packs>") : FString::Join(Entries, TEXT(", "));
}

void FKDFNetHandler::Register()
{
	// Called once per GameInstance CONSTRUCTION; the engine-level UModNetworkHandler outlives game
	// instances, so re-registering would stack duplicate join delegates (double sends, double denies).
	static bool bRegistered = false;
	if (bRegistered)
	{
		return;
	}

	UModNetworkHandler* NetworkHandler =
		GEngine != nullptr ? GEngine->GetEngineSubsystem<UModNetworkHandler>() : nullptr;
	if (NetworkHandler == nullptr)
	{
		UE_LOG(LogKDataForge, Warning, TEXT("Mod network handler unavailable — MP patchset validation disabled"));
		return;
	}
	bRegistered = true;

	// Client side: receive the server's checksum+manifest, compare against the local state.
	FMessageEntry& Entry = NetworkHandler->RegisterMessageType(GKDFChecksumMessage);
	Entry.bClientHandled = true;
	Entry.bServerHandled = false;
	Entry.MessageReceived.BindLambda(
		[](UNetConnection* Connection, FString Data)
		{
			FString ServerChecksum;
			FString ServerManifest;
			if (!Data.Split(TEXT("|"), &ServerChecksum, &ServerManifest))
			{
				ServerChecksum = Data;
			}
			const UKDFSubsystem* KDF = FindKDFForConnection(Connection);
			if (KDF == nullptr)
			{
				return; // no local KDataForge state to validate (should not happen — mod list already matched)
			}
			const FString LocalChecksum = ComputeChecksum(*KDF);
			if (LocalChecksum == ServerChecksum)
			{
				UE_LOG(LogKDataForge, Log, TEXT("MP patchset checksum OK (%s)"), *LocalChecksum);
				return;
			}
			const FString LocalManifest = BuildPackManifest(*KDF);
			UModNetworkHandler::CloseWithFailureMessage(
				Connection,
				FString::Printf(TEXT("KDataForge patchset mismatch.\nServer packs: %s\nYour packs: %s\n"
									 "Use the same DataForge packs as the server and try again."),
								*ServerManifest, *LocalManifest));
			UE_LOG(LogKDataForge, Error, TEXT("MP patchset mismatch — server %s vs local %s"), *ServerChecksum,
				   *LocalChecksum);
		});

	// Server side: push our checksum to every joining client during the handshake window.
	NetworkHandler->OnClientInitialJoin_Server().AddLambda(
		[](UNetConnection* Connection)
		{
			const UKDFSubsystem* KDF = FindKDFForConnection(Connection);
			if (KDF == nullptr)
			{
				return;
			}
			const FString Payload = FString::Printf(TEXT("%s|%s"), *ComputeChecksum(*KDF), *BuildPackManifest(*KDF));
			UModNetworkHandler::SendMessage(Connection, GKDFChecksumMessage, Payload);
		});

	UE_LOG(LogKDataForge, Log, TEXT("MP patchset validation registered (checksum handshake)"));
}

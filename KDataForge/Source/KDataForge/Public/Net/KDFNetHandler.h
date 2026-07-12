#pragma once

#include "CoreMinimal.h"

class UGameInstance;
class UKDFSubsystem;

/**
 * Phase 6 (3a): multiplayer patchset validation.
 *
 * Checksum: SHA1 over the canonical op stream (every applied op's target path, property path, op
 * id, and canonical post-value, in deterministic apply order) — never over YAML text, so line
 * endings/formatting/encoding cannot cause false mismatches.
 *
 * Join handshake: the server sends `<checksum>|<pack manifest>` through SML's UModNetworkHandler
 * during the pre-actor-channel join window; a client whose local checksum differs is denied with a
 * message listing both manifests. Op STREAMING to mismatched clients (3b) is deferred — mismatch
 * means deny.
 */
class KDATAFORGE_API FKDFNetHandler
{
public:
	/** Registers the message type + join delegates. Called once from the game instance module. */
	static void Register();

	/** SHA1 hex of the subsystem's applied patch records (identical packs ⇒ identical checksum). */
	static FString ComputeChecksum(const UKDFSubsystem& Subsystem);

	/** `ref@version` list of enabled packs (mismatch diagnostics). */
	static FString BuildPackManifest(const UKDFSubsystem& Subsystem);
};

#pragma once
#include "Subsystems/ResourceNodes/KBFLActorSpawnDescriptorBase.h"

#include "EngineUtils.h"
#include "KBFLLogging.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/KBFLResourceNodeSubsystem.h"

void UKBFLSpawnRequirement::OnInit_Implementation() {}
void UKBFLSpawnRequirement::OnClear_Implementation() {}
bool UKBFLSpawnRequirement::IsRequirementMet_Implementation(UKBFLActorSpawnDescriptorBase* Target) { return true; }
void UKBFLSpawnRequirement::ConfigureActor_Implementation(class UKBFLActorSpawnDescriptorBase* Target, AActor* Actor) {}
void UKBFLSpawnRequirement::OnActorSpawned_Implementation(class UKBFLActorSpawnDescriptorBase* Target, AActor* Actor) {}

bool UKBFLSpawnRequirement::IsRequirementMetActor_Implementation(class UKBFLActorSpawnDescriptorBase* Target,
																 AActor* Actor)
{
	return true;
}

void UKBFLActorSpawnDescriptorBase::BeginSpawning()
{
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "BeginSpawning called for descriptor: {0}", GetName());
	
	if (!AreRequirementsMet())
	{
		UE_LOGFMT(KBFLActorSpawnerLog, Warning, "Skipping BeginSpawning for '{0}' - requirements not met", GetName());
		return;
	}

	if (!CheckWorld())
	{
		UE_LOGFMT(KBFLActorSpawnerLog, Error, "BeginSpawning failed for '{0}' - CheckWorld failed", GetName());
		return;
	}

	if (!mSubsystem)
	{
		UE_LOGFMT(KBFLActorSpawnerLog, Error, "BeginSpawning failed for '{0}' - Subsystem is null", GetName());
		return;
	}

	UE_LOGFMT(KBFLActorSpawnerLog, Log, "Descriptor '{0}' - modifying values", GetName());
	ModifyValues();

	TArray<TSubclassOf<AActor>> SearchingClasses = GetSearchingActorClasses();
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "Descriptor '{0}' - searching for {1} actor class(es)", GetName(), SearchingClasses.Num());
	
	for (TSubclassOf<AActor> ActorClass : SearchingClasses)
	{
		TArray<AActor*> Founded;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorClass, Founded);
		if (Founded.Num() > 0)
		{
			UE_LOGFMT(KBFLActorSpawnerLog, Log, "Found {0} existing actor(s) of class {1}", Founded.Num(), ActorClass->GetName());
			mAllActors.Append(Founded);
		}
	}

	UE_LOGFMT(KBFLActorSpawnerLog, Log, "Total existing actors found: {0}", mAllActors.Num());

	if (GetActorClass() && GetActorFreeClass())
	{
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "Descriptor '{0}' - actor classes valid (ActorClass: {1}, FreeClass: {2})", 
			GetName(), GetActorClass()->GetName(), GetActorFreeClass()->GetName());

		TArray<AActor*> ActorArray;
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "Descriptor '{0}' - processing locations", GetName());
		ForeachLocations(ActorArray);
		
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "Descriptor '{0}' - processed {1} location(s)", GetName(), ActorArray.Num());
		AfterSpawning();

		if (mRemoveOld)
		{
			UE_LOGFMT(KBFLActorSpawnerLog, Log, "Descriptor '{0}' - removing old/wrong actors", GetName());
			RemoveWrongActors(ActorArray);
		}
	}
	else
	{
		UE_LOGFMT(KBFLActorSpawnerLog, Warning, "Descriptor '{0}' - invalid actor classes (ActorClass valid: {1}, FreeClass valid: {2})", 
			GetName(), GetActorClass() != nullptr, GetActorFreeClass() != nullptr);
	}

	UE_LOGFMT(KBFLActorSpawnerLog, Log, "Descriptor '{0}' - cleaning up for garbage collection", GetName());
	// free for garbage collector
	this->RemoveFromRoot();
	this->MarkAsGarbage();
}
void UKBFLActorSpawnDescriptorBase::InitRequirements()
{
	for (TSubclassOf<UKBFLSpawnRequirement> SpawnRequirement : mSpawnRequirements)
	{
		if (UKBFLSpawnRequirement* Requirement = NewObject<UKBFLSpawnRequirement>(this, SpawnRequirement))
		{
			Requirement->OnInit();
			mRequirements.Add(Requirement);
		}
	}
}

void UKBFLActorSpawnDescriptorBase::ClearRequirements()
{
	for (TObjectPtr<UKBFLSpawnRequirement> Requirement : mRequirements)
	{
		Requirement->OnClear();
	}
	mSpawnRequirements.Empty();
}
bool UKBFLActorSpawnDescriptorBase::AreRequirementsMet()
{
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "Checking requirements for descriptor '{0}' ({1} requirement(s))", GetName(), mRequirements.Num());
	
	for (TObjectPtr<UKBFLSpawnRequirement> Requirement : mRequirements)
	{
		if (!Requirement->IsRequirementMet(this))
		{
			UE_LOGFMT(KBFLActorSpawnerLog, Warning, "Requirement '{0}' not met for descriptor '{1}'", 
				Requirement->GetName(), GetName());
			return false;
		}
	}
	
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "All requirements met for descriptor '{0}'", GetName());
	return true;
}
bool UKBFLActorSpawnDescriptorBase::AreRequirementsMetActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	for (TObjectPtr<UKBFLSpawnRequirement> Requirement : mRequirements)
	{
		if (!Requirement->IsRequirementMetActor(this, Actor))
		{
			return false;
		}
	}
	return true;
}
void UKBFLActorSpawnDescriptorBase::ConfigureActor(AActor* Actor)
{
	for (TObjectPtr<UKBFLSpawnRequirement> Requirement : mRequirements)
	{
		Requirement->ConfigureActor(this, Actor);
	}
}
void UKBFLActorSpawnDescriptorBase::OnActorSpawned(AActor* Actor)
{
	for (TObjectPtr<UKBFLSpawnRequirement> Requirement : mRequirements)
	{
		Requirement->OnActorSpawned(this, Actor);
	}
}

void UKBFLActorSpawnDescriptorBase::ForeachLocations(TArray<AActor*>& ActorArray) {}

bool UKBFLActorSpawnDescriptorBase::CheckActorInRange(FTransform Transform, AActor*& OutActor)
{
	const TArray<AActor*> ActorsToIgnore = {};
	TArray<AActor*> OutActors = {};

	const bool Free =
		UKismetSystemLibrary::SphereOverlapActors(GetWorld(), Transform.GetLocation(), mCheckRange,
												  GetSphereCheckChannels(), GetActorClass(), ActorsToIgnore, OutActors);

	if (Free && OutActors.Num() > 0)
	{
		OutActor = OutActors[0];
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "CheckActorInRange: Found actor '{0}' via sphere overlap at location {1}", 
			OutActor->GetName(), Transform.GetLocation().ToString());
		ModifyCheckActor(OutActor, Transform);
	}
	else
	{
		if (mAllActors.Num() > 0)
		{
			UE_LOGFMT(KBFLActorSpawnerLog, Log, "CheckActorInRange: Sphere overlap failed, checking {0} cached actors by distance", 
				mAllActors.Num());
			
			for (AActor* Actor : mAllActors)
			{
				if (ensure(Actor))
				{
					if (Actor->GetClass() != GetActorClass())
					{
						continue;
					}

					if (FVector::Distance(Actor->GetActorLocation(), Transform.GetLocation()) < mCheckRange)
					{
						UE_LOGFMT(KBFLActorSpawnerLog, Warning, "Found actor '{0}' only by distance checking at {1}", 
							Actor->GetName(), Actor->GetActorLocation().ToString());
						OutActor = Actor;
						ModifyCheckActor(OutActor, Transform);
						return true;
					}
				}
			}
		}
	}

	UE_LOGFMT(KBFLActorSpawnerLog, Warning, 
		"CheckActorInRange: Found {0} actor(s), Class: {1}, Range: {2}, Location: {3}",
		OutActors.Num(), GetActorClass()->GetName(), mCheckRange, Transform.GetLocation().ToString());
	return Free;
}

void UKBFLActorSpawnDescriptorBase::ModifyCheckActor(AActor*& InActor, FTransform FoundTransform)
{
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "ModifyCheckActor: Processing actor '{0}', AllowToMove: {1}", 
		InActor->GetName(), mAllowToMove);
	
	if (mAllowToMove)
	{
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "Moving actor '{0}' from {1} to {2}", 
			InActor->GetName(), InActor->GetActorLocation().ToString(), FoundTransform.GetLocation().ToString());
		InActor->SetActorTransform(FoundTransform);
	}
}

bool UKBFLActorSpawnDescriptorBase::IsRangeFree(FTransform Transform)
{
	if (!mPreventSpawningByOverlapFreeActorClass)
	{
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "IsRangeFree: Spawn prevention disabled, returning true");
		return true;
	}

	const TArray<AActor*> ActorsToIgnore = {};
	TArray<AActor*> OutActors = {};

	const bool Free = !UKismetSystemLibrary::SphereOverlapActors(GetWorld(), Transform.GetLocation(), mCheckRange,
																 GetSphereCheckChannels(), GetActorFreeClass(),
																 ActorsToIgnore, OutActors);

	const bool Result = Free && OutActors.Num() == 0;
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "IsRangeFree: Location {0}, Range: {1}, Free: {2}, Overlapping actors: {3}", 
		Transform.GetLocation().ToString(), mCheckRange, Result, OutActors.Num());
	
	return Result;
}

void UKBFLActorSpawnDescriptorBase::RemoveWrongActors(TArray<AActor*>& ActorArray)
{
	if (GetActorClass())
	{
		int32 RemovedCount = 0;
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "RemoveWrongActors: Checking actors of class '{0}', Valid actors: {1}", 
			GetActorClass()->GetName(), ActorArray.Num());
		
		for (TActorIterator Actor(GetWorld(), GetActorClass()); Actor; ++Actor)
		{
			AActor* ActorPointer = *Actor;
			bool IsActorCorrect = ActorArray.Contains(ActorPointer);

			if (!IsActorCorrect && IsAllowedToRemoveActor(ActorPointer))
			{
				UE_LOGFMT(KBFLActorSpawnerLog, Warning, "Destroying wrong actor '{0}' at {1}",
					ActorPointer->GetName(), ActorPointer->GetTransform().ToString());
				Actor->Destroy();
				RemovedCount++;
			}
		}
		
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "RemoveWrongActors: Removed {0} actor(s)", RemovedCount);
	}
}

void UKBFLActorSpawnDescriptorBase::ApplyMaterialData(AActor* Actor, TMap<uint8, UMaterialInterface*> MaterialInfo)
{
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "ApplyMaterialData: Actor '{0}', Materials: {1}", 
		Actor->GetName(), MaterialInfo.Num());

	if (Actor && MaterialInfo.Num() > 0)
	{
		TArray<UStaticMeshComponent*> StaticMeshComponents;
		Actor->GetComponents(StaticMeshComponents);

		if (StaticMeshComponents.Num() > 0)
		{
			UStaticMeshComponent* StaticMesh = StaticMeshComponents[0];
			if (IsValid(StaticMesh))
			{
				int32 AppliedCount = 0;
				for (auto Info : MaterialInfo)
				{
					if (StaticMesh->GetNumMaterials() > Info.Key && Info.Value)
					{
						StaticMesh->SetMaterial(Info.Key, Info.Value);
						AppliedCount++;
					}
				}
				UE_LOGFMT(KBFLActorSpawnerLog, Log, "Applied {0} material(s) to actor '{1}'", AppliedCount, Actor->GetName());
			}
		}
		else
		{
			UE_LOGFMT(KBFLActorSpawnerLog, Warning, "No StaticMeshComponents found on actor '{0}'", Actor->GetName());
		}
	}
}

TArray<TSubclassOf<AActor>> UKBFLActorSpawnDescriptorBase::GetSearchingActorClasses() { return {}; }

bool UKBFLActorSpawnDescriptorBase::IsAllowedToRemoveActor(AActor* InActor)
{
	if (InActor)
	{
		return true;
	}
	return false;
}

bool UKBFLActorSpawnDescriptorBase::CheckWorld() const
{
	if (GetWorld())
	{
#if WITH_EDITOR
		const bool bMatches = FString("UEDPIE_0_").Append(mMapName) != GetWorld()->GetMapName();
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "CheckWorld (Editor): Expected '{0}', Current '{1}', Match: {2}", 
			FString("UEDPIE_0_").Append(mMapName), GetWorld()->GetMapName(), bMatches);
		return bMatches;
#else
		const bool bMatches = mMapName == GetWorld()->GetMapName();
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "CheckWorld: Expected '{0}', Current '{1}', Match: {2}", 
			mMapName, GetWorld()->GetMapName(), bMatches);
		return bMatches;
#endif
	}
	
	UE_LOGFMT(KBFLActorSpawnerLog, Error, "CheckWorld failed - World is null");
	return false;
}
void UKBFLActorSpawnDescriptorBase::ModifyValues() {}

TSubclassOf<AActor> UKBFLActorSpawnDescriptorBase::GetActorClass() { return AActor::StaticClass(); }

TSubclassOf<AActor> UKBFLActorSpawnDescriptorBase::GetActorFreeClass()
{
	return mActorFreeClass ? mActorFreeClass : GetActorClass();
}

AActor* UKBFLActorSpawnDescriptorBase::SpawnActorAtLocation(FTransform Transform, TSubclassOf<AActor> ClassToSpawn)
{
	UE_LOGFMT(KBFLActorSpawnerLog, Log, "Attempting to spawn actor of class '{0}' at location {1}", 
		ClassToSpawn->GetName(), Transform.GetLocation().ToString());
	
	if (AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(ClassToSpawn, Transform))
	{
		UE_LOGFMT(KBFLActorSpawnerLog, Log, "Actor deferred spawn successful, checking requirements");
		
		if (!AreRequirementsMetActor(NewActor))
		{
			UE_LOGFMT(KBFLActorSpawnerLog, Warning, "Actor '{0}' failed requirements check, destroying", NewActor->GetName());
			NewActor->Destroy();
			return nullptr;
		}

		ModifySpawnedActorPreSpawn(NewActor);
		ConfigureActor(NewActor);
		NewActor->FinishSpawning(Transform, true);
		OnActorSpawned(NewActor);
		ModifySpawnedActorPostSpawn(NewActor);

		UE_LOGFMT(KBFLActorSpawnerLog, Log, "Actor spawned successfully: '{0}' at {1}", 
			NewActor->GetName(), Transform.ToString());

		return NewActor;
	}
	
	UE_LOGFMT(KBFLActorSpawnerLog, Error, "Failed to spawn actor of class '{0}'", ClassToSpawn->GetName());
	return nullptr;
}

void UKBFLActorSpawnDescriptorBase::ModifySpawnedActorPreSpawn(AActor*& InActor)
{
	//
}

void UKBFLActorSpawnDescriptorBase::ModifySpawnedActorPostSpawn(AActor*& InActor)
{
	//
}

void UKBFLActorSpawnDescriptorBase::AfterSpawning()
{
	//
}

TArray<TEnumAsByte<EObjectTypeQuery>> UKBFLActorSpawnDescriptorBase::GetSphereCheckChannels()
{
	return mObjectTypeQuery;
}

void UKBFLActorSpawnDescriptorBase::SetSphereCheckChannels(TArray<TEnumAsByte<EObjectTypeQuery>> Channels)
{
	mObjectTypeQuery = Channels;
}

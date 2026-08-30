#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/SubclassOf.h"

#include "KDFResourceNodeSubsystem.generated.h"

class AActor;
class AFGNodeMeshActor;
class AFGResourceNodeBase;
class AFGResourceNodeFrackingCore;
class AFGResourceNodeFrackingSatellite;
class ULevel;
class UFGSchematic;
struct FKDFNodePurge;

UCLASS()
class KDATAFORGE_API UKDFResourceNodeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:

	void OnWorldActorsInitialized(const UWorld::FActorsInitializedParams& Params);

	void OnLevelAddedToWorld(ULevel* Level, UWorld* World);

	void OnActorSpawned(AActor* Actor);

	void OnActorDestroyed(AActor* Actor);

	UFUNCTION()
	void OnSchematicPurchased(TSubclassOf<UFGSchematic> PurchasedSchematic);

	void SweepWorld(UWorld& World);

	void SweepDeferred();

	static const FKDFNodePurge* FindMatchingPurge(const AFGResourceNodeBase* Node);

	bool TryPurgeNode(AFGResourceNodeBase* Node);

	void DestroyNode(AFGResourceNodeBase* Node);

	void DestroyMeshActorFor(const AFGResourceNodeBase* Node);

	void BuildSatelliteIndex(UWorld& World);
	void BuildOccupiedNodeIndex(UWorld& World);
	void BuildMeshIndex(UWorld& World);
	void IndexSatellite(AFGResourceNodeFrackingSatellite* Satellite);
	void IndexMeshActor(AFGNodeMeshActor* Mesh);

	void GetSatellitesOf(AFGResourceNodeFrackingCore* Core,
						 TArray<AFGResourceNodeFrackingSatellite*>& OutSatellites) const;

	bool IsNodeGroupOccupied(AFGResourceNodeBase* Node) const;

	void PurgeScannerResources();

	void PurgeKAPIAllowLists(const UClass* ResourceClass);

	void RefreshScannersAndRadarTowers();

	FDelegateHandle mSpawnHandle;
	FDelegateHandle mDestroyHandle;
	FDelegateHandle mActorsInitializedHandle;
	FDelegateHandle mLevelAddedHandle;

	TMultiMap<AFGResourceNodeFrackingCore*, TWeakObjectPtr<AFGResourceNodeFrackingSatellite>> mSatellitesByCore;

	TSet<const AActor*> mOccupiedNodes;

	TMap<const AFGResourceNodeBase*, TWeakObjectPtr<AFGNodeMeshActor>> mMeshByNode;

	TArray<TWeakObjectPtr<AFGNodeMeshActor>> mUnlinkedMeshes;

	bool bHasUnresolvedExtractors = false;

	bool bMeshIndexBuilt = false;
};

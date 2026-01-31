#pragma once

#include "CoreMinimal.h"
#include "FGColoredInstanceMeshProxy.h"
#include "KPCLOutlineSubsystem.h"
#include "KPCLOutlineActor.generated.h"

/**
Actor for represend outlines
*/
UCLASS()
class KPRIVATECODELIB_API AKPCLOutlineActor : public AActor
{
	GENERATED_BODY()

public:
	AKPCLOutlineActor()
	{
		bReplicates = false;
		PrimaryActorTick.bCanEverTick = 0;
	}

	virtual void BeginPlay() override
	{
		Super::BeginPlay();
		SetActorEnableCollision(false);
	};

	static uint8 GetStencilFromData(FOutlineData Data)
	{
		const uint8 OutlineType = static_cast<uint8>(Data.mOutlineType);
		const uint8 OutlineColorSlot = static_cast<uint8>(Data.mOutlineColorSlot);
		return OutlineColorSlot + OutlineType;
	}

private:
	FORCEINLINE void CreateOutlineMesh(UStaticMeshComponent* OtherMeshComponent)
	{
		if (OtherMeshComponent)
		{
			if (UStaticMesh* Mesh = OtherMeshComponent->GetStaticMesh())
			{
				FString MeshName = OtherMeshComponent->GetStaticMesh()->GetName();

				if (MeshName != FString("ClearanceBox") && MeshName != FString("Arrows"))
				{
					if (UFGColoredInstanceMeshProxy* MeshProxy = Cast<UFGColoredInstanceMeshProxy>(OtherMeshComponent))
					{
						if (MeshProxy->mBlockInstancing && !MeshProxy->mInstanceHandle.IsInstanced())
						{
							const uint8 Stencil = GetStencilFromData(mOutlineData);
							OtherMeshComponent->SetRenderCustomDepth(true);
							OtherMeshComponent->SetCustomDepthStencilValue(Stencil);
							mOutlinedComponents.AddUnique(OtherMeshComponent);
							return;
						}

						if (UStaticMeshComponent* NewComponent = NewObject<UStaticMeshComponent>(this))
						{
							NewComponent->AttachToComponent(GetRootComponent(),
							                                FAttachmentTransformRules::KeepRelativeTransform);
							NewComponent->SetStaticMesh(Mesh);

							// Apply Material to new component
							for (int i = 0; i < NewComponent->GetNumMaterials(); ++i)
							{
								if (i < OtherMeshComponent->GetNumMaterials())
								{
									if (OtherMeshComponent->GetMaterial(i))
									{
										NewComponent->SetMaterial(i, OtherMeshComponent->GetMaterial(i));
									}
								}
							}

							NewComponent->RegisterComponent();

							const uint8 Stencil = GetStencilFromData(mOutlineData);
							NewComponent->SetRenderCustomDepth(true);
							NewComponent->SetCustomDepthStencilValue(Stencil);

							UE_LOG(LogTemp, Warning, TEXT("Create Outline with stencil %d"), Stencil)

							NewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

							TArray<float> CustomPrimitivDatas;
							//OtherMeshComponent->SetHiddenInGame(true);
							if (MeshProxy->mInstanceHandle.IsInstanced())
							{
								MeshProxy->SetInstanced(false);
								CustomPrimitivDatas = MeshProxy->mInstanceHandle.GetCustomData();
							}

							if (CustomPrimitivDatas.Num() == 0)
							{
								CustomPrimitivDatas = OtherMeshComponent->GetCustomPrimitiveData().Data;
							}

							if (CustomPrimitivDatas.Num() > 0)
							{
								for (int i = 0; i < CustomPrimitivDatas.Num(); ++i)
								{
									NewComponent->SetCustomPrimitiveDataFloat(i, CustomPrimitivDatas[i]);
								}
							}

							FTransform Transform = OtherMeshComponent->GetComponentTransform();
							Transform.SetScale3D(Transform.GetScale3D() + FVector(.005));

							NewComponent->SetWorldTransform(Transform);
						}
					}
					else
					{
						const uint8 Stencil = GetStencilFromData(mOutlineData);
						OtherMeshComponent->SetRenderCustomDepth(true);
						OtherMeshComponent->SetCustomDepthStencilValue(Stencil);
						mOutlinedComponents.AddUnique(OtherMeshComponent);
					}
				}
			}
		}
	}

	FORCEINLINE virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
	{
		for (TWeakObjectPtr<UActorComponent> ActorComponent : mOutlinedComponents)
		{
			if (ActorComponent.IsValid())
			{
				if (USkeletalMeshComponent* SkeletalComponent = Cast<USkeletalMeshComponent>(ActorComponent.Get()))
				{
					SkeletalComponent->SetRenderCustomDepth(false);
					SkeletalComponent->SetCustomDepthStencilValue(0);
				}

				if (UFGColoredInstanceMeshProxy* MeshProxy = Cast<UFGColoredInstanceMeshProxy>(ActorComponent.Get()))
				{
					if (!MeshProxy->mInstanceHandle.IsInstanced() && !MeshProxy->mBlockInstancing)
					{
						MeshProxy->SetInstanced(true);
					}
					MeshProxy->SetRenderCustomDepth(false);
					MeshProxy->SetCustomDepthStencilValue(0);
				}
				else if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(ActorComponent.Get()))
				{
					Mesh->SetRenderCustomDepth(false);
					Mesh->SetCustomDepthStencilValue(0);
				}
			}
		}

		Super::EndPlay(EndPlayReason);
	};

public:
	FORCEINLINE bool CreateOutlineFromActor(FOutlineData OutlineData)
	{
		if (OutlineData.mActorToOutline)
		{
			mOutlineData = OutlineData;
			if (OutlineData.mActorToOutline)
			{
				for (UActorComponent* ComponentsByClass : OutlineData.mActorToOutline->GetComponents())
				{
					if (UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ComponentsByClass))
					{
						CreateOutlineMesh(MeshComponent);
						mOutlinedComponents.AddUnique(ComponentsByClass);
					}
					else if (USkeletalMeshComponent* SkeletalComponent = Cast<
						USkeletalMeshComponent>(ComponentsByClass))
					{
						SkeletalComponent->SetRenderCustomDepth(true);
						SkeletalComponent->SetCustomDepthStencilValue(GetStencilFromData(mOutlineData));
						mOutlinedComponents.AddUnique(ComponentsByClass);
					}
				}
			}
			return true;
		}
		return false;
	}

	UPROPERTY(Transient)
	FOutlineData mOutlineData;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UActorComponent>> mOutlinedComponents;
};

// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGPlayerController.h"
#include "FGRemoteCallObject.h"
#include "FGSwatchGroup.h"

#include "EnumStruc/RssStruc.h"

#include "RSSSignRCO.generated.h"

UCLASS(Blueprintable)
class RSS_API URSSSignRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	static URSSSignRCO* Get(UObject* WorldContext)
	{
		if (WorldContext && WorldContext->GetWorld())
		{
			if (AFGPlayerController* Controller =
					Cast<AFGPlayerController>(WorldContext->GetWorld()->GetFirstPlayerController()))
			{
				if (URSSSignRCO* RCO = Controller->GetRemoteCallObjectOfClass<URSSSignRCO>())
				{
					return RCO;
				}
			}
		}
		return nullptr;
	};

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable)
	void RCO_Server_UpdateSignData(AActor* building, FRssSignData Data);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable)
	void RCO_Server_OpenWidget(AActor* building, AFGPlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable)
	void RCO_Server_CloseWidget(AActor* building, AFGPlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, NetMulticast, WithValidation, Reliable)
	void RCO_MultiCast_ApplySignData(AActor* building, FRssSignData Data);

	UFUNCTION(BlueprintCallable, NetMulticast, WithValidation, Reliable)
	void RCO_MultiCast_RequestSignData(AActor* building, FRssSignRequestData Request);

	UFUNCTION(BlueprintCallable, Server, WithValidation, Reliable)
	void RCO_Server_RequestSignData(AActor* building, FRssSignRequestData Request);

	UFUNCTION(BlueprintCallable, Client, WithValidation, Reliable)
	void RCO_Client_RequestSignData(AActor* building, FRssSignRequestData Request);

	UFUNCTION(BlueprintCallable, Client, WithValidation, Reliable)
	void RCO_Client_PasteSignData(class ARssSignGun* SignGun);

	UPROPERTY(Replicated)
	bool bDummy = true;
};

UCLASS()
class RSS_API URssSwatchGroup_Default : public UFGSwatchGroup
{
	GENERATED_BODY()

public:
	URssSwatchGroup_Default() { mGroupName = FText::FromString("Really Simple Signs"); }
};

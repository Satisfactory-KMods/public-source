// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Network/KPCLNetworkBuildingBase.h"

#include "KPCLNetworkTerminal.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkTerminal : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	AKPCLNetworkTerminal();

	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual bool IsUseable_Implementation() const override;
	virtual void OnUse_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state) override;
	virtual FText GetLookAtDecription_Implementation(AFGCharacterPlayer* byCharacter,
													 const FUseState& state) const override;
	virtual FString GetNetworkId() const override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "KMods|Network")
	FText mNoCoreText;
};

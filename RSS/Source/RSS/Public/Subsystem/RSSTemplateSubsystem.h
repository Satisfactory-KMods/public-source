// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_Util.h"
#include "FGRemoteCallObject.h"
#include "FGSaveInterface.h"
#include "Subsystem/ModSubsystem.h"

#include "EnumStruc/RssStruc.h"

#include "RSSTemplateSubsystem.generated.h"

UCLASS()
class RSS_API ARSSTemplateSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	ARSSTemplateSubsystem();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin IFGSaveInterface Interface
	FORCEINLINE virtual bool ShouldSave_Implementation() const override { return true; };
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End IFGSaveInterface Interface

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "RSS Subsystems")
	static ARSSTemplateSubsystem* GetOldRSSTemplateSubsystem(UObject* WorldContext)
	{
		return Cast<ARSSTemplateSubsystem>(UKBFL_Util::GetSubsystem(WorldContext, StaticClass()));
	}

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	TArray<FRssTemplateSignData> GetTemplates(ESignType SignType);

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	TArray<FRssTemplateSignData> GetAllTemplates();

	void ClearTemplates() { mTemplates.Empty(); };

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	TArray<FRssTemplateSignData> GetTemplatesOfSignSize(ESignSize SignSize, ESignType SignType) const;

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	TArray<FRssTemplateSignData> GetAllTemplatesOfSignSize(ESignSize SignSize) const;

	UFUNCTION(BlueprintCallable, Category = "RSS Subsystems")
	void RemoveTemplate(int TemplateIdx);

	UFUNCTION(BlueprintCallable, Category = "RSS Subsystems")
	void AddTemplate(FRssSignData Data);

	UFUNCTION(BlueprintCallable, Category = "RSS Subsystems")
	void RenameTemplate(const FText& NewName, int TemplateIdx);

	UPROPERTY(BlueprintReadOnly, SaveGame, Replicated)
	TArray<FRssSignData> mTemplates;

	UPROPERTY(SaveGame)
	bool mWasWipeAfterFix = false;
};

UCLASS()
class RSS_API URSSTemplateSubsystemRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	UFUNCTION(Server, WithValidation, Reliable)
	void RCO_RemoveTemplate(ARSSTemplateSubsystem* Subsystem, int TemplateIdx);

	UFUNCTION(Server, WithValidation, Reliable)
	void RCO_AddTemplate(ARSSTemplateSubsystem* Subsystem, FRssSignData Data);

	UFUNCTION(Server, WithValidation, Reliable)
	void RCO_RenameTemplate(ARSSTemplateSubsystem* Subsystem, const FText& NewName, int TemplateIdx);

	UPROPERTY(Replicated)
	bool bDummy = true;
};

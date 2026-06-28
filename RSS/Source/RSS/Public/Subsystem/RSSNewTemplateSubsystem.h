// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_Util.h"
#include "FGRemoteCallObject.h"
#include "FGSaveInterface.h"
#include "Subsystem/ModSubsystem.h"

#include "EnumStruc/RssStruc.h"

#include "RSSNewTemplateSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FRssTemplates
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<FRssSignData> mTemplates;
};

UCLASS()
class RSS_API ARSSNewTemplateSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	ARSSNewTemplateSubsystem();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	UFUNCTION(BlueprintCallable, Category = "Data Manager")
	bool GenerateFromFileInVar();

	bool ReadTemplatesFromFile(FString& FileContent);
	FString GetFilePath();

	UFUNCTION(BlueprintCallable, Category = "Data Manager")
	bool MigrateFromOld();

	UFUNCTION(BlueprintCallable, Category = "Data Manager")
	bool SaveTemplatesToFile();

	UFUNCTION(BlueprintCallable, Category = "Data Manager", meta = (WorldContext = "WorldContext"))
	static FString TemplatesToString(UObject* WorldContext, FRssTemplates Templates);

	UFUNCTION(BlueprintCallable, Category = "Data Manager", meta = (WorldContext = "WorldContext"))
	static FRssTemplates StringToTemplates(UObject* WorldContext, FString Templates);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "RSS Subsystems")
	static ARSSNewTemplateSubsystem* GetNewRSSTemplateSubsystem(UObject* WorldContext,
																TSubclassOf<ARSSNewTemplateSubsystem> Class)
	{
		return Cast<ARSSNewTemplateSubsystem>(UKBFL_Util::GetSubsystem(WorldContext, Class));
	}

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	TArray<FRssTemplateSignData> GetTemplates(ESignType SignType);

	UFUNCTION(BlueprintPure, Category = "RSS Subsystems")
	TArray<FRssTemplateSignData> GetAllTemplates();

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

	UPROPERTY(BlueprintReadOnly)
	FRssTemplates mTemplates;

	UPROPERTY(SaveGame)
	bool mWasWipeAfterFix = false;

	UPROPERTY(EditDefaultsOnly, Category = "RSS Subsystems")
	FString mFileName = "RssTemplateData.txt";

	UPROPERTY(EditDefaultsOnly, Category = "RSS Subsystems")
	FString mAddFilePath = "";
};

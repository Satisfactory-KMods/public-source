#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Containers/Queue.h"
#include "Dom/JsonObject.h"
#include "UObject/StrongObjectPtr.h"
#include "FGIconLibrary.h"

#include "RSSIconExtractor.generated.h"

class UTexture2D;

struct FRSSIconExportTask
{

	TStrongObjectPtr<UTexture2D> Texture;
	FString AssetPath;
	FString ItemName;
	FString Category;
	FString Source;
};

struct FRSSIconManifestEntry
{
	FString Id;
	FString AssetPath;
	FString FileName;
	FString Source;
	FString ItemName;
	FString Category;
	int32 Width = 0;
	int32 Height = 0;

	TArray<FString> Aliases;
};

struct FRSSFontManifestEntry
{
	FString Id;
	FString AssetPath;
	FString FaceAssetPath;
	FString FileName;
	FString Family;
	FString Typeface;
	int32 Weight = 400;
	FString Style = TEXT("normal");
};

UCLASS()
class RSS_API URSSIconExtractor : public UObject
{
	GENERATED_BODY()

public:
	URSSIconExtractor();

	UFUNCTION(BlueprintCallable, Category = "RSS Icon Extractor")
	void BeginExtraction();

	void TickExtractionQueue(float DeltaSeconds);

	void FlushExtractionQueue();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RSS Icon Extractor")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RSS Icon Extractor")
	bool bAutoStartOnWorldInit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RSS Icon Extractor")
	int32 MaxExportsPerFrame = 5;

	UPROPERTY(BlueprintReadOnly, Category = "RSS Icon Extractor")
	int32 TotalTasksQueued = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RSS Icon Extractor")
	int32 TasksCompleted = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RSS Icon Extractor")
	FString LastErrorMessage;

private:

	void DiscoverAndQueueAllIcons();

	void QueueIconLibraryEntries();
	void ExportRSSFonts();

	void QueueTexture(UTexture2D* Texture, const FString& ItemName, const FString& Category);

	static FString IconTypeToCategory(EIconType IconType);

	void ExportIconTask(const FRSSIconExportTask& Task);
	void RenderTextureToBytes(UTexture2D* Texture, TArray<uint8>& OutPngBytes, int32& OutWidth, int32& OutHeight);

	void WriteManifestFile(const TArray<FRSSIconManifestEntry>& Entries);
	FString GetOutputDirectory() const;
	FString DeriveUniqueId(const FString& AssetPath, const FString& ItemName);
	FString DeriveSourceFromPath(const FString& AssetPath) const;
	FString DeriveCategoryFromPath(const FString& AssetPath, const FString& ItemName) const;

	TQueue<FRSSIconExportTask, EQueueMode::Mpsc> ExportQueue;
	TArray<FRSSIconManifestEntry> CompletedEntries;
	TArray<FRSSFontManifestEntry> CompletedFontEntries;
	bool bExtractionInProgress = false;
	bool bExtractionQueued = false;
	double ExtractionStartTime = 0.0;

	static constexpr int32 ManifestFlushInterval = 100;
	int32 LastManifestFlushAt = 0;

	TSet<FString> ProcessedAssetPaths;

	TMap<FString, TArray<FString>> TextureAliases;

	TMap<FString, int32> IdCollisionCounter;
};

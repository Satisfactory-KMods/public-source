// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Extractor/RSSIconExtractor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "BFL/KBFL_Util.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Math/Float16Color.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Buildables/FGBuildable.h"
#include "FGIconDatabaseSubsystem.h"
#include "FGIconLibrary.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"
#include "Resources/FGBuildingDescriptor.h"
#include "Resources/FGItemDescriptor.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "TextureResource.h"

namespace
{
	FString MakeFontId(const FString& FaceName)
	{
		FString Id = FaceName.ToLower();
		for (TCHAR& Character : Id)
		{
			if (!FChar::IsAlnum(Character))
			{
				Character = TEXT('-');
			}
		}
		while (Id.Contains(TEXT("--")))
		{
			Id.ReplaceInline(TEXT("--"), TEXT("-"));
		}
		return Id.TrimChar(TEXT('-'));
	}

	bool HasFontSignature(const TArray<uint8>& FontData, int32 Offset = 0)
	{
		if (!FontData.IsValidIndex(Offset + 3))
		{
			return false;
		}

		const uint8* Bytes = FontData.GetData() + Offset;
		if (Bytes[0] == 0x00 && Bytes[1] == 0x01 && Bytes[2] == 0x00 && Bytes[3] == 0x00)
		{
			return true;
		}

		return (Bytes[0] == 'O' && Bytes[1] == 'T' && Bytes[2] == 'T' && Bytes[3] == 'O') ||
			(Bytes[0] == 't' && Bytes[1] == 't' && Bytes[2] == 'c' && Bytes[3] == 'f') ||
			(Bytes[0] == 'w' && Bytes[1] == 'O' && Bytes[2] == 'F' && (Bytes[3] == 'F' || Bytes[3] == '2')) ||
			(Bytes[0] == 't' && Bytes[1] == 'r' && Bytes[2] == 'u' && Bytes[3] == 'e') ||
			(Bytes[0] == 't' && Bytes[1] == 'y' && Bytes[2] == 'p' && Bytes[3] == '1');
	}

	void UnwrapSerializedFontData(TArray<uint8>& FontData)
	{
		if (HasFontSignature(FontData) || FontData.Num() < 8)
		{
			return;
		}

		const uint8* Bytes = FontData.GetData();
		const uint32 PayloadSize = static_cast<uint32>(Bytes[0]) | (static_cast<uint32>(Bytes[1]) << 8) |
			(static_cast<uint32>(Bytes[2]) << 16) | (static_cast<uint32>(Bytes[3]) << 24);
		if (PayloadSize == 0 || PayloadSize > static_cast<uint32>(FontData.Num() - 4) ||
			!HasFontSignature(FontData, 4))
		{
			return;
		}

		const int32 PayloadEnd = 4 + static_cast<int32>(PayloadSize);
		for (int32 Index = PayloadEnd; Index < FontData.Num(); ++Index)
		{
			if (FontData[Index] != 0)
			{
				return;
			}
		}

		TArray<uint8> UnwrappedData;
		UnwrappedData.Append(FontData.GetData() + 4, static_cast<int32>(PayloadSize));
		FontData = MoveTemp(UnwrappedData);
	}

	FString DetectFontExtension(const TArray<uint8>& FontData, const UFontFace* FontFace)
	{
		if (FontData.Num() >= 4)
		{
			const uint8* Bytes = FontData.GetData();
			if (Bytes[0] == 'O' && Bytes[1] == 'T' && Bytes[2] == 'T' && Bytes[3] == 'O')
			{
				return TEXT("otf");
			}
			if (Bytes[0] == 't' && Bytes[1] == 't' && Bytes[2] == 'c' && Bytes[3] == 'f')
			{
				return TEXT("ttc");
			}
			if (Bytes[0] == 'w' && Bytes[1] == 'O' && Bytes[2] == 'F' && Bytes[3] == 'F')
			{
				return TEXT("woff");
			}
			if (Bytes[0] == 'w' && Bytes[1] == 'O' && Bytes[2] == 'F' && Bytes[3] == '2')
			{
				return TEXT("woff2");
			}
		}

		if (IsValid(FontFace))
		{
			const FString SourceExtension = FPaths::GetExtension(FontFace->SourceFilename).ToLower();
			if (SourceExtension == TEXT("ttf") || SourceExtension == TEXT("otf") ||
				SourceExtension == TEXT("ttc") || SourceExtension == TEXT("woff") ||
				SourceExtension == TEXT("woff2"))
			{
				return SourceExtension;
			}
		}

		return TEXT("ttf");
	}

	int32 TypefaceToWeight(const FString& Typeface)
	{
		const FString LowerTypeface = Typeface.ToLower();
		if (LowerTypeface.Contains(TEXT("extrabold")))
		{
			return 800;
		}
		if (LowerTypeface.Contains(TEXT("semibold")))
		{
			return 600;
		}
		if (LowerTypeface.Contains(TEXT("bold")))
		{
			return 700;
		}
		if (LowerTypeface.Contains(TEXT("light")))
		{
			return 300;
		}
		return 400;
	}
}

URSSIconExtractor::URSSIconExtractor() :
	bEnabled(false), bAutoStartOnWorldInit(false), MaxExportsPerFrame(5), TotalTasksQueued(0), TasksCompleted(0)
{
}

void URSSIconExtractor::BeginExtraction()
{
	if (bExtractionInProgress || bExtractionQueued)
	{
		UE_LOG(LogTemp, Warning, TEXT("RSSIconExtractor: Extraction already in progress"));
		return;
	}

	if (!bEnabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("RSSIconExtractor: Extraction is disabled in config"));
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		LastErrorMessage = TEXT("No valid world");
		return;
	}

	ExtractionStartTime = FPlatformTime::Seconds();
	bExtractionInProgress = true;
	TotalTasksQueued = 0;
	TasksCompleted = 0;
	LastManifestFlushAt = 0;
	ProcessedAssetPaths.Empty();
	CompletedEntries.Empty();
	CompletedFontEntries.Empty();
	IdCollisionCounter.Empty();
	TextureAliases.Empty();

	UE_LOG(LogTemp, Log, TEXT("RSSIconExtractor: Starting extraction..."));

	ExportRSSFonts();
	DiscoverAndQueueAllIcons();

	if (TotalTasksQueued == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RSSIconExtractor: No icons queued"));
		WriteManifestFile(CompletedEntries);
		bExtractionInProgress = false;
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("RSSIconExtractor: Queued %d icon tasks"), TotalTasksQueued);
}

void URSSIconExtractor::TickExtractionQueue(float DeltaSeconds)
{
	if (!bExtractionInProgress)
	{
		return;
	}

	int32 ExportCount = 0;
	FRSSIconExportTask Task;

	while (ExportCount < MaxExportsPerFrame && ExportQueue.Dequeue(Task))
	{
		ExportIconTask(Task);
		ExportCount++;
		TasksCompleted++;
	}

	if (ExportCount > 0 && TasksCompleted - LastManifestFlushAt >= ManifestFlushInterval)
	{
		WriteManifestFile(CompletedEntries);
		LastManifestFlushAt = TasksCompleted;
	}

	if (ExportQueue.IsEmpty())
	{
		WriteManifestFile(CompletedEntries);
		double ElapsedSeconds = FPlatformTime::Seconds() - ExtractionStartTime;
		UE_LOG(LogTemp, Log,
			   TEXT("RSSIconExtractor: Extraction complete. %d icons exported in %.1f seconds. Output: %s"),
			   TasksCompleted, ElapsedSeconds, *GetOutputDirectory());
		bExtractionInProgress = false;
	}
}

void URSSIconExtractor::FlushExtractionQueue()
{
	FRSSIconExportTask Task;
	while (ExportQueue.Dequeue(Task))
	{
		ExportIconTask(Task);
		TasksCompleted++;
	}

	if (bExtractionInProgress)
	{
		WriteManifestFile(CompletedEntries);
		bExtractionInProgress = false;
	}
}

void URSSIconExtractor::DiscoverAndQueueAllIcons()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UKBFLAssetDataSubsystem* AssetSubsystem = UKBFLAssetDataSubsystem::Get(World);
	if (!IsValid(AssetSubsystem))
	{
		LastErrorMessage = TEXT("Cannot access KBFLAssetDataSubsystem");
		return;
	}

	const int32 BeforeItems = TotalTasksQueued;
	for (TSubclassOf<UFGItemDescriptor> Descriptor : AssetSubsystem->GetAllItems())
	{
		if (!IsValid(Descriptor))
		{
			continue;
		}

		UTexture2D* Texture = UFGItemDescriptor::GetBigIcon(Descriptor);
		if (!IsValid(Texture))
		{
			Texture = UFGItemDescriptor::GetSmallIcon(Descriptor);
		}

		FString ItemName = UFGItemDescriptor::GetItemName(Descriptor).ToString();
		if (ItemName.IsEmpty())
		{
			ItemName = Descriptor->GetName();
		}

		QueueTexture(Texture, ItemName, DeriveCategoryFromPath(Descriptor->GetPathName(), ItemName));
	}
	UE_LOG(LogTemp, Log, TEXT("RSSIconExtractor: %d icons from items"), TotalTasksQueued - BeforeItems);

	const int32 BeforeBuildings = TotalTasksQueued;
	for (TSubclassOf<AFGBuildable> Buildable : AssetSubsystem->GetAllBuildable())
	{
		if (!IsValid(Buildable))
		{
			continue;
		}

		TSubclassOf<UFGBuildingDescriptor> Desc = AssetSubsystem->GetDescForBuildable(Buildable);
		if (!IsValid(Desc))
		{
			continue;
		}

		UTexture2D* Texture = UFGItemDescriptor::GetBigIcon(Desc);
		if (!IsValid(Texture))
		{
			Texture = UFGItemDescriptor::GetSmallIcon(Desc);
		}

		FString ItemName = UFGItemDescriptor::GetItemName(Desc).ToString();
		if (ItemName.IsEmpty())
		{
			ItemName = Desc->GetName();
		}

		QueueTexture(Texture, ItemName, TEXT("building"));
	}
	UE_LOG(LogTemp, Log, TEXT("RSSIconExtractor: %d icons from buildings"), TotalTasksQueued - BeforeBuildings);

	const int32 BeforeSchematics = TotalTasksQueued;
	for (TSubclassOf<UFGSchematic> Schematic : AssetSubsystem->GetAllSchematics())
	{
		if (!IsValid(Schematic))
		{
			continue;
		}

		UTexture2D* Texture = Cast<UTexture2D>(UFGSchematic::GetItemIcon(Schematic).GetResourceObject());
		if (!IsValid(Texture))
		{
			Texture = UFGSchematic::GetSmallIcon(Schematic);
		}

		FString SchematicName = UFGSchematic::GetSchematicDisplayName(Schematic).ToString();
		if (SchematicName.IsEmpty())
		{
			SchematicName = Schematic->GetName();
		}

		QueueTexture(Texture, SchematicName, TEXT("schematic"));
	}
	UE_LOG(LogTemp, Log, TEXT("RSSIconExtractor: %d icons from schematics"),
		   TotalTasksQueued - BeforeSchematics);

	const int32 BeforeResearch = TotalTasksQueued;
	for (TSubclassOf<UFGResearchTree> Tree : AssetSubsystem->GetAllResearchTrees())
	{
		if (!IsValid(Tree))
		{
			continue;
		}

		UTexture2D* Texture = Cast<UTexture2D>(UFGResearchTree::GetResearchTreeIcon(Tree).GetResourceObject());
		QueueTexture(Texture, Tree->GetName(), TEXT("research"));
	}
	UE_LOG(LogTemp, Log, TEXT("RSSIconExtractor: %d icons from research trees"),
		   TotalTasksQueued - BeforeResearch);

	QueueIconLibraryEntries();
}

void URSSIconExtractor::ExportRSSFonts()
{
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName("/RSS/Interface/Fonts"));
	Filter.ClassPaths.Add(UFont::StaticClass()->GetClassPathName());

	TArray<FAssetData> FontAssets;
	AssetRegistry.GetAssets(Filter, FontAssets);
	FontAssets.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.ToString() < B.PackageName.ToString();
	});

	const FString FontOutputDirectory = FPaths::Combine(GetOutputDirectory(), TEXT("rss"), TEXT("fonts"));
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*FontOutputDirectory) &&
		!PlatformFile.CreateDirectoryTree(*FontOutputDirectory))
	{
		UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: Failed to create font output directory %s"),
			   *FontOutputDirectory);
		return;
	}

	TSet<FString> ExportedFacePaths;
	for (const FAssetData& FontAsset : FontAssets)
	{
		UFont* Font = Cast<UFont>(FontAsset.GetAsset());
		const FCompositeFont* CompositeFont = IsValid(Font) ? Font->GetCompositeFont() : nullptr;
		if (!CompositeFont)
		{
			continue;
		}

		for (const FTypefaceEntry& TypefaceEntry : CompositeFont->DefaultTypeface.Fonts)
		{
			const UFontFace* FontFace = Cast<UFontFace>(TypefaceEntry.Font.GetFontFaceAsset());
			if (!IsValid(FontFace))
			{
				UE_LOG(LogTemp, Warning, TEXT("RSSIconExtractor: Font %s typeface %s has no UFontFace asset"),
					   *Font->GetPathName(), *TypefaceEntry.Name.ToString());
				continue;
			}

			const FString FaceAssetPath = FontFace->GetPathName();
			if (ExportedFacePaths.Contains(FaceAssetPath))
			{
				continue;
			}

			TArray<uint8> FontBytes;
			const FFontFaceDataConstRef FontFaceData = FontFace->GetFontFaceData();
			if (FontFaceData->HasData())
			{
				FontBytes = FontFaceData->GetData();
			}
			else
			{
				FFileHelper::LoadFileToArray(FontBytes, *FontFace->GetFontFilename());
			}
			UnwrapSerializedFontData(FontBytes);

			if (!HasFontSignature(FontBytes))
			{
				UE_LOG(LogTemp, Warning, TEXT("RSSIconExtractor: Invalid font data for %s"), *FaceAssetPath);
				continue;
			}

			const FString Id = MakeFontId(FontFace->GetName());
			const FString Extension = DetectFontExtension(FontBytes, FontFace);
			const FString FileName = FString::Printf(TEXT("rss/fonts/%s.%s"), *Id, *Extension);
			const FString FilePath = FPaths::Combine(GetOutputDirectory(), FileName);
			if (!FFileHelper::SaveArrayToFile(FontBytes, *FilePath))
			{
				UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: Failed to save font %s"), *FilePath);
				continue;
			}

			FRSSFontManifestEntry Entry;
			Entry.Id = Id;
			Entry.AssetPath = Font->GetPathName();
			Entry.FaceAssetPath = FaceAssetPath;
			Entry.FileName = FileName;
			Entry.Family = Font->GetName();
			Entry.Typeface = TypefaceEntry.Name.ToString();
			Entry.Weight = TypefaceToWeight(Entry.Typeface);
			Entry.Style = Entry.Typeface.Contains(TEXT("italic"), ESearchCase::IgnoreCase) ? TEXT("italic") : TEXT("normal");
			CompletedFontEntries.Add(MoveTemp(Entry));
			ExportedFacePaths.Add(FaceAssetPath);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("RSSIconExtractor: Exported %d RSS font faces"), CompletedFontEntries.Num());
}

void URSSIconExtractor::QueueTexture(UTexture2D* Texture, const FString& ItemName, const FString& Category)
{
	if (!IsValid(Texture))
	{
		return;
	}

	const FString TexturePath = Texture->GetPathName();
	if (ProcessedAssetPaths.Contains(TexturePath))
	{

		if (!ItemName.IsEmpty())
		{
			TextureAliases.FindOrAdd(TexturePath).AddUnique(ItemName);
		}
		return;
	}
	ProcessedAssetPaths.Add(TexturePath);

	FRSSIconExportTask Task;

	Task.Texture.Reset(Texture);
	Task.AssetPath = TexturePath;
	Task.ItemName = ItemName.IsEmpty() ? Texture->GetName() : ItemName;
	Task.Source = DeriveSourceFromPath(TexturePath);
	Task.Category = Category;

	ExportQueue.Enqueue(Task);
	TotalTasksQueued++;
}

void URSSIconExtractor::QueueIconLibraryEntries()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	AFGIconDatabaseSubsystem* IconDb = AFGIconDatabaseSubsystem::Get(World);
	if (!IsValid(IconDb))
	{
		UE_LOG(LogTemp, Warning, TEXT("RSSIconExtractor: No icon database, skipping icon-library pass"));
		return;
	}

	int32 QueuedFromLibrary = 0;
	for (const FIconData& IconData : IconDb->GetAllIconData())
	{
		UTexture2D* Texture = Cast<UTexture2D>(UFGIconLibrary::GetTextureFromIconData(IconData));
		if (!IsValid(Texture))
		{
			continue;
		}

		const FString TexturePath = Texture->GetPathName();
		if (ProcessedAssetPaths.Contains(TexturePath))
		{
			continue;
		}
		ProcessedAssetPaths.Add(TexturePath);

		FString ItemName = IconData.IconName.ToString();
		if (ItemName.IsEmpty())
		{
			ItemName = Texture->GetName();
		}

		FRSSIconExportTask Task;
		Task.Texture.Reset(Texture);
		Task.AssetPath = TexturePath;
		Task.ItemName = ItemName;
		Task.Source = DeriveSourceFromPath(TexturePath);
		Task.Category = IconTypeToCategory(IconData.IconType);

		ExportQueue.Enqueue(Task);
		TotalTasksQueued++;
		QueuedFromLibrary++;
	}

	UE_LOG(LogTemp, Log, TEXT("RSSIconExtractor: Queued %d additional icons from the icon libraries"),
		   QueuedFromLibrary);
}

FString URSSIconExtractor::IconTypeToCategory(EIconType IconType)
{
	switch (IconType)
	{
	case EIconType::ESIT_Building:
		return TEXT("building");
	case EIconType::ESIT_Part:
		return TEXT("part");
	case EIconType::ESIT_Equipment:
		return TEXT("equipment");
	case EIconType::ESIT_Monochrome:
		return TEXT("monochrome");
	case EIconType::ESIT_Material:
		return TEXT("material");
	case EIconType::ESIT_Custom:
		return TEXT("custom");
	case EIconType::ESIT_MapStamp:
		return TEXT("mapstamp");
	default:
		return TEXT("other");
	}
}

void URSSIconExtractor::ExportIconTask(const FRSSIconExportTask& Task)
{
	if (!Task.Texture.IsValid())
	{
		return;
	}

	TArray<uint8> PngBytes;
	int32 Width, Height;
	RenderTextureToBytes(Task.Texture.Get(), PngBytes, Width, Height);

	if (PngBytes.IsEmpty() || Width <= 0 || Height <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RSSIconExtractor: Failed to render %s"), *Task.Texture->GetName());
		return;
	}

	FString Id = DeriveUniqueId(Task.AssetPath, Task.ItemName);
	FString FileName = FString::Printf(TEXT("%s/%s/%s.png"), *Task.Source, *Task.Category, *Id);
	FString OutputDir = GetOutputDirectory();
	FString FilePath = FPaths::Combine(OutputDir, FileName);
	FString DirPath = FPaths::GetPath(FilePath);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*DirPath))
	{
		if (!PlatformFile.CreateDirectoryTree(*DirPath))
		{
			UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: Failed to create directory %s"), *DirPath);
			return;
		}
	}

	if (!FFileHelper::SaveArrayToFile(PngBytes, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: Failed to save PNG %s"), *FilePath);
		return;
	}

	FRSSIconManifestEntry Entry;
	Entry.Id = Id;
	Entry.AssetPath = Task.AssetPath;
	Entry.FileName = FileName;
	Entry.Source = Task.Source;
	Entry.ItemName = Task.ItemName;
	Entry.Category = Task.Category;
	Entry.Width = Width;
	Entry.Height = Height;
	if (const TArray<FString>* Aliases = TextureAliases.Find(Task.AssetPath))
	{
		Entry.Aliases = *Aliases;
		Entry.Aliases.Remove(Entry.ItemName);
	}

	CompletedEntries.Add(Entry);
}

void URSSIconExtractor::RenderTextureToBytes(UTexture2D* Texture, TArray<uint8>& OutPngBytes, int32& OutWidth,
											 int32& OutHeight)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(Texture))
	{
		return;
	}

	Texture->SetForceMipLevelsToBeResident(30.f);
	Texture->WaitForStreaming();

	OutWidth = FMath::Max(Texture->GetSizeX(), 1);
	OutHeight = FMath::Max(Texture->GetSizeY(), 1);
	if (const FTextureResource* TextureResource = Texture->GetResource())
	{
		if (TextureResource->GetSizeX() > 0 && TextureResource->GetSizeY() > 0)
		{
			OutWidth = static_cast<int32>(TextureResource->GetSizeX());
			OutHeight = static_cast<int32>(TextureResource->GetSizeY());
		}
	}

	UTextureRenderTarget2D* RenderTarget =
		UKismetRenderingLibrary::CreateRenderTarget2D(World, OutWidth, OutHeight, RTF_RGBA16f);
	if (!IsValid(RenderTarget))
	{
		UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: Failed to create render target"));
		return;
	}

	UKismetRenderingLibrary::ClearRenderTarget2D(World, RenderTarget, FLinearColor::Transparent);

	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize = FVector2D::ZeroVector;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(World, RenderTarget, Canvas, CanvasSize, Context);
	if (IsValid(Canvas))
	{
		Canvas->K2_DrawTexture(Texture, FVector2D::ZeroVector, CanvasSize, FVector2D::ZeroVector, FVector2D::UnitVector,
							   FLinearColor::White, BLEND_Opaque, 0.f, FVector2D::ZeroVector);
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, Context);

	FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
	if (Resource == nullptr)
	{
		return;
	}

	TArray<FFloat16Color> FloatPixels;
	if (!Resource->ReadFloat16Pixels(FloatPixels) || FloatPixels.Num() != OutWidth * OutHeight)
	{
		UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: ReadFloat16Pixels failed"));
		return;
	}

	TArray<FColor> Pixels;
	Pixels.Reserve(FloatPixels.Num());
	for (const FFloat16Color& FloatPixel : FloatPixels)
	{
		const FLinearColor LinearPixel(FloatPixel.R.GetFloat(), FloatPixel.G.GetFloat(), FloatPixel.B.GetFloat(),
									   FloatPixel.A.GetFloat());
		Pixels.Add(LinearPixel.ToFColor(true));
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
	TSharedPtr<IImageWrapper> PngWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!PngWrapper.IsValid() ||
		!PngWrapper->SetRaw(Pixels.GetData(), Pixels.Num() * sizeof(FColor), OutWidth, OutHeight, ERGBFormat::BGRA, 8))
	{
		return;
	}

	const TArray64<uint8> CompressedData = PngWrapper->GetCompressed();
	if (CompressedData.Num() > MAX_int32)
	{
		UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: Compressed PNG exceeds TArray size limit"));
		return;
	}
	OutPngBytes.Append(CompressedData.GetData(), static_cast<int32>(CompressedData.Num()));
}

void URSSIconExtractor::WriteManifestFile(const TArray<FRSSIconManifestEntry>& Entries)
{
	TArray<TSharedPtr<FJsonValue>> JsonArray;

	for (const FRSSIconManifestEntry& Entry : Entries)
	{
		TSharedPtr<FJsonObject> JsonEntry = MakeShared<FJsonObject>();
		JsonEntry->SetStringField(TEXT("id"), Entry.Id);
		JsonEntry->SetStringField(TEXT("assetPath"), Entry.AssetPath);
		JsonEntry->SetStringField(TEXT("fileName"), Entry.FileName);
		JsonEntry->SetStringField(TEXT("source"), Entry.Source);
		JsonEntry->SetStringField(TEXT("itemName"), Entry.ItemName);
		JsonEntry->SetStringField(TEXT("category"), Entry.Category);
		JsonEntry->SetNumberField(TEXT("width"), Entry.Width);
		JsonEntry->SetNumberField(TEXT("height"), Entry.Height);

		if (Entry.Aliases.Num() > 0)
		{
			TArray<TSharedPtr<FJsonValue>> AliasJson;
			for (const FString& Alias : Entry.Aliases)
			{
				AliasJson.Add(MakeShared<FJsonValueString>(Alias));
			}
			JsonEntry->SetArrayField(TEXT("aliases"), AliasJson);
		}

		JsonArray.Add(MakeShared<FJsonValueObject>(JsonEntry));
	}

	TSharedPtr<FJsonObject> RootJson = MakeShared<FJsonObject>();
	RootJson->SetArrayField(TEXT("icons"), JsonArray);

	TArray<TSharedPtr<FJsonValue>> FontJsonArray;
	for (const FRSSFontManifestEntry& Entry : CompletedFontEntries)
	{
		TSharedPtr<FJsonObject> JsonEntry = MakeShared<FJsonObject>();
		JsonEntry->SetStringField(TEXT("id"), Entry.Id);
		JsonEntry->SetStringField(TEXT("assetPath"), Entry.AssetPath);
		JsonEntry->SetStringField(TEXT("faceAssetPath"), Entry.FaceAssetPath);
		JsonEntry->SetStringField(TEXT("fileName"), Entry.FileName);
		JsonEntry->SetStringField(TEXT("family"), Entry.Family);
		JsonEntry->SetStringField(TEXT("typeface"), Entry.Typeface);
		JsonEntry->SetNumberField(TEXT("weight"), Entry.Weight);
		JsonEntry->SetStringField(TEXT("style"), Entry.Style);
		FontJsonArray.Add(MakeShared<FJsonValueObject>(JsonEntry));
	}
	RootJson->SetArrayField(TEXT("fonts"), FontJsonArray);

	FString OutputDir = GetOutputDirectory();
	FString ManifestPath = FPaths::Combine(OutputDir, TEXT("manifest.json"));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*OutputDir))
	{
		if (!PlatformFile.CreateDirectoryTree(*OutputDir))
		{
			UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: Failed to create output directory %s"), *OutputDir);
			return;
		}
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(RootJson.ToSharedRef(), Writer))
	{
		UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: Failed to serialize manifest"));
		return;
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *ManifestPath,
									   FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("RSSIconExtractor: Failed to write manifest to %s"), *ManifestPath);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("RSSIconExtractor: Wrote manifest with %d icons and %d fonts to %s"), Entries.Num(),
		   CompletedFontEntries.Num(), *ManifestPath);
}

FString URSSIconExtractor::GetOutputDirectory() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeIconManifest"));
}

FString URSSIconExtractor::DeriveUniqueId(const FString& AssetPath, const FString& ItemName)
{

	FString PathOnly = AssetPath.Left(AssetPath.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd));
	PathOnly.RemoveFromStart(TEXT("/"));

	TArray<FString> Parts;
	PathOnly.ParseIntoArray(Parts, TEXT("/"));

	FString RawId;
	for (const FString& Part : Parts)
	{
		if (!Part.IsEmpty() && Part != TEXT("Game"))
		{
			if (!RawId.IsEmpty())
			{
				RawId += TEXT("_");
			}
			RawId += Part.ToLower();
		}
	}

	if (RawId.IsEmpty())
	{
		RawId = ItemName.ToLower();
	}

	if (IdCollisionCounter.Contains(RawId))
	{
		IdCollisionCounter[RawId]++;
		return FString::Printf(TEXT("%s_%d"), *RawId, IdCollisionCounter[RawId]);
	}

	IdCollisionCounter.Add(RawId, 0);
	return RawId;
}

FString URSSIconExtractor::DeriveSourceFromPath(const FString& AssetPath) const
{

	FString Path = AssetPath;
	if (Path.StartsWith(TEXT("/")))
	{
		Path.RemoveFromStart(TEXT("/"));
	}

	int32 SlashIndex = Path.Find(TEXT("/"));
	if (SlashIndex != INDEX_NONE)
	{
		FString FirstSegment = Path.Left(SlashIndex);
		if (FirstSegment == TEXT("Game"))
		{
			return TEXT("factorygame");
		}
		return FirstSegment.ToLower();
	}

	return TEXT("unknown");
}

FString URSSIconExtractor::DeriveCategoryFromPath(const FString& AssetPath, const FString& ItemName) const
{
	FString Combined = (AssetPath + TEXT(" ") + ItemName).ToLower();

	if (Combined.Contains(TEXT("equipment")) || Combined.Contains(TEXT("gear")) || Combined.Contains(TEXT("weapon")))
	{
		return TEXT("equipment");
	}
	if (Combined.Contains(TEXT("resource")) || Combined.Contains(TEXT("ore")) || Combined.Contains(TEXT("ingot")) ||
		Combined.Contains(TEXT("material")))
	{
		return TEXT("resource");
	}
	if (Combined.Contains(TEXT("component")) || Combined.Contains(TEXT("part")))
	{
		return TEXT("component");
	}
	if (Combined.Contains(TEXT("fuel")) || Combined.Contains(TEXT("ammo")))
	{
		return TEXT("fuel");
	}
	if (Combined.Contains(TEXT("building")) || Combined.Contains(TEXT("machine")) || Combined.Contains(TEXT("factory")))
	{
		return TEXT("building");
	}

	return TEXT("item");
}

#include "Handlers/KDFAssetHandler.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Content/KDFDynamicContent.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "KDFLogging.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/KDFSubsystem.h"
#include "UObject/Package.h"

namespace
{
	EImageFormat FormatFromExtension(const FString& FilePath)
	{
		const FString Extension = FPaths::GetExtension(FilePath).ToLower();
		if (Extension == TEXT("png"))
		{
			return EImageFormat::PNG;
		}
		if (Extension == TEXT("jpg") || Extension == TEXT("jpeg"))
		{
			return EImageFormat::JPEG;
		}
		if (Extension == TEXT("tga"))
		{
			return EImageFormat::TGA;
		}
		if (Extension == TEXT("bmp"))
		{
			return EImageFormat::BMP;
		}
		return EImageFormat::Invalid;
	}

	UTexture2D* CreateTextureFromFile(const FString& AbsoluteFile, UObject* Outer, const FName& AssetName,
									  FString& OutError, UTexture2D* Existing = nullptr)
	{
		const EImageFormat Format = FormatFromExtension(AbsoluteFile);
		if (Format == EImageFormat::Invalid)
		{
			OutError = FString::Printf(TEXT("Unsupported image format '%s' (png/jpg/tga/bmp)"),
									   *FPaths::GetExtension(AbsoluteFile));
			return nullptr;
		}
		TArray<uint8> FileData;
		if (!FFileHelper::LoadFileToArray(FileData, *AbsoluteFile))
		{
			OutError = FString::Printf(TEXT("Could not read '%s'"), *AbsoluteFile);
			return nullptr;
		}

		IImageWrapperModule& ImageWrapperModule =
			FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		const TSharedPtr<IImageWrapper> Wrapper = ImageWrapperModule.CreateImageWrapper(Format);
		if (!Wrapper.IsValid() || !Wrapper->SetCompressed(FileData.GetData(), FileData.Num()))
		{
			OutError = TEXT("Image decode failed (corrupt file?)");
			return nullptr;
		}
		TArray<uint8> RawBGRA;
		if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, RawBGRA))
		{
			OutError = TEXT("Could not convert image to BGRA8");
			return nullptr;
		}

		TUniquePtr<FTexturePlatformData> NewPlatformData = MakeUnique<FTexturePlatformData>();
		NewPlatformData->SizeX = Wrapper->GetWidth();
		NewPlatformData->SizeY = Wrapper->GetHeight();
		NewPlatformData->PixelFormat = PF_B8G8R8A8;

		FTexture2DMipMap* Mip = new FTexture2DMipMap();
		NewPlatformData->Mips.Add(Mip);
		Mip->SizeX = Wrapper->GetWidth();
		Mip->SizeY = Wrapper->GetHeight();
		Mip->BulkData.Lock(LOCK_READ_WRITE);
		void* MipData = Mip->BulkData.Realloc(RawBGRA.Num());
		FMemory::Memcpy(MipData, RawBGRA.GetData(), RawBGRA.Num());
		Mip->BulkData.Unlock();

		UTexture2D* Texture = Existing != nullptr
			? Existing
			: NewObject<UTexture2D>(Outer, AssetName, RF_Public | RF_Standalone | RF_Transactional);
		Texture->ReleaseResource();
		if (FTexturePlatformData* OldPlatformData = Texture->GetPlatformData())
		{
			Texture->SetPlatformData(nullptr);
			delete OldPlatformData;
		}
		Texture->SetPlatformData(NewPlatformData.Release());

		Texture->SRGB = true;
		Texture->NeverStream = true;
		Texture->UpdateResource();
		return Texture;
	}
} // namespace

UKDFAssetHandler::UKDFAssetHandler()
{
	mRootType = TEXT("asset");
	mStage = EKDFStage::Assets;
}

bool UKDFAssetHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	const FKDFNode* Assets = Document.Find(TEXT("assets"));
	if (Assets == nullptr || !Assets->IsSequence() || Assets->Num() == 0)
	{
		Context.AddError(TEXT("asset document requires a non-empty 'assets' sequence"));
		return false;
	}
	bool bAnyValid = false;
	for (const TSharedRef<FKDFNode>& EntryRef : Assets->Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		if (!Entry.IsMap() || Entry.GetString(TEXT("id"), FString()).IsEmpty() ||
			Entry.GetString(TEXT("file"), FString()).IsEmpty())
		{
			Context.AddError(TEXT("Asset entry needs 'id' and 'file'"), Entry.Line);
			continue;
		}
		bAnyValid = true;
	}
	return bAnyValid;
}

bool UKDFAssetHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	const FKDFNode* Assets = Document.Find(TEXT("assets"));
	if (Subsystem == nullptr || Assets == nullptr || !Assets->IsSequence())
	{
		return false;
	}

	// The pack directory is where `file:` paths resolve from.
	FString PackDirectory;
	for (const FKDFPack& Pack : Subsystem->GetPacks())
	{
		if (Pack.mRef == Context.mPackRef)
		{
			PackDirectory = Pack.mDirectory;
			break;
		}
	}

	bool bAppliedAny = false;
	for (const TSharedRef<FKDFNode>& EntryRef : Assets->Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		const FString Id = Entry.GetString(TEXT("id"), FString());
		const FString RelativeFile = Entry.GetString(TEXT("file"), FString());
		if (Id.IsEmpty() || RelativeFile.IsEmpty())
		{
			continue; // reported by validation
		}
		if (Context.bDryRun)
		{
			continue;
		}

		FString AbsoluteFile = FPaths::ConvertRelativePathToFull(PackDirectory / RelativeFile);
		FPaths::NormalizeFilename(AbsoluteFile);
		FString NormalizedPackDirectory = FPaths::ConvertRelativePathToFull(PackDirectory);
		FPaths::NormalizeDirectoryName(NormalizedPackDirectory);
		if (FPaths::IsRelative(RelativeFile) == false ||
			!AbsoluteFile.StartsWith(NormalizedPackDirectory + TEXT("/"), ESearchCase::IgnoreCase))
		{
			Context.AddError(FString::Printf(TEXT("Asset file must stay inside its pack directory: %s"), *RelativeFile),
							 Entry.Line);
			continue;
		}
		const FString PackageName = FKDFDynamicContentRegistry::MakeAssetPackageName(Context.mPackRef);
		const FString AssetName = FKDFDynamicContentRegistry::MakeClassName(Context.mPackRef, Id);

		UPackage* Package = CreatePackage(*PackageName);
		if (Package == nullptr)
		{
			Context.AddError(TEXT("Could not create asset package"), Entry.Line);
			continue;
		}
		// Reload path: reuse the existing texture object so all references stay valid.
		if (UTexture2D* Existing = FindObject<UTexture2D>(Package, *AssetName))
		{
			FString Error;
			if (CreateTextureFromFile(AbsoluteFile, Package, FName(*AssetName), Error, Existing) == nullptr)
			{
				Context.AddError(FString::Printf(TEXT("%s: %s"), *RelativeFile, *Error), Entry.Line);
				continue;
			}
			Subsystem->RetainObject(Existing);
			bAppliedAny = true;
			++Context.mAppliedOpCount;
			if (Context.mPatchRecord != nullptr)
			{
				FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
				OpRecord.mTargetObjectPath = Existing->GetPathName();
				OpRecord.mPropertyPath = TEXT("<generated-texture>");
				OpRecord.mOp = EKDFOp::Set;
				OpRecord.mValueText = RelativeFile;
			}
			continue;
		}

		FString Error;
		UTexture2D* Texture = CreateTextureFromFile(AbsoluteFile, Package, FName(*AssetName), Error);
		if (Texture == nullptr)
		{
			Context.AddError(FString::Printf(TEXT("%s: %s"), *RelativeFile, *Error), Entry.Line);
			continue;
		}
		Subsystem->RetainObject(Texture);

		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().AssetCreated(Texture);

		bAppliedAny = true;
		++Context.mAppliedOpCount;
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = Texture->GetPathName();
			OpRecord.mPropertyPath = TEXT("<generated-texture>");
			OpRecord.mOp = EKDFOp::Set;
			OpRecord.mValueText = RelativeFile;
		}
		UE_LOG(LogKDataForge, Log, TEXT("Created texture %s from %s (%dx%d)"), *Texture->GetPathName(), *RelativeFile,
			   Texture->GetSizeX(), Texture->GetSizeY());
	}
	return bAppliedAny;
}

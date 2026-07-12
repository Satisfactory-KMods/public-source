#include "Content/KDFDynamicContent.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "KDFLogging.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Reflection/ClassGenerator.h"
#include "Reflection/KDFValueCodec.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystems/KDFSubsystem.h"
#include "UObject/CoreRedirects.h"
#include "UObject/Package.h"

namespace
{
	constexpr int32 ManifestVersion = 1;

	FString GetManifestPath() { return FPaths::ProjectSavedDir() / TEXT("KDataForge") / TEXT("GeneratedClasses.json"); }
} // namespace

void FKDFDynamicContentRegistry::Initialize(UKDFSubsystem* Owner)
{
	mOwner = Owner;
	LoadManifest();
}

FString FKDFDynamicContentRegistry::SanitizeToken(const FString& Token)
{
	FString Result;
	Result.Reserve(Token.Len());
	for (const TCHAR Char : Token)
	{
		Result.AppendChar(FChar::IsAlnum(Char) || Char == TEXT('_') ? Char : TEXT('_'));
	}
	return Result;
}

FString FKDFDynamicContentRegistry::MakeKey(const FString& PackRef, const FString& Id)
{
	return FString::Printf(TEXT("%d:%s%s"), PackRef.Len(), *PackRef, *Id);
}

bool FKDFDynamicContentRegistry::ValidateRuntimeIdentity(const FString& PackRef, const FString& Id,
	const bool bIsAsset, FString& OutError) const
{
	const FString SanitizedPack = SanitizeToken(PackRef);
	const FString SanitizedId = SanitizeToken(Id);
	for (const FKDFGeneratedRecord& Record : mManifest)
	{
		if (Record.bIsAsset == bIsAsset && (Record.mPackRef != PackRef || Record.mId != Id) &&
			SanitizeToken(Record.mPackRef) == SanitizedPack && SanitizeToken(Record.mId) == SanitizedId)
		{
			OutError = FString::Printf(
				TEXT("Generated runtime identity collision: (%s, %s) and (%s, %s) both sanitize to %s/%s"),
				*PackRef, *Id, *Record.mPackRef, *Record.mId, *SanitizedPack, *SanitizedId);
			return false;
		}
	}
	return true;
}

FString FKDFDynamicContentRegistry::MakeClassName(const FString& PackRef, const FString& Id)
{
	return FString::Printf(TEXT("Gen_%s_%s"), *SanitizeToken(PackRef), *SanitizeToken(Id));
}

FString FKDFDynamicContentRegistry::MakeClassPackageName(const FString& PackRef)
{
	return FString::Printf(TEXT("/KDataForge/Gen/%s"), *SanitizeToken(PackRef));
}

FString FKDFDynamicContentRegistry::MakeAssetPackageName(const FString& PackRef)
{
	return FString::Printf(TEXT("/KDataForge/GenAssets/%s"), *SanitizeToken(PackRef));
}

UClass* FKDFDynamicContentRegistry::GetOrCreateClass(const FString& PackRef, const FString& Id, UClass* ParentClass,
													 FString& OutError)
{
	if (!IsValid(ParentClass))
	{
		OutError = TEXT("Invalid parent class");
		return nullptr;
	}
	if (!ValidateRuntimeIdentity(PackRef, Id, false, OutError))
	{
		return nullptr;
	}
	const FString Key = MakeKey(PackRef, Id);
	if (UClass* const* Existing = mClassesByKey.Find(Key))
	{
		if ((*Existing)->GetSuperClass() != ParentClass)
		{
			OutError = FString::Printf(TEXT("Generated class '%s' already exists with parent %s — parent changes "
											"require a new id (add a redirect for save compatibility)"),
									   *Id, *(*Existing)->GetSuperClass()->GetPathName());
			return nullptr;
		}
		return *Existing;
	}

	// FClassGenerator must only ever be called once per (package, name) — the map above is that gate.
	UClass* NewClass =
		FClassGenerator::GenerateSimpleClass(*MakeClassPackageName(PackRef), *MakeClassName(PackRef, Id), ParentClass);
	if (NewClass == nullptr)
	{
		OutError = FString::Printf(TEXT("Class generation failed for '%s'"), *Id);
		return nullptr;
	}
	mClassesByKey.Add(Key, NewClass);
	RecordGenerated(PackRef, Id, ParentClass->GetPathName(), false);
	UE_LOG(LogKDataForge, Log, TEXT("Generated class %s (parent %s)"), *NewClass->GetPathName(),
		   *ParentClass->GetName());
	return NewClass;
}

UObject* FKDFDynamicContentRegistry::GetOrCreateAsset(const FString& PackRef, const FString& Id, UClass* AssetClass,
													  FString& OutError)
{
	if (!IsValid(AssetClass))
	{
		OutError = TEXT("Invalid asset class");
		return nullptr;
	}
	if (!ValidateRuntimeIdentity(PackRef, Id, true, OutError))
	{
		return nullptr;
	}
	const FString Key = MakeKey(PackRef, Id);
	if (const TWeakObjectPtr<UObject>* Existing = mAssetsByKey.Find(Key))
	{
		if (UObject* Asset = Existing->Get())
		{
			if (Asset->GetClass() != AssetClass)
			{
				OutError = FString::Printf(TEXT("Generated asset '%s' already exists with class %s"), *Id,
										   *Asset->GetClass()->GetPathName());
				return nullptr;
			}
			return Asset;
		}
	}

	UPackage* Package = CreatePackage(*MakeAssetPackageName(PackRef));
	if (Package == nullptr)
	{
		OutError = TEXT("Could not create asset package");
		return nullptr;
	}
	const FString AssetName = MakeClassName(PackRef, Id);
	UObject* Asset = NewObject<UObject>(Package, AssetClass, FName(*AssetName), RF_Public | RF_Standalone);
	if (Asset == nullptr)
	{
		OutError = FString::Printf(TEXT("Asset creation failed for '%s'"), *Id);
		return nullptr;
	}
	if (mOwner != nullptr)
	{
		mOwner->RetainObject(Asset); // GC protection — generated assets live for the game instance
	}

	// Announce to the asset registry so scanning subsystems (KAPI, RefinedRDApi, …) discover it.
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().AssetCreated(Asset);

	mAssetsByKey.Add(Key, Asset);
	RecordGenerated(PackRef, Id, AssetClass->GetPathName(), true);
	UE_LOG(LogKDataForge, Log, TEXT("Generated data asset %s (%s)"), *Asset->GetPathName(), *AssetClass->GetName());
	return Asset;
}

UClass* FKDFDynamicContentRegistry::FindGeneratedClass(const FString& PackRef, const FString& Id) const
{
	UClass* const* Found = mClassesByKey.Find(MakeKey(PackRef, Id));
	return Found != nullptr ? *Found : nullptr;
}

UObject* FKDFDynamicContentRegistry::FindGeneratedAsset(const FString& PackRef, const FString& Id) const
{
	const TWeakObjectPtr<UObject>* Found = mAssetsByKey.Find(MakeKey(PackRef, Id));
	return Found != nullptr ? Found->Get() : nullptr;
}

void FKDFDynamicContentRegistry::FindGeneratedClassesById(const FString& Id, TArray<UClass*>& OutMatches) const
{
	for (const FKDFGeneratedRecord& Record : mManifest)
	{
		if (Record.bIsAsset || !Record.mId.Equals(Id, ESearchCase::IgnoreCase))
		{
			continue;
		}
		if (UClass* const* Found = mClassesByKey.Find(MakeKey(Record.mPackRef, Record.mId)))
		{
			OutMatches.AddUnique(*Found);
		}
	}
	OutMatches.Sort([](const UClass& A, const UClass& B) { return A.GetPathName() < B.GetPathName(); });
}

void FKDFDynamicContentRegistry::FindGeneratedAssetsById(const FString& Id, TArray<UObject*>& OutMatches) const
{
	for (const FKDFGeneratedRecord& Record : mManifest)
	{
		if (!Record.bIsAsset || !Record.mId.Equals(Id, ESearchCase::IgnoreCase))
		{
			continue;
		}
		if (const TWeakObjectPtr<UObject>* Found = mAssetsByKey.Find(MakeKey(Record.mPackRef, Record.mId)))
		{
			if (UObject* Live = Found->Get())
			{
				OutMatches.AddUnique(Live);
			}
		}
	}
	OutMatches.Sort([](const UObject& A, const UObject& B) { return A.GetPathName() < B.GetPathName(); });
}

void FKDFDynamicContentRegistry::ApplyRedirects(const FString& PackRef, const TMap<FString, FString>& OldIdToNewId)
{
	if (OldIdToNewId.IsEmpty())
	{
		return;
	}
	TArray<FCoreRedirect> Redirects;
	for (const TTuple<FString, FString>& Redirect : OldIdToNewId)
	{
		const FString OldPath = MakeClassPackageName(PackRef) + TEXT(".") + MakeClassName(PackRef, Redirect.Key);
		const FString NewPath = MakeClassPackageName(PackRef) + TEXT(".") + MakeClassName(PackRef, Redirect.Value);
		Redirects.Emplace(ECoreRedirectFlags::Type_Class, OldPath, NewPath);
		UE_LOG(LogKDataForge, Log, TEXT("Redirect: %s -> %s"), *OldPath, *NewPath);
	}
	FCoreRedirects::AddRedirectList(Redirects, TEXT("KDataForge pack redirects"));
}

int32 FKDFDynamicContentRegistry::ReconstructTombstones(TArray<FString>& OutTombstoneKeys)
{
	int32 TombstoneCount = 0;
	for (const FKDFGeneratedRecord& Record : mManifest)
	{
		if (Record.bIsAsset)
		{
			continue; // assets are never tombstoned — empty configs would trip consumer key checks
		}
		const FString Key = MakeKey(Record.mPackRef, Record.mId);
		if (mClassesByKey.Contains(Key))
		{
			continue;
		}
		FString Error;
		UClass* ParentClass = FKDFValueCodec::ResolveClass(Record.mParentClassPath, Error);
		if (ParentClass == nullptr)
		{
			UE_LOG(LogKDataForge, Warning,
				   TEXT("Tombstone for '%s:%s' skipped — parent %s no longer exists (%s). Saves referencing it "
						"will lose those objects."),
				   *Record.mPackRef, *Record.mId, *Record.mParentClassPath, *Error);
			continue;
		}
		if (GetOrCreateClass(Record.mPackRef, Record.mId, ParentClass, Error) != nullptr)
		{
			mTombstoneKeys.Add(Key);
			OutTombstoneKeys.Add(Key);
			++TombstoneCount;
			UE_LOG(LogKDataForge, Warning,
				   TEXT("Tombstone class recreated for '%s:%s' — its YAML definition disappeared. Saves keep "
						"loading; re-add the definition (or a redirect) to restore full function."),
				   *Record.mPackRef, *Record.mId);
		}
	}
	return TombstoneCount;
}

bool FKDFDynamicContentRegistry::IsTombstone(const FString& PackRef, const FString& Id) const
{
	return mTombstoneKeys.Contains(MakeKey(PackRef, Id));
}

void FKDFDynamicContentRegistry::RecordGenerated(const FString& PackRef, const FString& Id, const FString& ParentPath,
												 bool bIsAsset)
{
	for (FKDFGeneratedRecord& Record : mManifest)
	{
		if (Record.mPackRef == PackRef && Record.mId == Id && Record.bIsAsset == bIsAsset)
		{
			if (Record.mParentClassPath != ParentPath)
			{
				Record.mParentClassPath = ParentPath;
				SaveManifest();
			}
			return;
		}
	}
	FKDFGeneratedRecord& Record = mManifest.AddDefaulted_GetRef();
	Record.mPackRef = PackRef;
	Record.mId = Id;
	Record.mParentClassPath = ParentPath;
	Record.bIsAsset = bIsAsset;
	SaveManifest();
}

void FKDFDynamicContentRegistry::LoadManifest()
{
	mManifest.Reset();
	FString FileText;
	if (!FFileHelper::LoadFileToString(FileText, *GetManifestPath()))
	{
		return; // first run
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogKDataForge, Warning, TEXT("Generated-content manifest is unreadable — starting fresh (%s)"),
			   *GetManifestPath());
		return;
	}
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Root->TryGetArrayField(TEXT("entries"), Entries))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& EntryValue : *Entries)
	{
		const TSharedPtr<FJsonObject>* EntryObject = nullptr;
		if (!EntryValue->TryGetObject(EntryObject))
		{
			continue;
		}
		FKDFGeneratedRecord& Record = mManifest.AddDefaulted_GetRef();
		Record.mPackRef = (*EntryObject)->GetStringField(TEXT("pack"));
		Record.mId = (*EntryObject)->GetStringField(TEXT("id"));
		Record.mParentClassPath = (*EntryObject)->GetStringField(TEXT("parent"));
		Record.bIsAsset = (*EntryObject)->GetBoolField(TEXT("asset"));
	}
	UE_LOG(LogKDataForge, Log, TEXT("Loaded generated-content manifest: %d record(s)"), mManifest.Num());
}

void FKDFDynamicContentRegistry::SaveManifest() const
{
	const FString ManifestPath = GetManifestPath();
	if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(ManifestPath), true))
	{
		UE_LOG(LogKDataForge, Error, TEXT("Could not create generated-content manifest directory: %s"),
			   *FPaths::GetPath(ManifestPath));
		return;
	}
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), ManifestVersion);
	TArray<TSharedPtr<FJsonValue>> Entries;
	for (const FKDFGeneratedRecord& Record : mManifest)
	{
		const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("pack"), Record.mPackRef);
		Entry->SetStringField(TEXT("id"), Record.mId);
		Entry->SetStringField(TEXT("parent"), Record.mParentClassPath);
		Entry->SetBoolField(TEXT("asset"), Record.bIsAsset);
		Entries.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Root->SetArrayField(TEXT("entries"), Entries);

	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root, Writer);
	if (!FFileHelper::SaveStringToFile(Output, *ManifestPath))
	{
		UE_LOG(LogKDataForge, Error, TEXT("Could not write generated-content manifest: %s"), *ManifestPath);
	}
}

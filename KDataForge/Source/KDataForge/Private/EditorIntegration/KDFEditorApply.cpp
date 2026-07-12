// Phase 7 — UE-editor integration: `KDF.ApplyToAssets [PathFilter]` applies cdo documents from the
// DataForge packs directly onto editor assets and marks their packages dirty, turning YAML packs
// into permanent asset edits. Editor builds only.

#if WITH_EDITOR

#include "HAL/IConsoleManager.h"
#include "KDFLogging.h"
#include "KDFNode.h"
#include "KDFYamlParser.h"
#include "Loader/KDFPackScanner.h"
#include "Misc/FileHelper.h"
#include "Reflection/KDFOpEngine.h"
#include "Reflection/KDFPatchUtil.h"
#include "Reflection/KDFPropertyPath.h"
#include "Reflection/KDFVanillaCache.h"
#include "Reflection/KDFValueCodec.h"
#include "UObject/Package.h"

namespace
{
	FKDFVanillaCache GEditorApplyVanillaCache;

	/** Resolve a cdo patch target in the editor: class path → CDO, object path → the asset itself. */
	UObject* ResolveEditorTarget(const FString& TargetPath, FString& OutError)
	{
		if (UClass* Class = FKDFValueCodec::ResolveClass(TargetPath, OutError))
		{
			return Class->GetDefaultObject();
		}
		if (UObject* Object = FSoftObjectPath(TargetPath).TryLoad())
		{
			return Object;
		}
		return nullptr;
	}

	void ApplyYamlToEditorAssets(const TArray<FString>& Args)
	{
		const FString PathFilter = Args.Num() > 0 ? Args[0] : FString();

		TArray<FKDFPack> Packs;
		TMap<FString, TArray<FString>> FilesByPack;
		TArray<FKDFDiagnostic> Diagnostics;
		FKDFPackScanner::ScanPacks(Packs, FilesByPack, Diagnostics);
		for (const FKDFDiagnostic& Diagnostic : Diagnostics)
		{
			UE_LOG(LogKDataForge, Warning, TEXT("ApplyToAssets: %s"), *Diagnostic.ToString());
		}

		int32 AppliedOps = 0;
		int32 TouchedAssets = 0;
		TSet<UPackage*> DirtyPackages;

		for (const FKDFPack& Pack : Packs)
		{
			const TArray<FString>* Files = FilesByPack.Find(Pack.mRef);
			if (Files == nullptr)
			{
				continue;
			}
			// Bare generated-content ids in this pack's cdo patches resolve against Pack.mRef first —
			// only takes effect if a KDataForge subsystem happens to be initialized (e.g. PIE running).
			const FKDFValueCodec::FPackScope PackScope(Pack.mRef);
			for (const FString& File : *Files)
			{
				FString FileText;
				FString ParseError;
				if (!FFileHelper::LoadFileToString(FileText, *File))
				{
					UE_LOG(LogKDataForge, Warning, TEXT("ApplyToAssets: could not read '%s'"), *File);
					continue;
				}
				// A file may hold several `---`-separated documents; only the `cdo` ones are relevant here.
				const TArray<TSharedPtr<FKDFNode>> Roots = FKDFYamlParser::ParseStringMulti(FileText, ParseError);
				if (!ParseError.IsEmpty())
				{
					UE_LOG(LogKDataForge, Warning, TEXT("ApplyToAssets: failed to parse '%s': %s"), *File, *ParseError);
					continue;
				}
				for (const TSharedPtr<FKDFNode>& Root : Roots)
				{
					if (!Root.IsValid() || !Root->IsMap() ||
						!Root->GetString(TEXT("type"), FString()).Equals(TEXT("cdo"), ESearchCase::IgnoreCase))
					{
						continue; // editor apply covers cdo documents; other types are runtime concepts
					}
					const FKDFNode* Patches = Root->Find(TEXT("patches"));
					if (Patches == nullptr || !Patches->IsSequence())
					{
						continue;
					}
					for (const TSharedRef<FKDFNode>& PatchRef : Patches->Sequence)
					{
						const FKDFNode& Patch = PatchRef.Get();
						const FKDFNode* Properties = Patch.Find(TEXT("properties"));
						if (Properties == nullptr || !Properties->IsSequence())
						{
							continue;
						}
						// `target:` may be a single path or a sequence — every entry patched the same way.
						for (const FString& TargetPath : FKDFPatchUtil::CollectTargetPaths(Patch))
						{
							if (!PathFilter.IsEmpty() && !TargetPath.StartsWith(PathFilter))
							{
								continue;
							}
							FString Error;
							UObject* Target = ResolveEditorTarget(TargetPath, Error);
							if (Target == nullptr)
							{
								UE_LOG(LogKDataForge, Warning, TEXT("ApplyToAssets: target '%s' not found"),
									   *TargetPath);
								continue;
							}
							bool bTouched = false;
							Target->Modify();
							for (const TSharedRef<FKDFNode>& EntryRef : Properties->Sequence)
							{
								FString PathString;
								EKDFOp Op = EKDFOp::Set;
								FKDFOpArgs OpArgs;
								FKDFPropertyPath PropertyPath;
								if (!FKDFOpEngine::ParseOpEntry(EntryRef.Get(), PathString, Op, OpArgs, Error) ||
									!FKDFPropertyPath::Parse(PathString, PropertyPath, Error))
								{
									UE_LOG(LogKDataForge, Warning, TEXT("ApplyToAssets: %s"), *Error);
									continue;
								}
								FKDFResolvedProperty PreResolved;
								if (FKDFPropertyResolver::Resolve(Target, PropertyPath, PreResolved, Error))
								{
									GEditorApplyVanillaCache.RecordSnapshot(
										Target, PathString,
										FKDFValueCodec::ExportText(PreResolved.mProperty, PreResolved.mValuePtr));
								}
								if (!FKDFOpEngine::ApplyOp(Target, PropertyPath, Op, OpArgs, Error))
								{
									UE_LOG(LogKDataForge, Warning, TEXT("ApplyToAssets: %s"), *Error);
									continue;
								}
								FKDFPatchUtil::PostWriteFixups(Target, PathString);
								++AppliedOps;
								bTouched = true;
							}
							if (bTouched)
							{
								++TouchedAssets;
								UPackage* Package = Target->GetOutermost();
								// CDO edits dirty the Blueprint asset's package (the generated class's outer).
								if (Target->HasAnyFlags(RF_ClassDefaultObject))
								{
									Package = Target->GetClass()->GetOutermost();
								}
								if (Package != nullptr && !Package->HasAnyFlags(RF_Transient))
								{
									Package->MarkPackageDirty();
									DirtyPackages.Add(Package);
								}
							}
						}
					}
				}
			}
		}

		UE_LOG(LogKDataForge, Display,
			   TEXT("KDF.ApplyToAssets: %d op(s) applied to %d asset(s), %d package(s) marked dirty%s — save the "
					"dirty packages to make the edits permanent"),
			   AppliedOps, TouchedAssets, DirtyPackages.Num(),
			   PathFilter.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (filter: %s)"), *PathFilter));
	}

	FAutoConsoleCommand GKDFApplyToAssetsCommand(
		TEXT("KDF.ApplyToAssets"),
		TEXT("Applies DataForge cdo YAML directly onto editor assets and marks packages dirty. "
			 "Optional arg: a path prefix filter, e.g. KDF.ApplyToAssets /Game/FactoryGame/Recipes"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&ApplyYamlToEditorAssets));
} // namespace

#endif // WITH_EDITOR

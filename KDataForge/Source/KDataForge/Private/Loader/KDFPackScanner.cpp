#include "Loader/KDFPackScanner.h"

#include "HAL/FileManager.h"
#include "KDFLogging.h"
#include "KDFYamlParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	constexpr int32 MaxScanDepth = 6;

	const TCHAR* PrunedDirectoryNames[] = {
		TEXT("Content"),		  TEXT("Saved"),   TEXT("Intermediate"), TEXT("Binaries"),
		TEXT("DerivedDataCache"), TEXT("Plugins"), TEXT("Source"),		 TEXT("Config")};

	bool IsPrunedDirectory(const FString& DirectoryName)
	{
		for (const TCHAR* Pruned : PrunedDirectoryNames)
		{
			if (DirectoryName.Equals(Pruned, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	void FindDataEditorRootsRecursive(const FString& Directory, int32 Depth, TArray<FString>& OutRoots)
	{
		if (Depth > MaxScanDepth)
		{
			return;
		}
		TArray<FString> SubDirectories;
		IFileManager::Get().FindFiles(SubDirectories, *(Directory / TEXT("*")), false, true);
		for (const FString& SubDirectory : SubDirectories)
		{
			if (IsPrunedDirectory(SubDirectory))
			{
				continue;
			}
			const FString FullPath = Directory / SubDirectory;

			const bool bNamedRoot = SubDirectory.Equals(TEXT("DataForge"), ESearchCase::IgnoreCase) ||
				SubDirectory.Equals(TEXT("data-editor"), ESearchCase::IgnoreCase);
			if (bNamedRoot || FPaths::FileExists(FullPath / TEXT("pack.yml")))
			{
				OutRoots.Add(FullPath);
			}
			FindDataEditorRootsRecursive(FullPath, Depth + 1, OutRoots);
		}
	}

	void AddScanDiagnostic(TArray<FKDFDiagnostic>& Diagnostics, EKDFSeverity Severity, const FString& File,
						   const FString& Message)
	{
		FKDFDiagnostic& Diagnostic = Diagnostics.AddDefaulted_GetRef();
		Diagnostic.mSeverity = Severity;
		Diagnostic.mFile = File;
		Diagnostic.mMessage = Message;
	}

	FKDFPack ReadPackManifest(const FString& DataEditorRoot, TArray<FKDFDiagnostic>& Diagnostics)
	{
		FKDFPack Pack;
		Pack.mDirectory = DataEditorRoot;

		Pack.mRef = FPaths::GetCleanFilename(FPaths::GetPath(DataEditorRoot));
		Pack.mName = Pack.mRef;

		const FString ManifestPath = DataEditorRoot / TEXT("pack.yml");
		if (!FPaths::FileExists(ManifestPath))
		{
			return Pack;
		}

		FString ManifestText;
		if (!FFileHelper::LoadFileToString(ManifestText, *ManifestPath))
		{
			AddScanDiagnostic(Diagnostics, EKDFSeverity::Warning, ManifestPath, TEXT("Could not read pack.yml"));
			return Pack;
		}
		FString ParseError;
		const TSharedPtr<FKDFNode> Manifest = FKDFYamlParser::ParseString(ManifestText, ParseError);
		if (!Manifest.IsValid() || !Manifest->IsMap())
		{
			AddScanDiagnostic(Diagnostics, EKDFSeverity::Warning, ManifestPath,
							  FString::Printf(TEXT("Invalid pack.yml: %s"), *ParseError));
			return Pack;
		}

		Pack.mRef = Manifest->GetString(TEXT("ref"), Pack.mRef);
		Pack.mName = Manifest->GetString(TEXT("name"), Pack.mRef);
		Pack.mVersion = Manifest->GetString(TEXT("version"), FString());
		Pack.mPriority = static_cast<int32>(Manifest->GetInt(TEXT("priority"), Pack.mPriority));
		Pack.bEnabled = Manifest->GetBool(TEXT("enabled"), true);
		Pack.bDebug = Manifest->GetBool(TEXT("debug"), false);
		Pack.mConditionsNode = Manifest->FindShared(TEXT("conditions"));
		Pack.mConditionBehaviorNode = Manifest->FindShared(TEXT("conditionBehaivor"));
		if (!Pack.mConditionBehaviorNode.IsValid())
		{
			Pack.mConditionBehaviorNode = Manifest->FindShared(TEXT("conditionBehavior"));
		}
		if (const FKDFNode* Dependencies = Manifest->Find(TEXT("dependencies")))
		{
			for (const TSharedRef<FKDFNode>& Dependency : Dependencies->Sequence)
			{
				Pack.mDependencies.Add(Dependency->GetString());
			}
		}
		if (const FKDFNode* Redirects = Manifest->Find(TEXT("redirects")))
		{
			for (const TTuple<FString, TSharedRef<FKDFNode>>& Redirect : Redirects->Map)
			{
				Pack.mRedirects.Add(Redirect.Key, Redirect.Value->GetString());
			}
		}
		return Pack;
	}
}

FString FKDFPackScanner::GetSearchRoot() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()); }

TArray<FString> FKDFPackScanner::FindDataEditorRoots()
{
	TArray<FString> Roots;
	FindDataEditorRootsRecursive(GetSearchRoot(), 0, Roots);
	return Roots;
}

void FKDFPackScanner::ScanPacks(TArray<FKDFPack>& OutPacks, TMap<FString, TArray<FString>>& OutFiles,
								TArray<FKDFDiagnostic>& Diagnostics)
{
	OutPacks.Reset();
	OutFiles.Reset();

	const TArray<FString> Roots = FindDataEditorRoots();
	UE_LOG(LogKDataForge, Log, TEXT("Found %d DataForge root(s) under %s"), Roots.Num(), *GetSearchRoot());

	TArray<FString> OutermostRoots;
	for (const FString& Root : Roots)
	{
		const bool bNestedInAnotherRoot = Roots.ContainsByPredicate(
			[&Root](const FString& Other) { return Other != Root && Root.StartsWith(Other + TEXT("/")); });
		if (!bNestedInAnotherRoot)
		{
			OutermostRoots.Add(Root);
		}
	}

	TMap<FString, TArray<FString>> FilesByRootPath;
	for (const FString& OutermostRoot : OutermostRoots)
	{
		TArray<FString> YamlFiles;
		IFileManager::Get().FindFilesRecursive(YamlFiles, *OutermostRoot, TEXT("*.yml"), true, false);
		IFileManager::Get().FindFilesRecursive(YamlFiles, *OutermostRoot, TEXT("*.yaml"), true, false, false);
		for (const FString& File : YamlFiles)
		{
			if (FPaths::GetCleanFilename(File).Equals(TEXT("pack.yml"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			FString OwningRoot;
			for (const FString& Root : Roots)
			{
				if (File.StartsWith(Root + TEXT("/")) && Root.Len() > OwningRoot.Len())
				{
					OwningRoot = Root;
				}
			}
			if (!OwningRoot.IsEmpty())
			{
				FilesByRootPath.FindOrAdd(OwningRoot).Add(File);
			}
		}
	}

	TSet<FString> SeenRefs;
	for (const FString& Root : Roots)
	{
		FKDFPack Pack = ReadPackManifest(Root, Diagnostics);
		if (!Pack.bEnabled)
		{
			AddScanDiagnostic(Diagnostics, EKDFSeverity::Info, Root,
							  FString::Printf(TEXT("Pack '%s' is disabled — skipped"), *Pack.mRef));
			continue;
		}
		if (SeenRefs.Contains(Pack.mRef))
		{
			AddScanDiagnostic(Diagnostics, EKDFSeverity::Warning, Root,
							  FString::Printf(TEXT("Duplicate pack ref '%s' — directory skipped"), *Pack.mRef));
			continue;
		}
		SeenRefs.Add(Pack.mRef);

		TArray<FString> YamlFiles = MoveTemp(FilesByRootPath.FindOrAdd(Root));
		YamlFiles.Sort();

		OutFiles.Add(Pack.mRef, MoveTemp(YamlFiles));
		OutPacks.Add(MoveTemp(Pack));
	}

	OutPacks.Sort([](const FKDFPack& A, const FKDFPack& B)
				  { return A.mPriority != B.mPriority ? A.mPriority > B.mPriority : A.mRef < B.mRef; });

	TSet<FString> AvailableRefs;
	for (const FKDFPack& Pack : OutPacks)
	{
		AvailableRefs.Add(Pack.mRef);
	}
	TSet<FString> InvalidRefs;
	bool bInvalidatedAny = true;
	while (bInvalidatedAny)
	{
		bInvalidatedAny = false;
		for (const FKDFPack& Pack : OutPacks)
		{
			if (InvalidRefs.Contains(Pack.mRef))
			{
				continue;
			}
			for (const FString& Dependency : Pack.mDependencies)
			{
				if (Dependency == Pack.mRef || !AvailableRefs.Contains(Dependency) || InvalidRefs.Contains(Dependency))
				{
					AddScanDiagnostic(
						Diagnostics, EKDFSeverity::Warning, Pack.mDirectory / TEXT("pack.yml"),
						FString::Printf(TEXT("Pack '%s' skipped: dependency '%s' is missing, invalid, or self-referential"),
							*Pack.mRef, *Dependency));
					InvalidRefs.Add(Pack.mRef);
					bInvalidatedAny = true;
					break;
				}
			}
		}
	}
	for (const FString& InvalidRef : InvalidRefs)
	{
		OutFiles.Remove(InvalidRef);
	}
	OutPacks.RemoveAll([&InvalidRefs](const FKDFPack& Pack) { return InvalidRefs.Contains(Pack.mRef); });

	TArray<FKDFPack> OrderedPacks;
	OrderedPacks.Reserve(OutPacks.Num());
	TSet<FString> EmittedRefs;
	while (OrderedPacks.Num() < OutPacks.Num())
	{
		const FKDFPack* NextPack = nullptr;
		for (const FKDFPack& Candidate : OutPacks)
		{
			if (EmittedRefs.Contains(Candidate.mRef))
			{
				continue;
			}
			const bool bDependenciesReady = Candidate.mDependencies.ContainsByPredicate(
				[&EmittedRefs](const FString& Dependency) { return !EmittedRefs.Contains(Dependency); }) == false;
			if (bDependenciesReady)
			{
				NextPack = &Candidate;
				break;
			}
		}
		if (NextPack == nullptr)
		{
			for (const FKDFPack& Pack : OutPacks)
			{
				if (!EmittedRefs.Contains(Pack.mRef))
				{
					AddScanDiagnostic(Diagnostics, EKDFSeverity::Warning, Pack.mDirectory / TEXT("pack.yml"),
						FString::Printf(TEXT("Pack '%s' skipped: dependency cycle detected"), *Pack.mRef));
					OutFiles.Remove(Pack.mRef);
				}
			}
			break;
		}
		OrderedPacks.Add(*NextPack);
		EmittedRefs.Add(NextPack->mRef);
	}
	OutPacks = MoveTemp(OrderedPacks);
}

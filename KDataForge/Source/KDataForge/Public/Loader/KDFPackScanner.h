#pragma once

#include "CoreMinimal.h"

#include "KDFTypes.h"
#include "Loader/KDFLoaderTypes.h"

/**
 * Filesystem discovery of DataForge content.
 * Recursively scans every `DataForge` directory under `<GameFolder>/FactoryGame` for packs
 * (`pack.yml`) and `.yml`/`.yaml` documents. A subfolder becomes its own (nested) pack root simply
 * by holding a `pack.yml` directly inside it, at any depth — e.g. `DataForge/rss/pack.yml` scopes an
 * "rss" pack to just that subtree, letting one DataForge root hold several independently
 * versioned/conditioned/enabled packs side by side. `DataForge` folders nested inside another
 * `DataForge` folder are supported the same way. Each file belongs to its deepest enclosing root.
 * Heavy engine directories (Content, Saved, Intermediate, …) are pruned; see Docs/YamlSchemaSpec.md.
 */
class KDATAFORGE_API FKDFPackScanner
{
public:
	/** Absolute FactoryGame project directory the scan starts from. */
	static FString GetSearchRoot();

	/** Every pack root under the search root — `DataForge`/`data-editor` directories and any
	 *  subfolder (at any nesting depth, including inside another such directory) holding its own
	 *  `pack.yml`. */
	static TArray<FString> FindDataEditorRoots();

	/**
	 * Scans all pack roots for packs and their YAML files.
	 * @param OutPacks     One pack per root (manifest or implicit).
	 * @param OutFiles     Absolute .yml/.yaml file paths per pack ref (pack.yml excluded; a nested
	 *                     root's files are never also listed under its parent).
	 * @param Diagnostics  Scan warnings (duplicate pack refs, unreadable manifests, …).
	 */
	static void ScanPacks(TArray<FKDFPack>& OutPacks, TMap<FString, TArray<FString>>& OutFiles,
						  TArray<FKDFDiagnostic>& Diagnostics);
};

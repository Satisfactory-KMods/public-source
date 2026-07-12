#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "KDFEditorModel.generated.h"

class UKDFSubsystem;
class UWorld;
struct FKDFNode;
struct FAssetData;

/** Value editor kind resolved for a property row. */
enum class EKDFRowKind : uint8
{
	Bool,
	Integer,
	Float,
	Enum,
	Text, // FString/FName/FText — plain text box
	Color, // FLinearColor/FColor — swatch + picker + raw text
	ClassRef, // class/soft-class reference — searchable picker + raw input
	ObjectRef, // object/soft-object reference — searchable picker + raw input
	Struct, // expandable: children = members
	Array, // expandable: children = elements, add/remove
	Set, // expandable: children = elements (edit per element; add/remove via YAML)
	Map, // expandable: children = values keyed by exported key text
	Complex // fallback — canonical text editing
};

/** One entry in the browser list. */
struct FKDFBrowserItem
{
	FString mDisplayName;
	FString mSubText; // class path / owning mod
	FString mModKey; // owning mod/path bucket for the mod filter (e.g. "/Game", "/RSS", "/Script/FactoryGame")
	TWeakObjectPtr<UObject> mObject;

	/**
	 * Set instead of mObject for an asset-registry entry that isn't loaded yet (a Blueprint item,
	 * building, recipe, or data asset the game hasn't referenced this session). Selecting the item
	 * force-loads it via this path and caches the result into mObject.
	 */
	FString mUnloadedPath;
};
using FKDFBrowserItemPtr = TSharedPtr<FKDFBrowserItem>;

/** One candidate for a class/object picker. */
struct FKDFPickCandidate
{
	FString mDisplayName;
	FString mPath; // committed as the property value
};
using FKDFPickCandidatePtr = TSharedPtr<FKDFPickCandidate>;

/**
 * One inspector row in the property tree. Children exist for structs (members), arrays/sets
 * (elements), and maps (values); they are rebuilt on refresh. Every row carries a full property
 * path in the op-engine grammar (`mIngredients[0].Amount`, `mMap["Key"].X`), so any depth commits
 * through the same pipeline.
 */
struct FKDFPropertyRowData
{
	FString mName;
	FString mPath;
	FString mTypeName;
	FString mValueText; // canonical text (editing + fallback display)
	FString mSummary; // containers/structs: "3 elements", struct type
	FString mTextNamespace;
	FString mTextKey;
	FString mTextSource;
	EKDFRowKind mKind = EKDFRowKind::Complex;
	bool bIsCategory = false;
	int32 mDepth = 0;
	int32 mElementIndex = INDEX_NONE; // set for array elements (remove anchors)
	bool bModified = false; // differs from the vanilla snapshot this session
	bool bIsArrayElement = false;
	FLinearColor mColorValue = FLinearColor::White; // valid when mKind == Color

	TArray<TSharedPtr<FString>> mEnumOptions;

	/** Candidates for ClassRef/ObjectRef pickers (built lazily on first request). */
	TArray<FKDFPickCandidatePtr> mPickCandidates;
	bool bPickCandidatesBuilt = false;

	TArray<TSharedPtr<FKDFPropertyRowData>> mChildren;
	TWeakPtr<FKDFPropertyRowData> mParent;
};
using FKDFPropertyRowPtr = TSharedPtr<FKDFPropertyRowData>;

/** Undo/redo entry: canonical pre/post text of one property write. */
struct FKDFUndoEntry
{
	TWeakObjectPtr<UObject> mTarget;
	FString mPath;
	FString mPreValue;
	FString mPostValue;
};

/**
 * Non-visual state of the in-game editor: browser categories and filtering, selection, the
 * inspector property TREE (reflection-driven, nested structs/arrays/maps), undo/redo, export, and
 * content-creation drafts. All edits go through UKDFSubsystem::ApplyEditorOp — the same pipeline
 * YAML uses — so the editor is just another op producer.
 */
UCLASS()
class KDATAFORGEEDITORUI_API UKDFEditorModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* World);

	// --- Browser ---
	const TArray<TSharedPtr<FString>>& GetCategories() const { return mCategories; }
	void SetCategory(const FString& Category);
	const FString& GetActiveCategory() const { return mActiveCategory; }
	void SetSearchText(const FString& Search);
	const TArray<FKDFBrowserItemPtr>& GetFilteredItems() const { return mFilteredItems; }

	/**
	 * Mod/path filter for the browser list — narrows the current category to one owning mod or
	 * content mount (e.g. "/RSS", "/Game" for the base game, "/Script/FactoryGame" for a native
	 * module, "/KDataForge/Gen/<PackRef>" for one pack's generated content). Rebuilt from the
	 * current category's items every RebuildBrowserSource call; "(All)" is always first and "/Game"
	 * is always offered even if the current category has no base-game entries.
	 */
	const TArray<TSharedPtr<FString>>& GetModOptions() const { return mModOptions; }
	void SetModFilter(const FString& Mod);
	const FString& GetActiveModFilter() const { return mActiveModFilter; }

	// --- Selection / inspector tree ---
	void Select(const FKDFBrowserItemPtr& Item);
	UObject* GetSelectedObject() const { return mSelectedObject.Get(); }
	FString GetSelectedTitle() const;
	FString GetSelectedSubtitle() const;
	const TArray<FKDFPropertyRowPtr>& GetRootRows() const { return mRootRows; }
	void RefreshPropertyRows();

	/** Lazily builds picker candidates for a ClassRef/ObjectRef row (search happens in the widget). */
	const TArray<FKDFPickCandidatePtr>& GetPickCandidates(const FKDFPropertyRowPtr& Row);

	/** Applies a value (canonical/user text) to a row through the standard op pipeline. */
	bool CommitRowValue(const FKDFPropertyRowPtr& Row, const FString& NewValueText, FString& OutMessage);

	/** Applies a localized FText using its namespace/key/source components. */
	bool CommitLocalizedText(const FKDFPropertyRowPtr& Row, const FString& Namespace, const FString& Key,
							 const FString& Source, FString& OutMessage);

	/** Appends a default-constructed element to an array row. */
	bool ArrayAddElement(const FKDFPropertyRowPtr& ArrayRow, FString& OutMessage);

	/** Removes an element row from its parent array. */
	bool ArrayRemoveElement(const FKDFPropertyRowPtr& ElementRow, FString& OutMessage);

	// --- Undo / redo ---
	bool CanUndo() const { return mUndoStack.Num() > 0; }
	bool CanRedo() const { return mRedoStack.Num() > 0; }
	bool Undo(FString& OutMessage);
	bool Redo(FString& OutMessage);

	// --- Export / preview / analysis ---
	bool ExportSelected(bool bDiffOnly, FString& OutMessage);
	FString GetSelectedYamlPreview(bool bDiffOnly) const;

	/** Diff tab: every session-modified property of the selection as `path: vanilla -> current`. */
	FString GetSelectedDiffText() const;

	/** References tab: recipes using/producing the selected class + schematics unlocking those recipes. */
	FString GetSelectedReferencesText() const;

	/**
	 * Re-scans DataForge YAML from disk and re-applies live-safe stages (same as chat's `/kdf
	 * reload`) — CDOChanges/GameTags/Localization/RuntimePatches only; new item/recipe/schematic
	 * classes still need a session restart to register. Returns the applied document count and
	 * fills OutReportText with the same summary `/kdf report` prints (packs, ops, warnings, errors).
	 */
	int32 ReloadFromDisk(FString& OutReportText);

	// --- Creation ---
	const TArray<TSharedPtr<FString>>& GetCreatableKinds() const { return mCreatableKinds; }

	/** RegisterAs options for `type: class` drafts ("", "recipe", "schematic", "research"). */
	const TArray<TSharedPtr<FString>>& GetRegisterAsOptions() const { return mRegisterAsOptions; }

	/**
	 * Parent (or `class:` for dataasset) candidates for the create pane, filtered to the kind's
	 * required base class — `class` returns every loaded class. Built lazily, cached per kind.
	 */
	const TArray<FKDFPickCandidatePtr>& GetCreateParentCandidates(const FString& Kind);

	bool CreateContentDraft(const FString& Kind, const FString& Id, const FString& ParentOrClassPath,
							const FString& RegisterAs, FString& OutMessage);

	int32 GetSessionEditCount() const;

private:
	void RebuildBrowserSource();
	void RebuildModOptions();
	void ApplyFilter();
	void AddClassItems(const TArray<UClass*>& Classes);

	/**
	 * Adds unloaded browser entries straight from the asset registry — no force-load. Used for
	 * Blueprint-backed items/buildings/recipes/data assets the game hasn't referenced this session,
	 * so the browser lists everything that exists, not just what happens to be in memory. Loading
	 * happens lazily in Select().
	 */
	void AddSoftAssetItems(const TArray<FAssetData>& Assets, bool bSubTextIsClassName);
	UKDFSubsystem* GetKDF() const;
	bool CommitRowNode(const FKDFPropertyRowPtr& Row, const TSharedRef<FKDFNode>& ValueNode, FString& OutMessage);

	/** Builds one row (children included for containers/structs) for a resolved property value. */
	FKDFPropertyRowPtr BuildRow(const FString& Name, const FString& Path, const FProperty* Property,
								const void* ValuePtr, int32 Depth, const TSharedPtr<FKDFPropertyRowData>& Parent);
	void BuildChildren(const FKDFPropertyRowPtr& Row, const FProperty* Property, const void* ValuePtr);

	TWeakObjectPtr<UWorld> mWorld;
	TArray<TSharedPtr<FString>> mCategories;
	TArray<TSharedPtr<FString>> mCreatableKinds;
	TArray<TSharedPtr<FString>> mRegisterAsOptions;
	TMap<FString, TArray<FKDFPickCandidatePtr>> mCreateParentCandidates;
	FString mActiveCategory;
	FString mSearchText;
	TArray<TSharedPtr<FString>> mModOptions;
	FString mActiveModFilter = TEXT("(All)");
	TArray<FKDFBrowserItemPtr> mSourceItems;
	TArray<FKDFBrowserItemPtr> mFilteredItems;
	TWeakObjectPtr<UObject> mSelectedObject;
	TArray<FKDFPropertyRowPtr> mRootRows;
	TArray<FKDFUndoEntry> mUndoStack;
	TArray<FKDFUndoEntry> mRedoStack;
};

// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "KDFEditorModel.generated.h"

class UKDFSubsystem;
class UWorld;
struct FKDFNode;
struct FAssetData;

enum class EKDFRowKind : uint8
{
	Bool,
	Integer,
	Float,
	Enum,
	Text,
	Color,
	ClassRef,
	ObjectRef,
	Struct,
	Array,
	Set,
	Map,
	Complex
};

struct FKDFBrowserItem
{
	FString mDisplayName;
	FString mSubText;
	FString mModKey;
	TWeakObjectPtr<UObject> mObject;

	FString mUnloadedPath;
};
using FKDFBrowserItemPtr = TSharedPtr<FKDFBrowserItem>;

struct FKDFPickCandidate
{
	FString mDisplayName;
	FString mPath;
};
using FKDFPickCandidatePtr = TSharedPtr<FKDFPickCandidate>;

struct FKDFPropertyRowData
{
	FString mName;
	FString mPath;
	FString mTypeName;
	FString mValueText;
	FString mSummary;
	FString mTextNamespace;
	FString mTextKey;
	FString mTextSource;
	EKDFRowKind mKind = EKDFRowKind::Complex;
	bool bIsCategory = false;
	int32 mDepth = 0;
	int32 mElementIndex = INDEX_NONE;
	bool bModified = false;
	bool bIsArrayElement = false;
	FLinearColor mColorValue = FLinearColor::White;

	TArray<TSharedPtr<FString>> mEnumOptions;

	TArray<FKDFPickCandidatePtr> mPickCandidates;
	bool bPickCandidatesBuilt = false;

	TArray<TSharedPtr<FKDFPropertyRowData>> mChildren;
	TWeakPtr<FKDFPropertyRowData> mParent;
};
using FKDFPropertyRowPtr = TSharedPtr<FKDFPropertyRowData>;

struct FKDFUndoEntry
{
	TWeakObjectPtr<UObject> mTarget;
	FString mPath;
	FString mPreValue;
	FString mPostValue;
};

UCLASS()
class KDATAFORGEEDITORUI_API UKDFEditorModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* World);

	const TArray<TSharedPtr<FString>>& GetCategories() const { return mCategories; }
	void SetCategory(const FString& Category);
	const FString& GetActiveCategory() const { return mActiveCategory; }
	void SetSearchText(const FString& Search);
	const TArray<FKDFBrowserItemPtr>& GetFilteredItems() const { return mFilteredItems; }

	const TArray<TSharedPtr<FString>>& GetModOptions() const { return mModOptions; }
	void SetModFilter(const FString& Mod);
	const FString& GetActiveModFilter() const { return mActiveModFilter; }

	void Select(const FKDFBrowserItemPtr& Item);
	UObject* GetSelectedObject() const { return mSelectedObject.Get(); }
	FString GetSelectedTitle() const;
	FString GetSelectedSubtitle() const;
	const TArray<FKDFPropertyRowPtr>& GetRootRows() const { return mRootRows; }
	void RefreshPropertyRows();

	const TArray<FKDFPickCandidatePtr>& GetPickCandidates(const FKDFPropertyRowPtr& Row);

	bool CommitRowValue(const FKDFPropertyRowPtr& Row, const FString& NewValueText, FString& OutMessage);

	bool CommitLocalizedText(const FKDFPropertyRowPtr& Row, const FString& Namespace, const FString& Key,
							 const FString& Source, FString& OutMessage);

	bool ArrayAddElement(const FKDFPropertyRowPtr& ArrayRow, FString& OutMessage);

	bool ArrayRemoveElement(const FKDFPropertyRowPtr& ElementRow, FString& OutMessage);

	bool CanUndo() const { return mUndoStack.Num() > 0; }
	bool CanRedo() const { return mRedoStack.Num() > 0; }
	bool Undo(FString& OutMessage);
	bool Redo(FString& OutMessage);

	bool ExportSelected(bool bDiffOnly, FString& OutMessage);
	FString GetSelectedYamlPreview(bool bDiffOnly) const;

	FString GetSelectedDiffText() const;

	FString GetSelectedReferencesText() const;

	const TArray<TSharedPtr<FString>>& GetCreatableKinds() const { return mCreatableKinds; }

	const TArray<TSharedPtr<FString>>& GetRegisterAsOptions() const { return mRegisterAsOptions; }

	const TArray<FKDFPickCandidatePtr>& GetCreateParentCandidates(const FString& Kind);

	bool CreateContentDraft(const FString& Kind, const FString& Id, const FString& ParentOrClassPath,
							const FString& RegisterAs, FString& OutMessage);

	int32 GetSessionEditCount() const;

private:
	void RebuildBrowserSource();
	void RebuildModOptions();
	void ApplyFilter();
	void AddClassItems(const TArray<UClass*>& Classes);

	void AddSoftAssetItems(const TArray<FAssetData>& Assets, bool bSubTextIsClassName);
	UKDFSubsystem* GetKDF() const;
	bool CommitRowNode(const FKDFPropertyRowPtr& Row, const TSharedRef<FKDFNode>& ValueNode, FString& OutMessage);

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

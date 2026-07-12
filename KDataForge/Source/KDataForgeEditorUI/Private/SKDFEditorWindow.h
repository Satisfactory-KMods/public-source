#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STreeView.h"

#include "KDFEditorModel.h"

class SEditableTextBox;
class SMultiLineEditableTextBox;
class SSearchBox;
class STextBlock;

/**
 * Root Slate widget of the in-game editor. Three panes over a dark FICSIT-styled chrome:
 * browser (virtualized list + category + search) · inspector (virtualized property TREE with
 * typed editors — nested structs, arrays with add/remove, maps, color swatches + picker,
 * searchable class/object pickers with raw-input fallback) · side pane (live YAML preview,
 * content creation, status). Pure code-built Slate — no Blueprint/UMG assets involved.
 */
class SKDFEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKDFEditorWindow) {}
	SLATE_ARGUMENT(TWeakObjectPtr<UKDFEditorModel>, Model)
	SLATE_EVENT(FSimpleDelegate, OnCloseRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Esc closes the editor — the window must be focusable for the key to reach it.
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// --- pane builders ---
	TSharedRef<SWidget> MakeTabButton(const FString& Label, int32 TabIndex);
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildBrowserPane();
	TSharedRef<SWidget> BuildInspectorPane();
	TSharedRef<SWidget> BuildSidePane();

	// --- browser ---
	TSharedRef<ITableRow> GenerateBrowserRow(FKDFBrowserItemPtr Item, const TSharedRef<STableViewBase>& Owner);
	void OnCategoryChanged(TSharedPtr<FString> NewCategory, ESelectInfo::Type SelectInfo);
	void OnModFilterChanged(TSharedPtr<FString> NewMod, ESelectInfo::Type SelectInfo);
	void OnSearchChanged(const FText& NewText);
	void OnBrowserSelectionChanged(FKDFBrowserItemPtr Item, ESelectInfo::Type SelectInfo);

	// --- inspector tree ---
	TSharedRef<ITableRow> GeneratePropertyRow(FKDFPropertyRowPtr Row, const TSharedRef<STableViewBase>& Owner);
	void OnGetRowChildren(FKDFPropertyRowPtr Row, TArray<FKDFPropertyRowPtr>& OutChildren);
	TSharedRef<SWidget> MakeValueEditor(const FKDFPropertyRowPtr& Row);
	TSharedRef<SWidget> MakeTextEditor(const FKDFPropertyRowPtr& Row);
	TSharedRef<SWidget> MakeReferencePicker(const FKDFPropertyRowPtr& Row);
	TSharedRef<SWidget> MakeColorEditor(const FKDFPropertyRowPtr& Row);
	TSharedRef<SWidget> MakeRefPickerMenu(FKDFPropertyRowPtr Row);
	void OpenColorPicker(FKDFPropertyRowPtr Row);

	/** Searchable parent/class picker menu for the create pane (candidates depend on the kind). */
	TSharedRef<SWidget> MakeCreateParentMenu();

	// --- actions ---
	void CommitRow(const FKDFPropertyRowPtr& Row, const FString& NewValue);
	FReply OnUndo();
	FReply OnRedo();
	FReply OnExport(bool bDiffOnly);
	FReply OnCreate();
	FReply OnReload();
	FReply OnClose();
	void SetStatus(const FString& Message, bool bError);
	void RefreshAll();
	void RefreshYamlPreview();

	/** Rebuilds the property rows and re-expands every node that was expanded before (by path). */
	void RefreshPropertyTreePreservingExpansion();

	/** Expands the (current) row with the given property path, if it exists after a rebuild. */
	void ExpandRowByPath(const FString& Path);

	/**
	 * Expands/collapses a row and all its descendants. Also bound as the tree's
	 * OnSetExpansionRecursive, which enables Shift+click on any expander arrow.
	 */
	void SetRowExpansionRecursive(FKDFPropertyRowPtr Row, bool bExpand);

	/** Expands/collapses the entire property tree (Expand All / Collapse All header buttons). */
	void SetAllExpansion(bool bExpand);

	/** Modal popup showing a load/reload report (used by OnReload). */
	void ShowResultDialog(const FString& Title, const FString& ReportText, bool bHasErrors);

	TWeakObjectPtr<UKDFEditorModel> mModel;
	FSimpleDelegate mOnCloseRequested;

	TSharedPtr<SListView<FKDFBrowserItemPtr>> mBrowserList;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> mModCombo;
	TSharedPtr<STreeView<FKDFPropertyRowPtr>> mPropertyTree;
	TSharedPtr<SMultiLineEditableTextBox> mYamlPreview;
	TSharedPtr<STextBlock> mStatusText;
	TSharedPtr<STextBlock> mSelectionTitle;
	TSharedPtr<STextBlock> mSelectionSubtitle;
	TSharedPtr<SEditableTextBox> mCreateIdBox;
	TSharedPtr<SEditableTextBox> mCreateParentBox;
	TSharedPtr<class SComboButton> mCreateParentPicker;
	TSharedPtr<FString> mCreateKind;
	TSharedPtr<FString> mCreateRegisterAs;
	bool bPreviewDiffOnly = true;
	bool bPropertyExpansionInitialized = false;
	FString mExpansionObjectPath;

	/** Active side-pane tab: 0 = YAML preview, 1 = Diff, 2 = References. */
	int32 mSideTab = 0;
};

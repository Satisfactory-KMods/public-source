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

class SKDFEditorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKDFEditorWindow) {}
	SLATE_ARGUMENT(TWeakObjectPtr<UKDFEditorModel>, Model)
	SLATE_EVENT(FSimpleDelegate, OnCloseRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:

	TSharedRef<SWidget> MakeTabButton(const FString& Label, int32 TabIndex);
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildBrowserPane();
	TSharedRef<SWidget> BuildInspectorPane();
	TSharedRef<SWidget> BuildSidePane();

	TSharedRef<ITableRow> GenerateBrowserRow(FKDFBrowserItemPtr Item, const TSharedRef<STableViewBase>& Owner);
	void OnCategoryChanged(TSharedPtr<FString> NewCategory, ESelectInfo::Type SelectInfo);
	void OnModFilterChanged(TSharedPtr<FString> NewMod, ESelectInfo::Type SelectInfo);
	void OnSearchChanged(const FText& NewText);
	void OnBrowserSelectionChanged(FKDFBrowserItemPtr Item, ESelectInfo::Type SelectInfo);

	TSharedRef<ITableRow> GeneratePropertyRow(FKDFPropertyRowPtr Row, const TSharedRef<STableViewBase>& Owner);
	void OnGetRowChildren(FKDFPropertyRowPtr Row, TArray<FKDFPropertyRowPtr>& OutChildren);
	TSharedRef<SWidget> MakeValueEditor(const FKDFPropertyRowPtr& Row);
	TSharedRef<SWidget> MakeTextEditor(const FKDFPropertyRowPtr& Row);
	TSharedRef<SWidget> MakeReferencePicker(const FKDFPropertyRowPtr& Row);
	TSharedRef<SWidget> MakeColorEditor(const FKDFPropertyRowPtr& Row);
	TSharedRef<SWidget> MakeRefPickerMenu(FKDFPropertyRowPtr Row);
	void OpenColorPicker(FKDFPropertyRowPtr Row);

	TSharedRef<SWidget> MakeCreateParentMenu();

	void CommitRow(const FKDFPropertyRowPtr& Row, const FString& NewValue);
	FReply OnUndo();
	FReply OnRedo();
	FReply OnExport(bool bDiffOnly);
	FReply OnCreate();
	FReply OnClose();
	void SetStatus(const FString& Message, bool bError);
	void RefreshAll();
	void RefreshYamlPreview();

	void RefreshPropertyTreePreservingExpansion();

	void ExpandRowByPath(const FString& Path);

	void SetRowExpansionRecursive(FKDFPropertyRowPtr Row, bool bExpand);

	void SetAllExpansion(bool bExpand);

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

	int32 mSideTab = 0;
};

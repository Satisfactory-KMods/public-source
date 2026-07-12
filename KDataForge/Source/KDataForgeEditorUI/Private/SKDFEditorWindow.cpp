#include "SKDFEditorWindow.h"

#include "Engine/Texture.h"
#include "Framework/Application/SlateApplication.h"
#include "KDFEditorStyle.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

using namespace KDFEditorStyle;

namespace
{
	const FSlateBrush* WhiteBrush() { return FCoreStyle::Get().GetBrush("WhiteBrush"); }

	TSharedRef<SWidget> MakeHeaderText(const FString& Text)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FSlateColor(Orange));
	}

	TSharedRef<SButton> MakeButton(const FString& Label, const FOnClicked& OnClicked)
	{
		return SNew(SButton)
			.OnClicked(OnClicked)
			.ButtonColorAndOpacity(FSlateColor(ButtonPrimary))
			.ContentPadding(FMargin(PadM, PadS))[SNew(STextBlock)
													 .Text(FText::FromString(Label))
													 .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
													 .ColorAndOpacity(FSlateColor(ButtonText))];
	}

	/** Small square icon-ish button for row actions ([+], [×]). */
	TSharedRef<SButton> MakeRowButton(const FString& Label, const FLinearColor& Tint, const FOnClicked& OnClicked)
	{
		return SNew(SButton)
			.OnClicked(OnClicked)
			.ButtonColorAndOpacity(FSlateColor(Tint))
			.ContentPadding(FMargin(6.f, 1.f))[SNew(STextBlock)
												   .Text(FText::FromString(Label))
												   .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
												   .ColorAndOpacity(FSlateColor(ButtonText))];
	}
} // namespace

void SKDFEditorWindow::Construct(const FArguments& InArgs)
{
	mModel = InArgs._Model;
	mOnCloseRequested = InArgs._OnCloseRequested;
	if (UKDFEditorModel* Model = mModel.Get())
	{
		mCreateKind = Model->GetCreatableKinds().Num() > 0 ? Model->GetCreatableKinds()[0] : nullptr;
		mCreateRegisterAs = Model->GetRegisterAsOptions().Num() > 0 ? Model->GetRegisterAsOptions()[0] : nullptr;
	}

	// clang-format off
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FSlateColor(PanelDark))
		.Padding(FMargin(PadL))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, PadM)
			[
				BuildToolbar()
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SSplitter)
				+ SSplitter::Slot().Value(0.26f) [ BuildBrowserPane() ]
				+ SSplitter::Slot().Value(0.46f) [ BuildInspectorPane() ]
				+ SSplitter::Slot().Value(0.28f) [ BuildSidePane() ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, PadM, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SAssignNew(mStatusText, STextBlock)
					.Text(FText::FromString(TEXT("Ready — edits apply live through the KDataForge pipeline")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FSlateColor(TextDim))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const UKDFEditorModel* CurrentModel = mModel.Get();
						const int32 Edits = CurrentModel != nullptr ? CurrentModel->GetSessionEditCount() : 0;
						return FText::FromString(FString::Printf(TEXT("%d edit(s) this session · Esc closes"),
																 Edits));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FSlateColor(TextDim))
				]
			]
		]
	];
	// clang-format on
	RefreshAll();
}

TSharedRef<SWidget> SKDFEditorWindow::BuildToolbar()
{
	UKDFEditorModel* Model = mModel.Get();

	// clang-format off
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, PadL, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("KDATAFORGE")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			.ColorAndOpacity(FSlateColor(Orange))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, PadM, 0)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(Model != nullptr ? &Model->GetCategories() : nullptr)
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
			{
				return SNew(STextBlock).Text(FText::FromString(*Option));
			})
			.OnSelectionChanged(this, &SKDFEditorWindow::OnCategoryChanged)
			[
				SNew(STextBlock).Text_Lambda([this]()
				{
					const UKDFEditorModel* CurrentModel = mModel.Get();
					return FText::FromString(CurrentModel != nullptr ? CurrentModel->GetActiveCategory()
																	 : FString());
				})
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, PadM, 0)
		[
			SAssignNew(mModCombo, SComboBox<TSharedPtr<FString>>)
			.OptionsSource(Model != nullptr ? &Model->GetModOptions() : nullptr)
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
			{
				return SNew(STextBlock).Text(FText::FromString(*Option));
			})
			.OnSelectionChanged(this, &SKDFEditorWindow::OnModFilterChanged)
			.ToolTipText(FText::FromString(TEXT("Filter by mod / content path — includes the base game (/Game)")))
			[
				SNew(STextBlock).Text_Lambda([this]()
				{
					const UKDFEditorModel* CurrentModel = mModel.Get();
					return FText::FromString(CurrentModel != nullptr ? CurrentModel->GetActiveModFilter()
																	 : FString());
				})
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 0, PadM, 0)
		[
			SNew(SSearchBox)
			.HintText(FText::FromString(TEXT("Search name or path…")))
			.OnTextChanged(this, &SKDFEditorWindow::OnSearchChanged)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, PadS, 0)
		[
			MakeButton(TEXT("Undo"), FOnClicked::CreateSP(this, &SKDFEditorWindow::OnUndo))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, PadS, 0)
		[
			MakeButton(TEXT("Redo"), FOnClicked::CreateSP(this, &SKDFEditorWindow::OnRedo))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, PadS, 0)
		[
			MakeButton(TEXT("Export Diff"),
					   FOnClicked::CreateLambda([this]() { return OnExport(true); }))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, PadS, 0)
		[
			MakeButton(TEXT("Export Full"),
					   FOnClicked::CreateLambda([this]() { return OnExport(false); }))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, PadL, 0)
		[
			MakeButton(TEXT("Reload"), FOnClicked::CreateSP(this, &SKDFEditorWindow::OnReload))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			MakeButton(TEXT("✕"), FOnClicked::CreateSP(this, &SKDFEditorWindow::OnClose))
		];
	// clang-format on
}

TSharedRef<SWidget> SKDFEditorWindow::BuildBrowserPane()
{
	UKDFEditorModel* Model = mModel.Get();

	// clang-format off
	return SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FSlateColor(PanelMid))
		.Padding(FMargin(PadM))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, PadS) [ MakeHeaderText(TEXT("BROWSER")) ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(mBrowserList, SListView<FKDFBrowserItemPtr>)
				.ListItemsSource(Model != nullptr ? &Model->GetFilteredItems() : nullptr)
				.OnGenerateRow(this, &SKDFEditorWindow::GenerateBrowserRow)
				.OnSelectionChanged(this, &SKDFEditorWindow::OnBrowserSelectionChanged)
				.SelectionMode(ESelectionMode::Single)
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SKDFEditorWindow::BuildInspectorPane()
{
	UKDFEditorModel* Model = mModel.Get();

	// clang-format off
	return SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FSlateColor(PanelMid))
		.Padding(FMargin(PadM))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					MakeHeaderText(TEXT("INSPECTOR"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, PadS, 0)
				[
					MakeRowButton(TEXT("Expand All"), ButtonSecondary, FOnClicked::CreateLambda([this]()
					{
						SetAllExpansion(true);
						return FReply::Handled();
					}))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeRowButton(TEXT("Collapse All"), ButtonSecondary, FOnClicked::CreateLambda([this]()
					{
						SetAllExpansion(false);
						return FReply::Handled();
					}))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, PadS, 0, 0)
			[
				SAssignNew(mSelectionTitle, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
				.ColorAndOpacity(FSlateColor(TextBright))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, PadM)
			[
				SAssignNew(mSelectionSubtitle, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(TextDim))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(mPropertyTree, STreeView<FKDFPropertyRowPtr>)
				.TreeItemsSource(Model != nullptr ? &Model->GetRootRows() : nullptr)
				.OnGenerateRow(this, &SKDFEditorWindow::GeneratePropertyRow)
				.OnGetChildren(this, &SKDFEditorWindow::OnGetRowChildren)
				.OnSetExpansionRecursive(this, &SKDFEditorWindow::SetRowExpansionRecursive)
				.SelectionMode(ESelectionMode::None)
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SKDFEditorWindow::BuildSidePane()
{
	UKDFEditorModel* Model = mModel.Get();

	// clang-format off
	return SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FSlateColor(PanelMid))
		.Padding(FMargin(PadM))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, PadS, 0)
				[
					MakeTabButton(TEXT("YAML"), 0)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, PadS, 0)
				[
					MakeTabButton(TEXT("Diff"), 1)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeTabButton(TEXT("Refs"), 2)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f) [ SNullWidget::NullWidget ]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SCheckBox)
					.Visibility_Lambda([this]() { return mSideTab == 0 ? EVisibility::Visible
																	   : EVisibility::Collapsed; })
					.IsChecked_Lambda([this]()
					{
						return bPreviewDiffOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						bPreviewDiffOnly = State == ECheckBoxState::Checked;
						RefreshYamlPreview();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("changes only")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FSlateColor(TextDim))
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, PadS)
			[
				SAssignNew(mYamlPreview, SMultiLineEditableTextBox)
				.IsReadOnly(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, PadM, 0, PadS) [ MakeHeaderText(TEXT("CREATE")) ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, PadS, 0)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(Model != nullptr ? &Model->GetCreatableKinds() : nullptr)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
					{
						return SNew(STextBlock).Text(FText::FromString(*Option));
					})
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewKind, ESelectInfo::Type)
					{
						mCreateKind = NewKind;
					})
					[
						SNew(STextBlock).Text_Lambda([this]()
						{
							return FText::FromString(mCreateKind.IsValid() ? *mCreateKind : TEXT("kind"));
						})
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, PadS, 0)
				[
					SAssignNew(mCreateIdBox, SEditableTextBox)
					.HintText(FText::FromString(TEXT("Id (e.g. MySuperIngot)")))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, PadS, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, PadS, 0)
				[
					SAssignNew(mCreateParentBox, SEditableTextBox)
					.HintText_Lambda([this]()
					{
						const FString Kind = mCreateKind.IsValid() ? *mCreateKind : FString();
						if (Kind == TEXT("dataasset"))
						{
							return FText::FromString(TEXT("class path (required)"));
						}
						if (Kind == TEXT("class") || Kind == TEXT("unlock"))
						{
							return FText::FromString(TEXT("parent class path (required)"));
						}
						return FText::FromString(TEXT("parent class path (optional — kind default)"));
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, PadS, 0)
				[
					SAssignNew(mCreateParentPicker, SComboButton)
					.OnGetMenuContent(this, &SKDFEditorWindow::MakeCreateParentMenu)
					.ButtonContent()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Pick…")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FSlateColor(ButtonText))
				]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeButton(TEXT("Create"), FOnClicked::CreateSP(this, &SKDFEditorWindow::OnCreate))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, PadS, 0, 0)
			[
				// Only `type: class` drafts can register into SML's content registry.
				SNew(SHorizontalBox)
				.Visibility_Lambda([this]()
				{
					return mCreateKind.IsValid() && *mCreateKind == TEXT("class") ? EVisibility::Visible
																				  : EVisibility::Collapsed;
				})
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, PadS, 0)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("register as")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FSlateColor(TextDim))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(Model != nullptr ? &Model->GetRegisterAsOptions() : nullptr)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
					{
						return SNew(STextBlock).Text(FText::FromString(*Option));
					})
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewOption, ESelectInfo::Type)
					{
						mCreateRegisterAs = NewOption;
					})
					[
						SNew(STextBlock).Text_Lambda([this]()
						{
							return FText::FromString(mCreateRegisterAs.IsValid() ? *mCreateRegisterAs
																				 : TEXT("(none)"));
						})
					]
				]
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SKDFEditorWindow::MakeCreateParentMenu()
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model == nullptr || !mCreateKind.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	const TSharedRef<TArray<FKDFPickCandidatePtr>> Filtered =
		MakeShared<TArray<FKDFPickCandidatePtr>>(Model->GetCreateParentCandidates(*mCreateKind));
	const TSharedRef<TArray<FKDFPickCandidatePtr>> All = MakeShared<TArray<FKDFPickCandidatePtr>>(*Filtered);

	// Same capture rule as MakeRefPickerMenu: build the list first, capture BY VALUE (the menu
	// outlives this function; a captured local reference would dangle).
	// clang-format off
	TSharedRef<SListView<FKDFPickCandidatePtr>> ListView =
		SNew(SListView<FKDFPickCandidatePtr>)
		.ListItemsSource(&Filtered.Get())
		.OnGenerateRow_Lambda([](FKDFPickCandidatePtr Candidate, const TSharedRef<STableViewBase>& Owner)
		{
			return SNew(STableRow<FKDFPickCandidatePtr>, Owner)
				.Padding(FMargin(PadS, 2.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Candidate->mDisplayName))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FSlateColor(TextBright))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Candidate->mPath))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
						.ColorAndOpacity(FSlateColor(TextDim))
					]
				];
		})
		.OnSelectionChanged_Lambda([this](FKDFPickCandidatePtr Candidate, ESelectInfo::Type SelectInfo)
		{
			if (Candidate.IsValid() && SelectInfo != ESelectInfo::Direct)
			{
				if (mCreateParentBox.IsValid())
				{
					mCreateParentBox->SetText(FText::FromString(Candidate->mPath));
				}
				if (mCreateParentPicker.IsValid())
				{
					mCreateParentPicker->SetIsOpen(false);
				}
			}
		})
		.SelectionMode(ESelectionMode::Single);

	return SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FSlateColor(PanelDark))
		.Padding(FMargin(PadM))
		[
			SNew(SBox).WidthOverride(460.f).HeightOverride(420.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, PadS)
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(FString::Printf(TEXT("Search %d classes…"), All->Num())))
					.OnTextChanged_Lambda([All, Filtered, ListView](const FText& NewText)
					{
						const FString Needle = NewText.ToString();
						Filtered->Reset();
						for (const FKDFPickCandidatePtr& Candidate : *All)
						{
							if (Needle.IsEmpty() || Candidate->mDisplayName.Contains(Needle) ||
								Candidate->mPath.Contains(Needle))
							{
								Filtered->Add(Candidate);
							}
						}
						ListView->RequestListRefresh();
					})
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					ListView
				]
			]
		];
	// clang-format on
}

TSharedRef<ITableRow> SKDFEditorWindow::GenerateBrowserRow(FKDFBrowserItemPtr Item,
														   const TSharedRef<STableViewBase>& Owner)
{
	// clang-format off
	return SNew(STableRow<FKDFBrowserItemPtr>, Owner)
		.Padding(FMargin(PadS, 3.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->mDisplayName))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.ColorAndOpacity(FSlateColor(TextBright))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->mSubText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(TextDim))
			]
		];
	// clang-format on
}

void SKDFEditorWindow::OnGetRowChildren(FKDFPropertyRowPtr Row, TArray<FKDFPropertyRowPtr>& OutChildren)
{
	OutChildren = Row->mChildren;
}

TSharedRef<SWidget> SKDFEditorWindow::MakeRefPickerMenu(FKDFPropertyRowPtr Row)
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model == nullptr)
	{
		return SNullWidget::NullWidget;
	}

	// Local copy the search filters; the source stays on the row (built once, lazily).
	const TSharedRef<TArray<FKDFPickCandidatePtr>> Filtered =
		MakeShared<TArray<FKDFPickCandidatePtr>>(Model->GetPickCandidates(Row));
	const TSharedRef<TArray<FKDFPickCandidatePtr>> All = MakeShared<TArray<FKDFPickCandidatePtr>>(*Filtered);

	// Built before the surrounding tree so the search lambda can capture the pointer BY VALUE
	// (the menu outlives this function; a captured local reference would dangle).
	// clang-format off
	TSharedRef<SListView<FKDFPickCandidatePtr>> ListView =
		SNew(SListView<FKDFPickCandidatePtr>)
		.ListItemsSource(&Filtered.Get())
		.OnGenerateRow_Lambda([](FKDFPickCandidatePtr Candidate, const TSharedRef<STableViewBase>& Owner)
		{
			return SNew(STableRow<FKDFPickCandidatePtr>, Owner)
				.Padding(FMargin(PadS, 2.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Candidate->mDisplayName))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FSlateColor(TextBright))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Candidate->mPath))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
						.ColorAndOpacity(FSlateColor(TextDim))
					]
				];
		})
		.OnSelectionChanged_Lambda([this, Row](FKDFPickCandidatePtr Candidate, ESelectInfo::Type SelectInfo)
		{
			if (Candidate.IsValid() && SelectInfo != ESelectInfo::Direct)
			{
				FSlateApplication::Get().DismissAllMenus();
				CommitRow(Row, Candidate->mPath);
			}
		})
		.SelectionMode(ESelectionMode::Single);

	TSharedRef<SWidget> Menu =
		SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FSlateColor(PanelDark))
		.Padding(FMargin(PadM))
		[
			SNew(SBox).WidthOverride(460.f).HeightOverride(420.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, PadS)
				[
					SNew(SSearchBox)
					.HintText(FText::FromString(FString::Printf(TEXT("Search %d candidates…"), All->Num())))
					.OnTextChanged_Lambda([All, Filtered, ListView](const FText& NewText)
					{
						const FString Needle = NewText.ToString();
						Filtered->Reset();
						for (const FKDFPickCandidatePtr& Candidate : *All)
						{
							if (Needle.IsEmpty() || Candidate->mDisplayName.Contains(Needle) ||
								Candidate->mPath.Contains(Needle))
							{
								Filtered->Add(Candidate);
							}
						}
						ListView->RequestListRefresh();
					})
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					ListView
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, PadS, 0, 0)
				[
					SNew(SEditableTextBox)
					.HintText(FText::FromString(TEXT("…or raw path (Enter to apply)")))
					.Text(FText::FromString(Row->mValueText))
					.OnTextCommitted_Lambda([this, Row](const FText& NewText, ETextCommit::Type CommitType)
					{
						if (CommitType == ETextCommit::OnEnter)
						{
							FSlateApplication::Get().DismissAllMenus();
							CommitRow(Row, NewText.ToString());
						}
					})
				]
			]
		];
	// clang-format on
	return Menu;
}
TSharedRef<SWidget> SKDFEditorWindow::MakeReferencePicker(const FKDFPropertyRowPtr& Row)
{
	UTexture* PreviewTexture = nullptr;
	if (Row->mKind == EKDFRowKind::ObjectRef && !Row->mValueText.IsEmpty() && Row->mValueText != TEXT("None"))
	{
		PreviewTexture = Cast<UTexture>(StaticLoadObject(UTexture::StaticClass(), nullptr, *Row->mValueText));
	}

	TSharedPtr<FSlateBrush> PreviewBrush;
	if (PreviewTexture != nullptr)
	{
		PreviewBrush = MakeShared<FSlateBrush>();
		PreviewBrush->SetResourceObject(PreviewTexture);
		PreviewBrush->ImageSize = FVector2D(96.f, 96.f);
	}

	// clang-format off
	TSharedRef<SWidget> Picker = SNew(SComboButton)
		.OnGetMenuContent_Lambda([this, Row]() { return MakeRefPickerMenu(Row); })
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text_Lambda([Row]() { return FText::FromString(Row->mSummary.IsEmpty() ? Row->mValueText
																					: Row->mSummary); })
			.ToolTipText_Lambda([Row]() { return FText::FromString(Row->mValueText); })
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FSlateColor(TextBright))
		];
	// clang-format on
	if (PreviewBrush.IsValid())
	{
		// Capture the brush in the image attribute so its resource remains alive with the widget.
		return SNew(SVerticalBox) +
			SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				.Padding(0, 0, 0, PadS)[SNew(SBox).WidthOverride(96.f).HeightOverride(
					96.f)[SNew(SScaleBox)
							  .Stretch(EStretch::ScaleToFit)
							  .StretchDirection(EStretchDirection::DownOnly)[SNew(SImage).Image_Lambda(
								  [PreviewBrush]() { return PreviewBrush.Get(); })]]] +
			SVerticalBox::Slot().AutoHeight()[Picker];
	}
	return Picker;
}

void SKDFEditorWindow::OpenColorPicker(FKDFPropertyRowPtr Row)
{
	FColorPickerArgs PickerArgs;
	PickerArgs.bUseAlpha = true;
	PickerArgs.bOnlyRefreshOnOk = true; // live edits go through the op pipeline once, on OK
	PickerArgs.InitialColor = Row->mColorValue;
	PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateLambda(
		[this, Row](FLinearColor NewColor)
		{
			Row->mColorValue = NewColor;
			// Commit in the property's own text form: FColor properties export as (R=…,G=…,B=…,A=…)
			// with byte values, FLinearColor with floats — round-trip via the type name.
			FString ValueText;
			if (Row->mTypeName.Contains(TEXT("FColor")))
			{
				const FColor AsColor = NewColor.ToFColor(true);
				ValueText = FString::Printf(TEXT("(R=%d,G=%d,B=%d,A=%d)"), AsColor.R, AsColor.G, AsColor.B, AsColor.A);
			}
			else
			{
				ValueText =
					FString::Printf(TEXT("(R=%f,G=%f,B=%f,A=%f)"), NewColor.R, NewColor.G, NewColor.B, NewColor.A);
			}
			CommitRow(Row, ValueText);
		});
	::OpenColorPicker(PickerArgs); // global from SColorPicker.h — the member overload would shadow it
}

TSharedRef<SWidget> SKDFEditorWindow::MakeColorEditor(const FKDFPropertyRowPtr& Row)
{
	// clang-format off
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, PadS, 0)
		[
			SNew(SBox).WidthOverride(44.f).HeightOverride(18.f)
			[
				SNew(SColorBlock)
				.Color_Lambda([Row]() { return Row->mColorValue; })
				.ShowBackgroundForAlpha(true)
				.OnMouseButtonDown_Lambda([this, Row](const FGeometry&, const FPointerEvent&)
				{
					OpenColorPicker(Row);
					return FReply::Handled();
				})
				.ToolTipText(FText::FromString(TEXT("Click to open the color picker")))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(SEditableTextBox)
			.Text_Lambda([Row]() { return FText::FromString(Row->mValueText); })
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.OnTextCommitted_Lambda([this, Row](const FText& NewText, ETextCommit::Type CommitType)
			{
				if (CommitType == ETextCommit::OnEnter && NewText.ToString() != Row->mValueText)
				{
					CommitRow(Row, NewText.ToString());
				}
			})
		];
	// clang-format on
}

TSharedRef<SWidget> SKDFEditorWindow::MakeTextEditor(const FKDFPropertyRowPtr& Row)
{
	if (Row->mTextKey.IsEmpty())
	{
		// Plain FText starts without a translation key. Toggling translation immediately converts it
		// into a keyed FText so the mode survives the next property-tree refresh.
		return SNew(SVerticalBox) +
			SVerticalBox::Slot().AutoHeight()[SNew(SCheckBox).OnCheckStateChanged_Lambda(
				[this, Row](ECheckBoxState State)
				{
					if (State != ECheckBoxState::Checked)
					{
						return;
					}
					UKDFEditorModel* Model = mModel.Get();
					FString Message;
					const bool bOk = Model != nullptr &&
						Model->CommitLocalizedText(Row, TEXT("KDataForgeEditor"), Row->mPath, Row->mTextSource,
												   Message);
					SetStatus(Message, !bOk);
					if (bOk)
					{
						RefreshAll();
					}
				})[SNew(STextBlock).Text(FText::FromString(TEXT("Use Translation")))]] +
			SVerticalBox::Slot().FillHeight(1.f).Padding(
				0, 2.f, 0, 0)[SNew(SMultiLineEditableTextBox)
								  .Text(FText::FromString(Row->mTextSource))
								  .HintText(FText::FromString(TEXT("Text")))
								  .AutoWrapText(true)
								  .OnTextCommitted_Lambda(
									  [this, Row](const FText& NewText, ETextCommit::Type CommitType)
									  {
										  if ((CommitType == ETextCommit::OnEnter ||
											   CommitType == ETextCommit::OnUserMovedFocus) &&
											  NewText.ToString() != Row->mTextSource)
										  {
											  CommitRow(Row, NewText.ToString());
										  }
									  })];
	}

	TSharedPtr<SEditableTextBox> NamespaceBox;
	TSharedPtr<SEditableTextBox> KeyBox;
	TSharedPtr<SMultiLineEditableTextBox> SourceBox;
	// clang-format off
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Translation string — edit key/source, then Apply")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(TextDim))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SCheckBox)
				.IsChecked(ECheckBoxState::Checked)
				.OnCheckStateChanged_Lambda([this, Row](ECheckBoxState State)
				{
					if (State != ECheckBoxState::Unchecked)
					{
						return;
					}
					UKDFEditorModel* Model = mModel.Get();
					FString Message;
					const bool bOk = Model != nullptr && Model->CommitRowValue(Row, Row->mTextSource, Message);
					SetStatus(Message, !bOk);
					if (bOk)
					{
						RefreshAll();
					}
				})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Use Translation")))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2.f, 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(0, 0, PadS, 0)
			[
				SAssignNew(NamespaceBox, SEditableTextBox)
				.Text(FText::FromString(Row->mTextNamespace))
				.HintText(FText::FromString(TEXT("Namespace")))
			]
			+ SHorizontalBox::Slot().FillWidth(0.3f).Padding(0, 0, PadS, 0)
			[
				SAssignNew(KeyBox, SEditableTextBox)
				.Text(FText::FromString(Row->mTextKey))
				.HintText(FText::FromString(TEXT("Translation key")))
			]
			+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(0, 0, PadS, 0)
			[
				SAssignNew(SourceBox, SMultiLineEditableTextBox)
				.Text(FText::FromString(Row->mTextSource))
				.HintText(FText::FromString(TEXT("Source / default text")))
				.AutoWrapText(true)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeRowButton(TEXT("Apply"), ButtonPrimary, FOnClicked::CreateLambda(
					[this, Row, NamespaceBox, KeyBox, SourceBox]()
					{
						UKDFEditorModel* Model = mModel.Get();
						FString Message;
						const bool bOk = Model != nullptr && Model->CommitLocalizedText(
							Row, NamespaceBox->GetText().ToString(), KeyBox->GetText().ToString(),
							SourceBox->GetText().ToString(), Message);
						SetStatus(Message, !bOk);
						if (bOk)
						{
							RefreshAll();
						}
						return FReply::Handled();
					}))
			]
		];
	// clang-format on
}

TSharedRef<SWidget> SKDFEditorWindow::MakeValueEditor(const FKDFPropertyRowPtr& Row)
{
	switch (Row->mKind)
	{
	case EKDFRowKind::Bool:
		// clang-format off
			return SNew(SCheckBox)
				.IsChecked_Lambda([Row]()
				{
					return Row->mValueText.Equals(TEXT("true"), ESearchCase::IgnoreCase)
							   ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this, Row](ECheckBoxState State)
				{
					CommitRow(Row, State == ECheckBoxState::Checked ? TEXT("True") : TEXT("False"));
				});
	// clang-format on
	case EKDFRowKind::Enum:
		// clang-format off
			return SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&Row->mEnumOptions)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
				{
					return SNew(STextBlock).Text(FText::FromString(*Option));
				})
				.OnSelectionChanged_Lambda([this, Row](TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
				{
					if (NewValue.IsValid() && SelectInfo != ESelectInfo::Direct)
					{
						CommitRow(Row, *NewValue);
					}
				})
				[
					SNew(STextBlock).Text_Lambda([Row]() { return FText::FromString(Row->mValueText); })
				];
	// clang-format on
	case EKDFRowKind::Color:
		return MakeColorEditor(Row);
	case EKDFRowKind::Text:
		return MakeTextEditor(Row);
	case EKDFRowKind::ClassRef:
	case EKDFRowKind::ObjectRef:
		return MakeReferencePicker(Row);
	case EKDFRowKind::Struct:
	case EKDFRowKind::Set:
	case EKDFRowKind::Map:
		// Containers/structs: summary label; children carry the editors.
		// clang-format off
			return SNew(STextBlock)
				.Text_Lambda([Row]() { return FText::FromString(Row->mSummary); })
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
				.ColorAndOpacity(FSlateColor(TextDim));
	// clang-format on
	case EKDFRowKind::Array:
		// clang-format off
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([Row]() { return FText::FromString(Row->mSummary); })
					.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
					.ColorAndOpacity(FSlateColor(TextDim))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeRowButton(TEXT("+"), ButtonSuccess, FOnClicked::CreateLambda([this, Row]()
					{
						UKDFEditorModel* Model = mModel.Get();
						FString Message;
						if (Model != nullptr)
						{
							const bool bOk = Model->ArrayAddElement(Row, Message);
							SetStatus(Message, !bOk);
							const FString ArrayPath = Row->mPath;
							RefreshAll();
							if (bOk)
							{
								// Show the freshly added element even if the array was collapsed.
								ExpandRowByPath(ArrayPath);
							}
						}
						return FReply::Handled();
					}))
				];
	// clang-format on
	default:
		// Numeric, text, and complex values commit through canonical text.
		// clang-format off
			return SNew(SEditableTextBox)
				.Text_Lambda([Row]() { return FText::FromString(Row->mValueText); })
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.OnTextCommitted_Lambda([this, Row](const FText& NewText, ETextCommit::Type CommitType)
				{
					if (CommitType == ETextCommit::OnEnter && NewText.ToString() != Row->mValueText)
					{
						CommitRow(Row, NewText.ToString());
					}
				});
		// clang-format on
	}
}

TSharedRef<ITableRow> SKDFEditorWindow::GeneratePropertyRow(FKDFPropertyRowPtr Row,
															const TSharedRef<STableViewBase>& Owner)
{
	if (Row->bIsCategory)
	{
		// Category roots intentionally start collapsed; STreeView owns their expander state.
		return SNew(STableRow<FKDFPropertyRowPtr>, Owner)
			.Padding(FMargin(0.f, 2.f))
				[SNew(SBorder)
					 .BorderImage(WhiteBrush())
					 .BorderBackgroundColor(FSlateColor(PanelMid))
					 .Padding(FMargin(PadM, PadS))
						 [SNew(SHorizontalBox) +
						  SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock)
																	.Text(FText::FromString(Row->mName))
																	.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
																	.ColorAndOpacity(FSlateColor(Orange))] +
						  SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock)
																 .Text(FText::FromString(FString::Printf(
																	 TEXT("%d field(s)"), Row->mChildren.Num())))
																 .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
																 .ColorAndOpacity(FSlateColor(TextDim))]]];
	}

	// Row actions: expandable rows get a recursive-expand button, array elements a remove button.
	const TSharedRef<SHorizontalBox> RowAction = SNew(SHorizontalBox);
	if (Row->mChildren.Num() > 0)
	{
		TSharedRef<SButton> ExpandButton = MakeRowButton(TEXT("⇊"), ButtonSecondary,
														 FOnClicked::CreateLambda(
															 [this, Row]()
															 {
																 SetRowExpansionRecursive(Row, true);
																 return FReply::Handled();
															 }));
		ExpandButton->SetToolTipText(
			FText::FromString(TEXT("Expand this and everything inside it (Shift+click the arrow does the same)")));
		RowAction->AddSlot().AutoWidth()[ExpandButton];
	}
	if (Row->bIsArrayElement)
	{
		RowAction->AddSlot().AutoWidth().Padding(
			Row->mChildren.Num() > 0 ? PadS : 0.f, 0.f, 0.f,
			0.f)[MakeRowButton(TEXT("×"), ButtonDanger,
							   FOnClicked::CreateLambda(
								   [this, Row]()
								   {
									   UKDFEditorModel* Model = mModel.Get();
									   FString Message;
									   if (Model != nullptr)
									   {
										   const bool bOk = Model->ArrayRemoveElement(Row, Message);
										   SetStatus(Message, !bOk);
										   RefreshAll();
									   }
									   return FReply::Handled();
								   }))];
	}

	const FLinearColor RowTint = Row->bModified
		? ModifiedTint
		: (Row->mDepth % 2 == 1 ? FLinearColor(1.f, 1.f, 1.f, 0.02f) : FLinearColor::Transparent);

	// clang-format off
	return SNew(STableRow<FKDFPropertyRowPtr>, Owner)
		.Padding(FMargin(0.f, 1.f))
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FSlateColor(RowTint))
			.Padding(FMargin(PadS, 3.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.38f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Row->mName))
						.ToolTipText(FText::FromString(Row->mPath))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(FSlateColor(TextBright))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Row->mTypeName))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
						.ColorAndOpacity(FSlateColor(TextDim))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.62f).VAlign(VAlign_Center)
				[
					MakeValueEditor(Row)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(PadS, 0, 0, 0)
				[
					RowAction
				]
			]
		];
	// clang-format on
}

void SKDFEditorWindow::OnCategoryChanged(TSharedPtr<FString> NewCategory, ESelectInfo::Type SelectInfo)
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model != nullptr && NewCategory.IsValid())
	{
		Model->SetCategory(*NewCategory);
		RefreshAll();
	}
}

void SKDFEditorWindow::OnModFilterChanged(TSharedPtr<FString> NewMod, ESelectInfo::Type SelectInfo)
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model != nullptr && NewMod.IsValid())
	{
		Model->SetModFilter(*NewMod);
		if (mBrowserList.IsValid())
		{
			mBrowserList->RequestListRefresh();
		}
	}
}

void SKDFEditorWindow::OnSearchChanged(const FText& NewText)
{
	if (UKDFEditorModel* Model = mModel.Get())
	{
		Model->SetSearchText(NewText.ToString());
		if (mBrowserList.IsValid())
		{
			mBrowserList->RequestListRefresh();
		}
	}
}

void SKDFEditorWindow::OnBrowserSelectionChanged(FKDFBrowserItemPtr Item, ESelectInfo::Type SelectInfo)
{
	if (UKDFEditorModel* Model = mModel.Get())
	{
		Model->Select(Item);
		const FString SelectedPath =
			Item.IsValid() && Item->mObject.IsValid() ? Item->mObject->GetPathName() : FString();
		if (SelectedPath != mExpansionObjectPath)
		{
			mExpansionObjectPath = SelectedPath;
			bPropertyExpansionInitialized = false;
		}
		RefreshPropertyTreePreservingExpansion();
		if (mSelectionTitle.IsValid())
		{
			mSelectionTitle->SetText(FText::FromString(Model->GetSelectedTitle()));
		}
		if (mSelectionSubtitle.IsValid())
		{
			mSelectionSubtitle->SetText(FText::FromString(Model->GetSelectedSubtitle()));
		}
		RefreshYamlPreview();
	}
}

void SKDFEditorWindow::CommitRow(const FKDFPropertyRowPtr& Row, const FString& NewValue)
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model == nullptr)
	{
		return;
	}
	FString Message;
	const bool bOk = Model->CommitRowValue(Row, NewValue, Message);
	SetStatus(Message, !bOk);
	if (bOk)
	{
		// Refresh the whole tree — the committed value may reshape children (refs, containers).
		RefreshPropertyTreePreservingExpansion();
		RefreshYamlPreview();
	}
}

void SKDFEditorWindow::RefreshPropertyTreePreservingExpansion()
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model == nullptr)
	{
		return;
	}

	// Rows are rebuilt from scratch on every refresh, so expansion state must survive by PATH —
	// the old shared pointers are gone afterwards.
	TSet<FString> ExpandedPaths;
	if (mPropertyTree.IsValid())
	{
		TSet<FKDFPropertyRowPtr> ExpandedRows;
		mPropertyTree->GetExpandedItems(ExpandedRows);
		for (const FKDFPropertyRowPtr& Row : ExpandedRows)
		{
			if (Row.IsValid())
			{
				ExpandedPaths.Add(Row->mPath);
			}
		}
	}

	Model->RefreshPropertyRows();

	if (mPropertyTree.IsValid())
	{
		mPropertyTree->RequestTreeRefresh();
		if (!bPropertyExpansionInitialized && Model->GetRootRows().Num() > 0)
		{
			TFunction<void(const TArray<FKDFPropertyRowPtr>&, bool)> ExpandCategories =
				[this, &ExpandCategories](const TArray<FKDFPropertyRowPtr>& Rows, bool bParentExcluded)
			{
				for (const FKDFPropertyRowPtr& Row : Rows)
				{
					const bool bExcluded = bParentExcluded || Row->mName == TEXT("Non-UPROPERTY / Internal");
					if (Row->bIsCategory && !bExcluded)
					{
						mPropertyTree->SetItemExpansion(Row, true);
					}
					ExpandCategories(Row->mChildren, bExcluded);
				}
			};
			ExpandCategories(Model->GetRootRows(), false);
			bPropertyExpansionInitialized = true;
		}
		else if (!ExpandedPaths.IsEmpty())
		{
			TFunction<void(const TArray<FKDFPropertyRowPtr>&)> ReExpand =
				[this, &ExpandedPaths, &ReExpand](const TArray<FKDFPropertyRowPtr>& Rows)
			{
				for (const FKDFPropertyRowPtr& Row : Rows)
				{
					if (ExpandedPaths.Contains(Row->mPath))
					{
						mPropertyTree->SetItemExpansion(Row, true);
					}
					ReExpand(Row->mChildren);
				}
			};
			ReExpand(Model->GetRootRows());
		}
	}
}

void SKDFEditorWindow::ExpandRowByPath(const FString& Path)
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model == nullptr || !mPropertyTree.IsValid())
	{
		return;
	}
	TFunction<bool(const TArray<FKDFPropertyRowPtr>&)> Expand =
		[this, &Path, &Expand](const TArray<FKDFPropertyRowPtr>& Rows) -> bool
	{
		for (const FKDFPropertyRowPtr& Row : Rows)
		{
			if (Row->mPath == Path)
			{
				mPropertyTree->SetItemExpansion(Row, true);
				return true;
			}
			if (Expand(Row->mChildren))
			{
				return true;
			}
		}
		return false;
	};
	Expand(Model->GetRootRows());
}

void SKDFEditorWindow::SetRowExpansionRecursive(FKDFPropertyRowPtr Row, bool bExpand)
{
	if (!mPropertyTree.IsValid() || !Row.IsValid())
	{
		return;
	}
	mPropertyTree->SetItemExpansion(Row, bExpand);
	for (const FKDFPropertyRowPtr& Child : Row->mChildren)
	{
		SetRowExpansionRecursive(Child, bExpand);
	}
}

void SKDFEditorWindow::SetAllExpansion(bool bExpand)
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model == nullptr || !mPropertyTree.IsValid())
	{
		return;
	}
	for (const FKDFPropertyRowPtr& Row : Model->GetRootRows())
	{
		SetRowExpansionRecursive(Row, bExpand);
	}
}

FReply SKDFEditorWindow::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return OnClose();
	}
	return SCompoundWidget::OnPreviewKeyDown(MyGeometry, InKeyEvent);
}

FReply SKDFEditorWindow::OnUndo()
{
	if (UKDFEditorModel* Model = mModel.Get())
	{
		FString Message;
		const bool bOk = Model->Undo(Message);
		SetStatus(Message, !bOk);
		RefreshAll();
	}
	return FReply::Handled();
}

FReply SKDFEditorWindow::OnRedo()
{
	if (UKDFEditorModel* Model = mModel.Get())
	{
		FString Message;
		const bool bOk = Model->Redo(Message);
		SetStatus(Message, !bOk);
		RefreshAll();
	}
	return FReply::Handled();
}

FReply SKDFEditorWindow::OnExport(bool bDiffOnly)
{
	if (UKDFEditorModel* Model = mModel.Get())
	{
		FString Message;
		const bool bOk = Model->ExportSelected(bDiffOnly, Message);
		SetStatus(Message, !bOk);
	}
	return FReply::Handled();
}

FReply SKDFEditorWindow::OnCreate()
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model != nullptr && mCreateKind.IsValid() && mCreateIdBox.IsValid())
	{
		FString Message;
		const bool bOk = Model->CreateContentDraft(
			*mCreateKind, mCreateIdBox->GetText().ToString(), mCreateParentBox->GetText().ToString(),
			mCreateRegisterAs.IsValid() ? *mCreateRegisterAs : FString(), Message);
		SetStatus(Message, !bOk);
	}
	return FReply::Handled();
}

FReply SKDFEditorWindow::OnReload()
{
	if (UKDFEditorModel* Model = mModel.Get())
	{
		FString ReportText;
		const int32 AppliedDocuments = Model->ReloadFromDisk(ReportText);
		RefreshAll();
		SetStatus(FString::Printf(TEXT("Reload done — %d document(s) applied"), AppliedDocuments), false);

		// A blunt but reliable "did anything go wrong" check — BuildReportString only ever appends
		// diagnostic lines starting with "[Warning]"/"[Error]" (Info is filtered out already).
		const bool bHasErrors = ReportText.Contains(TEXT("[Error]"));
		ShowResultDialog(TEXT("KDataForge Reload Result"), ReportText, bHasErrors);
	}
	return FReply::Handled();
}

FReply SKDFEditorWindow::OnClose()
{
	mOnCloseRequested.ExecuteIfBound();
	return FReply::Handled();
}

void SKDFEditorWindow::SetStatus(const FString& Message, bool bError)
{
	if (mStatusText.IsValid())
	{
		mStatusText->SetText(FText::FromString(Message));
		mStatusText->SetColorAndOpacity(FSlateColor(bError ? ErrorRed : OkGreen));
	}
}

void SKDFEditorWindow::ShowResultDialog(const FString& Title, const FString& ReportText, bool bHasErrors)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
									 .Title(FText::FromString(Title))
									 .ClientSize(FVector2D(560.f, 420.f))
									 .SizingRule(ESizingRule::UserSized)
									 .SupportsMinimize(false)
									 .SupportsMaximize(false);

	TWeakPtr<SWindow> WeakWindow = Window;

	// clang-format off
	Window->SetContent(
		SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FSlateColor(PanelDark))
		.Padding(FMargin(PadL))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, PadM)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Title))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
				.ColorAndOpacity(FSlateColor(bHasErrors ? ErrorRed : OkGreen))
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 0, 0, PadM)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush())
				.BorderBackgroundColor(FSlateColor(PanelMid))
				.Padding(FMargin(PadS))
				[
					SNew(SMultiLineEditableTextBox)
					.IsReadOnly(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
					.Text(FText::FromString(ReportText))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				MakeButton(TEXT("OK"), FOnClicked::CreateLambda([WeakWindow]()
				{
					if (TSharedPtr<SWindow> Pinned = WeakWindow.Pin())
					{
						Pinned->RequestDestroyWindow();
					}
					return FReply::Handled();
				}))
			]
		]
	);
	// clang-format on

	FSlateApplication::Get().AddModalWindow(Window, SharedThis(this));
}

TSharedRef<SWidget> SKDFEditorWindow::MakeTabButton(const FString& Label, int32 TabIndex)
{
	// clang-format off
	return SNew(SButton)
		.OnClicked_Lambda([this, TabIndex]()
		{
			mSideTab = TabIndex;
			RefreshYamlPreview();
			return FReply::Handled();
		})
		.ButtonColorAndOpacity_Lambda([this, TabIndex]()
		{
			return FSlateColor(mSideTab == TabIndex ? ButtonPrimary : ButtonSecondary);
		})
		.ContentPadding(FMargin(PadM, 2.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity_Lambda([this, TabIndex]()
			{
				return FSlateColor(ButtonText);
			})
		];
	// clang-format on
}

void SKDFEditorWindow::RefreshYamlPreview()
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model == nullptr || !mYamlPreview.IsValid())
	{
		return;
	}
	switch (mSideTab)
	{
	case 1:
		mYamlPreview->SetText(FText::FromString(Model->GetSelectedDiffText()));
		break;
	case 2:
		mYamlPreview->SetText(FText::FromString(Model->GetSelectedReferencesText()));
		break;
	default:
		mYamlPreview->SetText(FText::FromString(Model->GetSelectedYamlPreview(bPreviewDiffOnly)));
		break;
	}
}

void SKDFEditorWindow::RefreshAll()
{
	UKDFEditorModel* Model = mModel.Get();
	if (Model == nullptr)
	{
		return;
	}
	RefreshPropertyTreePreservingExpansion();
	if (mBrowserList.IsValid())
	{
		mBrowserList->RequestListRefresh();
	}
	if (mModCombo.IsValid())
	{
		mModCombo->RefreshOptions(); // mod options are rebuilt per-category — keep the dropdown in sync
	}
	if (mSelectionTitle.IsValid())
	{
		mSelectionTitle->SetText(FText::FromString(Model->GetSelectedTitle()));
	}
	if (mSelectionSubtitle.IsValid())
	{
		mSelectionSubtitle->SetText(FText::FromString(Model->GetSelectedSubtitle()));
	}
	RefreshYamlPreview();
}

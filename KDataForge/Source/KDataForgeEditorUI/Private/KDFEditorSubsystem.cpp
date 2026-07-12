#include "KDFEditorSubsystem.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "KDFEditorModel.h"
#include "SKDFEditorWindow.h"
#include "Subsystems/KDFSubsystem.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	FAutoConsoleCommand GKDFEditorToggleCommand(
		TEXT("KDF.Editor"), TEXT("Toggles the KDataForge in-game editor"),
		FConsoleCommandDelegate::CreateLambda(
			[]()
			{
				for (const FWorldContext& Context : GEngine->GetWorldContexts())
				{
					if (Context.WorldType != EWorldType::Game && Context.WorldType != EWorldType::PIE)
					{
						continue;
					}
					if (UGameInstance* GameInstance = Context.OwningGameInstance)
					{
						if (UKDFEditorSubsystem* Editor = GameInstance->GetSubsystem<UKDFEditorSubsystem>())
						{
							Editor->ToggleEditor();
							return;
						}
					}
				}
			}));
} // namespace

void UKDFEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency(UKDFSubsystem::StaticClass());
	if (UKDFSubsystem* KDF = GetGameInstance()->GetSubsystem<UKDFSubsystem>())
	{
		mToggleHandle = KDF->OnToggleEditorRequested().AddUObject(this, &UKDFEditorSubsystem::OnToggleRequested);
	}
}

void UKDFEditorSubsystem::Deinitialize()
{
	CloseEditor();
	if (UKDFSubsystem* KDF = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UKDFSubsystem>() : nullptr)
	{
		KDF->OnToggleEditorRequested().Remove(mToggleHandle);
	}
	Super::Deinitialize();
}

void UKDFEditorSubsystem::OnToggleRequested(APlayerController* /*Requester*/) { ToggleEditor(); }

bool UKDFEditorSubsystem::IsEditorOpen() const { return mWindow.IsValid(); }

APlayerController* UKDFEditorSubsystem::GetLocalPlayerController() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance != nullptr ? GameInstance->GetFirstLocalPlayerController() : nullptr;
}

void UKDFEditorSubsystem::ToggleEditor()
{
	if (IsEditorOpen())
	{
		CloseEditor();
	}
	else
	{
		OpenEditor();
	}
}

void UKDFEditorSubsystem::OpenEditor()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr || GameInstance->GetGameViewportClient() == nullptr)
	{
		return;
	}

	if (mModel == nullptr)
	{
		mModel = NewObject<UKDFEditorModel>(this);
	}
	mModel->Initialize(GameInstance->GetWorld());

	// clang-format off
	SAssignNew(mWindow, SKDFEditorWindow)
		.Model(mModel)
		.OnCloseRequested(FSimpleDelegate::CreateUObject(this, &UKDFEditorSubsystem::CloseEditor));
	// clang-format on

	mWindowContainer = SNew(SBox).Padding(FMargin(48.f))[mWindow.ToSharedRef()];
	GameInstance->GetGameViewportClient()->AddViewportWidgetContent(mWindowContainer.ToSharedRef(),
																	/*ZOrder*/ 1000);

	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(mWindow);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false); // clicking the viewport must not hide the cursor
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}

	// Satisfactory's own controller/UI stack re-hides the cursor on its next update — keep it
	// visible for as long as the editor window is open (cleared in CloseEditor).
	mCursorTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this,
										  [this](float)
										  {
											  if (APlayerController* PC = GetLocalPlayerController();
												  PC != nullptr && IsEditorOpen() && !PC->ShouldShowMouseCursor())
											  {
												  PC->SetShowMouseCursor(true);
											  }
											  return true;
										  }),
		0.1f);
}

void UKDFEditorSubsystem::CloseEditor()
{
	if (!mWindow.IsValid())
	{
		return;
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance != nullptr && GameInstance->GetGameViewportClient() != nullptr && mWindowContainer.IsValid())
	{
		GameInstance->GetGameViewportClient()->RemoveViewportWidgetContent(mWindowContainer.ToSharedRef());
	}
	mWindowContainer.Reset();
	mWindow.Reset();

	if (mCursorTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(mCursorTicker);
		mCursorTicker.Reset();
	}

	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);
	}
}

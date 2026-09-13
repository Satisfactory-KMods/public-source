// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "KDFEditorSubsystem.generated.h"

class APlayerController;
class SKDFEditorWindow;
class UKDFEditorModel;

UCLASS()
class KDATAFORGEEDITORUI_API UKDFEditorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "KDataForge|Editor")
	void ToggleEditor();

	UFUNCTION(BlueprintCallable, Category = "KDataForge|Editor")
	void CloseEditor();

	bool IsEditorOpen() const;

private:
	void OnToggleRequested(APlayerController* Requester);
	void OpenEditor();
	APlayerController* GetLocalPlayerController() const;

	UPROPERTY()
	TObjectPtr<UKDFEditorModel> mModel;

	TSharedPtr<SKDFEditorWindow> mWindow;
	TSharedPtr<SWidget> mWindowContainer;
	FDelegateHandle mToggleHandle;
	FTSTicker::FDelegateHandle mCursorTicker;
};

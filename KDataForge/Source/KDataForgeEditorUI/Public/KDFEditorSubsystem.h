#pragma once

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "KDFEditorSubsystem.generated.h"

class APlayerController;
class SKDFEditorWindow;
class UKDFEditorModel;

/**
 * Owns the in-game editor: model lifetime, Slate window creation, viewport attachment, and input
 * mode switching. Toggled via `/kdf editor` (through UKDFSubsystem::OnToggleEditorRequested) or
 * the `KDF.Editor` console command. Host-local UI — it edits through the same op pipeline as YAML.
 */
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
	TSharedPtr<SWidget> mWindowContainer; // the exact widget added to the viewport (surgical removal)
	FDelegateHandle mToggleHandle;
	FTSTicker::FDelegateHandle mCursorTicker; // keeps the cursor visible against FG's UI stack
};

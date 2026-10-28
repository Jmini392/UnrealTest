#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SKOptionsMenu.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class USlider;
class UTextBlock;
class UVerticalBox;

/**
 * Gear button in the top-right corner plus the options panel (sound, screen, save, load, quit).
 * The whole widget tree is built in code, so there is no UMG asset to maintain.
 * Toggled by the character's menu key (1), since Esc stops Play In Editor.
 */
UCLASS()
class SLEEPLESSKNIGHT_API USKOptionsMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Options")
	void ToggleMenu();

	UFUNCTION(BlueprintPure, Category = "Options")
	bool IsMenuOpen() const { return bOpen; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	enum class EPage : uint8 { Main, Sound, Screen };

	void BuildTree();
	void OpenMenu();
	void CloseMenu();
	void ShowPage(EPage Page);
	void SetStatus(const FString& Text);
	void RefreshScreenLabels();

	UButton* AddButton(UVerticalBox* Parent, const FString& Label);

	UFUNCTION() void OnGearClicked() { ToggleMenu(); }
	UFUNCTION() void OnSoundClicked() { ShowPage(EPage::Sound); }
	UFUNCTION() void OnScreenClicked() { ShowPage(EPage::Screen); }
	UFUNCTION() void OnBackClicked() { ShowPage(EPage::Main); }
	UFUNCTION() void OnSaveClicked();
	UFUNCTION() void OnLoadClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnVolumeChanged(float Value);
	UFUNCTION() void OnWindowModeClicked();
	UFUNCTION() void OnQualityClicked();

	UPROPERTY() TObjectPtr<UBorder> Panel;
	UPROPERTY() TObjectPtr<UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<UTextBlock> StatusText;
	UPROPERTY() TObjectPtr<UVerticalBox> PageMain;
	UPROPERTY() TObjectPtr<UVerticalBox> PageSound;
	UPROPERTY() TObjectPtr<UVerticalBox> PageScreen;
	UPROPERTY() TObjectPtr<USlider> VolumeSlider;
	UPROPERTY() TObjectPtr<UTextBlock> VolumeText;
	UPROPERTY() TObjectPtr<UTextBlock> WindowModeText;
	UPROPERTY() TObjectPtr<UTextBlock> QualityText;

	bool bOpen = false;
};

#include "UI/SKOptionsMenu.h"
#include "UI/SKGearIcon.h"
#include "Characters/SKCharacter.h"
#include "SKSaveGame.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(FText::FromString(Text));
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = Size;
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(Color));
		return Block;
	}

	const TCHAR* QualityName(int32 Level)
	{
		switch (Level)
		{
		case 0: return TEXT("낮음");
		case 1: return TEXT("보통");
		case 2: return TEXT("높음");
		case 3: return TEXT("최고");
		default: return TEXT("사용자 지정");
		}
	}
}

TSharedRef<SWidget> USKOptionsMenu::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

UButton* USKOptionsMenu::AddButton(UVerticalBox* Parent, const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->AddChild(MakeText(WidgetTree, Label, 20));
	UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
	ButtonSlot->SetPadding(FMargin(0.f, 5.f));
	ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	return Button;
}

void USKOptionsMenu::BuildTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Top-right corner: key hint and gear button.
	UHorizontalBox* Hud = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* HintSlot = Hud->AddChildToHorizontalBox(MakeText(WidgetTree, TEXT("[1]"), 16, FLinearColor(1.f, 1.f, 1.f, 0.8f)));
	HintSlot->SetVerticalAlignment(VAlign_Center);
	HintSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));

	USizeBox* GearBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	GearBox->SetWidthOverride(56.f);
	GearBox->SetHeightOverride(56.f);
	UButton* GearButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	GearButton->SetBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
	GearButton->AddChild(WidgetTree->ConstructWidget<USKGearIcon>(USKGearIcon::StaticClass()));
	GearButton->OnClicked.AddDynamic(this, &USKOptionsMenu::OnGearClicked);
	GearBox->AddChild(GearButton);
	Hud->AddChildToHorizontalBox(GearBox);

	UCanvasPanelSlot* HudSlot = Root->AddChildToCanvas(Hud);
	HudSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	HudSlot->SetAlignment(FVector2D(1.f, 0.f));
	HudSlot->SetPosition(FVector2D(-24.f, 24.f));
	HudSlot->SetAutoSize(true);

	// Options panel in the middle, hidden until opened.
	Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrush(FSlateColorBrush(FLinearColor(0.04f, 0.04f, 0.09f, 0.94f)));
	Panel->SetPadding(FMargin(36.f));
	Panel->SetVisibility(ESlateVisibility::Collapsed);

	USizeBox* PanelWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PanelWidth->SetWidthOverride(400.f);
	Panel->SetContent(PanelWidth);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	PanelWidth->AddChild(Column);

	TitleText = MakeText(WidgetTree, TEXT("옵션"), 30);
	UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TitleText);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));

	// Main page
	PageMain = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(PageMain);
	AddButton(PageMain, TEXT("사운드"))->OnClicked.AddDynamic(this, &USKOptionsMenu::OnSoundClicked);
	AddButton(PageMain, TEXT("화면"))->OnClicked.AddDynamic(this, &USKOptionsMenu::OnScreenClicked);
	AddButton(PageMain, TEXT("저장"))->OnClicked.AddDynamic(this, &USKOptionsMenu::OnSaveClicked);
	AddButton(PageMain, TEXT("불러오기"))->OnClicked.AddDynamic(this, &USKOptionsMenu::OnLoadClicked);
	AddButton(PageMain, TEXT("종료"))->OnClicked.AddDynamic(this, &USKOptionsMenu::OnQuitClicked);

	// Sound page
	PageSound = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(PageSound);
	VolumeText = MakeText(WidgetTree, TEXT(""), 20);
	PageSound->AddChildToVerticalBox(VolumeText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	VolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	VolumeSlider->SetMinValue(0.f);
	VolumeSlider->SetMaxValue(1.f);
	VolumeSlider->SetStepSize(0.05f);
	VolumeSlider->SetValue(FApp::GetVolumeMultiplier());
	VolumeSlider->OnValueChanged.AddDynamic(this, &USKOptionsMenu::OnVolumeChanged);
	PageSound->AddChildToVerticalBox(VolumeSlider)->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
	AddButton(PageSound, TEXT("뒤로"))->OnClicked.AddDynamic(this, &USKOptionsMenu::OnBackClicked);

	// Screen page
	PageScreen = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(PageScreen);
	UButton* WindowModeButton = AddButton(PageScreen, TEXT(""));
	WindowModeText = Cast<UTextBlock>(WindowModeButton->GetChildAt(0));
	WindowModeButton->OnClicked.AddDynamic(this, &USKOptionsMenu::OnWindowModeClicked);
	UButton* QualityButton = AddButton(PageScreen, TEXT(""));
	QualityText = Cast<UTextBlock>(QualityButton->GetChildAt(0));
	QualityButton->OnClicked.AddDynamic(this, &USKOptionsMenu::OnQualityClicked);
	AddButton(PageScreen, TEXT("뒤로"))->OnClicked.AddDynamic(this, &USKOptionsMenu::OnBackClicked);

	StatusText = MakeText(WidgetTree, TEXT(""), 16, FLinearColor(0.6f, 0.9f, 1.f));
	UVerticalBoxSlot* StatusSlot = Column->AddChildToVerticalBox(StatusText);
	StatusSlot->SetHorizontalAlignment(HAlign_Center);
	StatusSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetPosition(FVector2D::ZeroVector);
	PanelSlot->SetAutoSize(true);

	ShowPage(EPage::Main);
	OnVolumeChanged(VolumeSlider->GetValue());
	RefreshScreenLabels();
}

void USKOptionsMenu::ToggleMenu()
{
	if (bOpen)
	{
		CloseMenu();
	}
	else
	{
		OpenMenu();
	}
}

void USKOptionsMenu::OpenMenu()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	bOpen = true;
	Panel->SetVisibility(ESlateVisibility::Visible);
	ShowPage(EPage::Main);
	SetStatus(FString());
	VolumeSlider->SetValue(FApp::GetVolumeMultiplier());
	RefreshScreenLabels();

	PC->SetPause(true);
	PC->SetShowMouseCursor(true);
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
}

void USKOptionsMenu::CloseMenu()
{
	bOpen = false;
	Panel->SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void USKOptionsMenu::ShowPage(EPage Page)
{
	PageMain->SetVisibility(Page == EPage::Main ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PageSound->SetVisibility(Page == EPage::Sound ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PageScreen->SetVisibility(Page == EPage::Screen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	const TCHAR* Title = Page == EPage::Sound ? TEXT("사운드") : Page == EPage::Screen ? TEXT("화면") : TEXT("옵션");
	TitleText->SetText(FText::FromString(Title));
	SetStatus(FString());
}

void USKOptionsMenu::SetStatus(const FString& Text)
{
	StatusText->SetText(FText::FromString(Text));
}

void USKOptionsMenu::OnVolumeChanged(float Value)
{
	FApp::SetVolumeMultiplier(Value);
	VolumeText->SetText(FText::FromString(FString::Printf(TEXT("마스터 볼륨  %d%%"), FMath::RoundToInt(Value * 100.f))));
}

void USKOptionsMenu::RefreshScreenLabels()
{
	if (const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		const bool bWindowed = Settings->GetFullscreenMode() == EWindowMode::Windowed;
		WindowModeText->SetText(FText::FromString(FString::Printf(TEXT("화면 모드: %s"), bWindowed ? TEXT("창 모드") : TEXT("전체 화면"))));
		QualityText->SetText(FText::FromString(FString::Printf(TEXT("그래픽 품질: %s"), QualityName(Settings->GetOverallScalabilityLevel()))));
	}
}

void USKOptionsMenu::OnWindowModeClicked()
{
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		const bool bWindowed = Settings->GetFullscreenMode() == EWindowMode::Windowed;
		Settings->SetFullscreenMode(bWindowed ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
		Settings->ApplySettings(false);
		RefreshScreenLabels();
	}
}

void USKOptionsMenu::OnQualityClicked()
{
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		const int32 Current = Settings->GetOverallScalabilityLevel();
		Settings->SetOverallScalabilityLevel(Current < 0 ? 0 : (Current + 1) % 4);
		Settings->ApplySettings(false);
		RefreshScreenLabels();
	}
}

void USKOptionsMenu::OnSaveClicked()
{
	USKSaveGame* Save = Cast<USKSaveGame>(UGameplayStatics::CreateSaveGameObject(USKSaveGame::StaticClass()));
	const ASKCharacter* Leo = Cast<ASKCharacter>(GetOwningPlayerPawn());
	if (!Save || !Leo)
	{
		SetStatus(TEXT("저장할 수 없습니다"));
		return;
	}

	Leo->WriteSaveData(*Save);
	Save->MasterVolume = FApp::GetVolumeMultiplier();
	SetStatus(UGameplayStatics::SaveGameToSlot(Save, USKSaveGame::SlotName, 0) ? TEXT("저장했습니다") : TEXT("저장에 실패했습니다"));
}

void USKOptionsMenu::OnLoadClicked()
{
	if (!UGameplayStatics::DoesSaveGameExist(USKSaveGame::SlotName, 0))
	{
		SetStatus(TEXT("저장된 데이터가 없습니다"));
		return;
	}

	const USKSaveGame* Save = Cast<USKSaveGame>(UGameplayStatics::LoadGameFromSlot(USKSaveGame::SlotName, 0));
	ASKCharacter* Leo = Cast<ASKCharacter>(GetOwningPlayerPawn());
	if (!Save || !Leo)
	{
		SetStatus(TEXT("불러오지 못했습니다"));
		return;
	}

	Leo->ApplySaveData(*Save);
	VolumeSlider->SetValue(Save->MasterVolume);
	OnVolumeChanged(Save->MasterVolume);
	SetStatus(TEXT("불러왔습니다"));
}

void USKOptionsMenu::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#include "FilterManager.h"
#include "Components/PostProcessComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

AFilterManager::AFilterManager()
{
	PrimaryActorTick.bCanEverTick = false;

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	SetRootComponent(PostProcess);
	PostProcess->bUnbound = true; // 레벨 전체에 적용
	PostProcess->BlendWeight = 0.f; // 시작은 필터 없음
}

void AFilterManager::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	EnableInput(PC);
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AFilterManager::Filter1);
		InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AFilterManager::Filter2);
		InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AFilterManager::Filter3);
		InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AFilterManager::Filter4);
		InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AFilterManager::Filter5);
		InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AFilterManager::ToggleGrayShading);
	}
}

void AFilterManager::ToggleGrayShading()
{
	if (!bGrayShading)
	{
		if (!GrayMaterial)
		{
			GrayMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GrayShade.M_GrayShade"), nullptr, LOAD_NoWarn);
		}
		if (!GrayMaterial)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Red, TEXT("M_GrayShade 머티리얼이 없습니다. Python 스크립트를 먼저 실행하세요."));
			}
			return;
		}

		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (*It == this)
			{
				continue;
			}
			TArray<UMeshComponent*> Meshes;
			It->GetComponents<UMeshComponent>(Meshes);
			for (UMeshComponent* Mesh : Meshes)
			{
				const int32 Num = Mesh->GetNumMaterials();
				if (Num == 0)
				{
					continue;
				}
				FSavedMaterials& Saved = SavedMaterials.Add(Mesh);
				for (int32 i = 0; i < Num; ++i)
				{
					Saved.Materials.Add(Mesh->GetMaterial(i));
					Mesh->SetMaterial(i, GrayMaterial);
				}
			}
		}
		bGrayShading = true;
	}
	else
	{
		for (auto& Pair : SavedMaterials)
		{
			if (UMeshComponent* Mesh = Pair.Key)
			{
				for (int32 i = 0; i < Pair.Value.Materials.Num(); ++i)
				{
					Mesh->SetMaterial(i, Pair.Value.Materials[i]);
				}
			}
		}
		SavedMaterials.Empty();
		bGrayShading = false;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 2.f, FColor::Yellow,
			bGrayShading ? TEXT("Gray Shading: ON") : TEXT("Gray Shading: OFF"));
	}
}

void AFilterManager::ToggleFilter(int32 Index)
{
	// 같은 키를 다시 누르면 해제, 다른 키면 해당 필터로 교체
	ActiveFilter = (ActiveFilter == Index) ? 0 : Index;
	ApplyFilter(ActiveFilter);
}

void AFilterManager::ApplyFilter(int32 Index)
{
	if (Index == 0)
	{
		PostProcess->BlendWeight = 0.f;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 2.f, FColor::White, TEXT("Filter: OFF"));
		}
		return;
	}

	FPostProcessSettings S; // 기본값(모든 override 꺼짐)에서 시작
	FString Name;

	switch (Index)
	{
	case 1: // 흑백
		Name = TEXT("1: Grayscale");
		S.bOverride_ColorSaturation = true;
		S.ColorSaturation = FVector4(0.f, 0.f, 0.f, 1.f);
		S.bOverride_ColorContrast = true;
		S.ColorContrast = FVector4(1.15f, 1.15f, 1.15f, 1.f);
		break;

	case 2: // 세피아
		Name = TEXT("2: Sepia");
		S.bOverride_ColorSaturation = true;
		S.ColorSaturation = FVector4(0.35f, 0.35f, 0.35f, 1.f);
		S.bOverride_SceneColorTint = true;
		S.SceneColorTint = FLinearColor(1.0f, 0.82f, 0.6f);
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = 0.6f;
		break;

	case 3: // 차가운 푸른 톤 + 색수차
		Name = TEXT("3: Cold Blue");
		S.bOverride_SceneColorTint = true;
		S.SceneColorTint = FLinearColor(0.6f, 0.8f, 1.0f);
		S.bOverride_ColorSaturation = true;
		S.ColorSaturation = FVector4(0.8f, 0.8f, 0.8f, 1.f);
		S.bOverride_SceneFringeIntensity = true;
		S.SceneFringeIntensity = 2.0f;
		break;

	case 4: // 선명하고 따뜻한 비비드
		Name = TEXT("4: Vivid Warm");
		S.bOverride_ColorSaturation = true;
		S.ColorSaturation = FVector4(1.6f, 1.6f, 1.6f, 1.f);
		S.bOverride_ColorContrast = true;
		S.ColorContrast = FVector4(1.2f, 1.2f, 1.2f, 1.f);
		S.bOverride_SceneColorTint = true;
		S.SceneColorTint = FLinearColor(1.0f, 0.93f, 0.85f);
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 1.5f;
		break;

	case 5: // 공포/야간: 어둡고 탈색 + 필름 그레인 + 강한 비네트
		Name = TEXT("5: Horror Night");
		S.bOverride_ColorSaturation = true;
		S.ColorSaturation = FVector4(0.3f, 0.3f, 0.3f, 1.f);
		S.bOverride_ColorGain = true;
		S.ColorGain = FVector4(0.6f, 0.75f, 0.7f, 1.f);
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = 1.2f;
		S.bOverride_FilmGrainIntensity = true;
		S.FilmGrainIntensity = 0.6f;
		break;

	default:
		return;
	}

	PostProcess->Settings = S;
	PostProcess->BlendWeight = 1.f;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 2.f, FColor::Yellow, FString::Printf(TEXT("Filter: %s"), *Name));
	}
}

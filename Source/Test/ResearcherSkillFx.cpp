#include "ResearcherSkillFx.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

AResearcherSkillFx::AResearcherSkillFx()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->CastShadow = false;
	SetRootComponent(Mesh);
}

void AResearcherSkillFx::Play(EResearcherFxShape InShape, const FLinearColor& InColor, float InDuration, const FVector& InStartScale, const FVector& InEndScale, float InStartOpacity, float InEndOpacity)
{
	const bool bRing = InShape == EResearcherFxShape::Ring;
	const TCHAR* MeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	if (InShape == EResearcherFxShape::Sphere)
	{
		MeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	}
	else if (bRing)
	{
		MeshPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	}
	UStaticMesh* ShapeMesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
	if (ShapeMesh)
	{
		Mesh->SetStaticMesh(ShapeMesh);
	}
	// 반투명 발광 머티리얼. 없으면 기본 머티리얼로 보인다
	// 고리는 중심이 비어 있는 M_DroneWave(파라미터 Alpha), 나머지는 M_SkillFx(파라미터 Opacity)
	OpacityParam = bRing ? TEXT("Alpha") : TEXT("Opacity");
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, bRing ? TEXT("/Game/VFX/M_DroneWave.M_DroneWave") : TEXT("/Game/Materials/M_SkillFx.M_SkillFx")))
	{
		Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), InColor);
		Material->SetScalarParameterValue(OpacityParam, InStartOpacity);
		if (bRing)
		{
			Material->SetScalarParameterValue(TEXT("RingRadius"), RingRadius);
			Material->SetScalarParameterValue(TEXT("RingWidth"), RingWidth);
			Material->SetScalarParameterValue(TEXT("Glow"), RingGlow);
		}
		Mesh->SetMaterial(0, Material);
	}
	Duration = FMath::Max(InDuration, 0.02f);
	StartScale = InStartScale;
	EndScale = InEndScale;
	StartOpacity = InStartOpacity;
	EndOpacity = InEndOpacity;
	Age = 0.f;
	Mesh->SetRelativeScale3D(StartScale);
}

void AResearcherSkillFx::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / Duration, 0.f, 1.f);
	const float Eased = 1.f - FMath::Square(1.f - T); // 처음에 빠르게 퍼지고 끝에 느려진다
	Mesh->SetRelativeScale3D(FMath::Lerp(StartScale, EndScale, Eased));
	if (Material)
	{
		Material->SetScalarParameterValue(OpacityParam, FMath::Lerp(StartOpacity, EndOpacity, T));
	}
	if (T >= 1.f)
	{
		Destroy();
	}
}

void AResearcherSkillFx::SetRingStyle(float InRingRadius, float InRingWidth, float InGlow)
{
	RingRadius = InRingRadius;
	RingWidth = InRingWidth;
	RingGlow = InGlow;
}

#include "ResearcherLightningFx.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

AResearcherLightningFx::AResearcherLightningFx()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(false);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void AResearcherLightningFx::Play(float InRadius, float InDuration, const FLinearColor& InColor, int32 InBoltCount)
{
	Radius = FMath::Max(InRadius, 1.f);
	Duration = FMath::Max(InDuration, 0.05f);
	BoltCount = FMath::Max(InBoltCount, 1);
	Age = 0.f;
	FlickerTimer = 0.f;

	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_SkillFx.M_SkillFx")))
	{
		Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), InColor);
		Material->SetScalarParameterValue(TEXT("Opacity"), 1.f);
	}

	// 가닥마다 방향을 고르게 나눠 두고, 조금씩 어긋나게 한다
	BoltAngles.Reset();
	const float Offset = FMath::FRandRange(0.f, 360.f);
	for (int32 i = 0; i < BoltCount; ++i)
	{
		BoltAngles.Add(Offset + 360.f * i / BoltCount + FMath::FRandRange(-12.f, 12.f));
	}

	const int32 Total = BoltCount * SegmentsPerBolt;
	for (int32 i = 0; i < Total; ++i)
	{
		UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetGenerateOverlapEvents(false);
		Piece->CastShadow = false;
		Piece->SetStaticMesh(Cylinder);
		if (Material)
		{
			Piece->SetMaterial(0, Material);
		}
		Piece->SetupAttachment(GetRootComponent());
		Piece->RegisterComponent();
		Segments.Add(Piece);
	}
	BuildBolts(0.f);
}

void AResearcherLightningFx::BuildBolts(float T)
{
	const FVector Center = GetActorLocation();
	// 처음 30% 동안 바깥으로 뻗고, 그 뒤로는 끝까지 닿은 채 깜빡인다
	const float Reach = Radius * FMath::Clamp(T / 0.3f, 0.f, 1.f);
	const float Wobble = Radius * Jitter;

	for (int32 b = 0; b < BoltCount; ++b)
	{
		const FVector Direction = FRotator(0.f, BoltAngles[b], 0.f).Vector();
		const FVector Side = FVector::CrossProduct(Direction, FVector::UpVector);
		const float BoltReach = Reach * FMath::FRandRange(0.75f, 1.f);

		FVector Previous = Center + FVector(0.f, 0.f, 20.f);
		for (int32 k = 1; k <= SegmentsPerBolt; ++k)
		{
			const float Along = BoltReach * k / SegmentsPerBolt;
			// 가운데로 갈수록 크게 꺾이고, 시작점은 흔들리지 않는다. 바닥 근처에서 위로만 튄다
			const float Strength = (k == SegmentsPerBolt) ? 0.5f : 1.f;
			FVector Point = Center + Direction * Along
				+ Side * FMath::FRandRange(-Wobble, Wobble) * Strength
				+ FVector(0.f, 0.f, 20.f + FMath::FRandRange(0.f, Wobble * 1.2f) * Strength);

			UStaticMeshComponent* Piece = Segments[b * SegmentsPerBolt + (k - 1)];
			const FVector Delta = Point - Previous;
			const float Length = FMath::Max(Delta.Size(), 0.1f);
			Piece->SetVisibility(BoltReach > 1.f);
			Piece->SetWorldLocation((Previous + Point) * 0.5f);
			Piece->SetWorldRotation(FRotationMatrix::MakeFromZ(Delta).Rotator());
			Piece->SetWorldScale3D(FVector(Thickness / 100.f, Thickness / 100.f, Length / 100.f));
			Previous = Point;
		}
	}
}

void AResearcherLightningFx::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / Duration, 0.f, 1.f);

	FlickerTimer -= DeltaSeconds;
	if (FlickerTimer <= 0.f)
	{
		FlickerTimer = FlickerInterval;
		BuildBolts(T);
	}
	if (Material)
	{
		// 끝으로 갈수록 옅어진다. 간헐적으로 더 어두워져 지직거리는 느낌을 준다
		const float Flicker = FMath::FRandRange(0.65f, 1.f);
		Material->SetScalarParameterValue(TEXT("Opacity"), (1.f - FMath::Square(T)) * Flicker);
	}
	if (T >= 1.f)
	{
		Destroy();
	}
}

#include "ResearcherDrone.h"
#include "ResearcherCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AResearcherDrone::AResearcherDrone()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(30.f);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	// 사람/다른 드론은 막지 않고, 벽과 바닥만 막는다
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Collision->SetCanEverAffectNavigation(false);
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->CastShadow = true;

	// 드론 모델(Dron_UE2). 없으면 사각형으로 대체
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DroneMesh(TEXT("/Game/Characters/Resercher/Drone/Drone.Drone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DroneMesh.Succeeded())
	{
		Mesh->SetStaticMesh(DroneMesh.Object);
		Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	}
	else if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.25f));
	}

	// 음파 링: 평면 여러 장이 시간차를 두고 나온다. 드론이 기울어도 방향이 흔들리지 않도록 위치/회전은 월드 기준
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaveMaterial(TEXT("/Game/VFX/M_DroneWave.M_DroneWave"));
	bHasWaveMaterial = WaveMaterial.Succeeded();
	for (int32 i = 0; i < NumWaves; ++i)
	{
		UStaticMeshComponent* Wave = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Wave%d"), i));
		Wave->SetupAttachment(Collision);
		Wave->SetUsingAbsoluteLocation(true);
		Wave->SetUsingAbsoluteRotation(true);
		Wave->SetRelativeRotation(FRotator::ZeroRotator);
		Wave->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wave->SetCanEverAffectNavigation(false);
		Wave->CastShadow = false;
		if (PlaneMesh.Succeeded())
		{
			Wave->SetStaticMesh(PlaneMesh.Object);
		}
		if (WaveMaterial.Succeeded())
		{
			Wave->SetMaterial(0, WaveMaterial.Object);
		}
		WaveMeshes.Add(Wave);
	}

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Collision);
	Camera->SetRelativeLocation(FVector(EyeForward, 0.f, 0.f));
	Camera->bUsePawnControlRotation = false;
}

void AResearcherDrone::BeginPlay()
{
	Super::BeginPlay();

	LastLocation = GetActorLocation();

	// 링이 드론 몸체보다 작게 시작하도록 모델의 가로 크기에서 반지름을 구한다
	MaxWaveRadius = WaveRadius;
	if (MaxWaveRadius <= 0.f && Mesh->GetStaticMesh())
	{
		const FVector Extent = Mesh->GetStaticMesh()->GetBounds().BoxExtent * Mesh->GetRelativeScale3D().GetAbs();
		MaxWaveRadius = FMath::Max(Extent.X, Extent.Y) * WaveRadiusScale;
	}
	if (MaxWaveRadius <= 1.f)
	{
		MaxWaveRadius = 30.f;
	}

	for (UStaticMeshComponent* Wave : WaveMeshes)
	{
		// 평면 메시에는 기본 머티리얼이 있으므로, M_DroneWave를 찾았을 때만 링을 쓴다 (없으면 불투명한 판이 보인다)
		UMaterialInterface* Base = bHasWaveMaterial ? Wave->GetMaterial(0) : nullptr;
		UMaterialInstanceDynamic* Instance = Base ? UMaterialInstanceDynamic::Create(Base, this) : nullptr;
		if (Instance)
		{
			Wave->SetMaterial(0, Instance);
			WaveMaterials.Add(Instance);
		}
		else
		{
			Wave->SetVisibility(false); // M_DroneWave가 없으면 링을 그리지 않는다
		}
	}
}

void AResearcherDrone::UpdateWaves(float DeltaSeconds)
{
	const bool bShow = bShowWaves && !bViewActive && WaveMaterials.Num() == WaveMeshes.Num();
	if (!bShow || DeltaSeconds <= 0.f)
	{
		for (UStaticMeshComponent* Wave : WaveMeshes)
		{
			Wave->SetVisibility(false);
		}
		LastLocation = GetActorLocation();
		return;
	}

	// 실제로 움직인 속도로 "떠 있는 중"과 "움직이는 중"을 부드럽게 오간다
	const FVector Location = GetActorLocation();
	SmoothedVelocity = FMath::VInterpTo(SmoothedVelocity, (Location - LastLocation) / DeltaSeconds, DeltaSeconds, 6.f);
	LastLocation = Location;
	const float Speed = SmoothedVelocity.Size();
	if (Speed > 20.f)
	{
		LastBackDirection = -SmoothedVelocity / Speed; // 멈춰도 마지막 이동 방향의 뒤쪽을 기억한다
	}
	const float Move = FMath::Clamp(Speed / FMath::Max(MovingSpeedForFullWave, 1.f), 0.f, 1.f);

	// 링이 나가는 방향: 떠 있으면 아래, 움직이면 뒤. 이 방향이 링 평면의 법선이 된다
	FVector Direction = (FVector::DownVector * (1.f - Move) + LastBackDirection * Move).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = FVector::DownVector;
	}
	const FRotator Facing = FRotationMatrix::MakeFromZ(Direction).Rotator();
	const float Travel = FMath::Lerp(WaveDrop, MovingWaveTravel, Move);

	const float Period = FMath::Max(FMath::Lerp(IdleWavePeriod, MovingWavePeriod, Move), 0.1f);
	WavePhase = FMath::Frac(WavePhase + DeltaSeconds / Period);
	const float BaseAlpha = FMath::Lerp(IdleWaveAlpha, MovingWaveAlpha, Move);

	for (int32 i = 0; i < WaveMeshes.Num(); ++i)
	{
		const float T = FMath::Frac(WavePhase + static_cast<float>(i) / WaveMeshes.Num());
		// 큰 링으로 나와서 멀어지며 작아진다. 막 생길 때는 서서히 나타나고, 끝으로 갈수록 옅어진다
		const float Radius = FMath::Lerp(1.f, WaveEndScale, T) * MaxWaveRadius;
		const float FadeIn = FMath::Clamp(T / 0.1f, 0.f, 1.f);
		const float FadeOut = FMath::Pow(1.f - T, 1.2f);

		UStaticMeshComponent* Wave = WaveMeshes[i];
		Wave->SetVisibility(true);
		Wave->SetWorldLocation(Location + Direction * (WaveDepth + T * Travel));
		Wave->SetWorldRotation(Facing);
		const float Scale = Radius * 2.f / 100.f; // 기본 평면은 100cm
		Wave->SetRelativeScale3D(FVector(Scale, Scale, 1.f));
		WaveMaterials[i]->SetScalarParameterValue(TEXT("Alpha"), BaseAlpha * FadeIn * FadeOut);
		WaveMaterials[i]->SetVectorParameterValue(TEXT("Color"), WaveColor);
	}
}

FVector AResearcherDrone::GetEyeLocation() const
{
	return Camera->GetComponentLocation();
}

void AResearcherDrone::SetViewActive(bool bActive)
{
	bViewActive = bActive;
	PendingInput = FVector::ZeroVector;
	bRise = false;
	// 드론 카메라가 메시 바로 앞/안쪽이라 시야 전환 중에는 메시를 숨긴다
	Mesh->SetVisibility(!bActive);
}

void AResearcherDrone::SetParked(bool bInParked, const FVector& InLocation)
{
	bParked = bInParked;
	if (bInParked)
	{
		ParkedLocation = InLocation;
	}
}

void AResearcherDrone::AddFlightInput(const FVector& WorldDirection, float Scale)
{
	PendingInput += WorldDirection * Scale;
}

void AResearcherDrone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateWaves(DeltaSeconds);

	AResearcherCharacter* Character = OwnerCharacter.Get();
	if (!Character)
	{
		return;
	}
	const FVector ResearcherLocation = Character->GetActorLocation();
	const FRotator View = Character->GetViewRotation();
	BobTime += DeltaSeconds;

	// 방향은 연구가의 시점 (시야 전환 중에는 이것이 드론 카메라 방향이다). 고정된 드론은 방향도 그대로 둔다
	if (bViewActive || !bParked)
	{
		SetActorRotation(FRotator(View.Pitch, View.Yaw, 0.f));
	}

	if (!bViewActive)
	{
		// 고정된 채 연구가가 너무 멀어지면 고정을 풀어 돌아온다 (아래 따라가기가 순간이동시킨다)
		if (bParked && ParkReturnDistance > 0.f && FVector::Dist(ResearcherLocation, ParkedLocation) > ParkReturnDistance)
		{
			Character->ReleaseDrone();
		}

		FVector Wanted;
		if (bParked)
		{
			Wanted = ParkedLocation;
		}
		else
		{
			// 따라다니기: 연구가 옆 위쪽
			const FRotator Yaw(0.f, View.Yaw, 0.f);
			Wanted = ResearcherLocation + Yaw.RotateVector(FollowOffset) + FVector(0.f, 0.f, 8.f * FMath::Sin(BobTime * 2.f));
		}
		if (FVector::Dist(GetActorLocation(), Wanted) > TeleportDistance)
		{
			SetActorLocation(Wanted, false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			SetActorLocation(FMath::VInterpTo(GetActorLocation(), Wanted, DeltaSeconds, FollowSpeed), false, nullptr, ETeleportType::TeleportPhysics);
		}
		return;
	}

	// 비행 중: 입력대로 날아간다 (벽/바닥은 막힌다)
	FVector Velocity = PendingInput.GetClampedToMaxSize(1.f) * FlightSpeed;
	if (bRise)
	{
		Velocity.Z += RiseSpeed;
	}
	PendingInput = FVector::ZeroVector;
	if (!Velocity.IsNearlyZero())
	{
		AddActorWorldOffset(Velocity * DeltaSeconds, true);
	}

	// 연구가에게서 너무 멀어지지 않게
	if (MaxDistanceFromResearcher > 0.f)
	{
		const FVector ToDrone = GetActorLocation() - ResearcherLocation;
		if (ToDrone.Size() > MaxDistanceFromResearcher)
		{
			SetActorLocation(ResearcherLocation + ToDrone.GetSafeNormal() * MaxDistanceFromResearcher, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

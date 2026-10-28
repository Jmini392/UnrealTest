#include "MovingPlatform.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMovingPlatform::AMovingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	SetRootComponent(PlatformMesh);
	PlatformMesh->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		PlatformMesh->SetStaticMesh(Cube.Object);
	}
	PlatformMesh->SetRelativeScale3D(FVector(3.f, 3.f, 0.3f));
}

void AMovingPlatform::BeginPlay()
{
	Super::BeginPlay();

	StartLocation = GetActorLocation();
	EndLocation = GetActorTransform().TransformPosition(TargetOffset);
	bMoving = bAutoStart;
}

void AMovingPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bMoving)
	{
		return;
	}

	if (WaitRemaining > 0.f)
	{
		WaitRemaining -= DeltaSeconds;
		return;
	}

	const float Distance = FVector::Dist(StartLocation, EndLocation);
	if (Distance < KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Ease-in/out only reshapes the position; Alpha itself advances at constant speed.
	Alpha += (bForward ? 1.f : -1.f) * Speed * DeltaSeconds / Distance;

	bool bArrived = false;
	if (Alpha >= 1.f || Alpha <= 0.f)
	{
		Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
		bArrived = true;
	}

	const float Shaped = bEaseInOut ? FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f) : Alpha;
	SetActorLocation(FMath::Lerp(StartLocation, EndLocation, Shaped));

	if (bArrived)
	{
		OnReachedEnd(bForward);
		bForward = !bForward;
		WaitRemaining = WaitTime;
	}
}

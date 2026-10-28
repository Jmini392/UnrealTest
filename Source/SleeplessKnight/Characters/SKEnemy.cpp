#include "Characters/SKEnemy.h"
#include "GAS/SKAttributeSet.h"
#include "SKGameMode.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

ASKEnemy::ASKEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);
	GetCharacterMovement()->MaxWalkSpeed = 0.f;
	GetCharacterMovement()->GroundFriction = 4.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 800.f;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Body->SetStaticMesh(Cylinder.Object);
	}
	Body->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));

	SleepText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SleepText"));
	SleepText->SetupAttachment(RootComponent);
	SleepText->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	SleepText->SetText(FText::FromString(TEXT("Zzz")));
	SleepText->SetHorizontalAlignment(EHTA_Center);
	SleepText->SetWorldSize(60.f);
	SleepText->SetTextRenderColor(FColor::Cyan);
	SleepText->SetVisibility(false);

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<USKAttributeSet>(TEXT("Attributes"));
}

void ASKEnemy::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystem->InitAbilityActorInfo(this, this);
	Attributes->InitMaxHealth(MaxHealthValue);
	Attributes->InitHealth(MaxHealthValue);
	BodyBaseScale = Body->GetRelativeScale3D();
}

void ASKEnemy::ReceiveHit(AActor* Attacker, float Damage, const FVector& Knockback, bool bHeavy)
{
	if (bAsleep)
	{
		return;
	}

	AbilitySystem->ApplyModToAttribute(USKAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -Damage);

	LaunchCharacter(Knockback, true, true);
	SquashStrength = bHeavy ? HeavySquash : LightSquash;
	SquashTime = 0.f;

	if (Attacker)
	{
		// Turn toward the attacker so the flinch reads in the right direction.
		const FVector ToAttacker = (Attacker->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!ToAttacker.IsNearlyZero())
		{
			SetActorRotation(ToAttacker.Rotation());
		}
	}

	if (Attributes->GetHealth() <= 0.f)
	{
		FallAsleep();
	}
}

void ASKEnemy::FallAsleep()
{
	bAsleep = true;
	SleepText->SetVisibility(true);

	// Lie down and let the player walk over the sleeper.
	Body->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, -60.f));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	if (ASKGameMode* GM = GetWorld()->GetAuthGameMode<ASKGameMode>())
	{
		GM->NotifyEnemyAsleep();
	}
}

void ASKEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAsleep)
	{
		// Face the text toward the player camera.
		if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				const FVector ToCam = PC->PlayerCameraManager->GetCameraLocation() - SleepText->GetComponentLocation();
				SleepText->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
			}
		}
		return;
	}

	// Idle dance: bob and wiggle.
	DanceTime += DeltaSeconds;
	float Bob = FMath::Sin(DanceTime * 8.f) * 6.f;
	float Wiggle = FMath::Sin(DanceTime * 4.f) * 12.f;

	// Squash and stretch: damped spring. Positive = flattened.
	SquashTime += DeltaSeconds;
	const float A = SquashStrength * FMath::Exp(-7.f * SquashTime) * FMath::Cos(26.f * SquashTime);
	const FVector Scale(BodyBaseScale.X * (1.f + 0.6f * A), BodyBaseScale.Y * (1.f + 0.6f * A), BodyBaseScale.Z * (1.f - 0.5f * A));
	Body->SetRelativeScale3D(Scale);
	Body->SetRelativeLocation(FVector(0.f, 0.f, Bob));
	Body->SetRelativeRotation(FRotator(0.f, Wiggle, 0.f));
}

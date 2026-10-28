#include "SKGameMode.h"
#include "Characters/SKCharacter.h"
#include "Characters/SKEnemy.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

ASKGameMode::ASKGameMode()
{
	DefaultPawnClass = ASKCharacter::StaticClass();
	EnemyClass = ASKEnemy::StaticClass();
}

void ASKGameMode::BeginPlay()
{
	Super::BeginPlay();

	FVector Center = FVector::ZeroVector;
	if (const AActor* Start = UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass()))
	{
		Center = Start->GetActorLocation();
	}

	for (int32 i = 0; i < TestEnemyCount; ++i)
	{
		const float Angle = 2.f * PI * i / FMath::Max(TestEnemyCount, 1);
		const FVector Offset(FMath::Cos(Angle) * TestEnemyRadius, FMath::Sin(Angle) * TestEnemyRadius, 100.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (GetWorld()->SpawnActor<ASKEnemy>(EnemyClass, Center + Offset, FRotator::ZeroRotator, Params))
		{
			++SpawnedEnemies;
		}
	}

	ShowSleepCount();
}

void ASKGameMode::NotifyEnemyAsleep()
{
	++SleepCount;
	ShowSleepCount();
}

void ASKGameMode::ShowSleepCount() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 1000.f, FColor::Cyan,
			FString::Printf(TEXT("Put to sleep: %d / %d"), SleepCount, SpawnedEnemies), true, FVector2D(2.f));
	}
}

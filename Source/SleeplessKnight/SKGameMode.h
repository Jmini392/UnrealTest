#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SKGameMode.generated.h"

class ASKEnemy;

UCLASS()
class SLEEPLESSKNIGHT_API ASKGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASKGameMode();

	/** Called by an enemy when it falls asleep. Drives the "enemies put to sleep" counter. */
	void NotifyEnemyAsleep();

protected:
	virtual void BeginPlay() override;

	/** Graybox: dancers spawned in a ring around the player start so the combat can be tested without placing actors. */
	UPROPERTY(EditAnywhere, Category = "Prototype")
	int32 TestEnemyCount = 6;

	UPROPERTY(EditAnywhere, Category = "Prototype")
	float TestEnemyRadius = 700.f;

	UPROPERTY(EditAnywhere, Category = "Prototype")
	TSubclassOf<ASKEnemy> EnemyClass;

private:
	void ShowSleepCount() const;

	int32 SleepCount = 0;
	int32 SpawnedEnemies = 0;
};

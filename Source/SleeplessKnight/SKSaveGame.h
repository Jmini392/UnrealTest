#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SKSaveGame.generated.h"

/** Quick save: Leo's state and the volume setting. Enemy state is not saved yet. */
UCLASS()
class SLEEPLESSKNIGHT_API USKSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static const FString SlotName;

	UPROPERTY() FTransform PlayerTransform;
	UPROPERTY() FRotator ControlRotation = FRotator::ZeroRotator;
	UPROPERTY() float Health = 100.f;
	UPROPERTY() float UltGauge = 0.f;
	UPROPERTY() float MasterVolume = 1.f;
};

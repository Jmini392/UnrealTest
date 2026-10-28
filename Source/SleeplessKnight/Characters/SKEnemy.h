#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "SKEnemy.generated.h"

class UAbilitySystemComponent;
class USKAttributeSet;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Party monster. Never dies: when health runs out it falls asleep. Graybox version with placeholder shapes. */
UCLASS()
class SLEEPLESSKNIGHT_API ASKEnemy : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASKEnemy();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	/** Called by the attacker's ability. Knockback is a world-space launch velocity. */
	void ReceiveHit(AActor* Attacker, float Damage, const FVector& Knockback, bool bHeavy);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsAsleep() const { return bAsleep; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void FallAsleep();

	UPROPERTY(EditAnywhere, Category = "Enemy")
	float MaxHealthValue = 40.f;

	/** Squash and stretch strength on a normal hit (1.0 = body flattens to half height). */
	UPROPERTY(EditAnywhere, Category = "Enemy|Feel")
	float LightSquash = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Feel")
	float HeavySquash = 0.9f;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UTextRenderComponent> SleepText;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<USKAttributeSet> Attributes;

private:
	bool bAsleep = false;
	float SquashStrength = 0.f;
	float SquashTime = 0.f;
	float DanceTime = 0.f;
	FVector BodyBaseScale = FVector::OneVector;
};

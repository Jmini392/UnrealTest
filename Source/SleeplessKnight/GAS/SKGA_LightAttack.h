#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SKGA_LightAttack.generated.h"

class ASKCharacter;

/** Tuning for one hit of the light combo. Edit on the ability defaults (or a Blueprint child) while feeling it out. */
USTRUCT(BlueprintType)
struct FSKComboStep
{
	GENERATED_BODY()

	/** Time before the hit can land (anticipation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WindupTime = 0.12f;
	/** Time the hit volume is live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ActiveTime = 0.10f;
	/** Time after the hit before the next press stops chaining. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RecoveryTime = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 10.f;
	/** Horizontal knockback speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Knockback = 450.f;
	/** Small upward pop so enemies bounce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Pop = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HitStop = 0.03f;
	/** How far Leo steps forward when the swing starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Lunge = 250.f;
	/** Last hit of the chain: bigger reaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bFinisher = false;
};

/**
 * Up to 4-hit light attack chain. Pressing the input again while the ability is active buffers the next hit.
 * Phases are timer driven for now; swap them for montage notifies once Leo has animations.
 */
UCLASS()
class SLEEPLESSKNIGHT_API USKGA_LightAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USKGA_LightAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TArray<FSKComboStep> Steps;

	UPROPERTY(EditDefaultsOnly, Category = "Hit")
	float HitRadius = 100.f;

	/** Distance in front of Leo where the hit sphere is centered. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit")
	float HitReach = 130.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hit")
	float UltGaugePerHit = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDrawDebug = true;

private:
	enum class EPhase : uint8 { Windup, Active, Recovery };

	void BeginStep();
	void BeginActive();
	void TickActive();
	void BeginRecovery();
	void FinishCombo();
	void ClearTimers();

	ASKCharacter* GetLeo() const;
	const FSKComboStep& CurrentStep() const { return Steps[ComboIndex]; }

	EPhase Phase = EPhase::Windup;
	int32 ComboIndex = 0;
	bool bNextBuffered = false;
	TSet<TWeakObjectPtr<AActor>> HitThisStep;
	FTimerHandle PhaseTimer;
	FTimerHandle ActiveTickTimer;
};

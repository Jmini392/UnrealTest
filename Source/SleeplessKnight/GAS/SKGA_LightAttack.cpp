#include "GAS/SKGA_LightAttack.h"
#include "GAS/SKAttributeSet.h"
#include "Characters/SKCharacter.h"
#include "Characters/SKEnemy.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

USKGA_LightAttack::USKGA_LightAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	// Starting values. The 4th step is the finisher: slower, heavier, longer hit stop.
	Steps.SetNum(4);
	Steps[0] = FSKComboStep{0.10f, 0.10f, 0.25f, 10.f, 400.f, 120.f, 0.03f, 220.f, false};
	Steps[1] = FSKComboStep{0.10f, 0.10f, 0.25f, 10.f, 400.f, 120.f, 0.03f, 220.f, false};
	Steps[2] = FSKComboStep{0.12f, 0.10f, 0.28f, 12.f, 450.f, 150.f, 0.03f, 240.f, false};
	Steps[3] = FSKComboStep{0.22f, 0.12f, 0.40f, 20.f, 1100.f, 350.f, 0.08f, 300.f, true};
}

ASKCharacter* USKGA_LightAttack::GetLeo() const
{
	return Cast<ASKCharacter>(GetAvatarActorFromActorInfo());
}

void USKGA_LightAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !GetLeo() || Steps.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ComboIndex = 0;
	bNextBuffered = false;
	BeginStep();
}

void USKGA_LightAttack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (ComboIndex + 1 >= Steps.Num())
	{
		return;
	}

	if (Phase == EPhase::Recovery)
	{
		// Pressed inside the chain window after the hit: continue right away.
		ClearTimers();
		++ComboIndex;
		BeginStep();
	}
	else
	{
		// Pressed during windup or active: remembered and consumed when recovery starts.
		bNextBuffered = true;
	}
}

void USKGA_LightAttack::BeginStep()
{
	ASKCharacter* Leo = GetLeo();
	UWorld* World = Leo ? Leo->GetWorld() : nullptr;
	if (!World)
	{
		K2_EndAbility();
		return;
	}

	Phase = EPhase::Windup;
	bNextBuffered = false;
	HitThisStep.Reset();

	const FSKComboStep& Step = CurrentStep();

	Leo->SetMovementLocked(true);
	Leo->FaceInputOrCamera();
	Leo->PlayWeaponSwing(ComboIndex, Step.WindupTime + Step.ActiveTime);
	Leo->LaunchCharacter(Leo->GetActorForwardVector() * Step.Lunge, true, false);

	World->GetTimerManager().SetTimer(PhaseTimer, FTimerDelegate::CreateUObject(this, &USKGA_LightAttack::BeginActive), FMath::Max(Step.WindupTime, KINDA_SMALL_NUMBER), false);
}

void USKGA_LightAttack::BeginActive()
{
	ASKCharacter* Leo = GetLeo();
	if (!Leo)
	{
		K2_EndAbility();
		return;
	}

	Phase = EPhase::Active;
	UWorld* World = Leo->GetWorld();
	FTimerManager& TM = World->GetTimerManager();

	TM.SetTimer(ActiveTickTimer, FTimerDelegate::CreateUObject(this, &USKGA_LightAttack::TickActive), 0.016f, true, 0.f);
	TM.SetTimer(PhaseTimer, FTimerDelegate::CreateUObject(this, &USKGA_LightAttack::BeginRecovery), FMath::Max(CurrentStep().ActiveTime, KINDA_SMALL_NUMBER), false);
}

void USKGA_LightAttack::TickActive()
{
	ASKCharacter* Leo = GetLeo();
	if (!Leo)
	{
		return;
	}

	UWorld* World = Leo->GetWorld();
	const FSKComboStep& Step = CurrentStep();
	const FVector Forward = Leo->GetActorForwardVector();
	const FVector Center = Leo->GetActorLocation() + Forward * HitReach;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LightAttack), false, Leo);
	World->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(HitRadius), Params);

	if (bDrawDebug)
	{
		DrawDebugSphere(World, Center, HitRadius, 12, Step.bFinisher ? FColor::Red : FColor::Yellow, false, 0.05f);
	}

	bool bConnected = false;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ASKEnemy* Enemy = Cast<ASKEnemy>(Overlap.GetActor());
		if (!Enemy || Enemy->IsAsleep() || HitThisStep.Contains(Enemy))
		{
			continue;
		}
		HitThisStep.Add(Enemy);

		const FVector Dir = (Enemy->GetActorLocation() - Leo->GetActorLocation()).GetSafeNormal2D();
		const FVector Launch = Dir * Step.Knockback + FVector(0.f, 0.f, Step.Pop);
		Enemy->ReceiveHit(Leo, Step.Damage, Launch, Step.bFinisher);

		GetAbilitySystemComponentFromActorInfo()->ApplyModToAttribute(USKAttributeSet::GetUltGaugeAttribute(), EGameplayModOp::Additive, UltGaugePerHit);
		bConnected = true;
	}

	if (bConnected)
	{
		Leo->RequestHitStop(Step.HitStop);
	}
}

void USKGA_LightAttack::BeginRecovery()
{
	ASKCharacter* Leo = GetLeo();
	if (!Leo)
	{
		K2_EndAbility();
		return;
	}

	FTimerManager& TM = Leo->GetWorld()->GetTimerManager();
	TM.ClearTimer(ActiveTickTimer);
	Phase = EPhase::Recovery;

	// Chain immediately if the player pressed during the swing.
	if (bNextBuffered && ComboIndex + 1 < Steps.Num())
	{
		++ComboIndex;
		BeginStep();
		return;
	}

	Leo->SetMovementLocked(false);
	TM.SetTimer(PhaseTimer, FTimerDelegate::CreateUObject(this, &USKGA_LightAttack::FinishCombo), FMath::Max(CurrentStep().RecoveryTime, KINDA_SMALL_NUMBER), false);
}

void USKGA_LightAttack::FinishCombo()
{
	K2_EndAbility();
}

void USKGA_LightAttack::ClearTimers()
{
	if (const ASKCharacter* Leo = GetLeo())
	{
		FTimerManager& TM = Leo->GetWorld()->GetTimerManager();
		TM.ClearTimer(PhaseTimer);
		TM.ClearTimer(ActiveTickTimer);
	}
}

void USKGA_LightAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ClearTimers();
	if (ASKCharacter* Leo = GetLeo())
	{
		Leo->SetMovementLocked(false);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

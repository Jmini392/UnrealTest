#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "InputActionValue.h"
#include "Containers/Ticker.h"
#include "SKCharacter.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class USKAttributeSet;
class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class USKOptionsMenu;
class USKSaveGame;

/** Input ids passed to GAS (FGameplayAbilitySpec::InputID). */
namespace SKAbilityInput
{
	constexpr int32 LightAttack = 1;
}

/** Leo, the player character. Owns the ability system directly (single player, so no PlayerState needed). */
UCLASS()
class SLEEPLESSKNIGHT_API ASKCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASKCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	/** While locked, movement input is ignored (during attack swings). */
	void SetMovementLocked(bool bLocked) { bMovementLocked = bLocked; }

	/** Rotate to the held move direction so attacks go where the player points. Keeps the current facing if no direction is held. */
	void FaceInputOrCamera();

	/** Graybox swing of the placeholder weapon. Replace with a montage once Leo is animated. */
	void PlayWeaponSwing(int32 ComboIndex, float Duration);

	/** Freeze the game for a few frames on impact. Real-time duration, ignores time dilation. */
	void RequestHitStop(float Duration);

	/** Quick save support: copy Leo's state into / out of a save object. */
	void WriteSaveData(USKSaveGame& Out) const;
	void ApplySaveData(const USKSaveGame& In);

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void MoveReleased() { HeldMoveDirection = FVector::ZeroVector; }
	void Look(const FInputActionValue& Value);
	void OnLightAttackPressed();
	void OnToggleMenuPressed();

	/** Inputs are built in code so the project needs no input assets yet. Move to data assets once the layout settles. */
	void CreateDefaultInput();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Graybox")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Graybox")
	TObjectPtr<USceneComponent> WeaponPivot;

	UPROPERTY(VisibleAnywhere, Category = "Graybox")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<USKAttributeSet> Attributes;

	/** Granted on BeginPlay. Light attack is input id 1. */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> LightAttackAbility;

	UPROPERTY() TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookAction;
	UPROPERTY() TObjectPtr<UInputAction> LightAttackAction;
	UPROPERTY() TObjectPtr<UInputAction> HeavyAttackAction;
	UPROPERTY() TObjectPtr<UInputAction> DodgeAction;
	UPROPERTY() TObjectPtr<UInputAction> ToggleMenuAction;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USKOptionsMenu> OptionsMenuClass;

	UPROPERTY() TObjectPtr<USKOptionsMenu> OptionsMenu;

private:
	bool bMovementLocked = false;
	/** Camera-relative world direction of the held move input, tracked even while movement is locked. */
	FVector HeldMoveDirection = FVector::ZeroVector;

	// Weapon swing state
	int32 SwingIndex = 0;
	float SwingElapsed = 0.f;
	float SwingDuration = 0.f;
	bool bSwinging = false;

	FTSTicker::FDelegateHandle HitStopHandle;
};

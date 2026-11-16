#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ResearcherCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class AResearcherDrone;

/** WASD로 이동하고 마우스로 카메라를 돌리는 3인칭 캐릭터. 메시와 애님 BP는 Researcher 에셋을 기본값으로 쓴다. */
UCLASS()
class TEST_API AResearcherCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AResearcherCharacter();

public:
	/** 쉬프트를 누른 채 움직이는 중이면 true. 애님 BP가 달리기 포즈를 고르는 데 쓴다 */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSprinting() const { return bWantsToSprint; }

	/** 드론 시야(Q)로 들어가거나 나온다. 나올 때 드론은 그 자리에 고정된다 */
	void ToggleDroneView();

	/** 드론 고정 해제(E): 시야에서 나오고 드론이 연구가에게 돌아온다 */
	void ReleaseDrone();

	UFUNCTION(BlueprintPure, Category = "Drone")
	bool IsDroneViewActive() const { return bDroneView && Drone != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Drone")
	AResearcherDrone* GetDrone() const { return Drone; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void OnRisePressed();
	void OnRiseReleased();

	/** 입력 에셋이 비어 있으면 WASD / 마우스 기본 매핑을 코드로 만든다 */
	void BuildDefaultInput();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** 비워두면 WASD로 자동 생성 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	/** 비워두면 왼쪽 쉬프트로 자동 생성 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	/** 비워두면 Q로 자동 생성: 드론 시야 전환 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> DroneViewAction;

	/** 비워두면 E로 자동 생성: 드론 고정 해제 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> ReleaseDroneAction;

	/** 비워두면 스페이스로 자동 생성: 드론 시야에서 상승 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> RiseAction;

	UPROPERTY(Transient)
	TObjectPtr<AResearcherDrone> Drone;

	/** 걷기 속도 (cm/s). Walking 애니메이션 보폭에 맞춘다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 150.f;

	/** 쉬프트를 누르고 있을 때 속도 (cm/s). Medium_Run 보폭에 맞춰 조정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 350.f;

private:
	bool bWantsToSprint = false;
	bool bDroneView = false;
};

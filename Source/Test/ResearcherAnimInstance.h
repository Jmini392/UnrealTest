#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ResearcherAnimInstance.generated.h"

/** ABP_Researcher의 부모 클래스. 이동 속도를 계산해 Idle/Walk 전환에 쓰도록 노출한다. */
UCLASS()
class TEST_API UResearcherAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** 수평 이동 속도 (cm/s) */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Speed = 0.f;

	/** Speed가 임계값보다 크면 true. Idle -> Walk 전환 조건으로 사용 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsMoving = false;

	/** 쉬프트를 누른 채 움직이는 중이면 true. Walk -> Run 전환 조건으로 사용 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsRunning = false;
};

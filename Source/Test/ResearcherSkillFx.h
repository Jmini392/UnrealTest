#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResearcherSkillFx.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** 능력 연출의 모양 */
UENUM()
enum class EResearcherFxShape : uint8
{
	Sphere,
	Disc,  // 납작한 원반 (바닥에 퍼지는 고리)
	Ring   // 속이 빈 얇은 고리 평면 (충격파). M_DroneWave 머티리얼을 쓴다
};

/**
 * 능력 연출 한 번 (SYNKrono_prototype의 ASynkSkillFx를 옮긴 것).
 * 반투명 발광 모양이 StartScale에서 EndScale로 커지며 StartOpacity에서 EndOpacity로 옅어지다가 사라진다.
 * 스케일은 100cm 기준 배율이다 (구 1.0 = 지름 100cm). 머티리얼은 /Game/Materials/M_SkillFx (Color, Opacity).
 */
UCLASS()
class TEST_API AResearcherSkillFx : public AActor
{
	GENERATED_BODY()

public:
	AResearcherSkillFx();

	/** 모양과 변화를 정한다. 이 호출 뒤부터 Duration초 동안 재생하고 사라진다 */
	void Play(EResearcherFxShape InShape, const FLinearColor& InColor, float InDuration, const FVector& InStartScale, const FVector& InEndScale, float InStartOpacity, float InEndOpacity);

	/** Ring 모양일 때 고리 위치(0~1, 평면 반지름 비율)와 두께, 밝기. Play보다 먼저 부른다 */
	void SetRingStyle(float InRingRadius, float InRingWidth, float InGlow);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;

	FVector StartScale = FVector::OneVector;
	FVector EndScale = FVector::OneVector;
	float StartOpacity = 0.4f;
	float EndOpacity = 0.f;
	float Duration = 0.5f;
	FName OpacityParam = TEXT("Opacity");
	float RingRadius = 0.9f;
	float RingWidth = 0.12f;
	float RingGlow = 3.f;
	float Age = 0.f;
};

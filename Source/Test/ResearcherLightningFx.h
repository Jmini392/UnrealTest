#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResearcherLightningFx.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * 펄스 연출용 번개 줄기. 중심에서 바깥으로 뻗는 여러 가닥이 지그재그로 번쩍이다가 사라진다 (정전기장처럼).
 * 가닥은 가는 원기둥 조각을 이어 붙여 만들고, 짧은 간격으로 모양을 다시 뽑아 깜빡이게 한다.
 * 머티리얼은 /Game/Materials/M_SkillFx (Color, Opacity). 복제하지 않는 로컬 액터.
 */
UCLASS()
class TEST_API AResearcherLightningFx : public AActor
{
	GENERATED_BODY()

public:
	AResearcherLightningFx();

	/** Radius까지 뻗는 번개 가닥들을 Duration초 동안 재생한다. 호출 위치(액터 위치)가 중심이다 */
	void Play(float InRadius, float InDuration, const FLinearColor& InColor, int32 InBoltCount = 9);

	virtual void Tick(float DeltaSeconds) override;

	/** 번개 굵기 (cm) */
	UPROPERTY(EditAnywhere, Category = "Lightning") float Thickness = 4.f;
	/** 한 가닥을 이루는 꺾임 수 */
	UPROPERTY(EditAnywhere, Category = "Lightning") int32 SegmentsPerBolt = 8;
	/** 모양을 새로 뽑는 간격 (초). 작을수록 더 빠르게 깜빡인다 */
	UPROPERTY(EditAnywhere, Category = "Lightning") float FlickerInterval = 0.04f;
	/** 꺾임의 흔들림 크기 (반경에 대한 비율) */
	UPROPERTY(EditAnywhere, Category = "Lightning") float Jitter = 0.07f;

private:
	void BuildBolts(float T);

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Segments;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
	TArray<float> BoltAngles;

	float Radius = 300.f;
	float Duration = 0.5f;
	float Age = 0.f;
	float FlickerTimer = 0.f;
	int32 BoltCount = 0;
};

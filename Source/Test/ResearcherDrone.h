#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResearcherDrone.generated.h"

class AResearcherCharacter;
class UCameraComponent;
class USphereComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * 연구가의 드론 (SYNKrono_prototype의 ASynkDrone을 싱글 플레이용으로 옮긴 것).
 *  - 평소: 연구가 옆 위쪽을 따라다닌다 (살짝 흔들림).
 *  - 시야 전환(Q) 중: 연구가가 입력으로 자유롭게 날아다닌다. 카메라 방향은 연구가의 시점(컨트롤러 회전)을 그대로 쓴다.
 *  - 시야에서 나오면 드론은 그 자리에 고정되고, 해제(E)하거나 연구가가 너무 멀어지면 연구가에게 돌아온다.
 * 네트워크 복제, 분할 화면 캡처, 시간 반전(월드 이동)은 옮기지 않았다.
 */
UCLASS()
class TEST_API AResearcherDrone : public AActor
{
	GENERATED_BODY()

public:
	AResearcherDrone();

	void InitForCharacter(AResearcherCharacter* InOwnerCharacter) { OwnerCharacter = InOwnerCharacter; }

	/** 시야 전환 중 비행 입력 (월드 방향, 크기 0~1) */
	void AddFlightInput(const FVector& WorldDirection, float Scale);
	void SetRise(bool bInRise) { bRise = bInRise; }

	UCameraComponent* GetCameraComponent() const { return Camera; }
	FVector GetEyeLocation() const;

	/** 시야 전환 중인가 (캐릭터가 알려 준다) */
	void SetViewActive(bool bActive);

	/** 고정: 시야에서 나오면 드론이 그 자리에 남는다. 해제하면 연구가에게 돌아온다 */
	void SetParked(bool bInParked, const FVector& InLocation);
	bool IsParked() const { return bParked; }

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Drone") float FlightSpeed = 900.f;
	UPROPERTY(EditAnywhere, Category = "Drone") float RiseSpeed = 600.f;
	/** 연구가로부터 이 거리 이상 멀어지지 않는다 (cm, 0이면 제한 없음) */
	UPROPERTY(EditAnywhere, Category = "Drone") float MaxDistanceFromResearcher = 3000.f;
	/** 고정된 드론에서 연구가가 이 거리 이상 멀어지면 고정이 풀리고 드론이 돌아온다 (cm). 비행 한계보다 커야 한다 */
	UPROPERTY(EditAnywhere, Category = "Drone") float ParkReturnDistance = 3500.f;
	/** 이 거리 이상 떨어져 있으면 따라가지 않고 순간이동한다 (cm) */
	UPROPERTY(EditAnywhere, Category = "Drone|Follow") float TeleportDistance = 3000.f;
	UPROPERTY(EditAnywhere, Category = "Drone|Follow") FVector FollowOffset = FVector(-20.f, 90.f, 100.f);
	UPROPERTY(EditAnywhere, Category = "Drone|Follow") float FollowSpeed = 4.f;
	/** 드론 모델 크기. 모델이 너무 크거나 작으면 여기서 조정한다 */
	UPROPERTY(EditAnywhere, Category = "Drone|Mesh") FVector MeshScale = FVector(1.f);
	/** 카메라는 드론 중심에서 이만큼 앞 (시야 전환 중에는 메시를 숨기지만 충돌 구와 맞추는 값) */
	UPROPERTY(EditAnywhere, Category = "Drone|Mesh") float EyeForward = 28.f;

	/**
	 * 드론의 음파 링 이펙트. 큰 링이 작아지면서 멀어진다.
	 *  - 떠 있을 때: 드론 아래로 내려간다 (바닥과 평행한 링)
	 *  - 움직일 때: 드론 뒤쪽으로 빠져나간다 (이동 방향에 수직인 링). 두 상태 사이는 속도에 따라 부드럽게 바뀐다
	 */
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") bool bShowWaves = true;
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") FLinearColor WaveColor = FLinearColor(0.4f, 0.9f, 1.f, 1.f);
	/** 링이 생길 때(가장 클 때)의 반지름 (cm). 0이면 드론 모델 크기에서 자동으로 구한다 */
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float WaveRadius = 0.f;
	/** 자동일 때 모델 반지름에 곱하는 값. 1보다 작아야 링이 드론보다 작게 시작한다 */
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float WaveRadiusScale = 0.8f;
	/** 링이 줄어들었을 때 크기 (처음 크기의 비율) */
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float WaveEndScale = 0.1f;
	/** 링이 처음 생기는 위치: 드론 중심에서 아래(떠 있을 때) 또는 뒤(움직일 때)로 (cm) */
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float WaveDepth = 15.f;
	/** 링이 멀어지는 거리 (cm). 떠 있을 때(아래로) / 움직일 때(뒤로) */
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float WaveDrop = 40.f;
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float MovingWaveTravel = 90.f;
	/** 링 한 번이 나와서 사라지는 시간 (초). 가만히 있을 때 / 움직일 때 */
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float IdleWavePeriod = 1.8f;
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float MovingWavePeriod = 0.9f;
	/** 링의 진하기. 가만히 있을 때 / 움직일 때 */
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float IdleWaveAlpha = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float MovingWaveAlpha = 0.8f;
	/** 이 속도(cm/s) 이상이면 "움직이는 중" 값을 다 쓴다 */
	UPROPERTY(EditAnywhere, Category = "Drone|Wave") float MovingSpeedForFullWave = 300.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;

	static constexpr int32 NumWaves = 3;

	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> WaveMeshes;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> WaveMaterials;

private:
	void UpdateWaves(float DeltaSeconds);

	bool bHasWaveMaterial = false;
	float WavePhase = 0.f;
	float MaxWaveRadius = 30.f;
	FVector SmoothedVelocity = FVector::ZeroVector;
	FVector LastBackDirection = FVector(-1.f, 0.f, 0.f);
	FVector LastLocation = FVector::ZeroVector;
	TWeakObjectPtr<AResearcherCharacter> OwnerCharacter;
	bool bViewActive = false;
	bool bRise = false;
	bool bParked = false;
	FVector ParkedLocation = FVector::ZeroVector;
	FVector PendingInput = FVector::ZeroVector;
	float BobTime = 0.f;
};

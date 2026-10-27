#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FilterManager.generated.h"

class UPostProcessComponent;
class UMeshComponent;
class UMaterialInterface;

USTRUCT()
struct FSavedMaterials
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> Materials;
};

/** 레벨에 배치하면 플레이 중 1~5번 키로 화면 필터를 토글한다. */
UCLASS()
class TEST_API AFilterManager : public AActor
{
	GENERATED_BODY()

public:
	AFilterManager();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	UPostProcessComponent* PostProcess;

	/** 현재 활성화된 필터 번호 (1~5), 0이면 필터 없음 */
	int32 ActiveFilter = 0;

	void ToggleFilter(int32 Index);
	void ApplyFilter(int32 Index);

	void Filter1() { ToggleFilter(1); }
	void Filter2() { ToggleFilter(2); }
	void Filter3() { ToggleFilter(3); }
	void Filter4() { ToggleFilter(4); }
	void Filter5() { ToggleFilter(5); }

public:
	/** 6번 키: 월드의 모든 메시를 이 머티리얼로 교체 (비워두면 /Game/Materials/M_GrayShade 로드) */
	UPROPERTY(EditAnywhere, Category="Filter")
	TObjectPtr<UMaterialInterface> GrayMaterial;

private:
	UPROPERTY()
	TMap<TObjectPtr<UMeshComponent>, FSavedMaterials> SavedMaterials;

	bool bGrayShading = false;

	void ToggleGrayShading();
};

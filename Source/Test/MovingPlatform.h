#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovingPlatform.generated.h"

class UStaticMeshComponent;

/** Platform that travels back and forth between its start and a target point. Blueprint children tune the values and visuals. */
UCLASS(Blueprintable)
class TEST_API AMovingPlatform : public AActor
{
	GENERATED_BODY()

public:
	AMovingPlatform();

	/** Start moving (no effect if already moving). */
	UFUNCTION(BlueprintCallable, Category = "Platform")
	void StartMoving() { bMoving = true; }

	/** Stop in place. */
	UFUNCTION(BlueprintCallable, Category = "Platform")
	void StopMoving() { bMoving = false; }

	/** Fired each time the platform arrives at either end. bAtTarget is true at the target point, false back at the start. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Platform")
	void OnReachedEnd(bool bAtTarget);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	/** Target point relative to where the platform is placed. Drag the diamond handle in the viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform", meta = (MakeEditWidget = true))
	FVector TargetOffset = FVector(0.f, 0.f, 300.f);

	/** Travel speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform", meta = (ClampMin = "1"))
	float Speed = 200.f;

	/** Seconds to wait at each end. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform", meta = (ClampMin = "0"))
	float WaitTime = 1.f;

	/** Slow down near each end instead of a constant speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	bool bEaseInOut = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	bool bAutoStart = true;

private:
	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	/** 0 at the start point, 1 at the target. */
	float Alpha = 0.f;
	bool bForward = true;
	bool bMoving = false;
	float WaitRemaining = 0.f;
};

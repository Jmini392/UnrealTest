#include "ResearcherAnimInstance.h"
#include "GameFramework/Pawn.h"
#include "ResearcherCharacter.h"

void UResearcherAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* Pawn = TryGetPawnOwner();
	if (!Pawn)
	{
		Speed = 0.f;
		bIsMoving = false;
		bIsRunning = false;
		return;
	}

	Speed = Pawn->GetVelocity().Size2D();
	bIsMoving = Speed > 3.f;

	const AResearcherCharacter* Researcher = Cast<AResearcherCharacter>(Pawn);
	bIsRunning = bIsMoving && Researcher && Researcher->IsSprinting();
}

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ResearcherGameMode.generated.h"

/** 레벨의 World Settings > GameMode Override에 지정하면 AResearcherCharacter로 시작한다. */
UCLASS()
class TEST_API AResearcherGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AResearcherGameMode();
};

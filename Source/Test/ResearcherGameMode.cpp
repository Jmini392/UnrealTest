#include "ResearcherGameMode.h"
#include "ResearcherCharacter.h"

AResearcherGameMode::AResearcherGameMode()
{
	DefaultPawnClass = AResearcherCharacter::StaticClass();
}

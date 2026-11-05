// Wyrmhaven - wires up the camera pawn, controller and island.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "WyrmGameMode.generated.h"

class AWyrmIslandManager;

UCLASS()
class WYRMHAVEN_API AWyrmGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWyrmGameMode();

	/** Override with a Blueprint subclass to use your own art for buildings and dragons. */
	UPROPERTY(EditDefaultsOnly, Category = "Wyrmhaven")
	TSubclassOf<AWyrmIslandManager> IslandClass;

protected:
	virtual void BeginPlay() override;
};

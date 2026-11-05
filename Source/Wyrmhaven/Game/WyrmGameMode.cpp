#include "Game/WyrmGameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "World/WyrmCameraPawn.h"
#include "World/WyrmIslandManager.h"
#include "World/WyrmPlayerController.h"

AWyrmGameMode::AWyrmGameMode()
{
	DefaultPawnClass = AWyrmCameraPawn::StaticClass();
	PlayerControllerClass = AWyrmPlayerController::StaticClass();
	IslandClass = AWyrmIslandManager::StaticClass();
}

void AWyrmGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Levels don't need any setup: the island is created here unless one was placed by hand.
	if (!TActorIterator<AWyrmIslandManager>(GetWorld()) && IslandClass)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AWyrmIslandManager>(IslandClass, FTransform::Identity, Params);
	}
}

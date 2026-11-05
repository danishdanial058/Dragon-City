#include "World/WyrmIslandManager.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/WyrmGameSubsystem.h"
#include "Game/WyrmSaveGame.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WyrmDragonActor.h"
#include "World/WyrmStructureActor.h"
#include "World/WyrmVisuals.h"

AWyrmIslandManager::AWyrmIslandManager()
{
	PrimaryActorTick.bCanEverTick = true;
	StructureClass = AWyrmStructureActor::StaticClass();
	DragonClass = AWyrmDragonActor::StaticClass();

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(WyrmVisuals::CubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(WyrmVisuals::ConePath);

	const float Width = WyrmGrid::Size * WyrmGrid::CellSize + 400.f;
	const float WidthScale = Width / 100.f;

	// Grass top, clickable so taps on empty ground clear the selection.
	Ground = WyrmVisuals::MakePart(this, TEXT("Ground"), true);
	Ground->SetupAttachment(Root);
	Ground->SetStaticMesh(Cube.Object);
	Ground->SetRelativeScale3D(FVector(WidthScale, WidthScale, 2.f));
	Ground->SetRelativeLocation(FVector(0.f, 0.f, WyrmGrid::IslandTopZ - 100.f));

	// Upside-down rocky cone so the island floats in the sky.
	Underside = WyrmVisuals::MakePart(this, TEXT("Underside"), false);
	Underside->SetupAttachment(Root);
	Underside->SetStaticMesh(Cone.Object);
	Underside->SetRelativeScale3D(FVector(WidthScale * 1.2f, WidthScale * 1.2f, 40.f));
	Underside->SetRelativeRotation(FRotator(180.f, 0.f, 0.f));
	Underside->SetRelativeLocation(FVector(0.f, 0.f, WyrmGrid::IslandTopZ - 200.f - 2000.f));
}

void AWyrmIslandManager::BeginPlay()
{
	Super::BeginPlay();

	WyrmVisuals::SetColor(Ground, FLinearColor(0.28f, 0.62f, 0.22f));
	WyrmVisuals::SetColor(Underside, FLinearColor(0.36f, 0.27f, 0.20f));
	EnsureLighting();

	if (UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this))
	{
		Game->OnStateChanged.AddDynamic(this, &AWyrmIslandManager::SyncWorld);
	}
	SyncWorld();
}

void AWyrmIslandManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this))
	{
		Game->OnStateChanged.RemoveDynamic(this, &AWyrmIslandManager::SyncWorld);
	}
	Super::EndPlay(EndPlayReason);
}

void AWyrmIslandManager::EnsureLighting()
{
	if (TActorIterator<ADirectionalLight>(GetWorld()))
	{
		return; // the level already has a sun
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 3000.f), FRotator(-50.f, -35.f, 0.f), Params);
}

void AWyrmIslandManager::SyncWorld()
{
	UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this);
	const UWyrmSaveGame* State = Game ? Game->GetState() : nullptr;
	UWorld* World = GetWorld();
	if (!State || !World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	// Buildings.
	TSet<FGuid> Alive;
	for (const FWyrmStructure& Data : State->Structures)
	{
		Alive.Add(Data.Id);
		TObjectPtr<AWyrmStructureActor>& Actor = StructureActors.FindOrAdd(Data.Id);
		if (!IsValid(Actor))
		{
			Actor = World->SpawnActor<AWyrmStructureActor>(StructureClass, WyrmGrid::CellToWorld(Data.Cell), FRotator::ZeroRotator, Params);
		}
		if (Actor)
		{
			Actor->ApplyData(Data, *Game);
		}
	}
	for (auto It = StructureActors.CreateIterator(); It; ++It)
	{
		if (!Alive.Contains(It.Key()))
		{
			if (IsValid(It.Value()))
			{
				It.Value()->Destroy();
			}
			It.RemoveCurrent();
		}
	}

	// Dragons fly around their habitat.
	Alive.Reset();
	for (const FWyrmDragon& Data : State->Dragons)
	{
		const FWyrmStructure* Home = Game->FindStructure(Data.HabitatId);
		const Wyrm::SpeciesDef* Species = Game->FindSpeciesOf(Data);
		if (!Home || !Species)
		{
			continue;
		}
		Alive.Add(Data.Id);
		TObjectPtr<AWyrmDragonActor>& Actor = DragonActors.FindOrAdd(Data.Id);
		const FVector HomeLocation = WyrmGrid::CellToWorld(Home->Cell);
		if (!IsValid(Actor))
		{
			Actor = World->SpawnActor<AWyrmDragonActor>(DragonClass, HomeLocation, FRotator::ZeroRotator, Params);
		}
		if (Actor)
		{
			Actor->ApplyData(Data.Id, *Species, Data.Level, HomeLocation);
		}
	}
	for (auto It = DragonActors.CreateIterator(); It; ++It)
	{
		if (!Alive.Contains(It.Key()))
		{
			if (IsValid(It.Value()))
			{
				It.Value()->Destroy();
			}
			It.RemoveCurrent();
		}
	}
}

void AWyrmIslandManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Timers and gold counters change every second even when nothing else happens.
	StatusTimer -= DeltaSeconds;
	if (StatusTimer > 0.f)
	{
		return;
	}
	StatusTimer = 0.5f;
	if (const UWyrmGameSubsystem* Game = UWyrmGameSubsystem::Get(this))
	{
		for (const TPair<FGuid, TObjectPtr<AWyrmStructureActor>>& Pair : StructureActors)
		{
			if (IsValid(Pair.Value))
			{
				Pair.Value->RefreshStatus(*Game);
			}
		}
	}
}

// Wyrmhaven - builds the floating island and keeps building/dragon actors in sync with the save.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "WyrmIslandManager.generated.h"

class AWyrmDragonActor;
class AWyrmStructureActor;
class UStaticMeshComponent;

UCLASS()
class WYRMHAVEN_API AWyrmIslandManager : public AActor
{
	GENERATED_BODY()

public:
	AWyrmIslandManager();

	virtual void Tick(float DeltaSeconds) override;

	/** Spawns, moves or removes actors so the world matches the saved state. */
	UFUNCTION()
	void SyncWorld();

	/** Override these in a Blueprint subclass to use your own art. */
	UPROPERTY(EditDefaultsOnly, Category = "Wyrmhaven")
	TSubclassOf<AWyrmStructureActor> StructureClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wyrmhaven")
	TSubclassOf<AWyrmDragonActor> DragonClass;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> Ground;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> Underside;

private:
	void EnsureLighting();

	UPROPERTY()
	TMap<FGuid, TObjectPtr<AWyrmStructureActor>> StructureActors;

	UPROPERTY()
	TMap<FGuid, TObjectPtr<AWyrmDragonActor>> DragonActors;

	float StatusTimer = 0.f;
};

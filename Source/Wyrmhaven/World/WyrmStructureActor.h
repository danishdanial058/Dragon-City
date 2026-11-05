// Wyrmhaven - visual for one building on the island (habitat, farm, breeding cave, hatchery).
#pragma once

#include "CoreMinimal.h"
#include "Game/WyrmTypes.h"
#include "GameFramework/Actor.h"

#include "WyrmStructureActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWyrmGameSubsystem;

UCLASS()
class WYRMHAVEN_API AWyrmStructureActor : public AActor
{
	GENERATED_BODY()

public:
	AWyrmStructureActor();

	/** Applies the saved data (type, element colour, position). Safe to call repeatedly. */
	void ApplyData(const FWyrmStructure& Data, const UWyrmGameSubsystem& Game);

	/** Updates the floating status text and growth visuals. */
	void RefreshStatus(const UWyrmGameSubsystem& Game);

	const FGuid& GetStructureId() const { return StructureId; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> Top;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeMesh;

private:
	FGuid StructureId;
	EWyrmStructureType Type = EWyrmStructureType::Habitat;
	FString Title;
	float PulseTime = 0.f;
	bool bHasReward = false;
};

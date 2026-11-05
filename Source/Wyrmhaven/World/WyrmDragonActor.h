// Wyrmhaven - a dragon flying around its habitat.
// Uses a placeholder body built from engine shapes unless the species defines a "mesh".
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rules/WyrmCatalog.h"

#include "WyrmDragonActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class WYRMHAVEN_API AWyrmDragonActor : public AActor
{
	GENERATED_BODY()

public:
	AWyrmDragonActor();

	void ApplyData(const FGuid& InDragonId, const Wyrm::SpeciesDef& Species, int32 Level, const FVector& InHome);

	const FGuid& GetDragonId() const { return DragonId; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<USceneComponent> Root;

	/** Everything that bobs and turns. */
	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> Tail;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<USceneComponent> ShoulderLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<USceneComponent> ShoulderRight;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> WingLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> WingRight;

	/** Real art, used when the species has a mesh path. */
	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UStaticMeshComponent> CustomMesh;

private:
	void PickNewTarget();

	FGuid DragonId;
	FString SpeciesId;
	int32 AppliedLevel = 0;
	FVector Home = FVector::ZeroVector;
	FVector Target = FVector::ZeroVector;
	float FlapTime = 0.f;
	float RestTimer = 0.f;
	float Speed = 160.f;
	bool bHasHome = false;
};

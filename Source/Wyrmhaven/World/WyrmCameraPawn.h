// Wyrmhaven - angled top-down camera that pans and zooms over the island.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "WyrmCameraPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class WYRMHAVEN_API AWyrmCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AWyrmCameraPawn();

	virtual void Tick(float DeltaSeconds) override;

	/** Drag the world with a finger or mouse; Delta is in screen pixels. */
	void PanByScreenDelta(const FVector2D& Delta);

	/** Keyboard style panning; Input is -1..1 on each axis (X = right, Y = forward). */
	void PanByInput(const FVector2D& Input, float DeltaSeconds);

	/** Positive zooms in. */
	void Zoom(float Amount);

	UPROPERTY(EditAnywhere, Category = "Wyrmhaven")
	float MinDistance = 1800.f;

	UPROPERTY(EditAnywhere, Category = "Wyrmhaven")
	float MaxDistance = 9500.f;

	UPROPERTY(EditAnywhere, Category = "Wyrmhaven")
	float KeyboardPanSpeed = 2500.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<USpringArmComponent> Arm;

	UPROPERTY(VisibleAnywhere, Category = "Wyrmhaven")
	TObjectPtr<UCameraComponent> Camera;

private:
	void MoveClamped(const FVector& Offset);

	float TargetDistance = 5500.f;
};

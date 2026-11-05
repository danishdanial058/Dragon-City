// Wyrmhaven - touch and mouse controls: drag to pan, pinch / wheel to zoom, tap to select.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "WyrmPlayerController.generated.h"

class AWyrmCameraPawn;
class UWyrmHUDWidget;

UCLASS()
class WYRMHAVEN_API AWyrmPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWyrmPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	UWyrmHUDWidget* GetHud() const { return Hud; }

	/** Set to a Widget Blueprint that derives from UWyrmHUDWidget to restyle the interface. */
	UPROPERTY(EditDefaultsOnly, Category = "Wyrmhaven")
	TSubclassOf<UWyrmHUDWidget> HudClass;

protected:
	virtual void BeginPlay() override;

private:
	void HandlePointer(bool bDown, const FVector2D& Position);
	void SelectAt(const FVector2D& ScreenPosition);
	AWyrmCameraPawn* GetCameraPawn() const;

	UPROPERTY()
	TObjectPtr<UWyrmHUDWidget> Hud;

	bool bPointerDown = false;
	bool bDragging = false;
	FVector2D PressPosition = FVector2D::ZeroVector;
	FVector2D LastPosition = FVector2D::ZeroVector;
	float LastPinchDistance = 0.f;
};

#include "World/WyrmPlayerController.h"

#include "Engine/HitResult.h"
#include "InputCoreTypes.h"
#include "UI/WyrmHUDWidget.h"
#include "World/WyrmCameraPawn.h"
#include "World/WyrmDragonActor.h"
#include "World/WyrmStructureActor.h"

namespace
{
constexpr float GWyrmDragThresholdPixels = 12.f;
}

AWyrmPlayerController::AWyrmPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false; // selection is done by hand in PlayerTick
	bEnableTouchEvents = false;
	HudClass = UWyrmHUDWidget::StaticClass();
}

void AWyrmPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);

	if (HudClass)
	{
		Hud = CreateWidget<UWyrmHUDWidget>(this, HudClass);
		if (Hud)
		{
			Hud->AddToViewport();
		}
	}
}

AWyrmCameraPawn* AWyrmPlayerController::GetCameraPawn() const
{
	return Cast<AWyrmCameraPawn>(GetPawn());
}

void AWyrmPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	AWyrmCameraPawn* CameraPawn = GetCameraPawn();
	if (!CameraPawn)
	{
		return;
	}

	// Keyboard: WASD / arrows to pan, Q/E or +/- to zoom.
	FVector2D KeyInput = FVector2D::ZeroVector;
	KeyInput.X += (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right)) ? 1.f : 0.f;
	KeyInput.X -= (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left)) ? 1.f : 0.f;
	KeyInput.Y += (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up)) ? 1.f : 0.f;
	KeyInput.Y -= (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down)) ? 1.f : 0.f;
	CameraPawn->PanByInput(KeyInput, DeltaTime);

	if (WasInputKeyJustPressed(EKeys::MouseScrollUp) || WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::Add))
	{
		CameraPawn->Zoom(1.f);
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown) || WasInputKeyJustPressed(EKeys::Q) || WasInputKeyJustPressed(EKeys::Subtract))
	{
		CameraPawn->Zoom(-1.f);
	}

	// Two fingers: pinch zoom.
	float X1 = 0.f, Y1 = 0.f, X2 = 0.f, Y2 = 0.f;
	bool bTouch1 = false, bTouch2 = false;
	GetInputTouchState(ETouchIndex::Touch1, X1, Y1, bTouch1);
	GetInputTouchState(ETouchIndex::Touch2, X2, Y2, bTouch2);
	if (bTouch1 && bTouch2)
	{
		const float Distance = static_cast<float>(FVector2D::Distance(FVector2D(X1, Y1), FVector2D(X2, Y2)));
		if (LastPinchDistance > 0.f)
		{
			CameraPawn->Zoom((Distance - LastPinchDistance) * 0.02f);
		}
		LastPinchDistance = Distance;
		bDragging = true; // a pinch never counts as a tap
		bPointerDown = true;
		LastPosition = FVector2D(X1, Y1);
		return;
	}
	LastPinchDistance = 0.f;

	// One finger or the left mouse button: drag to pan, tap to select.
	FVector2D Position = LastPosition;
	bool bDown = false;
	if (bTouch1)
	{
		bDown = true;
		Position = FVector2D(X1, Y1);
	}
	else
	{
		bDown = IsInputKeyDown(EKeys::LeftMouseButton);
		float MouseX = 0.f, MouseY = 0.f;
		if (GetMousePosition(MouseX, MouseY))
		{
			Position = FVector2D(MouseX, MouseY);
		}
	}
	HandlePointer(bDown, Position);
}

void AWyrmPlayerController::HandlePointer(bool bDown, const FVector2D& Position)
{
	if (bDown && !bPointerDown)
	{
		bPointerDown = true;
		bDragging = false;
		PressPosition = Position;
		LastPosition = Position;
	}
	else if (bDown && bPointerDown)
	{
		if (!bDragging && FVector2D::Distance(Position, PressPosition) > GWyrmDragThresholdPixels)
		{
			bDragging = true;
		}
		if (bDragging)
		{
			if (AWyrmCameraPawn* CameraPawn = GetCameraPawn())
			{
				CameraPawn->PanByScreenDelta(Position - LastPosition);
			}
		}
		LastPosition = Position;
	}
	else if (!bDown && bPointerDown)
	{
		bPointerDown = false;
		if (!bDragging)
		{
			SelectAt(LastPosition);
		}
		bDragging = false;
	}
}

void AWyrmPlayerController::SelectAt(const FVector2D& ScreenPosition)
{
	if (!Hud)
	{
		return;
	}
	FHitResult Hit;
	if (GetHitResultAtScreenPosition(ScreenPosition, ECC_Visibility, false, Hit))
	{
		if (const AWyrmStructureActor* Structure = Cast<AWyrmStructureActor>(Hit.GetActor()))
		{
			Hud->ShowStructure(Structure->GetStructureId());
			return;
		}
		if (const AWyrmDragonActor* Dragon = Cast<AWyrmDragonActor>(Hit.GetActor()))
		{
			Hud->ShowDragon(Dragon->GetDragonId());
			return;
		}
	}
	Hud->ClosePanel();
}

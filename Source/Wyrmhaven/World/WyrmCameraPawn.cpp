#include "World/WyrmCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Game/WyrmTypes.h"
#include "GameFramework/SpringArmComponent.h"

AWyrmCameraPawn::AWyrmCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	Arm->SetupAttachment(Root);
	Arm->SetRelativeRotation(FRotator(-52.f, 45.f, 0.f));
	Arm->TargetArmLength = TargetDistance;
	Arm->bDoCollisionTest = false;
	Arm->bEnableCameraLag = true;
	Arm->CameraLagSpeed = 12.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Arm, USpringArmComponent::SocketName);
	Camera->SetFieldOfView(50.f);
}

void AWyrmCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(FVector(0.f, 0.f, WyrmGrid::IslandTopZ));
	SetActorRotation(FRotator::ZeroRotator);
}

void AWyrmCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, TargetDistance, DeltaSeconds, 10.f);
}

void AWyrmCameraPawn::MoveClamped(const FVector& Offset)
{
	const double Limit = WyrmGrid::Size * WyrmGrid::CellSize * 0.5;
	FVector Location = GetActorLocation() + Offset;
	Location.X = FMath::Clamp(Location.X, -Limit, Limit);
	Location.Y = FMath::Clamp(Location.Y, -Limit, Limit);
	SetActorLocation(Location);
}

void AWyrmCameraPawn::PanByScreenDelta(const FVector2D& Delta)
{
	const FRotator Yaw(0.f, Arm->GetRelativeRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	// Roughly keep the ground under the finger: farther away = bigger steps.
	const float UnitsPerPixel = Arm->TargetArmLength * 0.0012f;
	MoveClamped((-Right * Delta.X + Forward * Delta.Y) * UnitsPerPixel);
}

void AWyrmCameraPawn::PanByInput(const FVector2D& Input, float DeltaSeconds)
{
	if (Input.IsNearlyZero())
	{
		return;
	}
	const FRotator Yaw(0.f, Arm->GetRelativeRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	const float Speed = KeyboardPanSpeed * (Arm->TargetArmLength / 5500.f);
	MoveClamped((Right * Input.X + Forward * Input.Y) * Speed * DeltaSeconds);
}

void AWyrmCameraPawn::Zoom(float Amount)
{
	TargetDistance = FMath::Clamp(TargetDistance * FMath::Pow(0.9f, Amount), MinDistance, MaxDistance);
}

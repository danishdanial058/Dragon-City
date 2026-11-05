#include "World/WyrmStructureActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/WyrmGameSubsystem.h"
#include "Game/WyrmSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Rules/WyrmProgression.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WyrmVisuals.h"

AWyrmStructureActor::AWyrmStructureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Base = WyrmVisuals::MakePart(this, TEXT("Base"), true);
	Base->SetupAttachment(Root);
	Top = WyrmVisuals::MakePart(this, TEXT("Top"), true);
	Top->SetupAttachment(Root);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextBottom);
	Label->SetWorldSize(60.f);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 520.f));
	Label->SetTextRenderColor(FColor::White);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(WyrmVisuals::CubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(WyrmVisuals::CylinderPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(WyrmVisuals::SpherePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(WyrmVisuals::ConePath);
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	SphereMesh = Sphere.Object;
	ConeMesh = Cone.Object;
}

void AWyrmStructureActor::ApplyData(const FWyrmStructure& Data, const UWyrmGameSubsystem& Game)
{
	StructureId = Data.Id;
	Type = Data.Type;
	SetActorLocation(WyrmGrid::CellToWorld(Data.Cell));

	// Engine shapes are 100 units across with the pivot in the middle.
	switch (Data.Type)
	{
	case EWyrmStructureType::Habitat:
	{
		const Wyrm::HabitatDef* Def = Game.GetCatalog().FindHabitat(WyrmText::ToStd(Data.DefId));
		const FLinearColor Color = WyrmVisuals::ElementColor(Def ? Def->Elem : Wyrm::Element::Fire);
		Title = Def ? WyrmText::ToF(Def->Name) : TEXT("Habitat");
		Base->SetStaticMesh(CylinderMesh);
		Base->SetRelativeScale3D(FVector(5.4f, 5.4f, 0.3f));
		Base->SetRelativeLocation(FVector(0.f, 0.f, 15.f));
		WyrmVisuals::SetColor(Base, Color * 0.6f);
		Top->SetStaticMesh(CubeMesh);
		Top->SetRelativeScale3D(FVector(1.6f, 1.6f, 1.4f));
		Top->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
		WyrmVisuals::SetColor(Top, Color);
		break;
	}
	case EWyrmStructureType::Farm:
		Title = TEXT("Farm");
		Base->SetStaticMesh(CubeMesh);
		Base->SetRelativeScale3D(FVector(5.6f, 5.6f, 0.2f));
		Base->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
		WyrmVisuals::SetColor(Base, FLinearColor(0.35f, 0.22f, 0.10f));
		Top->SetStaticMesh(CubeMesh);
		WyrmVisuals::SetColor(Top, FLinearColor(0.45f, 0.85f, 0.20f));
		break;
	case EWyrmStructureType::BreedingCave:
		Title = TEXT("Breeding Cave");
		Base->SetStaticMesh(CylinderMesh);
		Base->SetRelativeScale3D(FVector(5.4f, 5.4f, 0.3f));
		Base->SetRelativeLocation(FVector(0.f, 0.f, 15.f));
		WyrmVisuals::SetColor(Base, FLinearColor(0.30f, 0.30f, 0.34f));
		Top->SetStaticMesh(ConeMesh);
		Top->SetRelativeScale3D(FVector(4.2f, 4.2f, 3.2f));
		Top->SetRelativeLocation(FVector(0.f, 0.f, 190.f));
		WyrmVisuals::SetColor(Top, FLinearColor(0.45f, 0.20f, 0.55f));
		break;
	case EWyrmStructureType::Hatchery:
		Title = TEXT("Hatchery");
		Base->SetStaticMesh(CylinderMesh);
		Base->SetRelativeScale3D(FVector(5.4f, 5.4f, 0.3f));
		Base->SetRelativeLocation(FVector(0.f, 0.f, 15.f));
		WyrmVisuals::SetColor(Base, FLinearColor(0.85f, 0.65f, 0.20f));
		Top->SetStaticMesh(SphereMesh);
		Top->SetRelativeScale3D(FVector(2.4f, 2.4f, 3.2f));
		Top->SetRelativeLocation(FVector(0.f, 0.f, 190.f));
		WyrmVisuals::SetColor(Top, FLinearColor(1.0f, 0.95f, 0.85f));
		break;
	}
	RefreshStatus(Game);
}

void AWyrmStructureActor::RefreshStatus(const UWyrmGameSubsystem& Game)
{
	const FWyrmStructure* Data = Game.FindStructure(StructureId);
	const UWyrmSaveGame* State = Game.GetState();
	if (!Data || !State)
	{
		return;
	}

	FString Status;
	bHasReward = false;
	switch (Data->Type)
	{
	case EWyrmStructureType::Habitat:
	{
		const Wyrm::HabitatDef* Def = Game.GetCatalog().FindHabitat(WyrmText::ToStd(Data->DefId));
		const int64 Gold = Game.GetHabitatGold(StructureId);
		Status = FString::Printf(TEXT("%s gold"), *WyrmText::Number(Gold));
		bHasReward = Def && Gold >= FMath::Max<int64>(1, Def->GoldCap / 4);
		break;
	}
	case EWyrmStructureType::Farm:
	{
		const Wyrm::CropDef* Crop = Game.GetCatalog().FindCrop(WyrmText::ToStd(Data->CropId));
		float Growth = 0.f;
		if (!Crop)
		{
			Status = TEXT("Empty - tap to plant");
		}
		else
		{
			const double Remaining = UWyrmGameSubsystem::SecondsUntil(Data->ReadyAtUtc);
			Growth = Crop->GrowSeconds > 0 ? 1.f - FMath::Clamp(static_cast<float>(Remaining / Crop->GrowSeconds), 0.f, 1.f) : 1.f;
			Status = Remaining > 0.0 ? WyrmText::ToF(Crop->Name) + TEXT(" ") + WyrmText::Duration(Remaining) : TEXT("Ready to harvest!");
			bHasReward = Remaining <= 0.0;
		}
		Top->SetVisibility(Crop != nullptr);
		Top->SetRelativeScale3D(FVector(4.6f, 4.6f, FMath::Max(0.05f, Growth * 0.8f)));
		Top->SetRelativeLocation(FVector(0.f, 0.f, 20.f + Growth * 40.f));
		break;
	}
	case EWyrmStructureType::BreedingCave:
		if (!State->Breeding.bActive)
		{
			Status = TEXT("Idle - tap to breed");
		}
		else
		{
			const double Remaining = UWyrmGameSubsystem::SecondsUntil(State->Breeding.ReadyAtUtc);
			Status = Remaining > 0.0 ? WyrmText::Duration(Remaining) : TEXT("Egg ready!");
			bHasReward = Remaining <= 0.0;
		}
		break;
	case EWyrmStructureType::Hatchery:
	{
		int32 Ready = 0;
		for (const FWyrmEgg& Egg : State->Eggs)
		{
			Ready += UWyrmGameSubsystem::SecondsUntil(Egg.HatchAtUtc) <= 0.0 ? 1 : 0;
		}
		Status = FString::Printf(TEXT("%d/%d eggs"), State->Eggs.Num(), Wyrm::MaxEggsInHatchery);
		if (Ready > 0)
		{
			Status += FString::Printf(TEXT(" - %d ready!"), Ready);
			bHasReward = true;
		}
		break;
	}
	}

	Label->SetText(FText::FromString(Title + TEXT("\n") + Status));
	Label->SetTextRenderColor(bHasReward ? FColor(255, 220, 60) : FColor::White);
}

void AWyrmStructureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Keep the label facing the camera.
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCamera = Camera->GetCameraLocation() - Label->GetComponentLocation();
		Label->SetWorldRotation(ToCamera.Rotation());
	}

	// Things that need attention gently bob.
	PulseTime += DeltaSeconds;
	const float Bob = bHasReward ? FMath::Sin(PulseTime * 4.f) * 20.f : 0.f;
	Label->SetRelativeLocation(FVector(0.f, 0.f, 520.f + Bob));
}

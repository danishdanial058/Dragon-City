#include "World/WyrmDragonActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/WyrmTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WyrmVisuals.h"

AWyrmDragonActor::AWyrmDragonActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(WyrmVisuals::SpherePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(WyrmVisuals::CubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(WyrmVisuals::ConePath);

	// Placeholder dragon, facing +X. Only the body is clickable.
	Body = WyrmVisuals::MakePart(this, TEXT("Body"), true);
	Body->SetupAttachment(Pivot);
	Body->SetStaticMesh(Sphere.Object);
	Body->SetRelativeScale3D(FVector(1.3f, 0.8f, 0.8f));

	Head = WyrmVisuals::MakePart(this, TEXT("Head"), false);
	Head->SetupAttachment(Pivot);
	Head->SetStaticMesh(Sphere.Object);
	Head->SetRelativeLocation(FVector(75.f, 0.f, 45.f));
	Head->SetRelativeScale3D(FVector(0.6f, 0.5f, 0.5f));

	Tail = WyrmVisuals::MakePart(this, TEXT("Tail"), false);
	Tail->SetupAttachment(Pivot);
	Tail->SetStaticMesh(Cone.Object);
	Tail->SetRelativeLocation(FVector(-90.f, 0.f, 0.f));
	Tail->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Tail->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.9f));

	ShoulderLeft = CreateDefaultSubobject<USceneComponent>(TEXT("ShoulderLeft"));
	ShoulderLeft->SetupAttachment(Pivot);
	ShoulderLeft->SetRelativeLocation(FVector(0.f, -25.f, 30.f));
	ShoulderRight = CreateDefaultSubobject<USceneComponent>(TEXT("ShoulderRight"));
	ShoulderRight->SetupAttachment(Pivot);
	ShoulderRight->SetRelativeLocation(FVector(0.f, 25.f, 30.f));

	WingLeft = WyrmVisuals::MakePart(this, TEXT("WingLeft"), false);
	WingLeft->SetupAttachment(ShoulderLeft);
	WingLeft->SetStaticMesh(Cube.Object);
	WingLeft->SetRelativeLocation(FVector(0.f, -55.f, 0.f));
	WingLeft->SetRelativeScale3D(FVector(0.7f, 1.1f, 0.06f));

	WingRight = WyrmVisuals::MakePart(this, TEXT("WingRight"), false);
	WingRight->SetupAttachment(ShoulderRight);
	WingRight->SetStaticMesh(Cube.Object);
	WingRight->SetRelativeLocation(FVector(0.f, 55.f, 0.f));
	WingRight->SetRelativeScale3D(FVector(0.7f, 1.1f, 0.06f));

	CustomMesh = WyrmVisuals::MakePart(this, TEXT("CustomMesh"), true);
	CustomMesh->SetupAttachment(Pivot);
	CustomMesh->SetVisibility(false);
}

void AWyrmDragonActor::ApplyData(const FGuid& InDragonId, const Wyrm::SpeciesDef& Species, int32 Level, const FVector& InHome)
{
	DragonId = InDragonId;
	const FString NewSpecies = WyrmText::ToF(Species.Id);
	const bool bSpeciesChanged = NewSpecies != SpeciesId;
	SpeciesId = NewSpecies;

	if (!bHasHome || !Home.Equals(InHome, 1.f))
	{
		Home = InHome;
		if (!bHasHome)
		{
			SetActorLocation(Home + FVector(0.f, 0.f, 250.f));
		}
		bHasHome = true;
		PickNewTarget();
	}

	if (bSpeciesChanged)
	{
		UStaticMesh* Art = nullptr;
		if (!Species.Mesh.empty())
		{
			Art = LoadObject<UStaticMesh>(nullptr, *WyrmText::ToF(Species.Mesh));
		}
		const bool bUseArt = Art != nullptr;
		CustomMesh->SetStaticMesh(Art);
		CustomMesh->SetVisibility(bUseArt);
		CustomMesh->SetCollisionEnabled(bUseArt ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		for (UStaticMeshComponent* Part : {Body.Get(), Head.Get(), Tail.Get(), WingLeft.Get(), WingRight.Get()})
		{
			Part->SetVisibility(!bUseArt);
		}
		Body->SetCollisionEnabled(bUseArt ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);

		const FLinearColor Main = WyrmVisuals::ElementColor(Wyrm::PrimaryElement(Species.Elements));
		const FLinearColor Accent = WyrmVisuals::ElementColor(Wyrm::SecondaryElement(Species.Elements));
		WyrmVisuals::SetColor(Body, Main);
		WyrmVisuals::SetColor(Head, Main);
		WyrmVisuals::SetColor(Tail, Accent);
		WyrmVisuals::SetColor(WingLeft, Accent);
		WyrmVisuals::SetColor(WingRight, Accent);

		// Rarer dragons are a little bigger and faster.
		Speed = 140.f + 30.f * static_cast<float>(Species.Rar);
	}

	if (Level != AppliedLevel)
	{
		AppliedLevel = Level;
		const float Scale = 0.55f + 0.012f * static_cast<float>(Level);
		Pivot->SetRelativeScale3D(FVector(Scale));
	}
}

void AWyrmDragonActor::PickNewTarget()
{
	const float Angle = FMath::FRandRange(0.f, 2.f * UE_PI);
	const float Radius = FMath::FRandRange(120.f, 300.f);
	Target = Home + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, FMath::FRandRange(220.f, 380.f));
	RestTimer = FMath::FRandRange(0.5f, 2.5f);
}

void AWyrmDragonActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FlapTime += DeltaSeconds;

	const FVector Location = GetActorLocation();
	const FVector ToTarget = Target - Location;
	const double Distance = ToTarget.Size();
	if (Distance > 10.0)
	{
		const FVector Step = ToTarget / Distance * FMath::Min(Distance, static_cast<double>(Speed * DeltaSeconds));
		SetActorLocation(Location + Step);
		const FRotator Facing(0.f, ToTarget.Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Facing, DeltaSeconds, 4.f));
	}
	else
	{
		RestTimer -= DeltaSeconds;
		if (RestTimer <= 0.f)
		{
			PickNewTarget();
		}
	}

	// Flap and bob.
	const float Flap = FMath::Sin(FlapTime * 9.f) * 35.f;
	ShoulderLeft->SetRelativeRotation(FRotator(0.f, 0.f, -Flap));
	ShoulderRight->SetRelativeRotation(FRotator(0.f, 0.f, Flap));
	Pivot->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(FlapTime * 2.f) * 12.f));
}

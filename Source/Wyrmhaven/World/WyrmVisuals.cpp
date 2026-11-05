#include "World/WyrmVisuals.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace WyrmVisuals
{
FLinearColor ElementColor(Wyrm::Element E)
{
	switch (E)
	{
	case Wyrm::Element::Fire:
		return FLinearColor(0.95f, 0.30f, 0.08f);
	case Wyrm::Element::Water:
		return FLinearColor(0.10f, 0.45f, 0.95f);
	case Wyrm::Element::Earth:
		return FLinearColor(0.55f, 0.36f, 0.18f);
	case Wyrm::Element::Air:
		return FLinearColor(0.70f, 0.90f, 0.98f);
	case Wyrm::Element::Nature:
		return FLinearColor(0.20f, 0.75f, 0.25f);
	case Wyrm::Element::Lightning:
		return FLinearColor(1.00f, 0.85f, 0.10f);
	case Wyrm::Element::Ice:
		return FLinearColor(0.60f, 0.95f, 1.00f);
	case Wyrm::Element::Metal:
		return FLinearColor(0.55f, 0.58f, 0.62f);
	case Wyrm::Element::Shadow:
		return FLinearColor(0.25f, 0.10f, 0.40f);
	case Wyrm::Element::Light:
		return FLinearColor(1.00f, 0.97f, 0.80f);
	default:
		return FLinearColor::White;
	}
}

void SetColor(UStaticMeshComponent* Component, const FLinearColor& Color)
{
	if (!Component)
	{
		return;
	}
	if (UMaterialInstanceDynamic* Material = Component->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

UStaticMeshComponent* MakePart(UObject* Outer, FName Name, bool bCollide)
{
	UStaticMeshComponent* Part = Outer->CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (bCollide)
	{
		Part->SetCollisionResponseToAllChannels(ECR_Ignore);
		Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	Part->SetCanEverAffectNavigation(false);
	Part->SetGenerateOverlapEvents(false);
	return Part;
}
} // namespace WyrmVisuals

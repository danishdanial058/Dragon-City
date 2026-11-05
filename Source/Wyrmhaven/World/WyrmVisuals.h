// Wyrmhaven - colours and placeholder model helpers.
// Every model in the prototype is built from engine basic shapes, so the game runs with zero art.
// Give a species a "mesh" path in GameData.json to replace its placeholder with real art.
#pragma once

#include "CoreMinimal.h"
#include "Rules/WyrmElements.h"

class UStaticMeshComponent;

namespace WyrmVisuals
{
inline const TCHAR* const CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
inline const TCHAR* const CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
inline const TCHAR* const SpherePath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
inline const TCHAR* const ConePath = TEXT("/Engine/BasicShapes/Cone.Cone");

WYRMHAVEN_API FLinearColor ElementColor(Wyrm::Element E);

/** Tints a component that uses the engine BasicShapeMaterial (vector parameter "Color"). */
WYRMHAVEN_API void SetColor(UStaticMeshComponent* Component, const FLinearColor& Color);

/** Creates a static mesh sub-object with no collision unless bCollide is set. */
WYRMHAVEN_API UStaticMeshComponent* MakePart(UObject* Outer, FName Name, bool bCollide);
} // namespace WyrmVisuals

// Wyrmhaven - saved state types and small helpers shared by the Unreal layer.
#pragma once

#include "CoreMinimal.h"
#include "Rules/WyrmElements.h"

#include <string>

#include "WyrmTypes.generated.h"

UENUM(BlueprintType)
enum class EWyrmStructureType : uint8
{
	Habitat,
	Farm,
	BreedingCave,
	Hatchery
};

/** One dragon the player owns. */
USTRUCT(BlueprintType)
struct FWyrmDragon
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FGuid Id;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FString SpeciesId;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FString Nickname;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	int32 Level = 1;

	/** Habitat the dragon lives in. */
	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FGuid HabitatId;
};

/** A building on the island grid. */
USTRUCT(BlueprintType)
struct FWyrmStructure
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FGuid Id;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	EWyrmStructureType Type = EWyrmStructureType::Habitat;

	/** Habitat definition id (habitats only). */
	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FString DefId;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FIntPoint Cell = FIntPoint::ZeroValue;

	/** Habitats: gold stored before LastCollectUtc (kept when dragons move in or out). */
	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	int64 BankedGold = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FDateTime LastCollectUtc;

	/** Farms: the crop growing, empty when idle. */
	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FString CropId;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FDateTime ReadyAtUtc;
};

USTRUCT(BlueprintType)
struct FWyrmBreedingJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FGuid ParentA;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FGuid ParentB;

	/** Rolled when breeding starts and revealed when the egg is collected. */
	UPROPERTY()
	FString ResultSpeciesId;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FDateTime ReadyAtUtc;
};

USTRUCT(BlueprintType)
struct FWyrmEgg
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FGuid Id;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FString SpeciesId;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FDateTime HatchAtUtc;
};

/** One line of the breeding odds table shown before breeding. */
USTRUCT(BlueprintType)
struct FWyrmBreedOddsEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FString SpeciesId;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FString Rarity;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	float Probability = 0.f;
};

namespace WyrmGrid
{
inline constexpr int32 Size = 10;
inline constexpr float CellSize = 700.f;
inline constexpr float IslandTopZ = 20.f;

inline bool IsInside(const FIntPoint& Cell)
{
	return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Size && Cell.Y < Size;
}

inline FVector CellToWorld(const FIntPoint& Cell)
{
	const float Half = Size * 0.5f;
	return FVector((Cell.X - Half + 0.5f) * CellSize, (Cell.Y - Half + 0.5f) * CellSize, IslandTopZ);
}
} // namespace WyrmGrid

namespace WyrmText
{
inline std::string ToStd(const FString& In)
{
	return std::string(TCHAR_TO_UTF8(*In));
}

inline FString ToF(const std::string& In)
{
	return FString(UTF8_TO_TCHAR(In.c_str()));
}

/** "1h 05m", "3m 20s", "12s". */
WYRMHAVEN_API FString Duration(double Seconds);

/** "Fire / Water". */
WYRMHAVEN_API FString Elements(Wyrm::ElementMask Mask);

/** "12,345". */
WYRMHAVEN_API FString Number(int64 Value);
} // namespace WyrmText

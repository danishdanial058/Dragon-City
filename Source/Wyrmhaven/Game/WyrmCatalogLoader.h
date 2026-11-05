// Wyrmhaven - reads Content/Data/GameData.json into a Wyrm::Catalog.
#pragma once

#include "CoreMinimal.h"
#include "Rules/WyrmCatalog.h"

#include <string>
#include <vector>

/** Values from the "newGame" and "buildings" sections of the data file. */
struct FWyrmStartingSetup
{
	int64 Gold = 5000;
	int64 Food = 200;
	int64 Gems = 30;
	int64 FarmCost = 300;
	int32 StarterFarms = 2;
	TArray<FString> StarterDragons;
	TArray<FString> StarterHabitats;
};

namespace WyrmCatalogLoader
{
/** Default location: <Project>/Content/Data/GameData.json. */
WYRMHAVEN_API FString DefaultDataPath();

/** Parses JSON text. Returns false and fills OutError on failure. */
WYRMHAVEN_API bool ParseCatalog(const FString& JsonText, Wyrm::Catalog& OutCatalog, FWyrmStartingSetup& OutSetup,
	FString& OutError);

WYRMHAVEN_API bool LoadCatalog(const FString& Path, Wyrm::Catalog& OutCatalog, FWyrmStartingSetup& OutSetup,
	FString& OutError);
} // namespace WyrmCatalogLoader

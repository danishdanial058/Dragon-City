// Wyrmhaven - everything that persists between sessions.
#pragma once

#include "CoreMinimal.h"
#include "Game/WyrmTypes.h"
#include "GameFramework/SaveGame.h"

#include "WyrmSaveGame.generated.h"

UCLASS()
class WYRMHAVEN_API UWyrmSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 SaveVersion = CurrentVersion;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	int64 Gold = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	int64 Food = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	int64 Gems = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	int32 PlayerLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	int64 PlayerXp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	int32 ArenaTier = 1;

	/** Unlucky breeds in a row; boosts Epic and Legendary odds. */
	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	int32 BreedPity = 0;

	/** Random number generator state, so reloading cannot be used to re-roll results. */
	UPROPERTY()
	int64 RngState = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	TArray<FWyrmDragon> Dragons;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	TArray<FWyrmStructure> Structures;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FWyrmBreedingJob Breeding;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	TArray<FWyrmEgg> Eggs;

	/** Species the player has ever owned (the "Dragonpedia"). */
	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	TArray<FString> Discovered;

	UPROPERTY(BlueprintReadOnly, Category = "Wyrmhaven")
	FDateTime LastSavedUtc;
};

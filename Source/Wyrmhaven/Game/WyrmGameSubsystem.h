// Wyrmhaven - owns the player's island and applies every game action.
//
// The UI and world actors only call functions on this subsystem and listen to OnStateChanged.
// All rules (odds, damage, formulas) come from the engine-independent code in Source/Wyrmhaven/Rules.
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Game/WyrmCatalogLoader.h"
#include "Game/WyrmTypes.h"
#include "Rules/WyrmBattle.h"
#include "Rules/WyrmCatalog.h"
#include "Rules/WyrmRng.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/UniquePtr.h"

#include "WyrmGameSubsystem.generated.h"

class UWyrmSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWyrmStateChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWyrmMessageSignature, const FText&, Message, bool, bIsError);

UCLASS()
class WYRMHAVEN_API UWyrmGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UWyrmGameSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Fired after any change to the saved state. */
	UPROPERTY(BlueprintAssignable, Category = "Wyrmhaven")
	FWyrmStateChangedSignature OnStateChanged;

	/** Short feedback for the player ("Not enough gold", "You got a Steamtail!"). */
	UPROPERTY(BlueprintAssignable, Category = "Wyrmhaven")
	FWyrmMessageSignature OnMessage;

	// ---- Data access -------------------------------------------------------------------------
	const Wyrm::Catalog& GetCatalog() const { return Catalog; }
	bool IsDataLoaded() const { return bDataLoaded; }
	const FString& GetDataError() const { return DataError; }
	const FWyrmStartingSetup& GetSetup() const { return Setup; }

	UFUNCTION(BlueprintPure, Category = "Wyrmhaven")
	UWyrmSaveGame* GetState() const { return State; }

	const FWyrmDragon* FindDragon(const FGuid& Id) const;
	const FWyrmStructure* FindStructure(const FGuid& Id) const;
	const Wyrm::SpeciesDef* FindSpeciesOf(const FWyrmDragon& Dragon) const;
	TArray<const FWyrmDragon*> GetDragonsIn(const FGuid& HabitatId) const;
	int32 CountStructures(EWyrmStructureType Type) const;
	int32 GetMaxStructures() const;
	FString GetDisplayName(const FWyrmDragon& Dragon) const;

	UFUNCTION(BlueprintPure, Category = "Wyrmhaven")
	int64 GetHabitatGold(const FGuid& HabitatId) const;

	UFUNCTION(BlueprintPure, Category = "Wyrmhaven")
	int32 GetHabitatGoldPerMinute(const FGuid& HabitatId) const;

	UFUNCTION(BlueprintPure, Category = "Wyrmhaven")
	static double SecondsUntil(const FDateTime& Utc);

	// ---- Building ----------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool BuildHabitat(const FString& HabitatDefId);

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool BuildFarm();

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool MoveStructure(const FGuid& StructureId, FIntPoint NewCell);

	UFUNCTION(BlueprintPure, Category = "Wyrmhaven")
	bool IsCellFree(FIntPoint Cell) const;

	// ---- Gold and food -----------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool CollectHabitat(const FGuid& HabitatId);

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	int64 CollectAllGold();

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool PlantCrop(const FGuid& FarmId, const FString& CropId);

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool HarvestFarm(const FGuid& FarmId);

	// ---- Dragons -----------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool FeedDragon(const FGuid& DragonId);

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool SellDragon(const FGuid& DragonId);

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool MoveDragon(const FGuid& DragonId, const FGuid& HabitatId);

	/** Buys an egg from the shop; it goes to the hatchery. */
	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool BuyEgg(const FString& SpeciesId);

	// ---- Breeding and hatching ---------------------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "Wyrmhaven")
	TArray<FWyrmBreedOddsEntry> GetBreedOdds(const FGuid& ParentA, const FGuid& ParentB, FString& OutError) const;

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool StartBreeding(const FGuid& ParentA, const FGuid& ParentB);

	/** Moves a finished breeding egg into the hatchery. */
	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool CollectBreedingEgg();

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool HatchEgg(const FGuid& EggId);

	// ---- Speed-ups (1 gem per started 5 minutes) ---------------------------------------------
	UFUNCTION(BlueprintPure, Category = "Wyrmhaven")
	int64 GemCostFor(const FDateTime& ReadyAtUtc) const;

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool SpeedUpBreeding();

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool SpeedUpEgg(const FGuid& EggId);

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool SpeedUpFarm(const FGuid& FarmId);

	// ---- Arena -------------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	bool StartArenaBattle();

	/** Plays one turn and returns the lines to show in the battle log. */
	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	TArray<FString> BattleUseSkill(int32 SkillIndex);

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	void EndBattle();

	UFUNCTION(BlueprintPure, Category = "Wyrmhaven")
	bool IsBattleActive() const { return ActiveBattle.IsValid(); }

	const Wyrm::Battle* GetBattle() const { return ActiveBattle.Get(); }

	/** The dragons that would fight: the strongest three. */
	TArray<const FWyrmDragon*> GetArenaTeam() const;

	// ---- Persistence -------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	void SaveNow();

	UFUNCTION(BlueprintCallable, Category = "Wyrmhaven")
	void ResetProgress();

	static const FString SaveSlotName;

private:
	void LoadOrCreate();
	void CreateNewGame();
	void Sanitize();
	bool AutoSaveTick(float DeltaTime);

	FWyrmDragon* FindDragonMutable(const FGuid& Id);
	FWyrmStructure* FindStructureMutable(const FGuid& Id);
	FWyrmStructure* FindFirstStructure(EWyrmStructureType Type);
	const Wyrm::HabitatDef* FindHabitatDefOf(const FWyrmStructure& Structure) const;
	bool FindFreeCell(FIntPoint& OutCell) const;
	FGuid FindHomeFor(const Wyrm::SpeciesDef& Species) const;
	int32 FreeSlots(const FWyrmStructure& Habitat) const;
	FWyrmStructure& AddStructure(EWyrmStructureType Type, const FString& DefId, const FIntPoint& Cell);
	FWyrmDragon& AddDragon(const FString& SpeciesId, const FGuid& HabitatId);
	void BankHabitatGold(FWyrmStructure& Habitat);

	bool Spend(int64 Gold, int64 Gems);
	void GainXp(int64 Amount);
	bool Fail(const FText& Reason);
	void Notify(const FText& Message);
	void Changed();
	void StoreRng();

	UPROPERTY()
	TObjectPtr<UWyrmSaveGame> State;

	Wyrm::Catalog Catalog;
	FWyrmStartingSetup Setup;
	bool bDataLoaded = false;
	FString DataError;

	Wyrm::Rng Random;
	TUniquePtr<Wyrm::Battle> ActiveBattle;
	FTSTicker::FDelegateHandle AutoSaveHandle;
};

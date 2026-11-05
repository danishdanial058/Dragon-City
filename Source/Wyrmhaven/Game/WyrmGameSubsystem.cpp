#include "Game/WyrmGameSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/WyrmSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Rules/WyrmBreeding.h"
#include "Rules/WyrmProgression.h"
#include "Wyrmhaven.h"

#include <algorithm>

#define LOCTEXT_NAMESPACE "WyrmGame"

const FString UWyrmGameSubsystem::SaveSlotName(TEXT("Wyrmhaven"));

namespace
{
constexpr float GWyrmAutoSaveSeconds = 20.f;

FText WyrmName(const std::string& Name)
{
	return FText::FromString(WyrmText::ToF(Name));
}
} // namespace

UWyrmGameSubsystem* UWyrmGameSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UWyrmGameSubsystem>() : nullptr;
}

void UWyrmGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bDataLoaded = WyrmCatalogLoader::LoadCatalog(WyrmCatalogLoader::DefaultDataPath(), Catalog, Setup, DataError);
	if (!bDataLoaded)
	{
		UE_LOG(LogWyrm, Error, TEXT("%s"), *DataError);
	}
	else
	{
		UE_LOG(LogWyrm, Log, TEXT("Loaded %d species, %d skills, %d habitats, %d crops"), static_cast<int32>(Catalog.Species.size()),
			static_cast<int32>(Catalog.Skills.size()), static_cast<int32>(Catalog.Habitats.size()), static_cast<int32>(Catalog.Crops.size()));
	}

	LoadOrCreate();

	AutoSaveHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UWyrmGameSubsystem::AutoSaveTick), GWyrmAutoSaveSeconds);
}

void UWyrmGameSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(AutoSaveHandle);
	SaveNow();
	ActiveBattle.Reset();
	Super::Deinitialize();
}

bool UWyrmGameSubsystem::AutoSaveTick(float /*DeltaTime*/)
{
	SaveNow();
	return true; // keep ticking
}

// ---- Persistence ---------------------------------------------------------------------------------

void UWyrmGameSubsystem::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		State = Cast<UWyrmSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
	}
	if (!State)
	{
		CreateNewGame();
	}
	else
	{
		Random.SetState(static_cast<uint64>(State->RngState));
		if (bDataLoaded)
		{
			Sanitize();
		}
	}
}

void UWyrmGameSubsystem::CreateNewGame()
{
	State = Cast<UWyrmSaveGame>(UGameplayStatics::CreateSaveGameObject(UWyrmSaveGame::StaticClass()));
	State->Gold = Setup.Gold;
	State->Food = Setup.Food;
	State->Gems = Setup.Gems;
	Random.SetState(static_cast<uint64>(FDateTime::UtcNow().GetTicks()) ^ 0x5DEECE66DULL);

	const int32 Mid = WyrmGrid::Size / 2;
	AddStructure(EWyrmStructureType::BreedingCave, FString(), FIntPoint(Mid - 1, Mid + 2));
	AddStructure(EWyrmStructureType::Hatchery, FString(), FIntPoint(Mid + 1, Mid + 2));

	int32 Column = Mid - 2;
	TArray<FGuid> Homes;
	for (const FString& HabitatId : Setup.StarterHabitats)
	{
		if (Catalog.FindHabitat(WyrmText::ToStd(HabitatId)))
		{
			Homes.Add(AddStructure(EWyrmStructureType::Habitat, HabitatId, FIntPoint(Column, Mid - 1)).Id);
			Column += 3;
		}
	}
	for (int32 Index = 0; Index < Setup.StarterFarms; ++Index)
	{
		AddStructure(EWyrmStructureType::Farm, FString(), FIntPoint(Mid + 3, Mid - 1 + Index));
	}

	for (const FString& SpeciesId : Setup.StarterDragons)
	{
		if (const Wyrm::SpeciesDef* Species = Catalog.FindSpecies(WyrmText::ToStd(SpeciesId)))
		{
			const FGuid Home = FindHomeFor(*Species);
			if (Home.IsValid())
			{
				AddDragon(SpeciesId, Home);
			}
		}
	}

	StoreRng();
	SaveNow();
}

void UWyrmGameSubsystem::Sanitize()
{
	// Remove anything that refers to data that no longer exists (e.g. a species deleted from the JSON).
	State->Dragons.RemoveAll([this](const FWyrmDragon& D) { return !Catalog.FindSpecies(WyrmText::ToStd(D.SpeciesId)); });
	State->Eggs.RemoveAll([this](const FWyrmEgg& E) { return !Catalog.FindSpecies(WyrmText::ToStd(E.SpeciesId)); });
	State->Structures.RemoveAll([this](const FWyrmStructure& S)
		{ return S.Type == EWyrmStructureType::Habitat && !Catalog.FindHabitat(WyrmText::ToStd(S.DefId)); });

	for (FWyrmDragon& Dragon : State->Dragons)
	{
		Dragon.Level = FMath::Clamp(Dragon.Level, 1, Wyrm::MaxDragonLevel);
		const FWyrmStructure* Home = FindStructure(Dragon.HabitatId);
		if (!Home || Home->Type != EWyrmStructureType::Habitat)
		{
			const Wyrm::SpeciesDef* Species = FindSpeciesOf(Dragon);
			Dragon.HabitatId = Species ? FindHomeFor(*Species) : FGuid();
		}
	}
	if (State->Breeding.bActive && !FindDragon(State->Breeding.ParentA))
	{
		State->Breeding = FWyrmBreedingJob();
	}
	State->SaveVersion = UWyrmSaveGame::CurrentVersion;
}

void UWyrmGameSubsystem::SaveNow()
{
	// Never overwrite a good save while the game data failed to load.
	if (!State || !bDataLoaded)
	{
		return;
	}
	StoreRng();
	State->LastSavedUtc = FDateTime::UtcNow();
	UGameplayStatics::SaveGameToSlot(State, SaveSlotName, 0);
}

void UWyrmGameSubsystem::ResetProgress()
{
	ActiveBattle.Reset();
	UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0);
	CreateNewGame();
	Notify(LOCTEXT("Reset", "Started a new island."));
	Changed();
}

void UWyrmGameSubsystem::StoreRng()
{
	if (State)
	{
		State->RngState = static_cast<int64>(Random.GetState());
	}
}

// ---- Lookups -------------------------------------------------------------------------------------

const FWyrmDragon* UWyrmGameSubsystem::FindDragon(const FGuid& Id) const
{
	return State ? State->Dragons.FindByPredicate([&Id](const FWyrmDragon& D) { return D.Id == Id; }) : nullptr;
}

FWyrmDragon* UWyrmGameSubsystem::FindDragonMutable(const FGuid& Id)
{
	return State ? State->Dragons.FindByPredicate([&Id](const FWyrmDragon& D) { return D.Id == Id; }) : nullptr;
}

const FWyrmStructure* UWyrmGameSubsystem::FindStructure(const FGuid& Id) const
{
	return State ? State->Structures.FindByPredicate([&Id](const FWyrmStructure& S) { return S.Id == Id; }) : nullptr;
}

FWyrmStructure* UWyrmGameSubsystem::FindStructureMutable(const FGuid& Id)
{
	return State ? State->Structures.FindByPredicate([&Id](const FWyrmStructure& S) { return S.Id == Id; }) : nullptr;
}

FWyrmStructure* UWyrmGameSubsystem::FindFirstStructure(EWyrmStructureType Type)
{
	return State ? State->Structures.FindByPredicate([Type](const FWyrmStructure& S) { return S.Type == Type; }) : nullptr;
}

const Wyrm::SpeciesDef* UWyrmGameSubsystem::FindSpeciesOf(const FWyrmDragon& Dragon) const
{
	return Catalog.FindSpecies(WyrmText::ToStd(Dragon.SpeciesId));
}

const Wyrm::HabitatDef* UWyrmGameSubsystem::FindHabitatDefOf(const FWyrmStructure& Structure) const
{
	return Structure.Type == EWyrmStructureType::Habitat ? Catalog.FindHabitat(WyrmText::ToStd(Structure.DefId)) : nullptr;
}

TArray<const FWyrmDragon*> UWyrmGameSubsystem::GetDragonsIn(const FGuid& HabitatId) const
{
	TArray<const FWyrmDragon*> Result;
	if (State)
	{
		for (const FWyrmDragon& Dragon : State->Dragons)
		{
			if (Dragon.HabitatId == HabitatId)
			{
				Result.Add(&Dragon);
			}
		}
	}
	return Result;
}

int32 UWyrmGameSubsystem::CountStructures(EWyrmStructureType Type) const
{
	int32 Count = 0;
	if (State)
	{
		for (const FWyrmStructure& S : State->Structures)
		{
			Count += S.Type == Type ? 1 : 0;
		}
	}
	return Count;
}

int32 UWyrmGameSubsystem::GetMaxStructures() const
{
	return Wyrm::MaxStructuresForPlayerLevel(State ? State->PlayerLevel : 1);
}

FString UWyrmGameSubsystem::GetDisplayName(const FWyrmDragon& Dragon) const
{
	if (!Dragon.Nickname.IsEmpty())
	{
		return Dragon.Nickname;
	}
	const Wyrm::SpeciesDef* Species = FindSpeciesOf(Dragon);
	return Species ? WyrmText::ToF(Species->Name) : Dragon.SpeciesId;
}

int32 UWyrmGameSubsystem::FreeSlots(const FWyrmStructure& Habitat) const
{
	const Wyrm::HabitatDef* Def = FindHabitatDefOf(Habitat);
	return Def ? FMath::Max(0, Def->Capacity - GetDragonsIn(Habitat.Id).Num()) : 0;
}

FGuid UWyrmGameSubsystem::FindHomeFor(const Wyrm::SpeciesDef& Species) const
{
	if (!State)
	{
		return FGuid();
	}
	for (const FWyrmStructure& S : State->Structures)
	{
		const Wyrm::HabitatDef* Def = FindHabitatDefOf(S);
		if (Def && Wyrm::CanLiveIn(Species, *Def) && FreeSlots(S) > 0)
		{
			return S.Id;
		}
	}
	return FGuid();
}

double UWyrmGameSubsystem::SecondsUntil(const FDateTime& Utc)
{
	return (Utc - FDateTime::UtcNow()).GetTotalSeconds();
}

int32 UWyrmGameSubsystem::GetHabitatGoldPerMinute(const FGuid& HabitatId) const
{
	int32 Total = 0;
	for (const FWyrmDragon* Dragon : GetDragonsIn(HabitatId))
	{
		if (const Wyrm::SpeciesDef* Species = FindSpeciesOf(*Dragon))
		{
			Total += Wyrm::ScaledGoldPerMinute(*Species, Dragon->Level);
		}
	}
	return Total;
}

int64 UWyrmGameSubsystem::GetHabitatGold(const FGuid& HabitatId) const
{
	const FWyrmStructure* Habitat = FindStructure(HabitatId);
	const Wyrm::HabitatDef* Def = Habitat ? FindHabitatDefOf(*Habitat) : nullptr;
	if (!Def)
	{
		return 0;
	}
	const double Elapsed = (FDateTime::UtcNow() - Habitat->LastCollectUtc).GetTotalSeconds();
	return Wyrm::HabitatGold(Habitat->BankedGold, GetHabitatGoldPerMinute(HabitatId), Elapsed, Def->GoldCap);
}

void UWyrmGameSubsystem::BankHabitatGold(FWyrmStructure& Habitat)
{
	Habitat.BankedGold = GetHabitatGold(Habitat.Id);
	Habitat.LastCollectUtc = FDateTime::UtcNow();
}

// ---- Helpers -------------------------------------------------------------------------------------

bool UWyrmGameSubsystem::Spend(int64 Gold, int64 Gems)
{
	if (State->Gold < Gold)
	{
		return Fail(FText::Format(LOCTEXT("NoGold", "Not enough gold (need {0})."), FText::AsNumber(Gold)));
	}
	if (State->Gems < Gems)
	{
		return Fail(FText::Format(LOCTEXT("NoGems", "Not enough gems (need {0})."), FText::AsNumber(Gems)));
	}
	State->Gold -= Gold;
	State->Gems -= Gems;
	return true;
}

void UWyrmGameSubsystem::GainXp(int64 Amount)
{
	State->PlayerXp += FMath::Max<int64>(0, Amount);
	while (State->PlayerXp >= Wyrm::PlayerXpToNextLevel(State->PlayerLevel))
	{
		State->PlayerXp -= Wyrm::PlayerXpToNextLevel(State->PlayerLevel);
		++State->PlayerLevel;
		State->Gems += 5;
		Notify(FText::Format(LOCTEXT("PlayerLevelUp", "Island level {0}! +5 gems and room for more buildings."),
			FText::AsNumber(State->PlayerLevel)));
	}
}

bool UWyrmGameSubsystem::Fail(const FText& Reason)
{
	OnMessage.Broadcast(Reason, true);
	return false;
}

void UWyrmGameSubsystem::Notify(const FText& Message)
{
	OnMessage.Broadcast(Message, false);
}

void UWyrmGameSubsystem::Changed()
{
	OnStateChanged.Broadcast();
}

bool UWyrmGameSubsystem::IsCellFree(FIntPoint Cell) const
{
	if (!WyrmGrid::IsInside(Cell) || !State)
	{
		return false;
	}
	return !State->Structures.ContainsByPredicate([&Cell](const FWyrmStructure& S) { return S.Cell == Cell; });
}

bool UWyrmGameSubsystem::FindFreeCell(FIntPoint& OutCell) const
{
	// Spiral outwards from the centre so the island grows naturally.
	const FIntPoint Centre(WyrmGrid::Size / 2, WyrmGrid::Size / 2);
	for (int32 Radius = 0; Radius < WyrmGrid::Size; ++Radius)
	{
		for (int32 Y = Centre.Y - Radius; Y <= Centre.Y + Radius; ++Y)
		{
			for (int32 X = Centre.X - Radius; X <= Centre.X + Radius; ++X)
			{
				const bool bOnRing = FMath::Abs(X - Centre.X) == Radius || FMath::Abs(Y - Centre.Y) == Radius;
				if (bOnRing && IsCellFree(FIntPoint(X, Y)))
				{
					OutCell = FIntPoint(X, Y);
					return true;
				}
			}
		}
	}
	return false;
}

FWyrmStructure& UWyrmGameSubsystem::AddStructure(EWyrmStructureType Type, const FString& DefId, const FIntPoint& Cell)
{
	FIntPoint FinalCell = Cell;
	if (!IsCellFree(FinalCell))
	{
		FindFreeCell(FinalCell);
	}
	FWyrmStructure& Structure = State->Structures.AddDefaulted_GetRef();
	Structure.Id = FGuid::NewGuid();
	Structure.Type = Type;
	Structure.DefId = DefId;
	Structure.Cell = FinalCell;
	Structure.LastCollectUtc = FDateTime::UtcNow();
	Structure.ReadyAtUtc = FDateTime::UtcNow();
	return Structure;
}

FWyrmDragon& UWyrmGameSubsystem::AddDragon(const FString& SpeciesId, const FGuid& HabitatId)
{
	if (FWyrmStructure* Home = FindStructureMutable(HabitatId))
	{
		BankHabitatGold(*Home);
	}
	FWyrmDragon& Dragon = State->Dragons.AddDefaulted_GetRef();
	Dragon.Id = FGuid::NewGuid();
	Dragon.SpeciesId = SpeciesId;
	Dragon.Level = 1;
	Dragon.HabitatId = HabitatId;
	State->Discovered.AddUnique(SpeciesId);
	return Dragon;
}

// ---- Building ------------------------------------------------------------------------------------

bool UWyrmGameSubsystem::BuildHabitat(const FString& HabitatDefId)
{
	const Wyrm::HabitatDef* Def = Catalog.FindHabitat(WyrmText::ToStd(HabitatDefId));
	if (!Def)
	{
		return Fail(LOCTEXT("UnknownHabitat", "Unknown habitat."));
	}
	FIntPoint Cell;
	if (State->Structures.Num() >= GetMaxStructures())
	{
		return Fail(LOCTEXT("TooManyBuildings", "Raise your island level to build more."));
	}
	if (!FindFreeCell(Cell))
	{
		return Fail(LOCTEXT("IslandFull", "No free space on the island."));
	}
	if (!Spend(Def->Cost, 0))
	{
		return false;
	}
	AddStructure(EWyrmStructureType::Habitat, HabitatDefId, Cell);
	GainXp(Def->Cost / 50);
	Notify(FText::Format(LOCTEXT("BuiltHabitat", "Built a {0}."), WyrmName(Def->Name)));
	Changed();
	return true;
}

bool UWyrmGameSubsystem::BuildFarm()
{
	FIntPoint Cell;
	if (State->Structures.Num() >= GetMaxStructures())
	{
		return Fail(LOCTEXT("TooManyBuildings2", "Raise your island level to build more."));
	}
	if (!FindFreeCell(Cell))
	{
		return Fail(LOCTEXT("IslandFull2", "No free space on the island."));
	}
	if (!Spend(Setup.FarmCost, 0))
	{
		return false;
	}
	AddStructure(EWyrmStructureType::Farm, FString(), Cell);
	GainXp(10);
	Notify(LOCTEXT("BuiltFarm", "Built a farm."));
	Changed();
	return true;
}

bool UWyrmGameSubsystem::MoveStructure(const FGuid& StructureId, FIntPoint NewCell)
{
	FWyrmStructure* Structure = FindStructureMutable(StructureId);
	if (!Structure || !IsCellFree(NewCell))
	{
		return Fail(LOCTEXT("CantMove", "That spot is taken."));
	}
	Structure->Cell = NewCell;
	Changed();
	return true;
}

// ---- Gold and food -------------------------------------------------------------------------------

bool UWyrmGameSubsystem::CollectHabitat(const FGuid& HabitatId)
{
	FWyrmStructure* Habitat = FindStructureMutable(HabitatId);
	if (!Habitat || Habitat->Type != EWyrmStructureType::Habitat)
	{
		return false;
	}
	const int64 Amount = GetHabitatGold(HabitatId);
	Habitat->BankedGold = 0;
	Habitat->LastCollectUtc = FDateTime::UtcNow();
	if (Amount <= 0)
	{
		return false;
	}
	State->Gold += Amount;
	GainXp(Amount / 100);
	Notify(FText::Format(LOCTEXT("Collected", "+{0} gold"), FText::AsNumber(Amount)));
	Changed();
	return true;
}

int64 UWyrmGameSubsystem::CollectAllGold()
{
	int64 Total = 0;
	for (FWyrmStructure& S : State->Structures)
	{
		if (S.Type == EWyrmStructureType::Habitat)
		{
			Total += GetHabitatGold(S.Id);
			S.BankedGold = 0;
			S.LastCollectUtc = FDateTime::UtcNow();
		}
	}
	if (Total > 0)
	{
		State->Gold += Total;
		GainXp(Total / 100);
		Notify(FText::Format(LOCTEXT("CollectedAll", "+{0} gold from all habitats"), FText::AsNumber(Total)));
		Changed();
	}
	return Total;
}

bool UWyrmGameSubsystem::PlantCrop(const FGuid& FarmId, const FString& CropId)
{
	FWyrmStructure* Farm = FindStructureMutable(FarmId);
	const Wyrm::CropDef* Crop = Catalog.FindCrop(WyrmText::ToStd(CropId));
	if (!Farm || Farm->Type != EWyrmStructureType::Farm || !Crop)
	{
		return false;
	}
	if (!Farm->CropId.IsEmpty())
	{
		return Fail(LOCTEXT("FarmBusy", "This farm is already growing something."));
	}
	if (!Spend(Crop->Cost, 0))
	{
		return false;
	}
	Farm->CropId = CropId;
	Farm->ReadyAtUtc = FDateTime::UtcNow() + FTimespan::FromSeconds(Crop->GrowSeconds);
	Changed();
	return true;
}

bool UWyrmGameSubsystem::HarvestFarm(const FGuid& FarmId)
{
	FWyrmStructure* Farm = FindStructureMutable(FarmId);
	if (!Farm || Farm->CropId.IsEmpty())
	{
		return false;
	}
	const Wyrm::CropDef* Crop = Catalog.FindCrop(WyrmText::ToStd(Farm->CropId));
	if (SecondsUntil(Farm->ReadyAtUtc) > 0.0)
	{
		return Fail(LOCTEXT("NotGrown", "Still growing."));
	}
	const int64 Food = Crop ? Crop->Food : 0;
	State->Food += Food;
	Farm->CropId.Reset();
	GainXp(Food / 10 + 1);
	Notify(FText::Format(LOCTEXT("Harvested", "+{0} food"), FText::AsNumber(Food)));
	Changed();
	return true;
}

// ---- Dragons -------------------------------------------------------------------------------------

bool UWyrmGameSubsystem::FeedDragon(const FGuid& DragonId)
{
	FWyrmDragon* Dragon = FindDragonMutable(DragonId);
	if (!Dragon)
	{
		return false;
	}
	if (Dragon->Level >= Wyrm::MaxDragonLevel)
	{
		return Fail(LOCTEXT("MaxLevel", "Already at max level."));
	}
	const int64 Cost = Wyrm::FoodToNextLevel(Dragon->Level);
	if (State->Food < Cost)
	{
		return Fail(FText::Format(LOCTEXT("NoFood", "Not enough food (need {0}). Harvest your farms."), FText::AsNumber(Cost)));
	}
	if (FWyrmStructure* Home = FindStructureMutable(Dragon->HabitatId))
	{
		BankHabitatGold(*Home); // income changes with level
	}
	State->Food -= Cost;
	++Dragon->Level;
	GainXp(Cost / 5 + 1);
	Notify(FText::Format(LOCTEXT("LevelUp", "{0} reached level {1}!"), FText::FromString(GetDisplayName(*Dragon)),
		FText::AsNumber(Dragon->Level)));
	Changed();
	return true;
}

bool UWyrmGameSubsystem::SellDragon(const FGuid& DragonId)
{
	const FWyrmDragon* Dragon = FindDragon(DragonId);
	if (!Dragon)
	{
		return false;
	}
	if (State->Dragons.Num() <= 1)
	{
		return Fail(LOCTEXT("LastDragon", "You can't sell your last dragon."));
	}
	if (State->Breeding.bActive && (State->Breeding.ParentA == DragonId || State->Breeding.ParentB == DragonId))
	{
		return Fail(LOCTEXT("IsBreeding", "This dragon is busy breeding."));
	}
	const Wyrm::SpeciesDef* Species = FindSpeciesOf(*Dragon);
	const int64 Value = Species ? Wyrm::SellValue(*Species, Dragon->Level) : 0;
	const FString Name = GetDisplayName(*Dragon);
	if (FWyrmStructure* Home = FindStructureMutable(Dragon->HabitatId))
	{
		BankHabitatGold(*Home);
	}
	State->Dragons.RemoveAll([&DragonId](const FWyrmDragon& D) { return D.Id == DragonId; });
	State->Gold += Value;
	Notify(FText::Format(LOCTEXT("Sold", "Sold {0} for {1} gold."), FText::FromString(Name), FText::AsNumber(Value)));
	Changed();
	return true;
}

bool UWyrmGameSubsystem::MoveDragon(const FGuid& DragonId, const FGuid& HabitatId)
{
	FWyrmDragon* Dragon = FindDragonMutable(DragonId);
	FWyrmStructure* Target = FindStructureMutable(HabitatId);
	const Wyrm::HabitatDef* Def = Target ? FindHabitatDefOf(*Target) : nullptr;
	const Wyrm::SpeciesDef* Species = Dragon ? FindSpeciesOf(*Dragon) : nullptr;
	if (!Dragon || !Def || !Species || Dragon->HabitatId == HabitatId)
	{
		return false;
	}
	if (!Wyrm::CanLiveIn(*Species, *Def))
	{
		return Fail(LOCTEXT("WrongHabitat", "That dragon can't live in this habitat."));
	}
	if (FreeSlots(*Target) <= 0)
	{
		return Fail(LOCTEXT("HabitatFull", "That habitat is full."));
	}
	if (FWyrmStructure* OldHome = FindStructureMutable(Dragon->HabitatId))
	{
		BankHabitatGold(*OldHome);
	}
	BankHabitatGold(*Target);
	Dragon->HabitatId = HabitatId;
	Changed();
	return true;
}

bool UWyrmGameSubsystem::BuyEgg(const FString& SpeciesId)
{
	const Wyrm::SpeciesDef* Species = Catalog.FindSpecies(WyrmText::ToStd(SpeciesId));
	if (!Species || Species->ShopGold <= 0)
	{
		return Fail(LOCTEXT("NotForSale", "That dragon can't be bought - breed it!"));
	}
	if (!FindFirstStructure(EWyrmStructureType::Hatchery))
	{
		return Fail(LOCTEXT("NoHatchery", "You need a hatchery."));
	}
	if (State->Eggs.Num() >= Wyrm::MaxEggsInHatchery)
	{
		return Fail(LOCTEXT("HatcheryFull", "The hatchery is full. Hatch an egg first."));
	}
	if (!Spend(Species->ShopGold, 0))
	{
		return false;
	}
	FWyrmEgg& Egg = State->Eggs.AddDefaulted_GetRef();
	Egg.Id = FGuid::NewGuid();
	Egg.SpeciesId = SpeciesId;
	Egg.HatchAtUtc = FDateTime::UtcNow() + FTimespan::FromSeconds(Species->HatchSeconds);
	Notify(FText::Format(LOCTEXT("BoughtEgg", "A {0} egg is in the hatchery."), WyrmName(Species->Name)));
	Changed();
	return true;
}

// ---- Breeding and hatching -----------------------------------------------------------------------

TArray<FWyrmBreedOddsEntry> UWyrmGameSubsystem::GetBreedOdds(const FGuid& ParentA, const FGuid& ParentB, FString& OutError) const
{
	TArray<FWyrmBreedOddsEntry> Result;
	const FWyrmDragon* A = FindDragon(ParentA);
	const FWyrmDragon* B = FindDragon(ParentB);
	if (!A || !B || ParentA == ParentB)
	{
		OutError = TEXT("Pick two different dragons.");
		return Result;
	}
	std::vector<Wyrm::BreedOdds> Odds;
	const Wyrm::BreedError Error = Wyrm::ComputeBreedOdds(Catalog, {WyrmText::ToStd(A->SpeciesId), A->Level},
		{WyrmText::ToStd(B->SpeciesId), B->Level}, State->BreedPity, Odds);
	if (Error != Wyrm::BreedError::None)
	{
		OutError = ANSI_TO_TCHAR(Wyrm::BreedErrorText(Error));
		return Result;
	}
	for (const Wyrm::BreedOdds& Entry : Odds)
	{
		const Wyrm::SpeciesDef* Species = Catalog.FindSpecies(Entry.SpeciesId);
		FWyrmBreedOddsEntry& Row = Result.AddDefaulted_GetRef();
		Row.SpeciesId = WyrmText::ToF(Entry.SpeciesId);
		Row.Name = Species ? WyrmText::ToF(Species->Name) : Row.SpeciesId;
		Row.Rarity = Species ? ANSI_TO_TCHAR(Wyrm::RarityName(Species->Rar)) : TEXT("");
		Row.Probability = static_cast<float>(Entry.Probability);
	}
	return Result;
}

bool UWyrmGameSubsystem::StartBreeding(const FGuid& ParentA, const FGuid& ParentB)
{
	if (!FindFirstStructure(EWyrmStructureType::BreedingCave))
	{
		return Fail(LOCTEXT("NoCave", "You need a breeding cave."));
	}
	if (State->Breeding.bActive)
	{
		return Fail(LOCTEXT("CaveBusy", "The breeding cave is busy."));
	}
	const FWyrmDragon* A = FindDragon(ParentA);
	const FWyrmDragon* B = FindDragon(ParentB);
	if (!A || !B || ParentA == ParentB)
	{
		return Fail(LOCTEXT("PickTwo", "Pick two different dragons."));
	}

	std::string ResultId;
	const Wyrm::BreedError Error = Wyrm::Breed(Catalog, {WyrmText::ToStd(A->SpeciesId), A->Level},
		{WyrmText::ToStd(B->SpeciesId), B->Level}, State->BreedPity, Random, ResultId);
	if (Error != Wyrm::BreedError::None)
	{
		return Fail(FText::FromString(ANSI_TO_TCHAR(Wyrm::BreedErrorText(Error))));
	}
	const Wyrm::SpeciesDef* Result = Catalog.FindSpecies(ResultId);
	State->BreedPity = Wyrm::NextPity(State->BreedPity, *Result);
	State->Breeding.bActive = true;
	State->Breeding.ParentA = ParentA;
	State->Breeding.ParentB = ParentB;
	State->Breeding.ResultSpeciesId = WyrmText::ToF(ResultId);
	State->Breeding.ReadyAtUtc = FDateTime::UtcNow() + FTimespan::FromSeconds(Result->BreedSeconds);
	StoreRng(); // the roll is final, even if the game is closed now
	SaveNow();
	Notify(LOCTEXT("Breeding", "The dragons are breeding..."));
	Changed();
	return true;
}

bool UWyrmGameSubsystem::CollectBreedingEgg()
{
	FWyrmBreedingJob& Job = State->Breeding;
	if (!Job.bActive)
	{
		return false;
	}
	if (SecondsUntil(Job.ReadyAtUtc) > 0.0)
	{
		return Fail(LOCTEXT("NotReady", "Not ready yet."));
	}
	if (State->Eggs.Num() >= Wyrm::MaxEggsInHatchery)
	{
		return Fail(LOCTEXT("HatcheryFull2", "The hatchery is full. Hatch an egg first."));
	}
	const Wyrm::SpeciesDef* Species = Catalog.FindSpecies(WyrmText::ToStd(Job.ResultSpeciesId));
	if (Species)
	{
		FWyrmEgg& Egg = State->Eggs.AddDefaulted_GetRef();
		Egg.Id = FGuid::NewGuid();
		Egg.SpeciesId = Job.ResultSpeciesId;
		Egg.HatchAtUtc = FDateTime::UtcNow() + FTimespan::FromSeconds(Species->HatchSeconds);
		Notify(FText::Format(LOCTEXT("GotEgg", "It's a {0} egg ({1})!"), WyrmName(Species->Name),
			FText::FromString(ANSI_TO_TCHAR(Wyrm::RarityName(Species->Rar)))));
	}
	Job = FWyrmBreedingJob();
	GainXp(25);
	Changed();
	return true;
}

bool UWyrmGameSubsystem::HatchEgg(const FGuid& EggId)
{
	const int32 Index = State->Eggs.IndexOfByPredicate([&EggId](const FWyrmEgg& E) { return E.Id == EggId; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	const FWyrmEgg Egg = State->Eggs[Index];
	if (SecondsUntil(Egg.HatchAtUtc) > 0.0)
	{
		return Fail(LOCTEXT("NotHatched", "The egg isn't ready to hatch."));
	}
	const Wyrm::SpeciesDef* Species = Catalog.FindSpecies(WyrmText::ToStd(Egg.SpeciesId));
	if (!Species)
	{
		return false;
	}
	const FGuid Home = FindHomeFor(*Species);
	if (!Home.IsValid())
	{
		return Fail(FText::Format(LOCTEXT("NeedHome", "Build or free up a {0} habitat first."),
			FText::FromString(WyrmText::Elements(Species->Elements))));
	}
	State->Eggs.RemoveAt(Index);
	AddDragon(Egg.SpeciesId, Home);
	static const int64 RarityXp[] = {20, 60, 150, 400};
	GainXp(RarityXp[static_cast<int32>(Species->Rar)]);
	Notify(FText::Format(LOCTEXT("Hatched", "{0} hatched!"), WyrmName(Species->Name)));
	Changed();
	return true;
}

// ---- Speed-ups -----------------------------------------------------------------------------------

int64 UWyrmGameSubsystem::GemCostFor(const FDateTime& ReadyAtUtc) const
{
	return Wyrm::GemsToSkip(SecondsUntil(ReadyAtUtc));
}

bool UWyrmGameSubsystem::SpeedUpBreeding()
{
	if (!State->Breeding.bActive)
	{
		return false;
	}
	if (!Spend(0, GemCostFor(State->Breeding.ReadyAtUtc)))
	{
		return false;
	}
	State->Breeding.ReadyAtUtc = FDateTime::UtcNow();
	Changed();
	return true;
}

bool UWyrmGameSubsystem::SpeedUpEgg(const FGuid& EggId)
{
	FWyrmEgg* Egg = State->Eggs.FindByPredicate([&EggId](const FWyrmEgg& E) { return E.Id == EggId; });
	if (!Egg || !Spend(0, GemCostFor(Egg->HatchAtUtc)))
	{
		return false;
	}
	Egg->HatchAtUtc = FDateTime::UtcNow();
	Changed();
	return true;
}

bool UWyrmGameSubsystem::SpeedUpFarm(const FGuid& FarmId)
{
	FWyrmStructure* Farm = FindStructureMutable(FarmId);
	if (!Farm || Farm->CropId.IsEmpty() || !Spend(0, GemCostFor(Farm->ReadyAtUtc)))
	{
		return false;
	}
	Farm->ReadyAtUtc = FDateTime::UtcNow();
	Changed();
	return true;
}

// ---- Arena ---------------------------------------------------------------------------------------

TArray<const FWyrmDragon*> UWyrmGameSubsystem::GetArenaTeam() const
{
	TArray<const FWyrmDragon*> Team;
	if (!State)
	{
		return Team;
	}
	for (const FWyrmDragon& Dragon : State->Dragons)
	{
		Team.Add(&Dragon);
	}
	Team.StableSort([](const FWyrmDragon& L, const FWyrmDragon& R) { return L.Level > R.Level; });
	if (Team.Num() > Wyrm::MaxArenaTeamSize)
	{
		Team.SetNum(Wyrm::MaxArenaTeamSize);
	}
	return Team;
}

bool UWyrmGameSubsystem::StartArenaBattle()
{
	if (ActiveBattle.IsValid() && !ActiveBattle->IsOver())
	{
		return true;
	}
	std::vector<Wyrm::Combatant> PlayerTeam;
	for (const FWyrmDragon* Dragon : GetArenaTeam())
	{
		Wyrm::Combatant C;
		if (Wyrm::MakeCombatant(Catalog, WyrmText::ToStd(Dragon->SpeciesId), Dragon->Level, WyrmText::ToStd(GetDisplayName(*Dragon)), C))
		{
			PlayerTeam.push_back(C);
		}
	}
	if (PlayerTeam.empty())
	{
		return Fail(LOCTEXT("NoTeam", "You need a dragon to fight."));
	}
	std::vector<Wyrm::Combatant> EnemyTeam = Wyrm::GenerateArenaTeam(Catalog, State->ArenaTier, Random);
	ActiveBattle = MakeUnique<Wyrm::Battle>(MoveTemp(PlayerTeam), MoveTemp(EnemyTeam), Random.NextU64());
	StoreRng();
	Changed();
	return true;
}

TArray<FString> UWyrmGameSubsystem::BattleUseSkill(int32 SkillIndex)
{
	TArray<FString> Lines;
	if (!ActiveBattle.IsValid() || ActiveBattle->IsOver())
	{
		return Lines;
	}
	for (const Wyrm::BattleEvent& Event : ActiveBattle->PlayTurn(SkillIndex))
	{
		Lines.Add(WyrmText::ToF(Wyrm::DescribeEvent(Event)));
	}
	if (ActiveBattle->IsOver())
	{
		if (ActiveBattle->Winner() == 0)
		{
			const Wyrm::BattleRewards Rewards = Wyrm::ComputeRewards(ActiveBattle->Team(1));
			State->Gold += Rewards.Gold;
			GainXp(Rewards.PlayerXp);
			++State->ArenaTier;
			Lines.Add(FString::Printf(TEXT("You earned %s gold and %lld XP. Arena tier %d unlocked!"),
				*WyrmText::Number(Rewards.Gold), static_cast<long long>(Rewards.PlayerXp), State->ArenaTier));
		}
		else
		{
			Lines.Add(TEXT("Level up your dragons with food and try again."));
		}
		SaveNow();
	}
	Changed();
	return Lines;
}

void UWyrmGameSubsystem::EndBattle()
{
	ActiveBattle.Reset();
	Changed();
}

#undef LOCTEXT_NAMESPACE

// Wyrmhaven - static game data (species, skills, habitats, crops).
// Filled from Content/Data/GameData.json by the Unreal layer, or built by hand in tests.
#pragma once

#include "WyrmElements.h"

#include <string>
#include <vector>

namespace Wyrm
{
enum class Rarity : uint8_t
{
	Common,
	Rare,
	Epic,
	Legendary,
	Count
};

const char* RarityName(Rarity R);
bool ParseRarity(const std::string& Name, Rarity& Out);

struct SkillDef
{
	std::string Id;
	std::string Name;
	Element Elem = Element::Fire;
	int Power = 40;     // % of attack dealt as damage before multipliers
	int Accuracy = 100; // 0..100
};

struct SpeciesDef
{
	std::string Id;
	std::string Name;
	ElementMask Elements = 0;
	Rarity Rar = Rarity::Common;
	int BaseHealth = 300;
	int BaseAttack = 80;
	int GoldPerMinute = 5;
	int BreedSeconds = 60;
	int HatchSeconds = 60;
	bool Breedable = true;
	int ShopGold = 0; // 0 = cannot be bought, only bred

	// Legendary "recipes": if both are set, the species can only come from this exact parent pair.
	std::string RecipeA;
	std::string RecipeB;

	std::vector<std::string> Skills; // unlocked in order as the dragon levels up
	std::string Mesh;                // optional art asset path; empty = placeholder model

	bool HasRecipe() const { return !RecipeA.empty() && !RecipeB.empty(); }
};

struct HabitatDef
{
	std::string Id;
	std::string Name;
	Element Elem = Element::Fire;
	int Capacity = 2;
	int GoldCap = 1000;
	int Cost = 1000;
};

struct CropDef
{
	std::string Id;
	std::string Name;
	int Cost = 0;
	int Food = 0;
	int GrowSeconds = 30;
};

struct Catalog
{
	std::vector<SkillDef> Skills;
	std::vector<SpeciesDef> Species;
	std::vector<HabitatDef> Habitats;
	std::vector<CropDef> Crops;

	const SkillDef* FindSkill(const std::string& Id) const;
	const SpeciesDef* FindSpecies(const std::string& Id) const;
	const HabitatDef* FindHabitat(const std::string& Id) const;
	const CropDef* FindCrop(const std::string& Id) const;

	// Returns a list of human readable data problems. Empty = data is consistent.
	std::vector<std::string> Validate() const;
};
} // namespace Wyrm

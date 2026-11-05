// Wyrmhaven - breeding rules.
//
// Rules:
//  * Both parents must be at least BreedMinLevel.
//  * Offspring elements must come from the parents' combined elements (Fire + Water can give
//    Fire, Water or a Fire/Water hybrid).
//  * Legendary species with a recipe only come from that exact parent pair.
//  * Odds are fully transparent: the UI shows the exact percentages computed here.
//  * Pity: every breed that does not produce an Epic or better boosts Epic/Legendary odds.
#pragma once

#include "WyrmCatalog.h"
#include "WyrmRng.h"

#include <string>
#include <vector>

namespace Wyrm
{
struct BreedParent
{
	std::string SpeciesId;
	int Level = 1;
};

struct BreedOdds
{
	std::string SpeciesId;
	double Probability = 0.0;
};

enum class BreedError : uint8_t
{
	None,
	UnknownSpecies,
	ParentTooLow,
	NoOffspring
};

struct BreedConfig
{
	int MinLevel = 4;
	double RarityWeights[static_cast<int>(Rarity::Count)] = {100.0, 30.0, 8.0, 2.0};
	double ParentSpeciesBonus = 1.5;    // offspring is a bit more likely to match a parent
	double RareBoostPerLevel = 0.01;    // per parent level above MinLevel (lower parent counts)
	double PityStep = 0.25;             // +25% Epic/Legendary weight per unlucky breed
	int PityCap = 20;
};

const char* BreedErrorText(BreedError Error);

// Fills Out with every possible offspring, sorted from most to least likely. Probabilities sum to 1.
BreedError ComputeBreedOdds(const Catalog& Data, const BreedParent& A, const BreedParent& B, int Pity,
	std::vector<BreedOdds>& Out, const BreedConfig& Config = BreedConfig());

// Rolls one offspring.
BreedError Breed(const Catalog& Data, const BreedParent& A, const BreedParent& B, int Pity, Rng& Random,
	std::string& OutSpeciesId, const BreedConfig& Config = BreedConfig());

// New pity counter after an offspring was produced.
int NextPity(int Pity, const SpeciesDef& Offspring, const BreedConfig& Config = BreedConfig());
} // namespace Wyrm

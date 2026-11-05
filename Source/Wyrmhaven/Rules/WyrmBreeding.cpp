#include "WyrmBreeding.h"

#include <algorithm>

namespace Wyrm
{
const char* BreedErrorText(BreedError Error)
{
	switch (Error)
	{
	case BreedError::None:
		return "OK";
	case BreedError::UnknownSpecies:
		return "Unknown dragon species";
	case BreedError::ParentTooLow:
		return "Both parents must be at least level 4";
	case BreedError::NoOffspring:
		return "These dragons cannot breed together";
	}
	return "Unknown error";
}

BreedError ComputeBreedOdds(const Catalog& Data, const BreedParent& A, const BreedParent& B, int Pity,
	std::vector<BreedOdds>& Out, const BreedConfig& Config)
{
	Out.clear();
	const SpeciesDef* SpeciesA = Data.FindSpecies(A.SpeciesId);
	const SpeciesDef* SpeciesB = Data.FindSpecies(B.SpeciesId);
	if (!SpeciesA || !SpeciesB)
	{
		return BreedError::UnknownSpecies;
	}
	if (A.Level < Config.MinLevel || B.Level < Config.MinLevel)
	{
		return BreedError::ParentTooLow;
	}

	const ElementMask Combined = static_cast<ElementMask>(SpeciesA->Elements | SpeciesB->Elements);
	const int LowerLevel = (std::min)(A.Level, B.Level);
	const double LevelBoost = 1.0 + Config.RareBoostPerLevel * static_cast<double>((std::max)(0, LowerLevel - Config.MinLevel));
	const int ClampedPity = (std::max)(0, (std::min)(Pity, Config.PityCap));
	const double PityBoost = 1.0 + Config.PityStep * static_cast<double>(ClampedPity);

	double Total = 0.0;
	for (const SpeciesDef& S : Data.Species)
	{
		if (!S.Breedable)
		{
			continue;
		}
		if (S.HasRecipe())
		{
			const bool bMatches = (A.SpeciesId == S.RecipeA && B.SpeciesId == S.RecipeB) ||
				(A.SpeciesId == S.RecipeB && B.SpeciesId == S.RecipeA);
			if (!bMatches)
			{
				continue;
			}
		}
		else if (!IsSubsetOf(S.Elements, Combined))
		{
			continue;
		}

		double Weight = Config.RarityWeights[static_cast<int>(S.Rar)];
		if (S.Rar != Rarity::Common)
		{
			Weight *= LevelBoost;
		}
		if (S.Rar == Rarity::Epic || S.Rar == Rarity::Legendary)
		{
			Weight *= PityBoost;
		}
		if (S.Id == A.SpeciesId || S.Id == B.SpeciesId)
		{
			Weight *= Config.ParentSpeciesBonus;
		}
		if (Weight <= 0.0)
		{
			continue;
		}
		Out.push_back(BreedOdds{S.Id, Weight});
		Total += Weight;
	}

	if (Out.empty() || Total <= 0.0)
	{
		Out.clear();
		return BreedError::NoOffspring;
	}
	for (BreedOdds& Entry : Out)
	{
		Entry.Probability /= Total;
	}
	std::stable_sort(Out.begin(), Out.end(),
		[](const BreedOdds& L, const BreedOdds& R) { return L.Probability > R.Probability; });
	return BreedError::None;
}

BreedError Breed(const Catalog& Data, const BreedParent& A, const BreedParent& B, int Pity, Rng& Random,
	std::string& OutSpeciesId, const BreedConfig& Config)
{
	std::vector<BreedOdds> Odds;
	const BreedError Error = ComputeBreedOdds(Data, A, B, Pity, Odds, Config);
	if (Error != BreedError::None)
	{
		return Error;
	}
	const double Roll = Random.NextDouble();
	double Cumulative = 0.0;
	for (const BreedOdds& Entry : Odds)
	{
		Cumulative += Entry.Probability;
		if (Roll < Cumulative)
		{
			OutSpeciesId = Entry.SpeciesId;
			return BreedError::None;
		}
	}
	OutSpeciesId = Odds.back().SpeciesId; // floating point safety
	return BreedError::None;
}

int NextPity(int Pity, const SpeciesDef& Offspring, const BreedConfig& Config)
{
	if (Offspring.Rar == Rarity::Epic || Offspring.Rar == Rarity::Legendary)
	{
		return 0;
	}
	return (std::min)(Pity + 1, Config.PityCap);
}
} // namespace Wyrm

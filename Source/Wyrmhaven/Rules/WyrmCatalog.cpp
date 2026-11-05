#include "WyrmCatalog.h"

#include <set>

namespace Wyrm
{
namespace
{
const char* const GWyrmRarityNames[static_cast<int>(Rarity::Count)] = {"Common", "Rare", "Epic", "Legendary"};

template <typename T>
const T* WyrmFindById(const std::vector<T>& Items, const std::string& Id)
{
	for (const T& Item : Items)
	{
		if (Item.Id == Id)
		{
			return &Item;
		}
	}
	return nullptr;
}

template <typename T>
void WyrmCheckUniqueIds(const std::vector<T>& Items, const char* Kind, std::vector<std::string>& Problems)
{
	std::set<std::string> Seen;
	for (const T& Item : Items)
	{
		if (Item.Id.empty())
		{
			Problems.push_back(std::string(Kind) + " with empty id");
		}
		else if (!Seen.insert(Item.Id).second)
		{
			Problems.push_back(std::string("Duplicate ") + Kind + " id '" + Item.Id + "'");
		}
	}
}
} // namespace

const char* RarityName(Rarity R)
{
	const int Index = static_cast<int>(R);
	return (Index >= 0 && Index < static_cast<int>(Rarity::Count)) ? GWyrmRarityNames[Index] : "Unknown";
}

bool ParseRarity(const std::string& Name, Rarity& Out)
{
	for (int Index = 0; Index < static_cast<int>(Rarity::Count); ++Index)
	{
		if (Name == GWyrmRarityNames[Index])
		{
			Out = static_cast<Rarity>(Index);
			return true;
		}
	}
	return false;
}

const SkillDef* Catalog::FindSkill(const std::string& Id) const { return WyrmFindById(Skills, Id); }
const SpeciesDef* Catalog::FindSpecies(const std::string& Id) const { return WyrmFindById(Species, Id); }
const HabitatDef* Catalog::FindHabitat(const std::string& Id) const { return WyrmFindById(Habitats, Id); }
const CropDef* Catalog::FindCrop(const std::string& Id) const { return WyrmFindById(Crops, Id); }

std::vector<std::string> Catalog::Validate() const
{
	std::vector<std::string> Problems;
	WyrmCheckUniqueIds(Skills, "skill", Problems);
	WyrmCheckUniqueIds(Species, "species", Problems);
	WyrmCheckUniqueIds(Habitats, "habitat", Problems);
	WyrmCheckUniqueIds(Crops, "crop", Problems);

	for (const SkillDef& Skill : Skills)
	{
		if (Skill.Power <= 0 || Skill.Accuracy <= 0 || Skill.Accuracy > 100)
		{
			Problems.push_back("Skill '" + Skill.Id + "' has invalid power/accuracy");
		}
	}

	for (const SpeciesDef& S : Species)
	{
		const int NumElements = CountElements(S.Elements);
		if (NumElements < 1 || NumElements > 2)
		{
			Problems.push_back("Species '" + S.Id + "' must have 1 or 2 elements");
		}
		if (S.BaseHealth <= 0 || S.BaseAttack <= 0 || S.GoldPerMinute < 0 || S.BreedSeconds < 0 || S.HatchSeconds < 0)
		{
			Problems.push_back("Species '" + S.Id + "' has invalid stats");
		}
		if (S.Skills.size() < 2)
		{
			Problems.push_back("Species '" + S.Id + "' needs at least 2 skills");
		}
		for (const std::string& SkillId : S.Skills)
		{
			const SkillDef* Skill = FindSkill(SkillId);
			if (!Skill)
			{
				Problems.push_back("Species '" + S.Id + "' uses unknown skill '" + SkillId + "'");
			}
		}
		if (!S.RecipeA.empty() || !S.RecipeB.empty())
		{
			if (!FindSpecies(S.RecipeA) || !FindSpecies(S.RecipeB))
			{
				Problems.push_back("Species '" + S.Id + "' has a recipe with unknown parents");
			}
		}
	}

	for (const HabitatDef& H : Habitats)
	{
		if (H.Capacity <= 0 || H.GoldCap <= 0 || H.Cost < 0)
		{
			Problems.push_back("Habitat '" + H.Id + "' has invalid numbers");
		}
	}

	for (const CropDef& C : Crops)
	{
		if (C.Cost < 0 || C.Food <= 0 || C.GrowSeconds < 0)
		{
			Problems.push_back("Crop '" + C.Id + "' has invalid numbers");
		}
	}

	// Every element needs a home, or dragons of that element could never hatch.
	for (int Index = 0; Index < ElementCount; ++Index)
	{
		bool bHasHome = false;
		for (const HabitatDef& H : Habitats)
		{
			bHasHome = bHasHome || static_cast<int>(H.Elem) == Index;
		}
		if (!bHasHome)
		{
			Problems.push_back(std::string("No habitat for element ") + ElementName(static_cast<Element>(Index)));
		}
	}
	return Problems;
}
} // namespace Wyrm

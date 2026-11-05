#include "WyrmProgression.h"

#include <algorithm>
#include <cmath>

namespace Wyrm
{
int64_t FoodToNextLevel(int Level)
{
	const int64_t L = (std::max)(1, Level);
	return 5 * L * L + 5;
}

int ScaledHealth(const SpeciesDef& Species, int Level)
{
	const int L = (std::max)(1, Level);
	return Species.BaseHealth + Species.BaseHealth * 12 * (L - 1) / 100;
}

int ScaledAttack(const SpeciesDef& Species, int Level)
{
	const int L = (std::max)(1, Level);
	return Species.BaseAttack + Species.BaseAttack * 10 * (L - 1) / 100;
}

int ScaledGoldPerMinute(const SpeciesDef& Species, int Level)
{
	const int L = (std::max)(1, Level);
	return Species.GoldPerMinute + Species.GoldPerMinute * 8 * (L - 1) / 100;
}

int UnlockedSkillCount(int Level, int TotalSkills)
{
	int Count = 2;
	if (Level >= 10)
	{
		Count = 3;
	}
	if (Level >= 20)
	{
		Count = 4;
	}
	return (std::min)(Count, (std::max)(0, TotalSkills));
}

int64_t HabitatGold(int64_t Banked, int64_t GoldPerMinute, double ElapsedSeconds, int64_t Cap)
{
	const double Elapsed = ElapsedSeconds > 0.0 ? ElapsedSeconds : 0.0;
	const int64_t Earned = static_cast<int64_t>(std::floor(static_cast<double>(GoldPerMinute) * Elapsed / 60.0));
	const int64_t Total = (std::max)(static_cast<int64_t>(0), Banked + Earned);
	return (std::min)(Cap, Total);
}

bool CanLiveIn(const SpeciesDef& Species, const HabitatDef& Habitat)
{
	return HasElement(Species.Elements, Habitat.Elem);
}

int64_t SellValue(const SpeciesDef& Species, int Level)
{
	static const int64_t RarityValue[static_cast<int>(Rarity::Count)] = {100, 500, 2000, 10000};
	const int64_t Base = Species.ShopGold > 0 ? Species.ShopGold / 4 : RarityValue[static_cast<int>(Species.Rar)];
	return Base + static_cast<int64_t>((std::max)(1, Level)) * 20;
}

int64_t GemsToSkip(double SecondsRemaining)
{
	if (SecondsRemaining <= 0.0)
	{
		return 0;
	}
	return static_cast<int64_t>(std::ceil(SecondsRemaining / 300.0));
}

int64_t PlayerXpToNextLevel(int Level)
{
	const int64_t L = (std::max)(1, Level);
	return 50 * L * L + 50;
}

int MaxStructuresForPlayerLevel(int PlayerLevel)
{
	return 8 + 2 * (std::max)(1, PlayerLevel);
}
} // namespace Wyrm

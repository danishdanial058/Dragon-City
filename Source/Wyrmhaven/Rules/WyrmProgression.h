// Wyrmhaven - levelling, economy and time formulas. All balance numbers live here.
#pragma once

#include "WyrmCatalog.h"

#include <cstdint>

namespace Wyrm
{
constexpr int MaxDragonLevel = 40;
constexpr int BreedMinLevel = 4;
constexpr int MaxEggsInHatchery = 4;
constexpr int MaxArenaTeamSize = 3;

// Food needed to go from Level to Level + 1.
int64_t FoodToNextLevel(int Level);

int ScaledHealth(const SpeciesDef& Species, int Level);
int ScaledAttack(const SpeciesDef& Species, int Level);
int ScaledGoldPerMinute(const SpeciesDef& Species, int Level);

// How many of a species' skills are usable at this level (2 at start, 3 at 10, 4 at 20).
int UnlockedSkillCount(int Level, int TotalSkills);

// Gold a habitat holds after ElapsedSeconds. Banked is gold already stored. Never exceeds Cap.
int64_t HabitatGold(int64_t Banked, int64_t GoldPerMinute, double ElapsedSeconds, int64_t Cap);

bool CanLiveIn(const SpeciesDef& Species, const HabitatDef& Habitat);

int64_t SellValue(const SpeciesDef& Species, int Level);

// Fair, transparent speed-ups: 1 gem per started 5 minutes.
int64_t GemsToSkip(double SecondsRemaining);

int64_t PlayerXpToNextLevel(int Level);
int MaxStructuresForPlayerLevel(int PlayerLevel);
} // namespace Wyrm

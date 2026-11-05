// Wyrmhaven - turn based arena battles (teams of up to 3).
#pragma once

#include "WyrmCatalog.h"
#include "WyrmRng.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Wyrm
{
struct Combatant
{
	std::string SpeciesId;
	std::string Name;
	ElementMask Elements = 0;
	int Level = 1;
	int MaxHealth = 1;
	int Health = 1;
	int Attack = 1;
	std::vector<SkillDef> Skills; // only skills unlocked at this level

	bool IsFainted() const { return Health <= 0; }
};

enum class BattleEventType : uint8_t
{
	SkillHit,
	SkillMissed,
	Fainted,
	SwitchedIn,
	Victory,
	Defeat
};

struct BattleEvent
{
	BattleEventType Type = BattleEventType::SkillHit;
	int Side = 0; // 0 = player, 1 = enemy
	std::string ActorName;
	std::string SkillName;
	int Damage = 0;
	float Effectiveness = 1.0f;
	bool bCritical = false;
};

// Human readable one-liner for logs and UI.
std::string DescribeEvent(const BattleEvent& Event);

bool MakeCombatant(const Catalog& Data, const std::string& SpeciesId, int Level, const std::string& Nickname,
	Combatant& Out);

// Average damage including accuracy and crits; used by the AI and for UI hints.
double ExpectedDamage(const Combatant& Attacker, const SkillDef& Skill, const Combatant& Defender);

class Battle
{
public:
	Battle(std::vector<Combatant> PlayerTeam, std::vector<Combatant> EnemyTeam, uint64_t Seed);

	bool IsOver() const { return WinnerSide >= 0; }
	int Winner() const { return WinnerSide; } // -1 still running, 0 player, 1 enemy
	int TurnNumber() const { return Turn; }

	const Combatant* Active(int Side) const;
	const std::vector<Combatant>& Team(int Side) const { return Teams[Side == 0 ? 0 : 1]; }

	// Player uses a skill, then the enemy answers if it is still standing.
	std::vector<BattleEvent> PlayTurn(int PlayerSkillIndex);

	static int ChooseAiSkill(const Combatant& Self, const Combatant& Target);

	static constexpr int MaxTurns = 200;

private:
	void Act(int Side, int SkillIndex, std::vector<BattleEvent>& Events);
	void ResolveFaint(int Side, std::vector<BattleEvent>& Events);
	void FinishIfStuck(std::vector<BattleEvent>& Events);

	std::vector<Combatant> Teams[2];
	int ActiveIndex[2] = {0, 0};
	int Turn = 0;
	int WinnerSide = -1;
	Rng Random;
};

struct BattleRewards
{
	int64_t Gold = 0;
	int64_t PlayerXp = 0;
};

BattleRewards ComputeRewards(const std::vector<Combatant>& EnemyTeam);

// Opponents scale with the arena tier (1 = first fight).
std::vector<Combatant> GenerateArenaTeam(const Catalog& Data, int Tier, Rng& Random);
} // namespace Wyrm

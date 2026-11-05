#include "WyrmBattle.h"

#include "WyrmProgression.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace Wyrm
{
namespace
{
constexpr double GWyrmCritChance = 0.10;
constexpr double GWyrmCritMultiplier = 1.5;
constexpr double GWyrmVarianceMin = 0.85;
constexpr double GWyrmVarianceSpan = 0.15;

double WyrmTeamHealthFraction(const std::vector<Combatant>& Team)
{
	double Health = 0.0;
	double Max = 0.0;
	for (const Combatant& C : Team)
	{
		Health += static_cast<double>((std::max)(0, C.Health));
		Max += static_cast<double>(C.MaxHealth);
	}
	return Max > 0.0 ? Health / Max : 0.0;
}
} // namespace

std::string DescribeEvent(const BattleEvent& Event)
{
	char Buffer[256];
	switch (Event.Type)
	{
	case BattleEventType::SkillHit:
	{
		const char* Suffix = "";
		if (Event.Effectiveness > 1.01f)
		{
			Suffix = " It's super effective!";
		}
		else if (Event.Effectiveness < 0.99f)
		{
			Suffix = " It's not very effective.";
		}
		std::snprintf(Buffer, sizeof(Buffer), "%s used %s for %d damage%s%s", Event.ActorName.c_str(),
			Event.SkillName.c_str(), Event.Damage, Event.bCritical ? " (critical!)" : "", Suffix);
		break;
	}
	case BattleEventType::SkillMissed:
		std::snprintf(Buffer, sizeof(Buffer), "%s used %s but missed", Event.ActorName.c_str(), Event.SkillName.c_str());
		break;
	case BattleEventType::Fainted:
		std::snprintf(Buffer, sizeof(Buffer), "%s fainted", Event.ActorName.c_str());
		break;
	case BattleEventType::SwitchedIn:
		std::snprintf(Buffer, sizeof(Buffer), "%s enters the arena", Event.ActorName.c_str());
		break;
	case BattleEventType::Victory:
		std::snprintf(Buffer, sizeof(Buffer), "Victory!");
		break;
	case BattleEventType::Defeat:
		std::snprintf(Buffer, sizeof(Buffer), "Defeat...");
		break;
	default:
		Buffer[0] = '\0';
		break;
	}
	return std::string(Buffer);
}

bool MakeCombatant(const Catalog& Data, const std::string& SpeciesId, int Level, const std::string& Nickname,
	Combatant& Out)
{
	const SpeciesDef* Species = Data.FindSpecies(SpeciesId);
	if (!Species)
	{
		return false;
	}
	const int ClampedLevel = (std::max)(1, (std::min)(Level, MaxDragonLevel));
	Out = Combatant();
	Out.SpeciesId = Species->Id;
	Out.Name = Nickname.empty() ? Species->Name : Nickname;
	Out.Elements = Species->Elements;
	Out.Level = ClampedLevel;
	Out.MaxHealth = ScaledHealth(*Species, ClampedLevel);
	Out.Health = Out.MaxHealth;
	Out.Attack = ScaledAttack(*Species, ClampedLevel);

	const int Unlocked = UnlockedSkillCount(ClampedLevel, static_cast<int>(Species->Skills.size()));
	for (int Index = 0; Index < Unlocked; ++Index)
	{
		if (const SkillDef* Skill = Data.FindSkill(Species->Skills[static_cast<size_t>(Index)]))
		{
			Out.Skills.push_back(*Skill);
		}
	}
	return !Out.Skills.empty();
}

double ExpectedDamage(const Combatant& Attacker, const SkillDef& Skill, const Combatant& Defender)
{
	const double Base = static_cast<double>(Attacker.Attack) * static_cast<double>(Skill.Power) / 100.0;
	const double Eff = static_cast<double>(Effectiveness(Skill.Elem, Defender.Elements));
	const double AverageVariance = GWyrmVarianceMin + GWyrmVarianceSpan * 0.5;
	const double AverageCrit = 1.0 + GWyrmCritChance * (GWyrmCritMultiplier - 1.0);
	const double HitChance = static_cast<double>(Skill.Accuracy) / 100.0;
	return Base * Eff * AverageVariance * AverageCrit * HitChance;
}

Battle::Battle(std::vector<Combatant> PlayerTeam, std::vector<Combatant> EnemyTeam, uint64_t Seed)
	: Random(Seed)
{
	Teams[0] = std::move(PlayerTeam);
	Teams[1] = std::move(EnemyTeam);
	if (Teams[0].empty())
	{
		WinnerSide = 1;
	}
	else if (Teams[1].empty())
	{
		WinnerSide = 0;
	}
}

const Combatant* Battle::Active(int Side) const
{
	const int S = Side == 0 ? 0 : 1;
	const int Index = ActiveIndex[S];
	if (Index < 0 || Index >= static_cast<int>(Teams[S].size()))
	{
		return nullptr;
	}
	return &Teams[S][static_cast<size_t>(Index)];
}

int Battle::ChooseAiSkill(const Combatant& Self, const Combatant& Target)
{
	int Best = 0;
	double BestDamage = -1.0;
	for (size_t Index = 0; Index < Self.Skills.size(); ++Index)
	{
		const double Damage = ExpectedDamage(Self, Self.Skills[Index], Target);
		if (Damage > BestDamage)
		{
			BestDamage = Damage;
			Best = static_cast<int>(Index);
		}
	}
	return Best;
}

void Battle::Act(int Side, int SkillIndex, std::vector<BattleEvent>& Events)
{
	const int Other = 1 - Side;
	Combatant& Attacker = Teams[Side][static_cast<size_t>(ActiveIndex[Side])];
	Combatant& Defender = Teams[Other][static_cast<size_t>(ActiveIndex[Other])];
	if (Attacker.Skills.empty())
	{
		return;
	}
	const int Clamped = (std::max)(0, (std::min)(SkillIndex, static_cast<int>(Attacker.Skills.size()) - 1));
	const SkillDef& Skill = Attacker.Skills[static_cast<size_t>(Clamped)];

	BattleEvent Event;
	Event.Side = Side;
	Event.ActorName = Attacker.Name;
	Event.SkillName = Skill.Name;

	if (Random.NextDouble() * 100.0 >= static_cast<double>(Skill.Accuracy))
	{
		Event.Type = BattleEventType::SkillMissed;
		Events.push_back(Event);
		return;
	}

	const float Eff = Effectiveness(Skill.Elem, Defender.Elements);
	const double Variance = GWyrmVarianceMin + GWyrmVarianceSpan * Random.NextDouble();
	const bool bCrit = Random.NextDouble() < GWyrmCritChance;
	const double Raw = static_cast<double>(Attacker.Attack) * static_cast<double>(Skill.Power) / 100.0 *
		static_cast<double>(Eff) * Variance * (bCrit ? GWyrmCritMultiplier : 1.0);
	const int Damage = (std::max)(1, static_cast<int>(std::lround(Raw)));

	Defender.Health = (std::max)(0, Defender.Health - Damage);
	Event.Type = BattleEventType::SkillHit;
	Event.Damage = Damage;
	Event.Effectiveness = Eff;
	Event.bCritical = bCrit;
	Events.push_back(Event);

	if (Defender.IsFainted())
	{
		ResolveFaint(Other, Events);
	}
}

void Battle::ResolveFaint(int Side, std::vector<BattleEvent>& Events)
{
	BattleEvent Fainted;
	Fainted.Type = BattleEventType::Fainted;
	Fainted.Side = Side;
	Fainted.ActorName = Teams[Side][static_cast<size_t>(ActiveIndex[Side])].Name;
	Events.push_back(Fainted);

	for (int Index = 0; Index < static_cast<int>(Teams[Side].size()); ++Index)
	{
		if (!Teams[Side][static_cast<size_t>(Index)].IsFainted())
		{
			ActiveIndex[Side] = Index;
			BattleEvent Switched;
			Switched.Type = BattleEventType::SwitchedIn;
			Switched.Side = Side;
			Switched.ActorName = Teams[Side][static_cast<size_t>(Index)].Name;
			Events.push_back(Switched);
			return;
		}
	}

	WinnerSide = 1 - Side;
	BattleEvent End;
	End.Type = WinnerSide == 0 ? BattleEventType::Victory : BattleEventType::Defeat;
	End.Side = WinnerSide;
	Events.push_back(End);
}

void Battle::FinishIfStuck(std::vector<BattleEvent>& Events)
{
	if (IsOver() || Turn < MaxTurns)
	{
		return;
	}
	// Extremely long fight: whoever has more health left (as a fraction) wins. Ties go to the enemy.
	WinnerSide = WyrmTeamHealthFraction(Teams[0]) > WyrmTeamHealthFraction(Teams[1]) ? 0 : 1;
	BattleEvent End;
	End.Type = WinnerSide == 0 ? BattleEventType::Victory : BattleEventType::Defeat;
	End.Side = WinnerSide;
	Events.push_back(End);
}

std::vector<BattleEvent> Battle::PlayTurn(int PlayerSkillIndex)
{
	std::vector<BattleEvent> Events;
	if (IsOver())
	{
		return Events;
	}
	++Turn;

	const int EnemyBefore = ActiveIndex[1];
	Act(0, PlayerSkillIndex, Events);

	// A freshly switched-in enemy does not get a free hit.
	if (!IsOver() && ActiveIndex[1] == EnemyBefore)
	{
		const Combatant* Enemy = Active(1);
		const Combatant* Player = Active(0);
		if (Enemy && Player)
		{
			Act(1, ChooseAiSkill(*Enemy, *Player), Events);
		}
	}

	FinishIfStuck(Events);
	return Events;
}

BattleRewards ComputeRewards(const std::vector<Combatant>& EnemyTeam)
{
	BattleRewards Rewards;
	for (const Combatant& C : EnemyTeam)
	{
		Rewards.Gold += 40 * static_cast<int64_t>(C.Level) + 60;
		Rewards.PlayerXp += 10 * static_cast<int64_t>(C.Level) + 10;
	}
	return Rewards;
}

std::vector<Combatant> GenerateArenaTeam(const Catalog& Data, int Tier, Rng& Random)
{
	const int SafeTier = (std::max)(1, Tier);
	Rarity MaxRarity = Rarity::Common;
	if (SafeTier >= 3)
	{
		MaxRarity = Rarity::Rare;
	}
	if (SafeTier >= 6)
	{
		MaxRarity = Rarity::Epic;
	}
	if (SafeTier >= 10)
	{
		MaxRarity = Rarity::Legendary;
	}

	std::vector<const SpeciesDef*> Pool;
	for (const SpeciesDef& S : Data.Species)
	{
		if (static_cast<int>(S.Rar) <= static_cast<int>(MaxRarity))
		{
			Pool.push_back(&S);
		}
	}

	std::vector<Combatant> Team;
	if (Pool.empty())
	{
		return Team;
	}
	const int Size = (std::min)(MaxArenaTeamSize, 1 + (SafeTier - 1) / 2);
	for (int Index = 0; Index < Size; ++Index)
	{
		const SpeciesDef* Species = Pool[static_cast<size_t>(Random.RangeInt(0, static_cast<int>(Pool.size()) - 1))];
		const int Level = (std::min)(MaxDragonLevel, 1 + (SafeTier - 1) * 2 + Random.RangeInt(0, 1));
		Combatant C;
		if (MakeCombatant(Data, Species->Id, Level, std::string("Wild ") + Species->Name, C))
		{
			Team.push_back(C);
		}
	}
	return Team;
}
} // namespace Wyrm

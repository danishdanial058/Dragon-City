// Unit tests for the engine-independent game rules in Source/Wyrmhaven/Rules.
// Build and run with:  make -C Tests
#include "WyrmBattle.h"
#include "WyrmBreeding.h"
#include "WyrmCatalog.h"
#include "WyrmElements.h"
#include "WyrmProgression.h"
#include "WyrmRng.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace Wyrm;

static int GFailures = 0;
static int GChecks = 0;

#define EXPECT(Cond)                                                                    \
	do                                                                                  \
	{                                                                                   \
		++GChecks;                                                                      \
		if (!(Cond))                                                                    \
		{                                                                               \
			++GFailures;                                                                \
			std::printf("  FAILED %s:%d: %s\n", __FILE__, __LINE__, #Cond);           \
		}                                                                               \
	} while (0)

static SkillDef MakeSkill(const char* Id, Element E, int Power, int Accuracy)
{
	SkillDef S;
	S.Id = Id;
	S.Name = Id;
	S.Elem = E;
	S.Power = Power;
	S.Accuracy = Accuracy;
	return S;
}

static SpeciesDef MakeSpecies(const char* Id, ElementMask Mask, Rarity R, std::vector<std::string> Skills)
{
	SpeciesDef S;
	S.Id = Id;
	S.Name = Id;
	S.Elements = Mask;
	S.Rar = R;
	S.Skills = std::move(Skills);
	return S;
}

static Catalog MakeTestCatalog()
{
	Catalog C;
	C.Skills = {MakeSkill("fire1", Element::Fire, 40, 100), MakeSkill("fire2", Element::Fire, 70, 85),
		MakeSkill("water1", Element::Water, 40, 100), MakeSkill("water2", Element::Water, 70, 85),
		MakeSkill("nature1", Element::Nature, 40, 100), MakeSkill("nature2", Element::Nature, 70, 85)};
	C.Species = {
		MakeSpecies("ember", MaskOf(Element::Fire), Rarity::Common, {"fire1", "fire2"}),
		MakeSpecies("tide", MaskOf(Element::Water), Rarity::Common, {"water1", "water2"}),
		MakeSpecies("moss", MaskOf(Element::Nature), Rarity::Common, {"nature1", "nature2"}),
		MakeSpecies("steam", MaskOf(Element::Fire) | MaskOf(Element::Water), Rarity::Rare,
			{"fire1", "water1", "fire2", "water2"}),
		MakeSpecies("bog", MaskOf(Element::Water) | MaskOf(Element::Nature), Rarity::Epic,
			{"water1", "nature1", "water2", "nature2"}),
	};
	SpeciesDef Legend = MakeSpecies("legend", MaskOf(Element::Fire) | MaskOf(Element::Nature), Rarity::Legendary,
		{"fire1", "nature1", "fire2", "nature2"});
	Legend.RecipeA = "steam";
	Legend.RecipeB = "bog";
	C.Species.push_back(Legend);

	for (int Index = 0; Index < ElementCount; ++Index)
	{
		HabitatDef H;
		H.Id = std::string("hab_") + ElementName(static_cast<Element>(Index));
		H.Name = H.Id;
		H.Elem = static_cast<Element>(Index);
		C.Habitats.push_back(H);
	}
	CropDef Crop;
	Crop.Id = "berry";
	Crop.Name = "Berry";
	Crop.Cost = 10;
	Crop.Food = 5;
	C.Crops.push_back(Crop);
	return C;
}

static double SumOdds(const std::vector<BreedOdds>& Odds)
{
	double Sum = 0.0;
	for (const BreedOdds& O : Odds)
	{
		Sum += O.Probability;
	}
	return Sum;
}

static bool OddsContain(const std::vector<BreedOdds>& Odds, const char* Id)
{
	for (const BreedOdds& O : Odds)
	{
		if (O.SpeciesId == Id)
		{
			return true;
		}
	}
	return false;
}

static double OddsOf(const std::vector<BreedOdds>& Odds, const char* Id)
{
	for (const BreedOdds& O : Odds)
	{
		if (O.SpeciesId == Id)
		{
			return O.Probability;
		}
	}
	return 0.0;
}

static void TestElements()
{
	std::printf("elements\n");
	// Every element is strong against exactly two and weak against exactly two.
	for (int A = 0; A < ElementCount; ++A)
	{
		int Strong = 0;
		int WeakTo = 0;
		for (int D = 0; D < ElementCount; ++D)
		{
			Strong += IsStrongAgainst(static_cast<Element>(A), static_cast<Element>(D)) ? 1 : 0;
			WeakTo += IsStrongAgainst(static_cast<Element>(D), static_cast<Element>(A)) ? 1 : 0;
		}
		EXPECT(Strong == 2);
		EXPECT(WeakTo == 2);
	}
	EXPECT(Effectiveness(Element::Water, Element::Fire) == 2.0f);
	EXPECT(Effectiveness(Element::Fire, Element::Water) == 0.5f);
	EXPECT(Effectiveness(Element::Fire, Element::Air) == 1.0f);
	EXPECT(Effectiveness(Element::Light, Element::Shadow) == 2.0f);
	EXPECT(Effectiveness(Element::Shadow, Element::Light) == 2.0f);
	const ElementMask NatureIce = static_cast<ElementMask>(MaskOf(Element::Nature) | MaskOf(Element::Ice));
	EXPECT(Effectiveness(Element::Fire, NatureIce) == 4.0f);
	EXPECT(CountElements(NatureIce) == 2);
	EXPECT(PrimaryElement(NatureIce) == Element::Nature);
	EXPECT(SecondaryElement(NatureIce) == Element::Ice);
	EXPECT(SecondaryElement(MaskOf(Element::Air)) == Element::Air);

	Element Parsed = Element::Fire;
	EXPECT(ParseElement("Lightning", Parsed) && Parsed == Element::Lightning);
	EXPECT(!ParseElement("Plasma", Parsed));
}

static void TestCatalog()
{
	std::printf("catalog\n");
	Catalog C = MakeTestCatalog();
	EXPECT(C.Validate().empty());
	EXPECT(C.FindSpecies("ember") != nullptr);
	EXPECT(C.FindSpecies("nope") == nullptr);

	C.Species.push_back(MakeSpecies("ember", MaskOf(Element::Fire), Rarity::Common, {"fire1", "missing"}));
	C.Habitats.pop_back();
	const std::vector<std::string> Problems = C.Validate();
	EXPECT(Problems.size() == 3); // duplicate id, unknown skill, element with no habitat
}

static void TestRng()
{
	std::printf("rng\n");
	Rng A(42);
	Rng B(42);
	for (int Index = 0; Index < 100; ++Index)
	{
		EXPECT(A.NextU64() == B.NextU64());
	}
	Rng C(7);
	for (int Index = 0; Index < 1000; ++Index)
	{
		const int V = C.RangeInt(3, 6);
		EXPECT(V >= 3 && V <= 6);
		const double D = C.NextDouble();
		EXPECT(D >= 0.0 && D < 1.0);
	}
	// Restoring a saved state continues the exact same sequence.
	Rng D(99);
	D.NextU64();
	Rng E(D.GetState());
	EXPECT(D.NextU64() == E.NextU64());
}

static void TestBreeding()
{
	std::printf("breeding\n");
	const Catalog C = MakeTestCatalog();
	std::vector<BreedOdds> Odds;

	EXPECT(ComputeBreedOdds(C, {"ember", 3}, {"tide", 10}, 0, Odds) == BreedError::ParentTooLow);
	EXPECT(ComputeBreedOdds(C, {"ember", 5}, {"ghost", 10}, 0, Odds) == BreedError::UnknownSpecies);

	// Pure + pure of the same element only gives that element.
	EXPECT(ComputeBreedOdds(C, {"ember", 4}, {"ember", 4}, 0, Odds) == BreedError::None);
	EXPECT(Odds.size() == 1 && Odds[0].SpeciesId == "ember");

	// Fire + Water gives fire, water or the hybrid; never nature.
	EXPECT(ComputeBreedOdds(C, {"ember", 4}, {"tide", 4}, 0, Odds) == BreedError::None);
	EXPECT(std::fabs(SumOdds(Odds) - 1.0) < 1e-9);
	EXPECT(OddsContain(Odds, "ember") && OddsContain(Odds, "tide") && OddsContain(Odds, "steam"));
	EXPECT(!OddsContain(Odds, "moss") && !OddsContain(Odds, "legend"));
	EXPECT(Odds.front().Probability >= Odds.back().Probability); // sorted

	// Legendary only from its exact recipe (either parent order).
	EXPECT(ComputeBreedOdds(C, {"bog", 10}, {"steam", 10}, 0, Odds) == BreedError::None);
	EXPECT(OddsContain(Odds, "legend"));
	EXPECT(ComputeBreedOdds(C, {"bog", 10}, {"ember", 10}, 0, Odds) == BreedError::None);
	EXPECT(!OddsContain(Odds, "legend"));

	// Pity and parent level raise rare odds.
	std::vector<BreedOdds> Lucky;
	EXPECT(ComputeBreedOdds(C, {"tide", 4}, {"moss", 4}, 0, Odds) == BreedError::None);
	EXPECT(ComputeBreedOdds(C, {"tide", 4}, {"moss", 4}, 10, Lucky) == BreedError::None);
	EXPECT(OddsOf(Lucky, "bog") > OddsOf(Odds, "bog"));
	std::vector<BreedOdds> HighLevel;
	EXPECT(ComputeBreedOdds(C, {"ember", 30}, {"tide", 30}, 0, HighLevel) == BreedError::None);
	EXPECT(ComputeBreedOdds(C, {"ember", 4}, {"tide", 4}, 0, Odds) == BreedError::None);
	EXPECT(OddsOf(HighLevel, "steam") > OddsOf(Odds, "steam"));

	// Pity counter.
	EXPECT(NextPity(3, *C.FindSpecies("ember")) == 4);
	EXPECT(NextPity(3, *C.FindSpecies("bog")) == 0);
	EXPECT(NextPity(20, *C.FindSpecies("tide")) == 20);

	// Rolled results follow the published odds.
	Rng Random(1234);
	std::map<std::string, int> Counts;
	const int Rolls = 20000;
	for (int Index = 0; Index < Rolls; ++Index)
	{
		std::string Result;
		EXPECT(Breed(C, {"ember", 4}, {"tide", 4}, 0, Random, Result) == BreedError::None);
		++Counts[Result];
	}
	EXPECT(ComputeBreedOdds(C, {"ember", 4}, {"tide", 4}, 0, Odds) == BreedError::None);
	for (const BreedOdds& O : Odds)
	{
		const double Observed = static_cast<double>(Counts[O.SpeciesId]) / Rolls;
		EXPECT(std::fabs(Observed - O.Probability) < 0.02);
	}
}

static void TestProgression()
{
	std::printf("progression\n");
	const Catalog C = MakeTestCatalog();
	const SpeciesDef& Ember = *C.FindSpecies("ember");
	EXPECT(FoodToNextLevel(1) == 10);
	EXPECT(FoodToNextLevel(3) == 50);
	EXPECT(ScaledHealth(Ember, 1) == Ember.BaseHealth);
	EXPECT(ScaledHealth(Ember, 11) > ScaledHealth(Ember, 10));
	EXPECT(ScaledAttack(Ember, 20) > ScaledAttack(Ember, 1));
	EXPECT(UnlockedSkillCount(1, 4) == 2);
	EXPECT(UnlockedSkillCount(10, 4) == 3);
	EXPECT(UnlockedSkillCount(25, 4) == 4);
	EXPECT(UnlockedSkillCount(25, 2) == 2);

	EXPECT(HabitatGold(0, 60, 30.0, 1000) == 30);
	EXPECT(HabitatGold(100, 60, 600.0, 300) == 300); // capped
	EXPECT(HabitatGold(50, 60, -10.0, 300) == 50);   // clock went backwards: no change
	EXPECT(HabitatGold(0, 10, 365.0 * 24 * 3600, 5000) == 5000);

	EXPECT(CanLiveIn(*C.FindSpecies("steam"), *C.FindHabitat("hab_Water")));
	EXPECT(!CanLiveIn(*C.FindSpecies("steam"), *C.FindHabitat("hab_Nature")));

	EXPECT(GemsToSkip(0.0) == 0);
	EXPECT(GemsToSkip(1.0) == 1);
	EXPECT(GemsToSkip(300.0) == 1);
	EXPECT(GemsToSkip(301.0) == 2);
	EXPECT(SellValue(Ember, 1) > 0);
	EXPECT(PlayerXpToNextLevel(2) > PlayerXpToNextLevel(1));
}

static void TestBattle()
{
	std::printf("battle\n");
	const Catalog C = MakeTestCatalog();

	Combatant Ember;
	EXPECT(MakeCombatant(C, "ember", 1, "", Ember));
	EXPECT(Ember.Skills.size() == 2);
	EXPECT(Ember.Health == Ember.MaxHealth);
	Combatant Steam;
	EXPECT(MakeCombatant(C, "steam", 10, "Puffy", Steam));
	EXPECT(Steam.Skills.size() == 3 && Steam.Name == "Puffy");
	EXPECT(!MakeCombatant(C, "nope", 1, "", Steam));

	// AI picks the super effective move: Water vs Fire.
	Combatant Tide;
	EXPECT(MakeCombatant(C, "tide", 5, "", Tide));
	Combatant Moss;
	EXPECT(MakeCombatant(C, "moss", 5, "", Moss));
	EXPECT(MakeCombatant(C, "steam", 10, "", Steam));
	EXPECT(Battle::ChooseAiSkill(Steam, Ember) == 2 || Battle::ChooseAiSkill(Steam, Ember) == 1);
	EXPECT(Steam.Skills[static_cast<size_t>(Battle::ChooseAiSkill(Steam, Ember))].Elem == Element::Water);
	EXPECT(ExpectedDamage(Tide, Tide.Skills[0], Ember) > ExpectedDamage(Tide, Tide.Skills[0], Moss));

	// Battles always finish and a much stronger team wins.
	int StrongWins = 0;
	for (uint64_t Seed = 1; Seed <= 50; ++Seed)
	{
		Combatant Strong;
		Combatant Weak;
		EXPECT(MakeCombatant(C, "tide", 30, "", Strong));
		EXPECT(MakeCombatant(C, "ember", 2, "", Weak));
		Battle Fight({Strong}, {Weak, Weak}, Seed);
		int Guard = 0;
		while (!Fight.IsOver() && Guard++ < 1000)
		{
			const Combatant* Me = Fight.Active(0);
			const Combatant* Them = Fight.Active(1);
			Fight.PlayTurn(Battle::ChooseAiSkill(*Me, *Them));
		}
		EXPECT(Fight.IsOver());
		StrongWins += Fight.Winner() == 0 ? 1 : 0;
	}
	EXPECT(StrongWins == 50);

	// Events and the final message.
	Combatant P;
	Combatant E;
	EXPECT(MakeCombatant(C, "tide", 40, "", P));
	EXPECT(MakeCombatant(C, "ember", 1, "", E));
	Battle Quick({P}, {E}, 5);
	std::vector<BattleEvent> AllEvents;
	while (!Quick.IsOver())
	{
		const std::vector<BattleEvent> Events = Quick.PlayTurn(1);
		AllEvents.insert(AllEvents.end(), Events.begin(), Events.end());
	}
	EXPECT(!AllEvents.empty() && AllEvents.back().Type == BattleEventType::Victory);
	EXPECT(Quick.PlayTurn(0).empty()); // no turns after the end
	EXPECT(!DescribeEvent(AllEvents.front()).empty());

	// Empty teams resolve immediately.
	Battle Empty({}, {E}, 1);
	EXPECT(Empty.IsOver() && Empty.Winner() == 1);

	// Arena teams scale with tier.
	Rng Random(77);
	const std::vector<Combatant> Tier1 = GenerateArenaTeam(C, 1, Random);
	const std::vector<Combatant> Tier9 = GenerateArenaTeam(C, 9, Random);
	EXPECT(Tier1.size() == 1);
	EXPECT(Tier9.size() == 3);
	EXPECT(Tier9[0].Level > Tier1[0].Level);
	for (const Combatant& Foe : Tier1)
	{
		EXPECT(C.FindSpecies(Foe.SpeciesId)->Rar == Rarity::Common);
	}
	const BattleRewards Rewards = ComputeRewards(Tier9);
	EXPECT(Rewards.Gold > 0 && Rewards.PlayerXp > 0);
}

int main()
{
	TestElements();
	TestCatalog();
	TestRng();
	TestBreeding();
	TestProgression();
	TestBattle();
	std::printf("\n%d checks, %d failures\n", GChecks, GFailures);
	return GFailures == 0 ? 0 : 1;
}

# Wyrmhaven – Game Design

Wyrmhaven is a dragon-collecting island builder: hatch dragons, house them in habitats that earn gold,
grow food to level them up, breed them for rarer species, and battle in the arena.

All numbers below live in `Content/Data/GameData.json` (content) or
`Source/Wyrmhaven/Rules/WyrmProgression.cpp` (formulas), so balancing never needs engine code changes.

## Core loop

```
 Habitats earn gold ──► spend gold on crops ──► food levels up dragons
        ▲                                               │
        │                                               ▼
 hatch new dragons ◄── breeding cave (level 4+) ◄── stronger, richer dragons
        │
        └──► arena battles ──► gold, island XP, higher tiers
```

## Elements

Ten elements. Each is strong against exactly two and weak against exactly two (unit tested).

| Attacker  | Strong against      |
|-----------|---------------------|
| Fire      | Nature, Ice         |
| Water     | Fire, Metal         |
| Earth     | Lightning, Fire     |
| Air       | Earth, Shadow       |
| Nature    | Water, Earth        |
| Lightning | Water, Air          |
| Ice       | Nature, Air         |
| Metal     | Ice, Light          |
| Shadow    | Light, Lightning    |
| Light     | Shadow, Metal       |

Strong = ×2 damage, weak = ×0.5, multiplied across both of a hybrid's elements (×0.25 … ×4).

## Dragons

24 original species: 10 pure (Common), 7 hybrids (Rare), 5 hybrids (Epic), 2 Legendaries.

* **Level 1–40.** Feeding costs `5·L² + 5` food per level.
* Health +12 % per level, attack +10 % per level, gold income +8 % per level.
* Skills: 2 at level 1, a 3rd at level 10, a 4th at level 20.
* A dragon can live in any habitat that matches one of its elements.

## Breeding

* Both parents must be level 4+.
* Offspring elements must be a subset of the parents' combined elements.
* Legendaries come only from a specific parent pair (shown in the Dragonpedia).
* Base weights: Common 100, Rare 30, Epic 8, Legendary 2; ×1.5 if the species matches a parent;
  +1 % rare-or-better weight for every level the weaker parent has above 4.
* **Odds are shown before breeding** – the exact percentages the game rolls with.
* **Pity:** each breed that isn't Epic+ adds +25 % Epic/Legendary weight (cap +500 %), reset on success.
* The result is rolled and saved when breeding starts, so closing the game can't re-roll it.

## Economy

| Thing         | Rule                                                                 |
|---------------|----------------------------------------------------------------------|
| Gold          | Habitats fill while you're away, up to their storage cap             |
| Food          | Farms: Sunberries 30 s, Emberroot 5 m, Dragonfruit 30 m, Starmelon 2 h |
| Gems          | 30 at start, +5 per island level                                     |
| Speed-ups     | 1 gem per started 5 minutes remaining – the price is always shown    |
| Island level  | XP from almost every action; each level allows 2 more buildings      |

## Arena

* Your three highest-level dragons fight in order against generated opponents.
* Opponents scale with tier: tier 1 is one level-1 common; tier 3+ adds rares, 6+ epics, 10+ legendaries.
* AI always picks the skill with the best expected damage; skill buttons show "strong!" / "weak" hints.
* Winning gives gold and XP and unlocks the next tier.

## Player-friendly choices (vs. typical games in the genre)

* No energy or stamina limits.
* No hidden odds and a pity system for rare breeds.
* Cheap, linear and predictable speed-ups.
* Tap a habitat to collect gold, tap a ripe farm to harvest – no extra menus.
* "Collect all gold" button.
* Works offline; progress is saved locally every 20 seconds.

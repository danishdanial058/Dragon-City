# Wyrmhaven

An original dragon-breeding island game for **Unreal Engine 5** (PC, Android, iOS).
Hatch dragons, build habitats that earn gold, farm food to level them up, breed rare hybrids
and fight in the arena.

> Inspired by the dragon-breeding genre (games like *Dragon City*), but every name, dragon and
> system here is original. It is not affiliated with or based on any existing game's assets.

## What's in the box

| Feature | Details |
|---|---|
| 24 dragons, 10 elements | Pure, hybrid, epic and legendary species with their own skills |
| Breeding | Element-based offspring, **odds shown before you breed**, pity system for rares |
| Hatchery & shop | Eggs with timers, buy common eggs with gold |
| Habitats & farms | Gold piles up while you're offline, four crops to grow food |
| Levelling | Feed dragons to level 40, unlock skills at 10 and 20 |
| Arena | Turn-based 3v3 battles with type matchups and scaling tiers |
| Dragonpedia | Tracks discovered species and shows legendary recipes |
| Saves | Automatic local save every 20 s |
| Runs with zero art | Placeholder dragons and buildings are built from engine shapes; swap in real models any time |

See [Docs/GameDesign.md](Docs/GameDesign.md) for the full design and balance numbers.

## Getting started

**You need:** Unreal Engine 5.5 (from the Epic Games Launcher) and a C++ compiler
(Visual Studio 2022 with the "Game development with C++" workload on Windows, Xcode on macOS).

1. Clone this repository.
2. Right-click `Wyrmhaven.uproject` → **Generate Visual Studio project files** (Windows), or just
   double-click it and let Unreal build the code.
3. Open the project and press **Play**. No level setup is needed – the island, camera and
   interface are created automatically.

If you use a different engine version, right-click the `.uproject` → **Switch Unreal Engine version**.

### Controls

| Action | Mouse / keyboard | Touch |
|---|---|---|
| Pan | Drag with left mouse, or WASD / arrows | Drag one finger |
| Zoom | Mouse wheel, or Q / E | Pinch |
| Select a building or dragon | Click | Tap |

Tapping a habitat collects its gold; tapping a ripe farm harvests it.

## Project layout

```
Wyrmhaven.uproject
Config/                     Engine, game and input settings (mobile-ready)
Content/Data/GameData.json  All dragons, skills, habitats, crops and starting values
Source/Wyrmhaven/
  Rules/   Pure C++ game rules (elements, breeding, battles, economy) – no engine code, unit tested
  Game/    Save game, data loader, game mode, and the subsystem that runs every game action
  World/   Island, buildings, dragons, camera and touch/mouse controls
  UI/      The in-game interface (built in C++, no widget assets required)
Tests/     Unit tests for Rules/ (make -C Tests)
Tools/     validate_data.py – checks GameData.json for mistakes
Docs/      Game design document
```

The engine-independent `Rules/` layer is deliberate: balance and logic can be tested in seconds
without opening Unreal, and the same rules could power a server later.

## Changing the game

**Add a dragon:** add an entry to `species` in `Content/Data/GameData.json`, then run
`python3 Tools/validate_data.py`. Pure dragons need an element habitat; hybrids can live in either
element's habitat; set `"recipe": ["parentA", "parentB"]` for a legendary.

**Add art:**
1. Import a model (FBX/glTF from Blender, or free assets from [Fab](https://www.fab.com/)).
2. Copy its reference path (right-click the asset → *Copy Reference*), e.g.
   `/Game/Dragons/SM_Emberling.SM_Emberling`.
3. Paste it into that species' `"mesh"` field. The placeholder is replaced automatically.

For animated (skeletal) dragons or custom buildings, make Blueprint subclasses of
`WyrmDragonActor` / `WyrmStructureActor` and set them on a Blueprint subclass of
`WyrmIslandManager` (`DragonClass` / `StructureClass`) and `WyrmGameMode` (`IslandClass`).

**Restyle the interface:** create a Widget Blueprint that derives from `WyrmHUDWidget` and set it as
`HudClass` on a Blueprint subclass of `WyrmPlayerController`.

## Building for phones

The project targets mobile hardware and landscape orientation by default
(`Config/DefaultEngine.ini`, package name `com.wyrmhaven.game`).

* **Android:** install Android Studio and the SDK/NDK versions Unreal asks for
  (*Edit → Project Settings → Platforms → Android → Configure Now*), then
  *Platforms → Android → Package Project*.
* **iOS:** requires a Mac with Xcode and an Apple developer account; *Platforms → iOS → Package Project*.

## Tests

```bash
python3 Tools/validate_data.py   # data sanity checks
make -C Tests                    # ~22,000 checks over elements, breeding, economy and battles
```

Both run automatically on GitHub for every push (`.github/workflows/rules-tests.yml`).
The Unreal-specific code (`Game/`, `World/`, `UI/`) needs the engine to compile and is not
built by CI.

## Roadmap ideas

* Drag-to-place buildings (the grid and `MoveStructure` already exist)
* Real dragon models and animations, sound and music
* Daily quests, events and an online arena (the `Rules/` layer can run on a server)
* Localisation (player-facing messages already use `LOCTEXT`)

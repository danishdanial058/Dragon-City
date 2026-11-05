#!/usr/bin/env python3
"""Checks Content/Data/GameData.json for mistakes before the game ever loads it.

Mirrors Wyrm::Catalog::Validate() so designers get feedback without opening Unreal.
Usage: python3 Tools/validate_data.py [path/to/GameData.json]
"""
import json
import sys
from pathlib import Path

ELEMENTS = ["Fire", "Water", "Earth", "Air", "Nature", "Lightning", "Ice", "Metal", "Shadow", "Light"]
RARITIES = ["Common", "Rare", "Epic", "Legendary"]


def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent / "Content/Data/GameData.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    problems = []

    def unique(items, kind):
        seen = set()
        for item in items:
            if not item.get("id"):
                problems.append(f"{kind} with empty id")
            elif item["id"] in seen:
                problems.append(f"duplicate {kind} id '{item['id']}'")
            seen.add(item.get("id"))
        return seen

    skills = {s["id"]: s for s in data.get("skills", [])}
    unique(data.get("skills", []), "skill")
    species_ids = unique(data.get("species", []), "species")
    unique(data.get("habitats", []), "habitat")
    unique(data.get("crops", []), "crop")

    for s in data.get("skills", []):
        if s.get("element") not in ELEMENTS:
            problems.append(f"skill '{s['id']}' has unknown element {s.get('element')}")
        if not (0 < s.get("power", 0)) or not (0 < s.get("accuracy", 0) <= 100):
            problems.append(f"skill '{s['id']}' has invalid power/accuracy")

    for sp in data.get("species", []):
        sid = sp["id"]
        els = sp.get("elements", [])
        if not 1 <= len(els) <= 2 or len(set(els)) != len(els):
            problems.append(f"species '{sid}' must have 1 or 2 different elements")
        for e in els:
            if e not in ELEMENTS:
                problems.append(f"species '{sid}' has unknown element {e}")
        if sp.get("rarity") not in RARITIES:
            problems.append(f"species '{sid}' has unknown rarity {sp.get('rarity')}")
        for key in ("baseHealth", "baseAttack"):
            if sp.get(key, 0) <= 0:
                problems.append(f"species '{sid}' needs a positive {key}")
        if len(sp.get("skills", [])) < 2:
            problems.append(f"species '{sid}' needs at least 2 skills")
        for sk in sp.get("skills", []):
            if sk not in skills:
                problems.append(f"species '{sid}' uses unknown skill '{sk}'")
        recipe = sp.get("recipe")
        if recipe is not None:
            if len(recipe) != 2 or any(p not in species_ids for p in recipe):
                problems.append(f"species '{sid}' has a recipe with unknown parents")

    homes = {h.get("element") for h in data.get("habitats", [])}
    for e in ELEMENTS:
        if e not in homes:
            problems.append(f"no habitat for element {e}")

    new_game = data.get("newGame", {})
    for d in new_game.get("starterDragons", []):
        if d not in species_ids:
            problems.append(f"starter dragon '{d}' does not exist")
    hab_ids = {h["id"] for h in data.get("habitats", [])}
    for h in new_game.get("starterHabitats", []):
        if h not in hab_ids:
            problems.append(f"starter habitat '{h}' does not exist")

    if problems:
        print(f"{path}: {len(problems)} problem(s)")
        for p in problems:
            print(f"  - {p}")
        return 1
    print(f"{path}: OK ({len(species_ids)} species, {len(skills)} skills, {len(hab_ids)} habitats)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

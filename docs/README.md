# Inside In Extremis: a field guide

Welcome to the slightly dusty engine room. This is a guide to what we've actually worked out about the original *In Extremis* DOS game, plus how **Extremis RE** puts those findings back together on modern hardware.

If you've ever wondered why an alien suddenly heads home, how a corridor becomes a different sector, or why an apparently ordinary wall is secretly an elevator door, you're in the right place.

No chronological "Step 204" reports here. Each chapter explains **one system**, with readable examples first and the byte-level details when they're useful.

## Pick a rabbit hole

| Topic | What you'll find |
| --- | --- |
| [Levels and GAME.LAB](systems/levels.md) | The 30 zones, sectors, geometry, routes, enemies and resource-bank IDs. Start here. |
| [Enemies](systems/enemies.md) | Behaviour modes, inter-sector navigation, sprites, hit masks and combat. |
| [Player and movement](systems/player.md) | Coordinates, controls, crouching, head movement, health and oxygen. |
| [Weapons and projectiles](systems/weapons.md) | Six weapon descriptors, shots, ammo, projectiles and hit testing. |
| [World, doors and elevators](systems/world.md) | What walls and connections really mean, and why opening a door wakes up aliens. |
| [Decorative objects](systems/decorations.md) | OBJET banks, drops, frame sequences, pixel patches and solid/ghost objects. |
| [Inventory and items](systems/items.md) | Fixed slots, cards, supplies, readers and the rather specific rules of picking things up. |
| [HUD, map and terminals](systems/interfaces.md) | The original 320×200 presentation, transfer terminals and modal interfaces. |
| [Audio](systems/audio.md) | DSS sound banks, channels, effects, and the odd business of timing. |
| [Rendering](systems/rendering.md) | DALLE walls, SOL floor/ceiling, sprites, palette and the software renderer. |

## The reference shelf

| Reference | Why bother? |
| --- | --- |
| [File formats](reference/file-formats.md) | A quick map of the file structures we've identified. |
| [Reverse-engineering notes](reference/reverse-engineering.md) | How we separate an EXE fact from a good guess. |
| [Performance](reference/performance.md) | What was measured, on what machine, and where the time goes. |
| [Known differences](reference/known-differences.md) | The bits we haven't solved yet, including current test limitations. |

## A few rules for reading this

- **Original DOS** means evidence traced to the executable, its data or a controlled original-game observation.
- **Port** means the implementation visible in the repository's source code. A faithful result in one subsystem does not imply the whole game is finished.
- **Adaptation** means a deliberate practical substitute, often timing-related. We'll call it what it is.
- **Unknown** means we haven't proved it. That's better than shipping a confident-sounding guess.

All EXE offsets in these chapters are **file offsets** unless a note says otherwise. Sprite and sector indices are generally **zero-based**; player-facing level numbers are **1–30**. The original game uses its own units and 720 angular units per full turn in several records.

### Where did the old reports go?

The first rounds of investigation produced many overlapping, date-stamped notes. We folded their useful findings into these chapters and removed the old reports from the live documentation. Nothing has been erased from Git history: the original audits can still be found in earlier commits if you need to trace an old experiment.

This guide is meant to grow **with verified discoveries**, not with a fresh report for every command we run.

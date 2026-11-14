# Original executable — reverse-engineering knowledge index

**Purpose:** find and reuse established reverse engineering before reopening `EXTREMIS.EXE`. This index is a lookup map, **not** a claim that every area is fully reconstructed. The accessible system chapters already contain substantial evidence; do not duplicate them indiscriminately.

Original reference: `EXTREMIS.EXE` SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`; `GAME.LAB` SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`.

For address conventions and evidence labels, consult [Reverse-engineering notes](reverse-engineering.md). Offsets below refer to **file** positions, whereas `[address]` denotes runtime memory. System chapters remain the canonical readable explanations; detailed EXE entries record instruction-level evidence.

## Where to look first

| Subsystem | Detailed EXE evidence | Existing system knowledge | Representative starting points / status |
| --- | --- | --- | --- |
| Doors, connectivity and passage | [Doors: original state word and automatic cycle](exe/doors.md) | [World](../systems/world.md) | `0x1074b`, `0x10a3c`, `0x1146d–0x114b9`, `0x1184f` — original mechanism traced; port still differs |
| Level structure and bank selection | — | [Levels](../systems/levels.md), [File formats](file-formats.md) | `GAME.LAB` 30 level records, sector/connection/enemy tables — parser implemented |
| Enemy AI, animation and hit masks | — | [Enemies](../systems/enemies.md) | `0xe11b–0xe127` mode 2 activation; `0xe696–0xe703` walking; `MONS*.SPT` 8-byte mask entries |
| Weapons and projectiles | — | [Weapons](../systems/weapons.md) | `0x56d7` six descriptors; 20 projectile slots; firing rate requires DOS timing scrutiny |
| Movement, posture, player survival | — | [Player](../systems/player.md) | `0x1656a–0x16a7d` movement/posture, `0x16a8e–0x16aab` bobbing, `0x165db–0x166bb` damage reaction |
| Decoration, animation and collision | — | [Decorations](../systems/decorations.md) | `0x10db7–0x11028` XY collision, `0x11e79–0x11ea5` OBJET6 timing, `0x14d1f–0x1517e` pixel patches |
| Wall textures, floors, ceilings, sprites | — | [Rendering](../systems/rendering.md) | `0x15795–0x15925` DALLE addressing; `0x12ac4` onward planes; sprite projection `0x11f83–0x1207f` |
| Inventory, cards and level transfer | — | [Items](../systems/items.md), [Interfaces](../systems/interfaces.md) | `GAME.LAB` zone access offset `+0x0e`; door/card routines `0x937c–0x95d7`; wider interactions incompletely verified |
| Audio and timebases | — | [Audio](../systems/audio.md), [Player](../systems/player.md) | PIT divisor 7457 (~160 Hz); main loop/frequency estimator are distinct; fixed 18 Hz in port is an adaptation |
| HUD, map and terminals | — | [Interfaces](../systems/interfaces.md) | `0xa608` dispatch, `0x114ba–0x1185c` connected-sector traversal |

## When to update

For each **new** investigation, create or extend one scoped entry under `docs/reference/exe/` with:

1. Original file hash/version, investigation date/source, and address conventions.
2. Short answer and precise DOS instructions/data (bounded ranges, branch conditions and caller paths).
3. Evidence classification: confirmed disassembly vs observed game behavior vs deduction vs unknown.
4. Difference between DOS, current port, and the tests; do not label an unimplemented rule as fixed.
5. Unresolved details and conditions which would justify reopening the EXE.

Avoid creating empty subsystem pages or megabytes of raw disassembly. The objective is **reliable retrieval**, not archiving every investigation transcript. The existing system chapters may be expanded directly for small discoveries.

See also [Known differences](known-differences.md) for the backlog (which can lag the latest local tests).

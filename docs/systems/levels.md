# Thirty levels in one LAB

The nice thing about *In Extremis* is that its levels aren't thirty unrelated piles of wall coordinates. They're built from a consistent set of records: **sectors**, **vertices**, **walls**, **objects**, **polygons**, **routes**, and **enemies**. Once you understand one level, you have the vocabulary to read the other twenty-nine.

The catch? A lot of the information lives behind little offset tables, and several systems interpret the *same* piece of data differently. That's where the reverse engineering gets interesting.

## Start with `GAME.LAB`

The first word gives the number of levels: **30**. It is followed by **31 little-endian 32-bit offsets**: the start of each level plus the end boundary for the last one. A level's address is `offset[i]`; its last byte is before `offset[i+1]`.

```text
GAME.LAB
  word      level count = 30
  dword[31] level offsets and final boundary
  level 1
  level 2
  ...
  level 30
```

Each level begins with the IDs of four resource banks:

| Relative offset | Data | Purpose |
| --- | --- | --- |
| `+0x00` | `palBank` | PAL colour palette |
| `+0x02` | `dalleBank` | DALLE wall bank; also selects the SOL plane bank |
| `+0x04` | `objetBank` | OBJET decorative-sprite bank |
| `+0x06` | `monsBank` | MONS enemy bank |
| `+0x08` | Offset | Sector-route data used by navigation |
| `+0x0A` | Offset | Connection-state words, including dynamic doors |
| `+0x0C` | Offset | Enemy records |
| `+0x0E` | Offset | Access-code data |
| `+0x12` | Word | Sector count, followed by the sector directory |

These aren't separate files per level. They're **sections inside the same level record**, reached through offsets relative to its beginning. The parser validates those offsets before reading them. For the exact implementation, see [`src/game_lab.cpp`](../../src/game_lab.cpp) and [`include/game_lab.h`](../../include/game_lab.h).

## Sectors: a room is more than four walls

A sector has a local origin `(gx, gy)`, a list of neighbours and its own vertices. Positions in the sector are **local**, so global coordinates are obtained by adding the sector origin:

```text
worldX = sector.gx + localX
worldY = sector.gy + localY
```

Each entry in the sector directory stores its relative byte offset plus `gx` and `gy`, all as 16-bit words. Inside the sector we then read:

| Record | What it contains |
| --- | --- |
| Neighbours | Indices of adjacent sectors; the order matters |
| Vertices | Local X/Y pairs |
| Walls | Two vertex indices, a `type` word and an `extra` word |
| Entity A | Vertex index, `type`, `Param1`, `Param2` |
| Polygons | Vertex lists, an `extra` word, and local polygon connections |

The parser keeps the original indices rather than flattening everything into generic triangles. That's important: **enemy paths, doors, access panels and collision routines still speak the language of sectors**.

### A concrete example: the starting position

The new-game template places the player in level 1, sector 0, at local `(300, 320)` with angle `180` on a **720-unit circle**. The sector's origin is `(3200, 0)`, so the resulting world position is `(3500, 320)`. The starting angle is a quarter turn.

That's not a coordinate somebody eyeballed from a screenshot: it is recovered from the original template and level geometry. This is exactly the sort of detail that prevents tiny alignment errors from snowballing later.

## Walls, connections, and doors

Each wall record is four words (8 bytes):

```text
vertexA | vertexB | type | extra
```

The low byte of `type` can select a wall material. Higher bits encode special roles. In the level-1 geometry, the `0x1000` family marks paired door/connection faces and the `0x2000` family supplies their side pieces. A pair of reversed faces in different sectors can share the same `extra` index into the connection-state table.

For visibility and several enemy decisions, the DOS code checks:

```c
(connectionStates[wall.extra] & 0x01ff) != 0
```

An opening door enables its connection **immediately**, even while its visible movement is still at zero. A closing door stays connected until the progress is gone. Unlocking (`0x0400`) alone doesn't make the passage open. See [World, doors and elevators](world.md) for the full distinction between *accessible*, *open* and *visible*.

## Routes are data, not clever pathfinding

`GAME.LAB` also contains per-sector **waypoints to other sectors**. A route entry supplies a destination sector and global `(x, y)` point. The port reads the table at header offset `+0x08` and checks destinations against the sector count.

This matters to the enemy AI. If an enemy has no original route between two places, inventing a path across the whole map would change the game. The reconstructed navigation favours the recorded links rather than a generic navmesh. The original sector graph is also used for deciding which enemies can activate or block the map.

## Enemies and objects are different lists

An `EntityA` record is only **8 bytes**:

| Offset | Field |
| --- | --- |
| `+0` | Vertex index |
| `+2` | Type and flags |
| `+4` | `Param1` |
| `+6` | `Param2` |

It's how the level locates things like OBJET decoration. `Param1` can be a vertical world anchor; `Param2` can be a timer, depending on the object's type. Don't assign one meaning to the entire field everywhere.

Enemy records live in a separate section and are **28 bytes each**. They track health, behaviour, animation state, sector, position, sprite flags and home coordinates. See [Enemies](enemies.md) for the complete layout.

## A few numbers worth remembering

| Item | Verified example |
| --- | --- |
| Levels in GAME.LAB | **30** |
| Level-1 sectors | **15** |
| Level-1 walls | **280** |
| Level-1 door pairs | **15** |
| Level-1 enemy records | **9** |
| Level-1 decorative records | **27** |

The port includes a general loader for all 30 levels and has recovered a broader elevator destination table (25 elevators, 55 destinations in the current investigation). **Loading a level is not the same as proving every gameplay event in that level works**. The latter still requires testing and original-game comparison.

## Don't “fix” the original data without checking

Two good examples from the shipped LAB illustrate why we preserve raw structure:

- **Level 3, sector 18** contains polygon indices that alias the same offset. They are legitimate distinct indices in the original lookup. Collapsing them would change references.
- **Level 28, sector 14, polygon 3** contains a vertex index outside the sector's vertex table. The parser preserves the relevant polygon table entry and connections without inventing replacement geometry.

The rule is simple: identify a strange byte sequence, check how DOS uses it, *then* decide whether the port needs special treatment.

## Where the reconstruction lives

[`game_lab.cpp`](../../src/game_lab.cpp) handles strict data reading; [`door.cpp`](../../src/door.cpp) and [`enemy_navigation.cpp`](../../src/enemy_navigation.cpp) interpret some of those relationships; [`sprite_scene.cpp`](../../src/sprite_scene.cpp) builds decoration from level entities. The game still reads original assets at runtime — the repository doesn't ship them.

**Original evidence:** sector and bank loading near EXE `0x11335`; sector traversal `0x114ba–0x1185c`; original player new-game template at file `0x1d8a0`. The most useful cross-reference is the parsed type definitions in `game_lab.h`.

*What remains uncertain?* The exact triggers for some level-specific events, nest locations and their destruction mechanics, and full start-to-finish validation of all 30 levels. See [Known differences](../reference/known-differences.md).

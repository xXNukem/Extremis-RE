# Aliens, bad decisions, and the sector next door

You shoot an alien from a doorway. It charges, then apparently changes its mind and goes home. Broken AI? Not necessarily. That's actually one of the more charming discoveries from the original executable: **some enemies are only allowed to pursue you within a specific sector**.

*In Extremis* doesn't have a modern “see player → chase player forever” routine. It has behaviour modes, recorded sector routes, animation tables and a surprisingly strict set of rules for when an enemy wakes up.

## Meet the 28-byte enemy

`GAME.LAB` stores enemy records as **14 little-endian words**. The loader retains their original order:

| Offset | Field | Why we care |
| --- | --- | --- |
| `+0` | Health | Damage and the removed-enemy sentinel |
| `+2` | Behaviour | Controls activation and pursuit |
| `+4` | State | Walking, waiting, attacking, reacting, dying |
| `+6` | Phase | Animation progress |
| `+8` | Sector | Position in the level's sector network |
| `+10`, `+12` | X, Y | Local or record-space position as interpreted by the engine |
| `+14` | Polygon | Current polygon reference |
| `+16` | Angle | **720 units per turn** |
| `+18` | Reserved | Preserved, not given an invented meaning |
| `+20` | Sprite/flags | Frame selection and flipping |
| `+22` | Elevation | Vertical/special-state component |
| `+24`, `+26` | Home X/Y | Where this enemy belongs |

A removed enemy uses a special **`0xffff` health value**. That matters not just for drawing a corpse: it also affects the motion detector and whether the map can open.

The nine **MONS** resource banks describe visual and behaviour families, not nine identical enemies with different textures. Bank 5 has a special jumping/displacement routine; bank 6 has stationary behaviour. Other banks have their own animations and reactions. Don't blindly copy the rules from MONS1 across all of them.

## The four behaviours

| Mode | What DOS does |
| --- | --- |
| **1** | Pursues within the player's sector; returns to its home position when the player leaves. A shot does **not** promote it into permanent pursuit. |
| **2** | Waits until its sector appears in the connected-sector set **or it is hit**; then changes to mode 3. |
| **3** | Can pursue across sectors using the available routes and connections. |
| **4** | Also supports inter-sector pursuit, with its own initial/state rules. |

The distinction between **2 → 3** and **1 staying 1** was recovered from the EXE, not guessed from a playtest. In particular, the hit routine around `0xe11b–0xe127` only promotes **mode 2**.

### An enemy can “see” you while looking the other way

The DOS *connected-sector* list is **not a raycast visibility check**. It is a traversal through the current sector and neighbouring sectors whose connection-state words pass `(state & 0x01ff) != 0`. No field-of-view test is required to wake a mode-2 enemy in that connected region.

A closed door can prevent activation. Start opening it and the connection becomes active immediately, before the visible door has moved. A mode-2 enemy behind you may then wake up. That's original behaviour, not cheating.

The same sector set also determines whether an **unremoved alien blocks the map**, producing `ATTENTION: AN ALIEN IS NEAR!!`. The port's shared implementation is in [`door.cpp`](../../src/door.cpp), used by [`enemy_system.cpp`](../../src/enemy_system.cpp).

## Navigation: use the links you're given

Enemies traverse the sector/polygon network in `GAME.LAB`. The port uses the original route links and checks dynamic doors. In recorded tests, modes 3/4 could not cross a closed passage, crossed after it opened, and didn't invent a route where there wasn't one.

Mode 1 has a very different loop: chase locally, then work back toward home. This is why doorway combat can feel strange if you're used to newer FPS enemies. **Fidelity sometimes means keeping the odd bits.**

Navigation is still a fidelity project, not a claim that every enemy in every level has been exhaustively playtested. Specific corner cases remain on the list.

## Animation is driven by state, not file order

MONS images are not a simple flipbook you advance from frame zero to the end. For the level-1 bank, the recovered routines include:

| State/action | EXE evidence | Example behaviour |
| --- | --- | --- |
| Walk | `0xe696–0xe703` | Frames 0–4; horizontal mirror alternates with the cycle |
| Wait | `0xe718–0xe74d` | A **32-entry** lookup table, not just a stationary image |
| Attack | `0xe2ae–0xe300` | Seven-phase table; initial phase is part of the original logic |
| React to damage | `0xe10d–0xe16c` | Brief reaction pose then resumes |
| Die | `0xe22b–0xe29b` | Temporary animation, then the removed sentinel |

**Don't assume** a contiguous image range belongs to one animation just because the indices line up neatly. That mistake can make a perfectly good alien suddenly display the wrong body part.

## The SPT files are not sprite animations

`MONS*.SPT` contains **8-byte masks per image**: an **8×8 hit map**. The original code projects a hit point into the frame's rectangle and checks the matching mask bit. In the recovered convention:

- **Bit 1** rejects the shot.
- **Bit 0** allows the hit.

Another lovely quirk: a horizontally flipped sprite does **not** necessarily have its SPT bits flipped again. Tests compared the original mask interpretation across multiple frames, directions and camera heights rather than replacing it with an approximate rectangle.

In the port, [`enemy_system.cpp`](../../src/enemy_system.cpp) owns enemy state and damage, [`enemy_navigation.cpp`](../../src/enemy_navigation.cpp) handles route movement, and [`enemy_effects.cpp`](../../src/enemy_effects.cpp) handles the special death/fragment effects.

## When an alien hits back

The original player reaction does more than lower a health bar. A hit can create a sequence of health loss and **physical knockback opposite the player's own facing direction** — not necessarily away from the attacker. The reconstructed reaction was traced around EXE `0x165db–0x166bb`.

The working port uses a fixed update step for deterministic behaviour. That is an explicit adaptation: the original DOS loop ties several enemy movements and animations to its iteration rate and an FPS estimator. **18 Hz in the port is not a proven fixed DOS tick rate.**

## Real example: level 1

There are **nine** enemy records in level 1, including waiting enemies in sectors **3, 5, 6, 7, 14, 12, 11, 10** and one mode-4 enemy. Their coordinates and home positions come from the level data, not from hand-placed guesses. The resource bank for an enemy is chosen per level.

A quick way to investigate an apparent bug is therefore: note the **level, sector, enemy index, behaviour and door states** first. “The alien went home” is not a bug report until we know whether it was a mode-1 enemy.

## Still on the workbench

Decorative-object collision must also be applied to enemy movement, including the DOS response that changes the enemy angle by **±80 angular units**. The later work on that system is not part of the repository snapshot documented here. Specific enemy routes and the final sequence need continued playtesting. See [Known differences](../reference/known-differences.md).

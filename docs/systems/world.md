# Walls, doors, and the strange geography of an elevator

At first glance, a wall is a line segment with a texture. In *In Extremis*, that same line can also be a **sector connection, a sliding door, a terminal, a trigger or part of an elevator**. Knowing the wall's coordinates isn't enough; the `type` and `extra` words do a lot of the heavy lifting.

## The four words that make a wall

A `GAME.LAB` wall has this structure:

```text
uint16 vertexA
uint16 vertexB
uint16 type
uint16 extra
```

Coordinates come from the sector's vertex table, plus its global origin. The low material byte selects a DALLE texture. High bits and families identify special surfaces.

On level 1, paired `0x1000` connection faces and `0x2000` side pieces build the geometry of fifteen doors. Each door is represented by **two reversed faces**, one seen from either neighbouring sector. Both reference a connection-state entry using `extra`. This is why treating the two faces as independent doors can produce nonsense.

## A door has more than one meaning of “open”

Each query reads a different part of the same connection state.

1. **Is it unlocked?** That involves the `0x0400` state bit and access-control logic.
2. **Can an actor cross it?** The connection branch requires the opening bit and phase above 60, then applies actor and position rules.
3. **Can a projectile pass?** Its ray uses the visible span of the camera-facing leaf. An invisible leaf can still block an actor while the connection is descending.
4. **Do sectors count as connected?** DOS tests `(state & 0x01ff) != 0`.

The last question matters for enemy activation and map restrictions. It is **not** equivalent to `progress > 0`.

| Door phase | DOS sector connection |
| --- | --- |
| Closed (`0` or `0x0400`) | Off |
| Opening begins (`0x0100`, progress 0) | **On immediately** |
| Partly/open completely | On |
| Closing with progress remaining | On |
| Fully closed again | Off |

The original program sets the opening bit at `0x10a3c–0x10a40`, advances progress at `0x11498–0x114b2`, and checks the mask at `0x1184f`. Unlocking alone changes only the lock bit. The port's [`doors::connectionEnabled`](../../src/extremis/world/door.cpp) gives the connection query a single owner.

## “Visible sector” does not mean “on screen”

The DOS constructs a list by walking neighbouring sectors through enabled connections, with cycle protection. It does this **before geometry clipping and FOV decisions**. An alien behind the player may therefore be in a connected/visible sector as far as gameplay is concerned.

That same list is used when:

- A behaviour-2 enemy becomes active.
- The game refuses to open the map because an alien is nearby.

Think of it as the **reachable set of sectors through currently enabled links**, not as the set of pixels you can literally see.

## Moving the door face

A moving door does not simply fade out. The renderer crops/retracts its visible horizontal span while retaining the original material. Two reversed source faces must remain geometrically consistent or textures can appear mirrored or stretched.

The current port derives door visibility from the original 16-bit connection state: the phase rises to **90**, reverses and descends to **0**, with the leaf fully hidden at **60**. The motion uses a nominal PIT-derived counter instead of the former provisional one-second stroke; this reproduces the traced state arithmetic, **not necessarily the precise DOS wall-clock cadence**. The user verified automatic opening and closing in Debug with a 26/26 passing suite on 2026-11-14. The activation audit found that the native 180-unit trigger is also the stopping line, with passage only after phase 60; the near-door wait follows those rules. Its perceived duration still needs a controlled comparison against running DOS. See [the instruction-level evidence and live implementation status](../reference/exe/doors.md).

## The elevators are part of the level network

Elevator interfaces, destinations and access codes were recovered from original tables. The ongoing investigation has identified **25 elevators and 55 destinations**, although that is not a claim that every travel edge and every final-game condition has been manually checked.

The cabin's visual materials, door state, destination list, and player arrival are separate concerns. A recurring visual issue is **elevator textures appearing on unrelated corridor walls** when neighbouring surfaces are interpreted incorrectly. This is on the backlog; no speculative material remapping belongs in the reference implementation.

## Collision is not a single giant function

The port distinguishes static wall segments, dynamic door faces, and (in the actively developed local work) decorative-object bounding boxes. The DOS checks some of these in a specific order, and enemies have different reactions from the player.

Do not replace all three with a universal circular collision test and expect original movement. A flag or an extra word can determine whether a seemingly solid-looking object is actually passable.

**Where to look:** [`door.cpp`](../../src/extremis/world/door.cpp), [`door_control.cpp`](../../src/extremis/world/door_control.cpp), [`player_collision.cpp`](../../src/extremis/world/player_collision.cpp), [`game_lab.cpp`](../../src/extremis/resources/game_lab.cpp). For render projection and wall material selection see [Rendering](rendering.md).

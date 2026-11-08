# The player: one pair of boots, two survival bars

The player in *In Extremis* isn't a floating camera with a health number tacked on. Movement, posture, oxygen, hit reactions and even the camera's tiny bounce are governed by their own rules. Get one of them wrong and the game starts *feeling* wrong, even when the screenshots look fine.

## Coordinates and orientation

The original world is arranged in sectors. The player has local coordinates within a sector, plus an angle measured in **720 units per full turn**. The reconstructed renderer uses its own camera representation and converts positions into world space for drawing.

The verified new-game start is **level 1, sector 0, local `(300, 320)`, angle `180/720`**. With the sector's `(3200, 0)` origin, that gives `(3500, 320)` in world coordinates.

The core movement logic is in [`player_control.cpp`](../../src/player_control.cpp); SDL keyboard and mouse capture belong in [`player_input.cpp`](../../src/player_input.cpp). Keeping them separate makes movement testable without starting a window.

## Controls in the port

| Input | Function |
| --- | --- |
| W / S | Forward / backward |
| A / D | Strafe left / right |
| Mouse | Horizontal look |
| Left / right arrow | Turn |
| Shift (held) | Run |
| Alt (held) | Crouch |
| Left mouse button | Fire equipped weapon |
| E | Contextual action / confirmation |
| I | Inventory |
| Z / X | Use injector / oxygen reserve |
| M or original physical map key | Open map, when allowed |
| TAB in Debug builds | Developer menu; **not** a standard DOS control |

Modal screens consume their own controls; closing one should not send a stray action or leave the player walking because a key was held down. The port deliberately resets mouse deltas on focus/context changes.

## Walking, running and walls

Movement uses a vector derived from the player's angle and normalizes diagonals so two keys do not make the character run faster than intended. Static walls use collision geometry; dynamic doors contribute their own blocking faces.

The public source currently has **provisional values** such as 300 world units/s for walking, a factor of two for running, and a circular wall-collision radius. These are engineering choices, **not** measured original constants. The DOS code instead calculates movement through its current-step and FPS-estimator machinery.

A hit may force **physical recoil**: movement back along the opposite of the player's current facing direction, subject to wall collision, while turning remains possible. This is not a screen shake pretending to be knockback.

### Why a doorway matters

The world collision system and the sector graph are related but not interchangeable. A player can be blocked by a moving door face while an enemy's sector activation depends on the state of that connection. See [World](world.md) and [Enemies](enemies.md).

## Crouching and eye height

The original code adjusts eye height over successive updates: **165 standing**, dropping by 10 units per update toward **100 crouched**, then rising on release. Below a height of 150, running acceleration is no longer allowed. No separate, smaller collision footprint has been demonstrated, so don't invent one.

The corresponding port logic is in `player_control.cpp`, with the crouch input in `player_input.cpp`.

## The headbob has a few surprises

Original DOS camera bounce uses the table:

```text
[0, 5, 8, 9, 10, 9, 8, 5, 0]
```

The value is **added to posture height**, so a standing player can see an eye height of **165–175**. It isn't a centred sine wave. The phase advances **only if X or Y actually changed** after movement resolution. Stop, turn in place, or hit a wall and the phase remains where it was; the camera doesn't automatically return to the centre.

Walking and running use the same amplitude. Speed changes the phase increment. The original also has a peculiar cycle reset linked to its footstep sound request — it is **not** just `phase %= 144`.

These details were recovered in the ongoing local reconstruction. **Headbob changes may not yet be present in the public `main` branch**; refer to the source history and [known differences](../reference/known-differences.md) before treating this section as a claim about a released build.

## Health, oxygen and the genuinely alarming clock

The original starting values are simple enough:

| Resource | Starting internal value | Behaviour |
| --- | ---: | --- |
| Health | **1,000** | Damage subtracts internal units; injector restores full health |
| Oxygen | **288,000** | Consumed over time; a reserve **exchanges** values rather than simply adding them |
| PIT clock | **~160 ticks/s** | Nominal timer rate, derived from divisor 7457 |

In the port, [`player_vitals.cpp`](../../src/player_vitals.cpp) tracks this state separately from the HUD. Nominal oxygen consumption uses the 160 Hz clock, doubles for increased-use conditions, and pauses when gameplay simulation is inactive. Health and oxygen deaths are distinct causes. The UI merely displays what the simulation owns.

A diagnostic F5/F7 path exists for tests in Debug builds. It is **not** a player feature in Release. The current Release test fixture that assumes those diagnostic controls are available is documented as a test-configuration issue.

## What's faithful, what's still adapted?

The posture limits, original health/oxygen scales, special impact direction and headbob lookup come from recovered DOS routines. The port's rendering-independent fixed steps, several movement speeds and collision geometry are adaptations. The DOS also uses a special impact-height table distinct from normal headbob; that table isn't yet restored.

**Original reference points:** movement/posture `0x1656a–0x16a7d`, impact reaction `0x165db–0x166bb`, normal bob lookup `0x16a8e–0x16aab`, new-game template at file `0x1d8a0`.

# Weapons: one in your hands, twenty shots in flight

Forget a modern weapon wheel. The original *In Extremis* inventory revolves around **one equipped weapon at a time**, with ammunition, weapon exchanges and a small fixed pool of active projectiles. That design shapes almost everything about combat.

## Six weapon descriptors

The executable contains **six 112-byte descriptors** (starting at file `0x56d7`). The port preserves a typed version in [`include/weapon_fire.h`](../../include/weapon_fire.h).

| Weapon index | Fire input | Original `wait` field | Notes |
| --- | --- | ---: | --- |
| 0 | Single press | 4 | A new trigger press is required |
| 1 | Hold to repeat | 3 | Automatic |
| 2 | Hold to repeat | 2 | Uses alternating muzzle positions |
| 3 | Hold to repeat | 3 | Automatic |
| 4 | Single press | 6 | A new trigger press is required |
| 5 | Hold to repeat | 1 | Longer visual stage sequence |

These numbers are **descriptor fields**, not universal milliseconds or measured rounds per second. The DOS uses its iteration/FPS estimator when interpreting waits. Converting them straight into wall-clock delay would be inventing a new mechanic.

Each descriptor also gives firing geometry and projectile frame data: speed, aiming reference, muzzle offsets, lateral position and animation stages. That explains why the visible gun, the projectile sprite and the actual hit test need to be treated as related but distinct things.

## Pulling the trigger

The important sequence is:

1. Confirm a real press or a held trigger on an automatic weapon.
2. Check the equipped weapon and available ammunition.
3. Find a free slot in the **20-projectile pool**.
4. Accept the shot and spend one unit of ammunition.
5. Update the projectile through the game simulation, not from the draw call.

A button press that can't create a valid shot shouldn't quietly consume ammunition. The reconstructed path lives in [`src/weapon_fire.cpp`](../../src/weapon_fire.cpp); the item slots and ammo belong to [`player_inventory.cpp`](../../src/player_inventory.cpp).

The currently equipped weapon is also represented in the original HUD, which uses its own graphics and selection rules. The code does not pretend the character is carrying a modern arsenal behind the scenes.

## Hits: a mask, not a guess

When a shot reaches an enemy, the hit test can use the enemy's per-image **8×8 SPT mask**. **Bit 1 rejects**, **bit 0 accepts**. This avoids the trap of treating a huge transparent area of a sprite as a solid rectangle.

The same rules apply when sprites are mirrored: the recovered DOS code does not automatically mirror the SPT mask bits a second time. Details and enemy reaction states are covered in [Enemies](enemies.md).

## Weapon exchange and empty magazines

Supplies and weapons are taken through the original panel/inventory flow; a weapon exchange is not the same thing as adding another gun to a hidden collection. The DOS behaviour when ammunition runs out has been investigated, but precise selection and edge cases still need validation across the full campaign.

We also still need to compare the **visible shot origin** to the original weapon's muzzle and the on-screen crosshair. A projectile that hits correctly but appears to fly out of the screen centre can still feel plainly wrong.

## Where to look

[`weapon_fire.h`](../../include/weapon_fire.h) is the compact descriptor catalogue; [`weapon_fire.cpp`](../../src/weapon_fire.cpp) advances shots; [`player_inventory.h`](../../include/player_inventory.h) owns the equipped weapon and ammunition; [`enemy_system.cpp`](../../src/enemy_system.cpp) handles enemy damage and reaction. The original bank graphics are read at runtime — they are not distributed here.

**Original evidence:** weapon table near file `0x56d7`; 20-slot projectile handling and firing cadence traced in the executable; the FPS estimator at `[0x1422]` is distinct from the PIT's 160 Hz clock. Timing fidelity is still an explicit area of work.

# Known differences and unfinished mysteries

This page is the anti-hype section. Extremis RE is already able to do a lot, but **“loads all thirty levels” isn't the same as “every level has been completed and checked against DOS.”** Here's what we know still needs attention.

## Latest door verification (2026-11-14)

The user built Debug and reported **26/26 CTest passing** after a test-only correction of the expected coordinates for the diagonal Q14 door case, then confirmed opening and automatic closing during gameplay. These are port results, **not** a controlled DOS comparison. Doors currently start opening extremely close to the player, producing a brief movement stall. **Next:** verify the original DOS activation distance before changing the faithful default; consider an optional earlier-opening mode only as a clearly labeled adaptation. Door cycle/counting is integrated; exact elapsed-time fidelity and some special paths remain to be verified.

The older test/fix snapshots below are kept as **historical records** and must not be treated as the current `main` status.

## Historical snapshot: public code versus local work

The documentation branch is based on the public `main` tree. A substantial batch of newer work was developed **locally and had not yet been pushed** when this documentation was prepared. It includes:

- Headbob with persistent phase and original footstep timing behaviour.
- Original bank-filtered **11 decorative falls**.
- Whole-image decoration cycles for **OBJET2, OBJET3, OBJET4 and OBJET9**.
- Re-enabled OBJET patch composition and rotating columns.
- Per-bank original OBJET XY collision for players and enemies, including the enemy ±80 angular response; covered by `objetCollisions` in `enemy_tests`.

Those findings are explained in the appropriate chapters, but **they should not be read as claims about the published `main` binaries**. Merge the tested code changes separately before changing this status. The documentation work does **not** modify the engine source.

## Historical test status (superseded by newer Debug result above)

A recent local Release build compiled successfully and ran **24 CTest cases**: **22 passed, 2 failed**. These failures are tracked rather than quietly hidden:

| Test | Symptom | What is known |
| --- | --- | --- |
| `survival_session` | `Nonfatal resource-loss fixture failed` | Test uses F5/F7 diagnostic keys, but Release builds the `survival_session` library without `EXTREMIS_DEBUG_MENU`. The fixture therefore cannot drive the expected resource changes. Fix the **test linkage/build setup**, not Release gameplay by enabling cheats. |
| `closed_doors` | Moving door texture stretched/mirrored or vertical extent differs | A **pre-existing** texture test failure, reproduced before the new door-visibility logic. Actual renderer issue remains to investigate. |

The individual sprite, enemy, weapon, map and player collision tests in that run passed. That doesn't replace looking at the game, but it is useful evidence.

## Mechanics still on the bench

| Topic | Open question / next work |
| --- | --- |
| **Enemy vs OBJET collision** | Implemented in `enemy_system.cpp` / `objet_collision.cpp`, including normal and MONS5 movement and ±80 angular-unit response; broader manual route checks remain |
| **OBJET6 timer** | Recover exact eligibility from DOS sector/vertex processing; current ray renderer doesn't produce the identical `0xffff` visibility marker |
| **Object heights** | No incorrect static height has been proved; identify a specific level/bank/image if something looks wrong |
| **Decorative timing** | Main decorative simulation currently uses provisional **18 Hz**; falling cadence remains an adaptation until calibrated |
| **Door fidelity** | Original 0–90–0 state cycle integrated; compare trigger distance, player stall, exact scheduling and special connections against DOS |
| **Elevator texture leakage** | Some cabin textures appear on adjoining corridor walls |
| **Headbob impact table** | Normal bob table recovered, special hit-height table not yet installed |
| **Player death presentation** | Original MORT0/MORT1 graphics and GAMEOVER are integrated. Health, oxygen, ship destruction and victory return to the custom menu. Host pacing remains adapted. Green GOUTTE is a separate enemy effect. See [evidence](exe/player-death.md). |
| **Bomb inventory slot** | The port implements placement, fuse, explosion and damage. DOS runs a separate arming animation and shares its RNG with movement during the shock. See [port comparison](exe/nests-and-bomb.md#port-comparison-and-remaining-checks). |
| **Alien nests** | The port reuses enemy slots, changes the sprite after a close blast and persists the mutation on travel. It uses the current connected-sector set, where DOS uses the previous one. Live DOS comparison is pending. See [generator evidence](exe/nests-and-bomb.md#reusing-enemy-slots). |
| **Colored proximity lights and optics** | The port applies lamps, optics and transient tints to composed RGBA pixels quantized to RGB6. DOS programs an indexed VGA palette. The port discards the first resumed gameplay interval after a blocking interface. DOS sets the first delta after ordinary inventory return to 1. [Evidence and validation](exe/colored-lights.md#port-status-and-verification-boundary). |
| **Motion detector cadence** | The port advances the sweep at the 18 Hz enemy simulation rate. DOS advances it once per gameplay loop. [Projection, marker placement and timing evidence](exe/motion-detector.md). |
| **`ATTENTION: POWER IS SURGING`** | Original global timer, full DAC darkening, text and audio recovered. Port effect integrated with an independent random stream. DOS observation pending. See [evidence](exe/power-surging.md). |
| **Greyscale modal background** | Check original locker/elevator palette behaviour before adding an effect |
| **Map and transfer audio** | Confirm sound triggers and reproduce them accurately |
| **Inventory full** | Complete original replacement/selection behaviour |
| **Endgame and MONS9** | Verify final triggers and special reader sequence |
| **Password continuation** | LOAD GAME uses the [16 preset checkpoints](exe/password-continuation.md) and starts from a pristine world. The menu response uses host time and the arrival notice uses the port overlay. DOS uses VGA retrace timing and palette transitions. |
| **Campaign playthrough** | Manual verification of all 30 levels and their transitions |
| **Performance / aspect ratios** | Plane renderer cost and proper 4:3, 16:9, 21:9 handling |

## Next practical work

First compare the port's door opening threshold and short movement stall against the original DOS in a reproducible level-1 scenario. If the observed behavior differs, fix only the demonstrated discrepancy; if DOS behaves the same, keep the original behavior as the default and treat earlier opening as an optional quality-of-life adaptation. Then prioritize the remaining gameplay discrepancies and original effects listed above using specific reproducible cases, without reopening already verified investigations.

The important rule is one the game has taught us repeatedly: **don't “correct” something because it looks strange until we've checked what the DOS actually does**. Doors can connect before they look open; enemies can retreat home; ceiling drops can be intentional. That's part of the game we're trying to preserve.

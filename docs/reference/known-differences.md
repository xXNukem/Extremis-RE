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
| **Player death presentation** | Original MORT0/MORT1 graphics, GAMEOVER and new-game menu path integrated; host-paced retrace/unpaced loops remain adapted. Native OPTIONS/password continuation are unavailable. Green GOUTTE is a separate enemy effect. See [evidence](exe/player-death.md). |
| **Mines** | Placement, arming, consumption, explosion and damage need complete verification |
| **Alien nests** | Even **which levels contain them** is not yet established; no invented list |
| **`ATTENTION: POWER IS SURGING`** | Determine trigger, whole-level flicker mechanism and gameplay consequences |
| **Greyscale modal background** | Check original locker/elevator palette behaviour before adding an effect |
| **Map and transfer audio** | Confirm sound triggers and reproduce them accurately |
| **Inventory full** | Complete original replacement/selection behaviour |
| **Endgame and MONS9** | Verify final triggers and special reader sequence |
| **Save/load** | Full state persistence and cross-level restoration still need a finished, tested solution |
| **Campaign playthrough** | Manual verification of all 30 levels and their transitions |
| **Performance / aspect ratios** | Plane renderer cost and proper 4:3, 16:9, 21:9 handling |

## Next practical work

First compare the port's door opening threshold and short movement stall against the original DOS in a reproducible level-1 scenario. If the observed behavior differs, fix only the demonstrated discrepancy; if DOS behaves the same, keep the original behavior as the default and treat earlier opening as an optional quality-of-life adaptation. Then prioritize the remaining gameplay discrepancies and original effects listed above using specific reproducible cases, without reopening already verified investigations.

The important rule is one the game has taught us repeatedly: **don't “correct” something because it looks strange until we've checked what the DOS actually does**. Doors can connect before they look open; enemies can retreat home; ceiling drops can be intentional. That's part of the game we're trying to preserve.

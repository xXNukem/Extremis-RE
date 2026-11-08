# Known differences and unfinished mysteries

This page is the anti-hype section. Extremis RE is already able to do a lot, but **“loads all thirty levels” isn't the same as “every level has been completed and checked against DOS.”** Here's what we know still needs attention.

## First: public code versus local work

The documentation branch is based on the public `main` tree. A substantial batch of newer work was developed **locally and had not yet been pushed** when this documentation was prepared. It includes:

- Headbob with persistent phase and original footstep timing behaviour.
- Original bank-filtered **11 decorative falls**.
- Whole-image decoration cycles for **OBJET2, OBJET3, OBJET4 and OBJET9**.
- Re-enabled OBJET patch composition and rotating columns.
- Per-bank original OBJET XY collision tables and an in-progress player collision implementation.

Those findings are explained in the appropriate chapters, but **they should not be read as claims about the published `main` binaries**. Merge the tested code changes separately before changing this status. The documentation work does **not** modify the engine source.

## Current test status

A recent local Release build compiled successfully and ran **24 CTest cases**: **22 passed, 2 failed**. These failures are tracked rather than quietly hidden:

| Test | Symptom | What is known |
| --- | --- | --- |
| `survival_session` | `Nonfatal resource-loss fixture failed` | Test uses F5/F7 diagnostic keys, but Release builds the `survival_session` library without `EXTREMIS_DEBUG_MENU`. The fixture therefore cannot drive the expected resource changes. Fix the **test linkage/build setup**, not Release gameplay by enabling cheats. |
| `closed_doors` | Moving door texture stretched/mirrored or vertical extent differs | A **pre-existing** texture test failure, reproduced before the new door-visibility logic. Actual renderer issue remains to investigate. |

The individual sprite, enemy, weapon, map and player collision tests in that run passed. That doesn't replace looking at the game, but it is useful evidence.

## Mechanics still on the bench

| Topic | Open question / next work |
| --- | --- |
| **Enemy vs OBJET collision** | Apply original per-sector boxes and **±80 angular-unit** enemy response; don't invent sprite-shaped circles |
| **OBJET6 timer** | Recover exact eligibility from DOS sector/vertex processing; current ray renderer doesn't produce the identical `0xffff` visibility marker |
| **Object heights** | No incorrect static height has been proved; identify a specific level/bank/image if something looks wrong |
| **Decorative timing** | Main decorative simulation currently uses provisional **18 Hz**; falling cadence remains an adaptation until calibrated |
| **Door timing** | Reproduce DOS 60–90 progress/return behaviour and physical edge cases |
| **Elevator texture leakage** | Some cabin textures appear on adjoining corridor walls |
| **Headbob impact table** | Normal bob table recovered, special hit-height table not yet installed |
| **Player death presentation** | Restore full-screen original green splash/goo effect |
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

## A sensible order

First fix **the Release test fixture** and examine `closed_doors`. Then manually verify the new local decoration/player collision work, implement enemy-vs-OBJET response, and finish OBJET6's eligibility/timing if the extra architectural work is justified. After that: inventory/mines, original palette effects, special level events, save/load and the full campaign.

The important rule is one the game has taught us repeatedly: **don't “correct” something because it looks strange until we've checked what the DOS actually does**. Doors can connect before they look open; enemies can retreat home; ceiling drops can be intentional. That's part of the game we're trying to preserve.

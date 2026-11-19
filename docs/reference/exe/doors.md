# DOS doors: original state word, phase, requests, and automatic closing

## Door crossing, actor clearance, shots and interaction audit (2026-11-19)

**Source and scope.** Targeted read-only x86 disassembly of the local EXE with the reference SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`. Campaign geometry and sprite resources came from the local installation, with `GAME.LAB` SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`. Code offsets are absolute file positions. Bracketed addresses are DOS memory. EXE evidence below is static. The port traces use controlled proposals and the original geometry, plus synthetic fixtures. They are **Port-tested** and **DATA-confirmed** where indicated.

### Player crossing

- **EXE-confirmed.** The connection branch sets the normal clearance to 180 at `0x10a4a`, including the jumps that bypass requesting opening. For a blocked state, `0x10b2c–0x10b34` skips the face when the proposed normal is already negative. From normal ≥180 and inside the tangential limits, the closed-word correction uses 182 (`0x10b89 → 0x10c1e–0x10cbe`). The original movement has a capped step scale of 30, doubled to 60 for running at `0x1658d–0x165b5`. Player heading gates the request rather than the physical passage check.
- **Port-tested / Adapted.** In the synthetic x=500 door, a closed-state proposal from (480,200) to (520,200), heading π, previously reached (520,200) retaining source sector 0. A rotated synthetic face (500,500)→(640,379) also let a 0.001-unit crossing through because the rounded Q14 normal was +1. The port checks the actual swept segment before committing a blocked crossing. It retains the old position for these rejected aggregate proposals. The check follows intersected connections into the next sectors when a large proposal can cross an open face and then a blocked face. It also retains the existing request, correction, phase-64 and fractional-passage paths. The port still resolves native movement requests once per aggregate, so intermediate enemy substeps can observe the proposed camera pose before the final correction.

### Enemy clearance and the sprite plane

- **EXE-confirmed.** Normal walking `0xe607–0xe650` and MONS5 forward movement `0xefd3–0xf01c` call the wall routine with mode `0xffff`, except behavior ≥3 outside the player sector, which uses `0xfffe`. MONS5 recoil `0xeae3–0xeb11` uses `0xffff`. Mode `0xffff` branches at `0x10929–0x10931` to generic wall processing. It uses the 80-unit normal clearance initialized at `0x108b1` and the +2 correction at `0x10c8b–0x10cbe`. Mode `0xfffe` reaches the 180-unit path and for a nonzero non-passable masked state restores the normal to 180 without player tangential centering (`0x10b2c–0x10b84`). The original actor connection mode can request opening without the player's [120,240) filter at `0x10a17–0x10a2d`. The main loop calls enemies, player, then door updater at `0x7026`, `0x7039`, `0x7078`.
- **Port diagnosis.** Movement and the sprite had the same world center, but the port stopped approaching actors at a circular radius of 30. For a closed x=500, y=100..300 door, center (470,200), and camera (650,350) aimed at the center, original MONS1 frame 0 opaque texel (10,45) maps to billboard XY≈(509.61,152.47), with depth≈234.31 against door depth≈250.35. Original MONS5 frame 0 texel (8,26) maps to XY≈(507.61,154.87), with depth≈234.31 against door depth≈246.83. Those pixels are in front under the billboard projection. This is a geometric reproduction with original art and a controlled camera. It does not measure DOS sprite clipping.
- **Port-tested / Adapted.** `doors::moveEnemy` applies the supported 80+2 or 180/182 clearance to recognized non-passable faces for both monster paths, before sector location and destination OBJET correction. If clearance changes the path, it uses the existing sweep again to keep static walls effective. The port keeps its existing navigation, opening policy and passable-sector behavior. Generic-mode actors already inside clearance when a leaf reappears are not automatically displaced. The precise DOS result for an actor already on the plane when the leaf reappears remains unverified. The optional original MONS5 movement fixture fails at enemy index 13 in both `48d7f48` and the current build.

### Projectiles and visible faces

- **EXE-confirmed.** `0x11609–0x11666` passes the connection words and projected wall/index output tables to `0x117fe`. At `0x11abd–0x11ae8`, a connection wall is skipped from phase 60. Below 60, `0x11aed–0x11b42` retracts the first camera-space endpoint toward the second. `0x11b48–0x11c7d` projects and stores the shortened 30-byte wall record and depth index. The shot routine at `0x17d93–0x17dc5` looks up those same projected records via `[0x5100]` and `[0x50fc]`. It classifies with `flags & 0xc000` (`0x17dd3–0x17df3`), tests projected Y against `[record+2,record+6)`, projected X against the record bounds at +18/+20 and shot depth against the indexed wall depth. A hit sets state 2 and phase 1 at `0x17e4e–0x17e61`.
- **Port-tested / Adapted.** The port had used the full movement blocker for a projectile until the movement passage branch was enabled, even when the panel was visually hidden on descent. `DoorControl` now derives directed projectile leaves with the same `doors::visibleFace` function as rendering. The existing ray-distance and enemy priority logic uses the camera-facing leaf and nearest static wall for both travel and flight visualization. For x=500, phase 30, the x-positive shot at y=225 crosses the plane from the opening side and later hits the wall at x=560. A shot at that same y hits x≈500 if the phase reaches 15 before the next step. This preserves the port's world-space ray and aiming model. DOS uses its projected coordinate and depth comparisons, so exact impacts at projected edges may differ.

### Action, backing and opposite faces

- **EXE-confirmed.** The configured key test `0x16d9a–0x16db0` sets action `[0x5407]`, passed separately from heading and player mode at `0x168b6–0x168e8`. Ordinary doors use the directed wall array of the current sector (`0x10793–0x107b4`), the previous/proposed proximity intervals and relative heading [120,240). Pressing the action key facing away does not request ordinary opening. Backing into the door facing away does not substitute travel direction for heading. An ordinary door can open from either authored face when approached with the required relative heading, with or without action. An eligible stationary update in range can request opening.
- **EXE-confirmed.** Explicit action selects a `0x3xxx` panel at `0x10b9c–0x10c18`. Its proposed normal is [0,90), tangent [0,wall length), and relative heading [120,240). Selection is dispatched at `0x16925–0x1698c`. Card-reader class 3 checks the indexed lock at `0xa567–0xa5aa`. The low parameter byte indexes the connection and the high byte identifies the card (`0x937c–0x9395`). Successful validation `0x95a1–0x95d7` changes `0x0400` rather than requesting opening. Common modal exit `0xa617–0xa62b` clears the phase table retaining locks. The special communication and shuttle routes use state indices 44 and 45 and sector-specific gates (`0x704c → 0xa937`, `0x7051 → 0xb4b2`). Panel access on the opposite face depends on whether that sector actually authors a facing panel.
- **Port comparison.** The ordinary request already follows heading and access checks. The existing panel selector rejects a directly backward heading. Its floating-point cone, ray occlusion and nearest-panel selection are separate from the exact DOS action-sector wall scan. This audit found no backwards activation discrepancy requiring an interaction change. It did not reproduce cross-sector panel selection or confirm all special interactions in live DOS.

### Deterministic comparison and validation

**Preexisting in `48d7f48`.** The same probe compiled and linked for the reference and current port ran 199,056 records per build, covering real geometry in levels 1, 3, 20, 22 and 23 and both monster banks with original art in an isolated two-sector fixture. Two examples use level 1, connection index 1, sector indices 1→2, with the closed face (3400,1700)→(3600,1700).

| Controlled case | `48d7f48` | Current port |
| --- | --- | --- |
| Backing, state 0, heading −π/2, (3450,1680)→(3450,1720) | Ended at (3450,1720) with source sector unchanged | Rejected the crossing and retained (3450,1680), sector 1 |
| Projectile, descending phase 30, (3450,1660), heading π/2. Visible face (3500,1700)→(3600,1700) | Impact on the full blocker at (3450,1700) | Continued through the clear quarter to (3450,1740) |
| MONS1 and MONS5 in the isolated closed-door fixture, x=1000 and motion toward the face from x=790 | Centers reached x=970 in sector 0 | Both stopped at x=818 in sector 0 |

Of the 7,992 controlled proposals ending beyond a phase-zero face in the five-level trace, including locked words, the reference accepted all 7,992 and the corrected port retained the source side in all 7,992. This tests injected proposals and is not a frequency estimate for actual play. The before/after CSV, benchmark source and timed runs are local under `build/door-incidence/`.

**Validation.** Debug and Release CTest passed 40/40. Original-resource checks scanned 30 levels, 1,346 directed recognized door faces including 28 locked faces, and 251 panels including backwards and reverse-face checks. All 30 resource-bank sets loaded. The renderer matched 2,469,600 reference column intersections across the campaign. Thirty fixed-camera snapshots matched the archived renderer reference byte for byte, including color, depth, covered wall rows, composition and SDL output with fisheye on and off. These fixed-camera comparisons do not compare the deliberately changed enemy positions or successful shot-through behavior.

**Microbenchmark.** The same probe ran in before/after/after/before order in each configuration. Median of the two run means for each variant, in microseconds per call:

| Probe | Debug `48d7f48` → current | Release `48d7f48` → current |
| --- | ---: | ---: |
| Player connection proposal | 0.407 → 0.450 | 0.043 → 0.040 |
| Movement world and leaf rebuild | 1.640 → 5.373 | 0.304 → 0.539 |
| Initial projectile step | 33.488 → 50.763 | 1.198 → 1.685 |
| Enemy update in the two-sector fixture | 1.823 → 2.037 | 0.133 → 0.143 |

The current rebuild and projectile step handle the additional directed leaf geometry. Enemy timing is for the controlled one-actor fixture. These samples measure the instrumented calls, with different outcomes for corrected movements and shots. They do not measure gameplay FPS.

## Open-passage slowdown in the port (2026-11-19)

**Port-tested.** A deterministic trace using level 1, connection word index 1, from sector 1 to sector 2 (zero-based indices) reproduced a second slowdown after the opening wait. The static collision solver returned the requested motion. At position (3500,1600) with passable state `0x013d`, a 1 ms walking update requested (0,0.3) and applied (0,0). At the first passable update of a 4 ms approach, position (3500,1520), it requested (0,1.2) and applied (0,1). The defect ended outside the passage region when the player could reach it. The pre-refactor revision `48d7f48` and the port before the patch matched on 32,352 trace rows after excluding measured CPU times. The input `GAME.LAB` matched the reference hash in the index.

**Implementation.** The port integrates real-valued elapsed-time motion, then the connection routine quantizes its proposal into signed local words. Previously the inverse Q14 transform wrote the integer result back every frame, discarding the fractional part even in an open passage without a tangential clamp. [`DoorControl::constrainPlayer`](../../../src/extremis/world/door_control.cpp) now applies the existing signed integer correction as a delta to the real-valued proposal in that case. The traced Q14 transforms (`0x1117a–0x111ee`), passage threshold (`0x10a53–0x10a64`), phase-64 writes and clamp/coordinate reconstruction (`0x10aa7–0x10b23`) still govern the integer calculation. Closed/opening corrections and actual tangential clamps still write absolute corrected coordinates.

**Verification.** `fractionalPassage` in `door_control_tests` verifies both directions, walking and running, frontal and oblique approaches with 1, 4, 7, 16, 33 and 56 ms updates. The trace with original geometry also exercised a closed-door approach, the opening wait, crossing at the first passable update, and simultaneous forward/strafe input. The native 180-unit opening stop and tangential clamp remain observable. This comparison reuses the inherited EXE findings below and measures injected-time port simulation. Real-time DOS cadence and full gameplay FPS remain unmeasured.

## Current port status (2026-11-14; supersedes historical implementation notes below)

**Implemented and user-tested in Debug:** the runtime uses `Level::connectionStates` as the authoritative per-connection word; `doors::dos` handles the original 0→90→0 phase cycle; `doors::dos::Counter` supplies nominal PIT-derived increments; the main loop orders movement/request, door update, then counter reset. `DoorControl::constrainPlayer` handles the door-relative Q14 movement correction and player sector transition. Rendering, connectivity, interactions, collision and audio use the live table. Entry/reset behavior is handled by `DoorControl::beginFrame`/`resetCounter`.

**Validation evidence:** on 2026-11-14 the user rebuilt Debug and reported **26/26 CTest passing**, after correcting a diagonal Q14 regression test's expected position from `(443,302)` to `(445,301)` using the recovered lookup table; no engine-code change was made for that correction. The user manually confirmed doors open and close during gameplay. This is **Port-tested**, not proof of complete timing equivalence to the DOS executable.

**Activation audit (2026-11-14):** the near-door stop follows the recovered request and movement rules; no activation discrepancy was found in the ordinary approach path. The static comparison is detailed below. Its perceived duration has **not** been measured against running DOS. Keep the default distance unchanged; manually compare both sides, card access, suspension/resume and special connections.

**Timing fidelity limitation:** the PIT divisor, accumulator update and DOS phase arithmetic are recovered, but modern main-loop scheduling differs and the complete original suspension/interaction call paths remain unverified. Never claim exact real-time duration from automated tests alone.

**Source of truth:** `src/door.cpp`, `src/door_clock.cpp`, `src/door_control.cpp`, `src/main.cpp`, tests `dos_door_tests`, `door_clock_tests`, `door_interaction_collision`. Historical paragraphs below preserve how the implementation was reached; references to an unintegrated or provisional port describe **earlier snapshots**, not current `main`.

**Source:** read-only Codex disassembly of `EXTREMIS.EXE`, reported on 2026-11-14. The analyst used the repository's LLVM disassembler; the locked Ghidra project was not opened. This file faithfully preserves the **reported findings**, not an independent second disassembly or a completed gameplay implementation.

- Original `EXTREMIS.EXE` SHA-256: `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397` (reported match).
- All `0x...` code offsets below are **absolute EXE file offsets**. Values like `[0x1428]`, `[0x142c]`, `[0x5196]`, `[0x5427]` are runtime memory addresses.
- Evidence labels: `EXE-traced` = direct instruction/call-path evidence in the research report; `Inferred` = consequence extrapolated from code, not demonstrated dynamic observation; `Unresolved` = recheck original when implementing.

## Core result

The original does **not** have a separate `Open` wait timer. A **16-bit per-connection state word** includes an 8-bit phase, an upward-direction/open-request bit and an access-lock bit. Opening counts upward to **90**, reverses direction, and eventually counts back down to **0**. The door leaf is fully hidden as soon as the phase reaches **60**.

| Bits | EXE-traced interpretation |
| --- | --- |
| Low byte (`AL`) | Phase of leaf animation and visually open interval |
| `0x0100` | Upward phase direction / opening request (`AH` bit 0) |
| `0x0400` | Restriction on ordinary opening (card-lock bit) |

**Normal phase transitions** (assuming accepted opening request, no special writes):

| Phase and direction | Result |
| --- | --- |
| Phase 0, no `0x0100` | Remains closed |
| `0x0100` and phase below 60 | Leaf retracts |
| `0x0100` and phase 60–89 | Leaf hidden, phase continues upward |
| Phase >=90 during upward update | Set phase exactly 90; clear `0x0100`; discard overflow |
| Downward phase 90–60 | Leaf remains invisible |
| Downward phase 59–1 | Leaf closes visibly |
| Downward subtraction becomes negative | Set phase to zero |

A repeated accepted opening request sets `0x0100`; it **does not** toggle the direction. While descending it can therefore cause the phase to ascend again.

### Phase updater and byte arithmetic

- Updater at **`0x1146d–0x114b9`**, called at **`0x7078`**, scans the whole connection-state table located via `[0x5196]` (entry count read at `[esi-2]`).
- `0x11484–0x1148b`: `mov bx,[0x1428]` then `shr bx,1`; updater uses `BL` as delta. Effective amount: `low_byte(word[0x1428] >> 1)`.
- `0x1146e–0x11476`: skip update when `[0x5427] != 0`.

Selected decisive instructions as transcribed in the investigation (verify against original bytes if any discrepancy arises):

```asm
0x1149a  test ah,1
0x1149d  jne  opening
0x1149f  sub  al,bl
0x114a1  jns  save
0x114a3  xor  al,al
0x114a7  add  al,bl
0x114a9  cmp  al,90
0x114ab  jl   save
0x114ad  xor  ah,1
0x114b0  mov  al,90
0x114b2  mov  [esi-2],ax
```

**Precision warning:** these are **8-bit AL operations** with signed branch conditions (`JNS`, `JL`). Do not blindly replace them with floating point or larger-integer saturation; large deltas and wraparound differ. Exceeding 90 drops the excess rather than consuming the remaining delta on the descending branch within the same update.

### What drives the delta (and what is NOT known)

| EXE offsets | Evidence |
| --- | --- |
| `0x184d–0x1857` | Write PIT divisor **7457** via port `0x40`, implying ~160 interrupts per second |
| `0x18c6–0x18d1` | Increment `[0x1428]` only when `[0x142c] == 0` |
| `0x71d6` | Main loop resets `[0x1428]` to **1** |
| `0x11484–0x1148b` | Door updater divides tick count by 2 with integer right shift; uses `BL` |

Do **not** convert 60, 90 or `[0x1428]` into an exact number of elapsed seconds. Loop scheduling, integer truncation, suspensions and the overflow-clipping at 90 influence actual timing. The report did **not** establish an exact real-time duration, nor all effects of the suspension flag or interactions with other systems.

### Temporal ordering: evidence from the subsequent read-only investigation

The following **EXE-confirmed findings are inherited from the 2026-11-14
temporal investigation in this conversation**, which verified the reference
SHA-256 above. They were not re-disassembled during clock implementation and
are not DOS-observed timing measurements:

- `0x7039` calls movement at `0x1656a`, before the door update call at
  `0x7078`. The normal path resets `[0x1428]` to 1 later, at `0x71d6`.
  Reading the counter and resetting it are distinct events.
- After the inventory call at `0x6f29`, the return path resets the counter at
  `0x6f7d`; the map return path does so at `0x7001`. The common interaction
  exit resets it at `0xa636`. These resets prevent carrying the entire modal
  interval into the next door update, without proving that the PIT stops.
- `[0x5427]` is initialized to `0xffff` at `0x6ac6` and cleared at `0x75b2`.
  It must not be equated with a generic modal-open flag.
- Examples of separate counter inhibition are writes of `0xffff` / zero to
  `[0x142c]` at `0x6c6c` / `0x6c84` and `0x7226` / `0x7234`.
  Exhaustive attribution of these regions to simulation modes remains unknown.

**Historical pre-integration observation:** the port previously called `DoorControl::prepareFrame` from `PlayerInput::update` before movement and capture/focus checks and used a floating-point one-second door update. This has been replaced in the current port. Enemy simulation's adapted 18 Hz step and oxygen timing remain distinct from the door counter's read/reset lifecycle.

## Visibility, sector connectivity, and movement permission differ

| Property | EXE-traced rule | Offsets |
| --- | --- | --- |
| Leaf visible | Phase **< 60** | `0x11ad0–0x11ae8` |
| Partial leaf geometry | Remaining span `(60 - phase) / 60`, integer-domain source arithmetic; do not assume continuous float math | `0x11aed–0x11b42` |
| Sectors connected | `(state & 0x01ff) != 0` | `0x1184f`; traversal `0x114ba–0x11555` |
| Passage-processing branch | `(state & 0x01ff) > 0x013c` | `0x10a53–0x10a64` |

The last condition, on normal ascending states, means bit `0x0100` **and** phase **> 60**. It is not equivalent to visually hiding the door at 60, nor to finding a physically large enough circular gap in the port. Sector connectivity can become true as soon as an opening request sets `0x0100`, even at phase zero.

## Ordinary opening requests: movement, not the use key

The investigated player-side request runs through the **movement/connection routine `0x1074b`**; the caller is at **`0x168e8`**. No use-key test guards this ordinary request branch. The action argument occurs later, for interactive surfaces, near `0x10baa`. The specific filters reported for an accepted opening request include:

- The surface represents a connection: `0x10916–0x10923`.
- Position/proximity geometry tests: `0x10937–0x10982`.
- Wall flag `0x0800` must be absent: `0x109a8–0x109b1`.
- `0x0400` locked bit must be absent: `0x109c5–0x109cc`.
- Normalized angular difference in **[120, 240)** of the original game's 720-unit turn: `0x10a03–0x10a2d`.

Accepted request:

```asm
0x10a3c  or  ax,0x0100
0x10a40  mov [esi],ax
```

**Reported behavior:** closed -> starts opening; already ascending -> stays ascending; descending -> starts ascending again from current phase. If no more requests or special writes occur, the cycle eventually finishes and closes automatically. This finding differs from the port's `E` interaction and reversible toggle.

### Player in the doorway: original special write

The movement routine can write phase 64, direction ascending and only the previously retained `0x0400` lock bit:

```asm
0x10aa7 / 0x10ad0  and ax,0x0400
0x10aab / 0x10ad4  or  ax,0x0140
0x10aaf / 0x10ad8  mov [esi],ax
```

This branch requires `(state & 0x01ff) > 0x013c` plus movement/geometry filters. If the proposed normal coordinate is non-negative, it writes `0x0140`. If negative, the routine considers the neighboring sector and writes `0x0140` when it remains >= -180. An approach path also clamps tangential positions below 80 to 82 and above 120 to 118 (`0x10af8–0x10b23`).

**Inferred (not dynamic-proof for all positions):** repeated movement-path calls satisfying the criteria may hold the door visually open around the player's passage. This is **not** an unconditional 'anything overlaps -> reopen' rule. The updater itself shows no general obstacle search; handling of other actors/objects remains unproven.

## Cards, special walls, and other writers

- Wall parameters encode door and required-card identifiers around `0x937c–0x9395`; accepted card comparison `0x95a1–0x95a9`.
- Card acceptance toggles `0x0400` at `0x95c2–0x95d7`, changing authorization **without opening** the leaf or changing phase; dispatch checks existing lock at `0xa584–0xa58b`.
- Wall flag `0x0800` prevents the ordinary opening request; its full semantic role is **unresolved**.
- `0x7b23–0x7b37`, `0x7b90–0x7ba4`, `0xa617–0xa62b`: certain routes force each state to `state & 0x0400` (close while retaining lock). The last is common exit from selected interactions; calling context needs further validation.
- Special consumers of entries 44 and 45 exist at `0xa953`, `0xb4ce`; do not generalize them to every door.
- No second generic 'code-locked door state machine' was established by this investigation.

## Historical differences to the pre-integration port (superseded)

| Port location | Incompatibility to resolve |
| --- | --- |
| `src/door.cpp` `update` | Provisional 1-second float stroke; stops in `State::Open` with no reverse phase cycle |
| `include/door.h` | `progress` and four enum states do not represent a single DOS 16-bit connection word or two hidden leaf intervals |
| `src/door.cpp` `toggle` | Manual opening/closing inversion vs original upward open request |
| `src/door_control.cpp` `interact` | Ray/use-key selection vs movement-triggered ordinary opening |
| `src/door_control.cpp` `advanceSafely` | Circular overlap/reopening preserving progress vs conditional phase-64 write in movement routine |
| `src/door_control.cpp` collision | Partial leaf/clearance test vs original passage status criterion |
| `src/door.cpp` connectivity | Similar intent in normal cases; should ultimately derive from state word |

Tests that currently enshrine port adaptations (must be revised deliberately when original fidelity is implemented):

- `tests/door_tests.cpp`: one-second stroke and equivalence of 60 fine vs one coarse step.
- `tests/door_control_tests.cpp`: manual toggle during opening; passing a partially open circle-sized gap; proximity/circle safety reversal.

**Recommended fidelity regression phases:** `0, 59, 60, 61, 64, 89, 90`; ascending and descending; repeated open requests; lock preservation; connectivity at request time; odd/zero/large deltas and overflow; cases involving passage and orientation.

## Historical implementation milestones (the isolated APIs were subsequently integrated)

### Isolated state updater implemented (2026-11-14)

**Port-tested against the inherited report above, not independently re-disassembled:**
`doors::dos` in `include/door.h` and `src/door.cpp` now accepts a 16-bit
connection word and an explicit 8-bit increment. It implements the reported
`0x1149a–0x114b2` byte operations, automatic reversal at 90, excess discard,
and preservation of the lock and all unrelated high bits. `increment(counter)`
implements `low_byte(counter >> 1)` from `0x11484–0x1148b`; it supplies no clock
or conversion to seconds. Separate queries expose leaf visibility, connectivity,
and the passage-branch entry condition (not complete movement authorization).

`tests/dos_door_tests.cpp` (`dos_door_states`) covers phases 0, 59, 60, 61,
64, 89 and 90 in both directions with deltas 0, 1 and 3, high-bit retention,
the complete cycle, signed comparison boundaries, byte wraparound and odd
counter truncation. In particular, opening byte results 128–255 stay ascending
under signed `JL`; downward results with bit 7 set become zero under `JNS`,
regardless of borrow. These are consequences of the already reported instructions.

**Historical milestone:** this API initially had no gameplay caller. The counter lifecycle, movement-triggered requests, conditional phase-64 writes and live connection-table consumers were subsequently integrated as summarized in **Current port status** above. Remaining fidelity questions must still be checked against the original.

### Isolated clock and whole-table adapter (2026-11-14)

`include/door_clock.h` / `src/door_clock.cpp` provide `doors::dos::Counter`,
initialized at an explicit reset-to-1 boundary. `advance` accepts injected
`steady_clock` timestamps in nanoseconds, defaulting to the monotonic clock.
Integer arithmetic accumulates nominal **1,193,182 Hz / 7457** interrupts,
retains the fractional remainder and wraps the word modulo 65536. The nominal
PC input frequency is the same convention already used by `weapon_fire.cpp`;
it is not a new measurement of the original hardware oscillator.

`inhibit` first accounts for the preceding interval under the old gate, then
changes the gate. While inhibited, PIT phase advances but counter increments
are discarded. `value` only reads; `reset` only writes 1, preserving phase and
inhibition. Call `advance` at the reset boundary first if time has elapsed.
Equal/backward injected samples do not advance or rebase the clock.

`doors::dos::updateTable` in `src/door.cpp` computes `increment(counter)` once
from a supplied snapshot and calls the existing state updater once per table
entry. It does not read/reset the clock, split large increments, filter locked
entries, or decide the `[0x5427]` skip gate. The caller owns whether to call it.
**Historical milestone:** neither API had gameplay callers at this stage. They were integrated later; see **Current port status** above.

**Port-tested:** `tests/door_clock_tests.cpp` (`dos_door_clock`) covers fractional
boundaries and chunking, zero increments, 16-bit wrap including multiple wraps,
inhibition/resumption, reset retaining phase/gate, snapshot/query/update/reset
separation, and whole-table updates with locks, mixed directions and large deltas.

**Fidelity limit:** this reproduces a nominal PIT-driven counter, not exact DOS
main-loop cadence or interrupt latency. Its initial PIT phase is a chosen epoch.
Different read/reset schedules can produce different door increments for the
same wall-clock interval. Rendering placement, all suspension contexts, native
pause semantics and movement integration remain unresolved; focus loss and the
Debug menu require explicit port policy. No fixed door duration is established.

The report does **not** establish complete accuracy for: special code/terminal paths, walls with `0x0800`, movement by other actors, suspension semantics and other writers, or dynamic observations when the player remains in the aperture or turns away. A DOS trace is needed to corroborate the exact scheduling/visible behavior. Confirm local source/test state before editing; **no gameplay code was changed during this investigation**, and a proposed fixed-second auto-close patch was explicitly rejected as inauthentic.

**Original report is summarized here; if a new finding contradicts this page, inspect the original instructions and amend this record with the evidence instead of silently replacing it.**

### Historical integration dependency: movement correction and sector ownership (resolved in current port)

The attempted runtime integration reviewed existing local disassembly artifacts
(`build/audio-debug/executable-disassembly.txt` and
`build/panel-debug/step203-collision.txt`), without disassembling the EXE again.
The details below are **inherited instruction evidence**, not independent
re-verification or DOS-observed behavior. They extend the geometric summary:

- `0x108ba–0x1090f` transforms both the previous and proposed positions relative
  to the wall origin. `0x10937–0x10982` tests overlap of their normal interval
  with [-180,180] and tangential interval with [0,200], inclusive. This is not
  a Euclidean proximity radius or a test of only the final position.
- `0x10a4a` sets the normal correction distance to 180. In the non-passage
  branch, `0x10b2c–0x10b89` distinguishes negative proposed normal, zero masked
  state, and nonzero state. For the player-mode argument zero, `0x10b48` calls
  `0x111ef–0x11223`, which moves the proposed tangential coordinate toward 100
  by 10, clamping at 100. `0x10b4d–0x10b80` then reconstructs the proposed
  position with normal 180. These are positional writes per call, not recovered
  world-units-per-second speeds.
- The passage branch selects the neighbor at `0x10a74–0x10a88`. Its phase-64
  writes and tangential clamps are followed by inverse coordinate conversion
  at `0x10adc–0x10b23`. `0x10d47–0x10db0` handles the sector-change sentinel,
  adds the old sector origin, loads the new sector and subtracts its origin.
- `0x1117a–0x111ee` uses signed products and high-word extraction after two
  shifts for the coordinate rotation; replacing this with normalized floating
  point dot products would introduce different boundary rounding.

**Historical gap, resolved by the later integration:** `Camera` previously stored global doubles without authoritative sector ownership. `player::update` in
`src/player_control.cpp` applies `beforeWalls`, then the circular
`collision::move`, then `constrain`; enemy stepping can split these calls.
The door correction's position, sector transition and invocation cadence need
an explicit integration design and boundary regressions together. Merely
rebuilding a segment world from the phase predicate cannot reproduce these
writes. Applying native corrections before/after the current circular solver
without validating their ordering would be an unverified adaptation.

No runtime migration was made **in this earlier investigation**. That movement path was subsequently integrated. The questions about full DOS loop cadence and suspension flags remain open.

### Activation/stall audit (2026-11-14)

**Evidence:** reused the existing `build/audio-debug/executable-disassembly.txt`
and `build/panel-debug/step203-collision.txt` excerpts for the reference EXE
SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
This is a comparison against inherited **EXE-confirmed** instructions, not a
new binary/hash verification or **DOS-observed** measurement. No new disassembly.

- `0x10759–0x1078c` builds bounds from the proposed position; endpoint filters
  at `0x107db–0x10823` require both endpoints inside [x−300,x+300] and
  [y−300,y+300]. These bounds are a preliminary filter, not an opening radius.
- The Q14 transforms and interval tests at `0x108ba–0x10982` use previous
  **and** proposed positions: normal interval overlaps [−180,180], tangent
  interval overlaps [0,200], including endpoints. The player heading relative
  to the directed wall must be [120,240) in 720 units (`0x10a03–0x10a2d`);
  strafing/backing in does not substitute movement direction for heading.
- The accepted request (`0x10a3c–0x10a40`) and normal correction to 180
  (`0x10a4a`, `0x10b2c–0x10b80`) occur on the same call. The latter moves the
  tangent toward 100 by 10 per call. Passage requires masked state >0x013c,
  not merely invisible phase 60 (`0x10a53–0x10a64`). Thus stopping while a
  closed door opens follows the original rules; there is no advance trigger
  outside the stopping line. Passage clamps and phase-64 writes remain as above.
- Caller `0x16851–0x168fd` passes previous and object-corrected proposed
  coordinates, heading and player-mode zero to `0x1074b` at `0x168e8`.
  The previously recovered main-loop order is movement, door update, reset.

**Current port comparison:** `DoorControl::constrainPlayer` implements these
ordinary-door tests and corrections. `PlayerInput::move` uses `staticWorld()`,
which excludes 0x1000 faces, before the single aggregate connection correction
in `main.cpp`; `movementWorld()`'s closed-door segments do not stop the player
before requesting. Object, static circular and enemy collision can alter the
proposal, but no extra delay was demonstrated for unobstructed approaches.
Door update follows the request in the same active frame (entry/resume skips
one update by existing port policy). No gameplay code changed.

**Port-tested:** `movementApproach` in `tests/door_control_tests.cpp` exercises
actual `player::update` plus static collision then connection correction, both
faces, walking/running and 1/60 and 1/18 second samples. It checks activation
at the quantized 180 boundary and stopping through phase 60, with advance at
61. Existing tests cover angular/tangential boundaries, diagonal Q14 rounding,
locks, special flags, sector changes and request-before-update ordering.
Debug configure/build succeeded and the full CTest suite passed **26/26** in
this audit, including the extended door test. These results do not establish
original wall-clock timing or obstacle equivalence.

**Unknown/Adapted:** global double movement, rounding at the DOS boundary,
provisional speeds, circular static collision, enemy/posture substeps and
modern frame scheduling remain port adaptations. The 18 Hz substeps do not
multiply door requests/corrections; those run once per aggregate frame.
No exact waiting time or complete DOS movement fidelity is claimed.

**Future QoL, not implemented:** an opt-in earlier request could set only the
opening bit outside the native trigger while retaining access/orientation
filters, native passage corrections and phase updater. It would change when
the cycle starts (and therefore when closing can occur), even without changing
animation speed or closing rules; repeated distant requests could also extend
opening. Define that policy explicitly and keep the faithful default unchanged.

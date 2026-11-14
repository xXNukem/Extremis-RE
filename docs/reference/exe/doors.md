# DOS doors: original state word, phase, requests, and automatic closing

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

## Concrete differences to the existing port (at investigation time)

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

## What still requires new work

The report does **not** establish complete accuracy for: special code/terminal paths, walls with `0x0800`, movement by other actors, suspension semantics and other writers, or dynamic observations when the player remains in the aperture or turns away. A DOS trace is needed to corroborate the exact scheduling/visible behavior. Confirm local source/test state before editing; **no gameplay code was changed during this investigation**, and a proposed fixed-second auto-close patch was explicitly rejected as inauthentic.

**Original report is summarized here; if a new finding contradicts this page, inspect the original instructions and amend this record with the evidence instead of silently replacing it.**

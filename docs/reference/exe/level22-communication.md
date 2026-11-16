# Level 22 recorded communication (2026-11-16)

## Scope and evidence

**EXE-confirmed:** targeted 32-bit LLVM decoding with the existing
`tools/inspect_original_code.py`; direct reads of referenced strings/tables.
**DATA-confirmed:** bounded GAME.LAB parsing using the layout in
`src/game_lab.cpp`, and resource inspection with existing `decode_spr`.
Sources independently hash-checked:

- EXTREMIS.EXE: `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`
- GAME.LAB: `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`

Offsets are absolute file positions; brackets denote DOS memory. Only the
static strings/tables listed here use pointer +0x200 to locate file bytes.
No DOS execution, translation, implementation or gameplay test was performed.
Existing door opening semantics and timer-update placement are inherited from
[Doors](doors.md) and [Self-destruction](self-destruction.md), not re-investigated.

## Exact trigger and location

**EXE-confirmed:** gameplay calls `0x704c → 0xa937`. The event routine is
`0xa937–0xafb4`, with local presentation helpers `0xafb5–0xb262` and text
helpers `0xb263–0xb41a`. The caller follows the normal gameplay update and
its `[0x5202]==-1` gate (`0x7039–0x704c`). Event predicates:

- `0xa937–0xa952`: zone `[0x51f0]==21` (**human level 22**) and
  sector `[0x51ee]==16` (**zero-based**).
- `0xa953–0xa96e`: connection word `[[0x5196]+0x58]`, **connection 44**,
  must have both AL and AH nonzero. This tests the whole high byte, not
  specifically bit 0. `[0x5196]` points after the connection-count word
  (`0x1136f–0x1137c`).
- `0xa96f–0xa9a1`: signed AL ≤60 sets black palette effect
  `[0x53f9]=-1`, RGB `[0x53fd..0x53ff]=0`, amount `[0x53fb]=AL*100/60`,
  and returns. With ordinary door states this is the opening transition.
- Signed AL >60 (normally 61..90): `0xa9a2–0xa9a7` clears the entire
  connection word, then enters the blocking dialog. No action-key, inventory,
  security, countdown or message-completed prerequisite is tested here.

**DATA-confirmed:** level record starts at LAB `0x299d6`; connection 44 is
LAB `0x29e3e`, initially zero. Sector 16's directory entry is `0x29a4a`,
sector data `0x2b7d2`, origin **(2800,1200)**, neighbors **17,15,44**.
Its wall **2**, LAB `0x2b83e`, type **0x1017**, connects to sector **44**
wall **0**, LAB `0x2e16e`, also type 0x1017. The shared doorway is at
world **(3200,1300)–(3400,1300)**; sector 16's local endpoints are
(600,100) and (400,100). Sector 44 has origin (3000,1000).
Thus this is the doorway approached **from sector 16 toward room 44**,
not an arbitrary room-entry callback. Ordinary movement opening the door
supplies the trigger; the event itself does not test exact player coordinates.
No authored room name has been established.

## Presentation, choices and pages

**EXE-confirmed:**

1. Stop gameplay voices (`0xa9a8 → 0x13090`); allocate three 64800-byte
   buffers and one 4096-byte decode scratch buffer (`0xa9ad–0xa9e8`).
2. Load/decode `CON_VUE.VGA`, load `CON_VUET.SPR`, black DAC, copy the
   image and fade brightness **0,2,…,100** (`0xa9e9–0xaa5e`).
3. Type the system-on question centered at y=20 (`0xaa5f–0xaabc`, helper
   `0xb2a5–0xb32d`: counter wait strictly >8 per character). Display
   YES at y=30 and NO at y=40, centered with proportional font widths.
   YES is selected initially. Selected choice blinks with accumulator <80,
   resetting at ≥160; other choice remains visible (`0xaabd–0xabdb`).
4. `0xabdc–0xac1b`: common input `0x16cd9`; `[0x5405]==1` selects YES,
   other nonzero selects NO, `[0x5407]!=0` confirms. Fade **100,98,…,0**.
   **NO branches straight to common return at 0xaf34** (`0xac30–0xac3d`).
5. **YES:** load/decode `CON_ECR1.VGA` and `CON_ECR2.VGA`, then load
   `CON_VISA.SPR` (`0xac3e–0xacb0`). Install the first image with the same
   fade-in. Animate sprite IDs **70,71,72,73**, at (46,53), each with
   counter wait strictly >20 (`0xace3–0xad61`).
6. Type **seven pages in table order**. Page routine `0xafb5–0xb18c`
   clears text rectangle x=96..255, y=19..150 in both backgrounds;
   starts at (96,19), advances x by 4 per non-CR byte, CR resets x=96
   and advances y by 6. Glyphs use CON_VISA.SPR and the shared local
   character mapper; wait strictly >4 per character.
7. While typing and waiting for acknowledgement, alternate the two
   backgrounds at phase 80 of a 160-unit accumulator. Animate portrait
   and bottom scanning indicator using `0xb18d–0xb262`. At page end,
   poll the action input until confirmation (`0xad9d–0xae5f`); confirmation
   starts the next page, or ends after page seven. No automatic page timeout.
8. Clear the text area, reverse sprites **73,72,71,70** at (46,53),
   >20 units each; fade to black (`0xae60–0xaf33`), then common return.

**Controls/interruption:** `0x16d5c–0x16db8` maps configured up/down/action
keys to those input fields. Static defaults at EXE `0x5615/0x5617/0x5619`
are **0x60 / 0x61 / 0x39** (engine extended up/down codes and Space action).
These are configurable DOS bindings, not a hard-coded E requirement.
The mapper also calls its existing optional device handler `0x16fa7`;
that device mapping was not expanded for this task.

There is **no Escape branch in the YES/NO loop**, opening/closing animations,
question typing, character typing or page-acknowledgement loop. Instead,
`0xb06c–0xb096` checks raw Escape (scan 1) **at entry to each page**;
if set, clears that key with `0x2ad1`, sets page index to count+10 and
returns. `0xadd2` then takes the normal closing-animation path. Therefore
Escape can abort at a page boundary, but is not an immediate interrupt:
during a page it must still be held/latched when the next page starts,
after the action acknowledgement. On the last page action already ends
normally. Action does not fast-forward typing; it is polled after typing.
Physical key carryover/holding and device behavior need DOS observation.

## Original text locations (no prose copied)

Question: EXE **0x21e04–0x21e25**, NUL at **0x21e26** (system activation).
Choice storage **0x21e27–0x21e2e**: YES pointer starts at 0x21e27 and
includes the following NO text up to the shared NUL; NO pointer starts at
0x21e2b. Preserve this original layout rather than assuming separate strings.
Page count **7** at `0x21e2f`, seven little-endian pointers at
**0x21e31–0x21e4c**. Below, end is the exclusive prose boundary / NUL offset.
Content descriptions identify the data; they are not translations to ship.

| Page | Start | NUL | Bytes | Subject |
| --- | --- | --- | --- | --- |
| 1 | 0x21e4d | 0x21f3e | 241 | Recorded last message; hope the recipient can succeed |
| 2 | 0x21f3f | 0x22215 | 726 | Probe discovers unusual asteroid; laboratory ship retrieves it |
| 3 | 0x22216 | 0x22406 | 496 | Artificial surface and undeciphered markings |
| 4 | 0x22407 | 0x22697 | 656 | Attempted deciphering; drilling to reach movement inside |
| 5 | 0x22698 | 0x22941 | 681 | Warning understood too late; breeding creature escapes and infestation spreads |
| 6 | 0x22942 | 0x22c75 | 819 | Airborne pathogens, suits, blocked self-destruction and course toward a nearby star |
| 7 | 0x22c76 | 0x22d70 | 250 | Failed attempt; appeal to finish the task; end-of-message marker |

The inspected EXE mixes English and French in page 1, then French in pages
2–7. Preserve source bytes and CR layout; do not silently normalize or
translate. The earlier self-destruction note's `0x2294f–0x22d6f` was a
bounded narrative excerpt, **not page boundaries**; the table above supplies
the exact strings. The routine references no other narrative pages or second
question. The subsequent shuttle event is separate and is not part of this
communication.

## Effects, sound and return state

**EXE-confirmed / DATA-confirmed:** resource-name strings at EXE
`0xb439`, `0xb44a`, `0xb45c`, `0xb46e`, `0xb480` select respectively:

| Resource | Bytes | Use |
| --- | --- | --- |
| CON_VUE.VGA | 32307 | Prompt background and embedded palette |
| CON_VUET.SPR | 5026 | 76 proportional prompt glyphs |
| CON_ECR1.VGA | 21145 | First communication background and fade palette |
| CON_ECR2.VGA | 20600 | Alternating background; decoded palette is not separately installed here |
| CON_VISA.SPR | 26892 | 78 sprites: 3×5 glyphs, 48×64 portraits, 31×11 indicator |

No separate PAL, ANM or DSS is loaded; no speech, beep or new sound is
requested by this event. It stops existing voices on entry. Timing thresholds
are counter units / CPU iterations, **not measured wall-clock durations**.
Portrait helper copies 48×64 at image (46,53), using table at `0xb492–0xb4b1`:
`73,75,76,75,73,75,76,76,75,76,75,73,74,73,74,76`, adding 1 for the sprite
lookup. Index is `(phase>>3)`, phase increments and wraps with `&127` on
**each helper call**, including separate calls for each background. Indicator
helper clears a 291×11 band from x=11,y=181, copies sprite lookup 78,
advances x by 4 and reverses at 11/271 (initial x=11, delta=4).

Both answers and Escape share `0xaf34–0xafb4`:

- Set local player **(500,300), angle 180**, retaining level and sector 16:
  world **(3300,1500)**. The triggering connection was already zeroed.
- Clear `[0x605b]` (existing HUD message state), free all four allocations,
  invoke HUD restoration `0x13984`, set `[0x5425]=-1`.
- Reset `[0x1428]=1`, `[0x142a]=159`, clear the configured action-key bit
  via `0x2ad1`, and clear `[0x16fb1]`.

There is **no completion flag** and no write to security flags/count,
self-destruction remaining value, inventory, access-code permissions or
campaign destination. The repeatability conclusion follows from the complete
routine's predicates/writes plus the already recovered ordinary door-opening
rule: **reopen connection 44 from sector 16 to replay**, after either YES,
NO or Escape. Zeroing the connection closes it; it is not a permanent lock.
Repeated-play behavior has not yet been observed in DOS.

Gameplay is blocked throughout the dialog: there are no movement/enemy/vitals
updates here. The PIT counter continues to serve its UI animation waits, but
return discards accumulated UI ticks. In particular, the inherited destruction
update is in gameplay, not the interrupt handler: this dialog does not decrement
or activate self-destruction. It supplies narrative information only.

## Port status and reconstruction boundary

No matching event controller/dispatch exists in current `src/`, `include/`
or tests; shared CON_VUET usage elsewhere is not this communication. This task
changed documentation only. Enough static evidence exists to implement the
bounded event without analyzing unrelated cinematics.

Remaining **Unknown / DOS-observation required**: actual perceived cadence,
portrait/background animation appearance, palette-transition appearance,
held-action behavior across page boundaries, delayed Escape behavior, optional
device controls and repeated activation in a real run. These are verification
limits, not permission to substitute a modern dialog or invent gameplay effects.

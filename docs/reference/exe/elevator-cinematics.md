# Elevator and shuttle transitions — original DOS specification

Investigation: **2026-11-15**, documentation only; no engine, PSEQ or FSEQ edits.
**EXE-confirmed** below means a fresh bounded instruction read, not a DOS run.
**DATA-confirmed** means original files parsed; **Port-tested** refers only to
specified decoder probes. No running-DOS observation or wall-clock calibration
was performed. Existing introduction/ending notes supply the established common
palette, ANM, SPR and PIT semantics; their callers here were independently read.

Reference files, SHA-256 independently checked:

- `EXTREMIS.EXE`: `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`
- `GAME.LAB`: `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`

All code/data offsets are **absolute EXE file offsets**. `[address]` means
runtime memory. For the static resource/text/descriptor pointers explicitly
listed here, file position is pointer+`0x200`; this is not a universal mapping.
Human level numbers are **1–30**, DOS zone indices **0–29**, sectors zero-based.
A route's **floor** is a separate byte, not its level/zone number.

## Result and scope

There are **two distinct travel presentation paths**, not six interchangeable
movies:

1. **Ordinary elevator:** one procedural `A_LEVEL.VGA` + `A_LEVEL.SPR`
   sequence, with **four motion variants** (positive/negative floor difference,
   one floor/more than one). No ANM is played here.
2. **Shuttle evacuation:** one multipart, one-way **level 22 → level 23**
   transition, with interactive boarding/start prompts, cockpit composites,
   six ANMs and a final sprite composition. All six proposed ANM candidates
   belong to this path. It is not selected by the elevator destination table.

PSEQ and the final victory FSEQ remain separate. `FSEQ2_1.VGA` is **reused as
a background** at the end of the shuttle journey; this does not invoke FSEQ.
`VAIS_EXP.ANM` is also used by the ship-expiry death branch (AX=2,
`0x17662–0x1766d`), which is **not** an additional successful travel sequence.
See [death](player-death.md) and [ending](ending.md).

## 1. Ordinary elevator

### Dispatcher, selection and load boundary

`0xa508–0xa51a` dispatches interaction type `[0x5202]==0` to
**`0xce1a–0xd63a`**. The travel-specific part is **`0xd27b–0xd63a`**;
its full-screen composer is **`0xd96d–0xda44`**.

`0xce99–0xceb6` selects the 54-byte lift record at file
`0xdacf + 54*[0x5204]`. Its first four records are four bytes each:
`exists, destinationSector, destinationZone, destinationFloor`; local arrival
X/Y/angle are words at +16/+18/+20. UI row maps to `(~row)&3`.
The existing [original_travel.h](../../../include/original_travel.h) already
contains all 25 descriptors, 55 extant destinations and card permission lists.
No replacement destination table is needed.

At `0xd29e–0xd2d8`, compare the selected **sector AND zone** with
`[0x51ee]` and `[0x51f0]`. If either differs, write local arrival X/Y/angle
at `[0x51da/0x51dc/0x51de]`, and set `[0x520a]=-1`. This flag was cleared
on modal entry (`0xce34`). Selecting the current pair leaves it zero.

The complete boundary is:

1. Close/shrink the CH_LEVEL panel (`0xd2e5–0xd3e8`, BANK[11] on voice 0,
   then stop-all `0x13090`). When travelling, its palette fades with the
   shrink using BX=100,96,…,4 (`0xd3c1–0xd3e3`).
2. **Before changing zone/sector**, call `0x7b83` at `0xd3f4` if travelling.
   `0x7b83–0x7c40` resolves the old sector, reduces connection words to
   `state & 0x0400`, writes the old level record to `game.lab` and
   frees its resource banks. The filename pointer is `0x496c` (string at
   file `0x4b6c`), not an indirect memory pointer.
3. Write destination sector/zone (`0xd3ff/0xd405`), capture old
   `[0x51f2]`, write destination floor there, and compute **signed word
   delta = destinationFloor − oldFloor** (`0xd40c–0xd41e`).
4. Free modal temporary buffers. If `[0x520a]==0`, skip the entire travel
   image block (`0xd446 → 0xd606`). Otherwise load/decode `A_LEVEL.VGA`,
   load `A_LEVEL.SPR`, play the procedural journey and fade it out.
5. Restore drawing state with `0x13984` (`0xd613`). If travelling,
   **only now load the destination** via `0xd62f → 0x791e`; otherwise
   reinstall the current palette (`0xd623–0xd628`). Stop all voices and
   return (`0xd634–0xd63a`). `0x791e–0x7b82` reads the selected level,
   selects resource banks and resets transient rendering/weapon/motion state.

Thus the movie is **after old-level save/release, before destination load**.
It neither chooses arrival coordinates nor resets the inventory. No simulation,
health/oxygen update or card removal occurs in its blocking draw loops.
Do not copy the DOS file-write mechanism into the new presentation layer;
world persistence is a separate existing engine responsibility.

### Which journeys / same-level distinction

The predicate is **changed (zone,sector)**, not simply changed level. A
hypothetical same-zone/different-sector route would take the same save/movie/load
branch in DOS. However, **none of the shipped 25 descriptors has two extant
entries in the same zone**. Selecting the current destination is a no-travel
modal dismissal, not a same-level cinematic. The port's same-level transfer
branch is useful but must not be cited as evidence for a shipped DOS movie.

The following is the complete descriptor-derived mapping (**DATA-confirmed**,
file `0xdacf–0xe014`). Each cell is `human level / sector / floor`. Distinct
entries in a row use the same elevator presentation, subject to the existing
card permissions. This describes the table, not an end-to-end DOS playthrough.
One-entry lifts 0 and 19 have no distinct destination in their own descriptor.

| Lift ID | Extant destinations |
| --- | --- |
| 0 | 1/0/2 |
| 1 | 1/9/2, 2/0/1 |
| 2 | 2/14/1, 3/0/0 |
| 3 | 4/9/1, 3/22/0 |
| 4 | 6/10/3, 5/0/2, 4/0/1 |
| 5 | 7/0/4, 6/21/3 |
| 6 | 6/0/3, 8/0/2, 9/0/1 |
| 7 | 9/1/1, 10/0/0 |
| 8 | 11/0/2, 12/0/1, 10/1/0 |
| 9 | 12/1/1, 13/0/0 |
| 10 | 14/0/1, 13/1/0 |
| 11 | 15/0/2, 14/1/1 |
| 12 | 15/1/2, 14/2/1 |
| 13 | 16/0/3, 15/2/2 |
| 14 | 17/0/4, 16/1/3 |
| 15 | 17/1/4, 18/1/3 |
| 16 | 19/0/5, 17/2/4, 18/0/3 |
| 17 | 20/0/7, 21/0/6, 19/1/5 |
| 18 | 22/0/7, 21/1/6 |
| 19 | 23/1/8 |
| 20 | 23/0/8, 25/0/7, 24/0/6 |
| 21 | 23/2/8, 26/0/7 |
| 22 | 28/1/8, 26/1/7, 27/0/5 |
| 23 | 28/0/8, 29/1/5 |
| 24 | 30/0/6, 29/0/5 |

For example, level 1→2 is delta −1, despite increasing the level number;
level 29→28 through lift 23 is delta +3. Distance greater than one uses the
same long variant for two or three floors. There is no lift-type-specific film.

### Composition, motion and timing

Resource references: `0xd47b` → `data\a_level.vga` (string file `0xcdc6`),
`0xd4cd` → `data\a_level.spr` (file `0xcdd7`). VGA is 320×200; SPR contains
one **97×105**, zero-anchor image. Decoded background inspection shows a
perspective elevator shaft; this is resource inspection, not DOS observation.

`0xd4e2–0xd51a` chooses the motion endpoints:

| Signed delta | Initial q | Destination q |
| --- | ---: | ---: |
| < −1 | 460 | 0 |
| −1 | 260 | 0 |
| +1 | 0 | 260 |
| all other values | 0 | 460 |

The last branch includes delta 0, but no distinct equal-floor pair occurs in
these descriptors. Do not turn that fallback into an invented horizontal movie.

**draw(q)** (`0xd96d–0xda44`), with signed multiplication/division truncating
toward zero, restores the full indexed VGA background on every call, then
transparent-scales **sprite 1** into:

```text
x = 81  + trunc(19*q/200)
y = 146 - trunc(140*q/200)
w = 69  + trunc(28*q/200)
h = 74  + trunc(31*q/200)
```

Clip to x=0..319,y=0..199, transparent index 0, no anchor subtraction;
nearest-source scaling is the established `0x2055` helper. Draw finishes
with **`0x1714`** (one retrace wait, then copy canvas to screen).

First draw(qStart), then VGA-palette brightness **0,2,…,100** (each
`0x17d2` waits retrace). Start BANK[11], voice 0, again at `0xd539`.
The draw/move loop follows exactly (**EXE-confirmed**, `0xd546–0xd5ca`):

```text
q = start; speed = 4
if delta >= 0:
    do:
        draw(q)
        step = speed >> 2
        if q > end/2 - 10: speed -= 1
        else:             speed += 1
        q += step
    while q <= end
else:
    do:
        draw(q)
        step = speed >> 2
        if q < start/2 + 8: speed = max(1, speed-1)
        else:              speed += 1
        q -= step
    while q > end
```

Use word-sized arithmetic; do not replace this acceleration with linear
interpolation or use PIT ticks to advance it. The last displayed q is the
last loop iteration, **not** a forced exact endpoint. Positive q=0 is drawn
again after the initial fade. After movement, wait **30 VGA retraces**
(`0xd5cc → 0x170a`), stop all voices, fade brightness **100,98,…,0**, free
image buffers. This path has no text, ANM callback or music sequencer.

BANK[11] is `moteur ascenseur`, volume **64**, continuous=255, priority=1;
DSS is 5000 Hz, 3853 samples, loop flag 65535, loop bounds (0,3852).
Use original looping until the explicit stop, not the port's present modal
motor convenience call that forces one-shot playback.

**Keys:** the full-screen journey (`0xd44c–0xd606`) has no keyboard poll,
Escape comparison or early-exit branch. Neither Escape nor another ordinary
key skips it. Host close/focus handling is a separate implementation policy.
The surrounding modal clears the configured action-key state on entry
(`0xce20 → 0x2ad1`); that is not a generic post-movie key-buffer flush.

## 2. Shuttle: level 22 → 23

**2026-11-16 state-machine follow-up:** see
[Ship self-destruction and refused evacuation](self-destruction.md) for fresh
EXE/LAB evidence on activation, countdown, pauses, rejection and expiry. The
travel scenes below were not re-investigated. `[0x60b7]` is specifically the
**completed-security count**, not remaining time; the latter is dword
`[0x60b9]`, initialized to 48000 (displayed five-minute budget). Completing
all sixteen security readers in level22/sector43 activates it.

Neither refusal permanently consumes the shuttle: zeroing connection45 closes
and unlocks it, and ordinary approach can reopen it and repeat the prompts.
Both refusals and eligibility failures return to sector38/(300,900), retaining
cards, vitals, security state and remaining time. Gameplay resumes the countdown;
the blocking prompts do not run it. Expiry selects AX=2 ship-explosion death,
then GAME OVER/main menu. Normal success disables counting at `0xc76d`, after
the journey, while retaining the unused time value. The follow-up also records
the untested last-update expiry/boarding edge; do not assume rescue after the
exit flag has already been latched.

### Trigger and interactive prelude

Main gameplay calls **`0x7051 → 0xb4b2`**. `0xb4b2–0xb523` requires:

- `[0x51f0]==21` (level 22), `[0x51ee]==38`.
- Connection word at `[ [0x5196] + 0x5a ]` (connection **45**) has both
  AL and AH nonzero.
- While signed AL ≤60, set global palette effect `[0x53f9]=-1`, black
  RGB at `[0x53fd..0x53ff]`, amount `unsigned(AL)*100/60`, then return.
- When signed AL >60 (normal door phases 61..90), **zero that connection
  word**, stop gameplay voices and enter the blocking prelude.

The original LAB probe locates level 22 at file `0x299d6`, connection 45
at **LAB file `0x29e40`** (section begins with a count word), initially zero.
Walls **sector 38/wall 0** and **sector 45/wall 0**, type `0x1017`, reference
that connection. This is a door/sector event, not an E-press on an elevator.

`0xb528–0xbb32`:

1. `NAV_HANG.VGA`, `CON_VUET.SPR` prompt font; black DAC, display background,
   fade brightness 0,2,…,100. Ask whether to enter the shuttle.
2. Yes (default) / No; selected row blinks for accumulator<80, resets at
   ≥160. Title y=20, choices y=30/40, horizontally centered using font
   widths. Poll `0x16cd9`; `[0x5405]==1` chooses Yes, other nonzero chooses
   No; `[0x5407]` confirms (configured action; input mapper also supports
   its existing device inputs). No ordinary Escape branch is present.
3. Fade 100,98,…,0. No exits through `0xc78f`.
4. Yes loads `NAV_INTE.VGA`, composites **NAV_INS2 sprite 89 at (0,39)**
   into the background, saves ten rows beginning at image y=20 for later
   prompt removal, fades in 0,2,…,100. Ask whether to start, same UI.
5. No goes through the interior fade at `0xbb1a–0xbb2e`, then exits.
   Yes additionally requires **one of `[0x5453/55/57]==0x2005`** and
   **`[0x60b7]==16`** (`0xba4c–0xba80`). These are the exact card and
   security/autodestruction conditions; do not substitute an elevator card,
   presence of any security card, or an approximate progress percentage.
6. On failure, restore the saved prompt strip, center the appropriate
   original error text, wait for the configured action, then fade/exit.
   This check does not consume the card. `[0x60b7]` increments in the
   security-panel path at `0x95f3`; its value 16 also gates the destruction
   countdown at `0x16c57–0x16cc1` and expiry death at `0x76dd–0x76f4`.

Original localized prompt text is read from the EXE; file starts:
`0x22d71` boarding, `0x22d98` Yes, `0x22d9c` No, `0x22da0` start,
`0x22dc2` missing card, `0x22dfb` autodestruction off; all NUL-terminated.
`CON_VUET.SPR` string is at `0xb44a`, load at `0xb593`.
Do not embed a copy of the original prose into the repository.

### Confirmed scene order

All ANMs are one complete forward pass, frame 0 through count−1, **mode −2,
RGB=(0,0,0)**. This blacks the palette before playback and directly installs
the animation palette on its first frame; it is not a crossfade. The shared
player forces palette entry 0 black, invokes the callback before copying,
waits retrace to copy each frame, then waits while PIT counter ≤interval and
resets it. **The last frame receives its wait too.** No frame catch-up skipping.

All cinematic sounds below use **voice 0, volume 48**. Stop the previous
voice before loading/starting the next, including repeats of the same sample.
There is no music-sequencer call or wait for audio completion.

| Stage | Exact resources and playback | Sound | Caller range |
| --- | --- | --- | --- |
| Cockpit startup | Existing NAV_INTE + NAV_INS2 background; remove prompt strip, NAV_INST font + NAV_INS2 animated instruments. Two typed scripts, ending on second terminator | ALARME4.DSS | `0xbb33–0xbcd2` |
| Hangar departure | NAV_HANG.VGA + NAV_SEQ2.SPR, three 12-step phases below; black cut, direct palette install | ALARME3.DSS | `0xbcd3–0xbf09` |
| NAV3 | NAV_SEQ3.ANM, **61 frames**, interval **16**, callback **`0xcae6`** with NAV_INST + NAV_INS2 | DEPART.DSS | `0xbf0a–0xbfea` |
| NAV4 | NAV_SEQ4.ANM, **49 frames**, interval **19**, no callback | BARGE2.DSS | `0xbfeb–0xc06a` |
| Explosion | VAIS_EXP.ANM, **60 frames**, interval **11**, no callback | EXPLO.DSS | `0xc06b–0xc0ea` |
| Cruise cockpit | Reload NAV_INTE.VGA + NAV_INST.SPR + NAV_INS2.SPR; instrument/text callback and 200 procedural stars; black cut, direct palette install | CABINE.DSS | `0xc0eb–0xc3cf` |
| NAV7 | NAV_SEQ7.ANM, **60 frames**, interval **21**, no callback | BARGE2.DSS | `0xc3d0–0xc44f` |
| NAV8 | NAV_SEQ8.ANM, **62 frames**, interval **20**, no callback | BARGE1.DSS | `0xc450–0xc4cf` |
| Base arrival | ARR_BASE.ANM, **62 frames**, interval **20**, no callback | BARGE1.DSS restarted | `0xc4d0–0xc54f` |
| Enter base | FSEQ2_1.VGA + INBASE.SPR, sprite motion below; clear screen + black DAC, direct palette install on first composition | VENT.DSS | `0xc550–0xc722` |

The six ANMs total **354 decoded frames**. NAV_INST has 69 images,
NAV_INS2 89, NAV_SEQ2 16, INBASE 7; all have zero anchors.
NAV resources' filename strings occupy file `0xc8fd–0xc9e3`; individual
ANM filename-immediate instructions are `0xbfdd, 0xc05d, 0xc0dd, 0xc442,
0xc4c2, 0xc542`. Their audio strings are in `0xc870–0xc8fc`.

### Hangar sprite phases

`0xbd7f–0xbf09`: render into the **persistent background**, without restoring
it between iterations. Put NAV_SEQ2 image `c+1` at **(91,145)**. Initially
c=0; at each counter **>26**, reset counter, increment phase step and c;
if c==4, set c=2. Thus 1,2,3,4,3,4,… . Three consecutive phases each end
after **12** such increments, resetting the step counter but **preserving c**.
During the middle phase also draw sprite `step+5` at **(16,21)** (images
5..16); first and third phases draw only the lower animated strip. Copies
via `0x1745` have no explicit retrace wait in this composition loop.

### Cockpit instruments and text callback

**`0xcae6–0xcd29`**, nine 14-byte records at file **`0xc9fe–0xca7b`**.
Each record is seven signed words: `state,x,y,width,halfHeight,count,firstSprite`.
The fixed geometry/animation fields are:

| Record | x,y | width,halfHeight | count | first sprite |
| --- | --- | --- | ---: | ---: |
| 0 | 40,11 | 73,16 | 5 | 1 |
| 1 | 216,10 | 71,18 | 18 | 8 |
| 2 | 88,110 | 22,14 | 10 | 26 |
| 3 | 120,110 | 22,14 | 8 | 36 |
| 4 | 152,114 | 4,6 | 13 | 44 |
| 5 | 161,114 | 4,6 | 13 | 57 |
| 6 | 170,114 | 4,6 | 13 | 44 |
| 7 | 188,109 | 3,15 | 13 | 70 |
| 8 | 203,110 | 30,14 | 6 | 83 |

On startup record 0 state=0x8000 and others=−1 (`0xbbe8–0xbc16`).
For each callback, in record order:

- −1: disabled.
- Negative other than −1: let h=state&0x7fff. Scale the first image to
  (x,y+halfHeight−h,width,2*h); record 1 uses image **7** instead.
  Increment h **per callback**, irrespective of the animation-tick flag.
  When h==halfHeight, set this state=0 and next record state=0x8000
  (except the last); otherwise store h|0x8000. The next record can begin
  its opening in the same callback. h=0 produces a zero-height draw.
- Nonnegative: draw firstSprite+state at (x,y). Record 1 first draws its
  static image 7 at (x,y), then its animated image at **(x+35,y+2)**.
  Add `[0xc7f2]` (0 or 1) to state; wrap to zero on equality with count.

Then draw NAV_INS2 image **6 at (121,0)**, switch sprite bank to NAV_INST,
and redraw the first `[0xc7fa]` characters of the active script, initially 1.
Origin **(125,5)**, space advances 4, CR resets x to125 and adds5 to y;
other glyphs use `0xb32e` mapping and width+1 (normal tiny glyphs 3×4).
Inspect the next byte; if NUL set `[0xc7fc]=-1`, otherwise increment
character count and set status=0. **Text advances per callback**, not per
PIT tick. The positive instrument-state advance alone uses the tick flag.

Scripts are NUL-terminated at these **EXE file** offsets (length excludes NUL):

| File start | Length | Selection |
| --- | ---: | --- |
| `0x22e2c` | 178 | Startup system checks |
| `0x22edf` | 197 | Evacuation message, after first terminator |
| `0x22fa5` | 197 | Cruise direction, repeats until accumulated time >3200 |
| `0x2306b` | 214 | Later cruise message, repeats until scene completion |

Trailing blanks belong to the scripts and affect completion. Read bounded
localized text from the user's EXE as PSEQ already does; do not store its prose.

Startup (`0xbc28–0xbccd`) restores background each iteration, invokes callback,
copies, clears tick flag, then if `[0x1428]>20`, sets flag=1 and resets counter.
It ends when the **second** script is terminated. No retrace wait in that loop.
NAV3 sets tick flag=1 and invokes the same callback once per decoded frame;
**records and script position are carried forward**, not reinitialized.

### Cruise stars and finish condition

`0xc1b9–0xc3cf`: choose cruise script 1, count=1, tick flag=1; call callback
once before directly installing the background palette. Create **200** stars
at file code `0xc1db–0xc220`, using `0x3dac` three times per record:

```text
X = rng() & 2047; negate X if remaining-star-count is odd (counts 200..1)
Z = (rng() & 4095) + 1000
Y = (rng() & 1023) - 500
```

Total-time word=0, tick flag=1, PIT counter=0. Each iteration:

1. If script 1 ended, reset its character count; if total>3200 switch to
   script 2. If script 2 ended, reset its count as well.
2. Callback **mutates the persistent cockpit background**; copy it to the
   working screen, then project each star (signed division truncates to zero):
   `sx=160+trunc(256*X/Z)`, `sy=71+trunc(256*Y/Z)`.
3. Only within x=0..319,y=39..103: plot if underlying indexed pixel is 0.
   Color = table[min(11,Z/500)], bytes at file `0xca7c–0xca87`:
   **255,6,13,17,25,31,43,82,132,159,188,248**.
4. In-bounds stars decrement Z by **10 per composition iteration** whether
   or not a nonzero pixel prevented drawing; if resulting Z<1000 add4000.
   Out-of-bounds stars add4000 to their **un-decremented** Z.
5. Copy screen without explicit retrace wait; tick flag=0. If PIT counter>20,
   add the **whole counter** to the word total, reset counter, flag=1.
6. End only when **total>4800 AND current script status==−1**.

Star movement and typewriter speed are CPU-loop-dependent; the total duration
condition is a separate PIT-based measure. Do not replace both by one fixed
animation frequency or claim exact seconds from 4800. RNG seed/state and
DOS-speed visual comparison remain uncalibrated, as for PSEQ's procedural stars.

### Base entry sprite motion

`0xc5e7–0xc701`: restore FSEQ2_1 background each iteration. Draw INBASE
image p at **x=154**, initial y=150, p=1, direction=+1, delta-index=0.
Install palette when y==150. Wait while PIT counter **≤10**, reset it,
then add the next y delta from file `0xcac8–0xcadf`:

```text
-1, -3, -2, -1, 0, 0, -2, -2, -1, -1, -1, -1
```

Advance p by direction; when p reaches7 set direction=−1, when p reaches1
set direction=+1. Wrap delta-index at12. Continue only while updated y>100;
no forced draw at y=100. Stop VENT, fade brightness100,98,…,0. No final
extra hold or audio-completion wait.

### Keyboard / Escape is inherited, not a new skip policy

Neither startup instruments, hangar animation, cruise loop, base-entry loop,
nor fade loops poll ordinary keys. The prelude uses the existing gameplay
input mapper and action binding, not PSEQ's Escape handler.

The six ANMs call shared `0x1470f`. Its wait loop `0x14904–0x14926` polls
`0x2a27` **only if `[0x14958]!=0`**; only AL=27 exits the current ANM early.
Normal completion writes zero (`0x14943`); early Escape leaves the nonzero
value. **No instruction in the shuttle routine initializes this word and no
caller branch tests the ANM completion status.** Thus:

- Entry flag=0: no Escape polling in any of the six movies.
- Entry flag nonzero: Escape can terminate the current movie; the next stage
  still starts. First normally completed movie clears the flag and disables
  later movie skipping. Other keys are consumed without skipping when polled.

The inherited state can depend on presentation history: startup initializes
zero (`0x63c9`), while PSEQ explicitly sets −1 before its ANMs and its early
exit cleanup (`0x20c79–0x20cd6`) does not clear it. Do not silently copy FSEQ's
explicit Escape-disabled entry or PSEQ's skip-the-whole-presentation behavior.
Dynamic key-buffer/device behavior needs a DOS comparison; the above conditional
control flow is already sufficient to specify the player interface.

### Player/world state after shuttle

The successful journey does **not** load the next level until every scene and
the final fade have finished:

- `0xc723–0xc732`: release cinematic audio buffer, reload gameplay sound bank
  (`0x12de4`), save/release **old level** via `0x7b83`.
- `0xc737–0xc76c`: local X=**320**, Y=**300**, angle=**0**, sector=**1**,
  zone=**22** (level23), floor=**8**.
- `0xc76d–0xc77e`: `[0x60b7]=0` (completed-security count, disabling countdown),
  `[0x605b]=0` (message state); load level with `0xc77f → 0x791e`;
  set `[0x520a]=-1`.
- Both success and refusal clear transient words `[0x51d6]` and `[0x51e0]`,
  and the active/time words of six HUD damage-marker records
  `[0x5493 + 10*i]` and `+2`, i=0..5 (`0xc7bc–0xc80e`). These are not
  inventory slots: marker activation/expiry is visible at `0xe356–0xe365`
  and `0x13f5b–0x13faa`. Do not reinterpret them as weapon/ammo resets.
- Free four visual buffers; restore renderer state, set `[0x5425]=-1`,
  `[0x1428]=1`, `[0x142a]=159`, clear configured action's key-state bit with
  `0x2ad1`, clear `[0x16fb1]`, return (`0xc80f–0xc86b`).

On refusal/failure (`0xc78f–0xc7bb`), reposition to local **(300,900)**,
angle0 in the existing level/sector; no destination load. The trigger connection
was already zeroed before the prompts. The security bytes `[0x60a7..]` are
not cleared by the successful branch: do not replace its explicit count reset
with a blanket inventory/security reset.

Neither success nor refusal writes player health/oxygen/inventory slots.
The main world simulation does not run inside this blocking routine; hardware
PIT/audio continue. No destruction-countdown update is called inside it.

### Audio headers (DATA-confirmed)

All listed standalone samples are 5000 Hz. Original loop flags/bounds must be
preserved; the explicit stop controls loops. There is no arbitrary sound timer.

| Sample | Samples | Loop flag | Loop start/end |
| --- | ---: | ---: | --- |
| ALARME4 | 29164 | 65535 | 4 / 29163 |
| ALARME3 | 36278 | 65535 | 0 / 36277 |
| DEPART | 18278 | 65535 | 0 / 18277 |
| BARGE2 | 17954 | 0 | 0 / 0 |
| EXPLO | 30509 | 0 | 0 / 0 |
| CABINE | 20792 | 65535 | 0 / 20791 |
| BARGE1 | 17986 | 0 | 0 / 0 |
| VENT | 25395 | 65535 | 0 / 25394 |

## 3. Port integration (2026-11-16)

**Implemented / Port-tested:** conventional elevator presentation is now in
`original_elevator.cpp` (deterministic retrace steps) and
`original_elevator_player.cpp` (blocking SDL adapter). Original EXE evidence
above is inherited from the completed investigation; no new disassembly.

- `ItemSession::Travel` has optional elevator metadata: lift ID, source floor
  and destination floor, captured from the selected descriptor after the existing
  permission check. Source matches both zone and sector. Direction never comes
  from zone numbers. Selecting the current pair closes the modal without a
  transfer or coordinate write, resolving the historical discrepancy below.
- `main.cpp` preflights destination resources using the existing loader, then
  calls the presenter before committing camera/world/resources. Error preserves
  the old world; window close cancels the pending transfer and exits. Same-zone
  transfers keep the existing no-reload path. No DOS LAB writes were added.
- Debug and restart requests have no elevator metadata and bypass presentation.
  The blocking presenter owns no simulation state; gameplay, enemies, doors,
  survival and HUD do not update while it runs. Return clears input and resets
  elapsed time/door timing before resuming the existing loop.
- Four floor-delta variants use the documented word motion, integer geometry,
  16.16 nearest-source scaling, transparent zero and full background restoration.
  Entry/exit each have 51 brightness steps; hold is 30 retraces. The last drawn
  q is retained. `GameAudio::elevatorMotor` uses native looping BANK[11] after
  reveal, stopping after the hold; modal motor behavior is unchanged.
- Escape and other ordinary inputs are consumed without skipping. Window close
  remains available. Rendering scale/logical size/draw color are restored on
  success, error and close. Missing graphics reject the transfer; unavailable
  audio retains the existing silent-play fallback.

**Timing adaptation:** one simulated retrace per nominal 1/70 second, with no
skipped procedural draws. Host rendering/scheduling can lengthen intervals;
this is not calibrated running-DOS timing. Losing focus freezes presentation
and stops the motor; regaining focus resumes and restarts its native loop.
That focus policy is a host adaptation. The preview uses the identical player.

**Validation:** Debug configure/build succeeded; full CTest **31/31 passed**.
`original_elevator_tests` checks all four golden trajectories (draw counts
58/75/57/75, final q 260/458/2/2), geometry/clipping/transparency/background
restoration, 51-step fades, 30-retrace hold and motor boundaries. Interaction
checks cover all extant source/destination pairs, current-destination dismissal,
permissions, source/destination floor metadata, arrival coordinates and retained
health/oxygen/inventory across request and item-level load. Existing map/security
persistence tests remain passing. Resource-backed tests decoded A_LEVEL and
verified BANK[11] loops, SDL Escape/close handling, renderer restoration and
missing-resource Error. The original LAB interaction probe passed; the Debug
catalog probe checked all **55 arrivals** have no elevator metadata. Restart's
ordinary Travel construction is likewise tested without metadata. These are
not a manual in-game or DOS visual comparison.

To view all four variants consecutively (up one, up several, down one, down
several), with original audio and no campaign navigation, from the repo root:

```sh
./build/debug/elevator_preview /Users/kurlos/Desktop/In_Extremis/DATA
```

The window title identifies each variant; Escape cannot skip, window close
ends the preview. `--check` validates/runs the four graphic sequences without
opening a window. The target is included in the normal Debug build.

At the time of the elevator integration, the level22 shuttle, destruction
countdown and AX=2 remained unimplemented. See the shuttle update below and
the self-destruction note for subsequent work.
PSEQ, FSEQ, combat and menus were not changed by elevator integration.
Manual visual/audio review of the four variants and DOS timing comparison
remain pending. The surrounding CH_LEVEL panel's prior modal presentation is
unchanged; this integration implements the documented A_LEVEL travel block.

### Standalone shuttle audiovisual player (2026-11-16)

**Implemented / Port-tested:** `original_shuttle.cpp` owns the ten-stage
post-confirmation presentation; `original_shuttle_player.cpp` is its blocking
SDL adapter. `shuttle_preview` is created only for Debug configurations. No
campaign caller, boarding/start prompts, eligibility checks or player/world
mutation were added. PSEQ, FSEQ and conventional elevator code are unchanged.
The EXE evidence above is inherited, not independently re-disassembled here.

The existing ANM/VGA/SPR readers and DSS mixer are reused. Four bounded script
reads come from the user's EXE at the documented offsets and lengths; no prose
or original assets are included. The player validates frame/sprite counts and
bounds. Instrument opening, persistent callback state through NAV3, all 36
hangar steps, 200 projected/moving stars and the oscillating base sprite are
procedural. Each scene stops/restarts voice 0 at volume 48, preserving native
DSS loop metadata; VENT stops before the 51-step final fade. ANMs include the
last-frame wait, black palette cuts and retrace-gated copies.

The API accepts the inherited ANM Escape flag. It defaults to zero in the
standalone preview; `--inherited-escape` enables the documented alternate entry
state. Escape only affects the current eligible ANM, and normal movie completion
clears eligibility. Other scenes do not skip. Window close exits; resource
errors return Error; renderer settings are restored. Focus loss pauses both
presentation and audio, a host policy.

**Adapted:** CPU composition is nominally 70 iterations/second, independently
of PIT accumulation (1193182/7457 nominal ticks/second) and 70 Hz retrace events.
No animation frames are skipped to catch up. Host scheduling can lengthen the
sequence. Stars use a deterministic host RNG seed, not a recovered DOS seed.
These policies are not claims of observed DOS speed or exact elapsed duration.

**Validation:** Debug configure/build and full CTest **33/33 passed**. Synthetic
checks cover scene/resource/sound order, strict PIT thresholds, last-frame wait,
inherited Escape behavior, all hangar phases and persistent composition, base
trajectory and fade. An explicit original-resource test completes all scenes,
checks sample lengths/rates/native loops and exercises SDL dummy-driver quit,
renderer restoration and missing-resource Error. The preview `--check` also
completes the resource-backed journey. Manual audiovisual review and running-DOS
cadence/RNG comparison remain pending; no new original-game facts were found.

From the repository root:

```sh
./build/debug/shuttle_preview /Users/kurlos/Desktop/In_Extremis/DATA
./build/debug/shuttle_preview /Users/kurlos/Desktop/In_Extremis/DATA --check
./build/debug/original_shuttle_tests /Users/kurlos/Desktop/In_Extremis/DATA
```

### Historical pre-integration review (2026-11-15; superseded for elevators)

The following records the earlier code state and original integration contract.
The conventional-elevator gaps and requirements 1–3 are resolved above;
shuttle campaign requirement 4 is now integrated below; presentation requirement 5
is implemented independently above.

**Historical code inspection, not current implementation status:**

- [ItemSession::confirm](../../../src/item_session.cpp), Elevator case,
  validates the existing route/card and constructs `Travel{zone,arrival}`,
  then resets the modal/clears input. `Travel` in
  [item_session.h](../../../include/item_session.h) currently carries **neither
  floor nor lift/cause**. The original route already has the floor byte.
- [main.cpp](../../../src/main.cpp), `items.takeTravel()` block: restart and
  Debug requests share the transport container. Same-zone transfers replace
  the camera without loading the world, invalidate input, reset door timing
  and `previousTicks`. Other transfers stage all destination resources,
  construct the camera from exact route arrival, then swap active state.
  `SurvivalSession` and collected inventory survive ordinary travel;
  `items.load` preserves map/security arrays. Restart alone resets the session.
- A same-zone/current-sector selection currently still requests `Travel` and
  can reset camera position; DOS's current-pair selection skips the movie and
  coordinate write. This is a **review finding, not fixed here**. Gate a movie
  on the original changed-pair predicate without silently changing transport.
- No NAV/A_LEVEL presentation or shuttle door trigger exists in the current
  engine. Security-panel acceptance stores private `securities_[16]` in
  `ItemSession`; there is no corresponding exposed shuttle/countdown state.
  The complete shuttle trigger is therefore more than attaching a movie to
  the existing elevator callback.

Original integration requirements (status updated above):

1. Add an explicit transport presentation cause and captured source/destination
   floor metadata **at validated elevator selection**. Keep its existing zone,
   sector, local arrival and permissions unchanged. Never infer direction from
   level numbers. Debug teleport/new game must not accidentally play a movie.
2. Implement an independent, blocking/stateful elevator presenter with the
   exact q arithmetic, clipped scaler, 51-step fades, retrace pacing, 30-retrace
   hold, and BANK[11] loop. Do not call PSEQ/FSEQ as a generic player.
3. The user-visible destination must remain absent until the presenter ends.
   Preserve the current resource staging/error recovery: preflight resources
   without replacing the active world, present, then commit the already staged
   transfer. Preflight is a host robustness detail; DOS's actual load is later.
   The presenter must have **no writable ownership of inventory, destinations,
   health, map access or saved world state**. Reset elapsed time/input at the
   existing return boundary, and suspend gameplay/doors/enemies/HUD updates.
4. Implement shuttle as a **separate campaign-event owner**: connection45,
   level/sector checks, existing card data, exact security/countdown predicate,
   prompts, refusal reposition and successful one-way arrival. Expose the
   required campaign state deliberately; do not synthesize eligibility from
   the current level alone. Emit a success/refusal result and apply the
   documented limited state changes outside the visual player. Do not turn
   shuttle arrival into new-game reset or final-victory/FSEQ dispatch.
5. Implement its instrument callback, original text reads, stars, ANM stage
   ordering and inherited ANM-input flag. Separate the variable composition
   iteration cadence from PIT counters and retrace operations. A chosen host
   cadence/RNG is **Adapted** until measured in DOS.

**Existing reusable code:** `anm_format::load/loadVga`, `spr_format::load`,
indexed 320×200 canvases, DSS `audio::loadSample` / `audio::Mixer`, palette
conversion, and SDL texture/window lifecycle. Both current cinematic presenters
are **scene-specific**, not generic public ANM renderers:
[original_intro.cpp](../../../src/original_intro.cpp) has internal scaled
sprites, text, stars and palette operations; [original_ending.cpp](../../../src/original_ending.cpp)
has strict tick/retrace `step` semantics and no-skip policy. Their presenter
and audio classes are privately coupled to PSEQ/FSEQ scenes. Reuse decoder
APIs and established arithmetic, or narrowly share helpers in a later approved
implementation; do not alter either existing sequence's behavior to fit NAV.
The shuttle font/script spacing and callback rules differ from PSEQ.

## 4. Validation and remaining verification

A temporary C++17 probe outside the repository linked **unchanged**
`src/anm_format.cpp` and `src/spr_format.cpp`, and successfully decoded all
**15** listed graphic files: six ANMs/354 frames, four VGA stills and five SPR
banks/182 images. Every SPR anchor was zero. Another temporary probe linked
unchanged `game_lab.cpp` and checked level22/connection45 and its two walls.
Original route descriptors were independently read from the bounded EXE table;
all 55 extant entries match the existing header. The existing read-only
`tools/extract_audio.py` parser validated the eight standalone sounds and
BANK[11]. No engine test changes or full gameplay test run were needed for
this documentation-only task. These checks are **Port-tested decoding**, not
DOS-observed playback.

Reproduction of instruction evidence uses the existing
`tools/inspect_original_code.py EXE START END` (**default 32-bit decoding**)
for the bounded code ranges above. In particular do not decode this protected
code as 16-bit simply because the executable is a DOS program.

Remaining verification is narrowly scoped:

- Running-DOS elapsed time, CRT cadence and visual comparison, especially
  CPU-dependent instrument opening, text and cruise-star speed; RNG state.
- Runtime keyboard/pending-input behavior with completed versus Escape-aborted
  PSEQ before shuttle entry; preserve the recovered flag-dependent rule meanwhile.
- End-to-end campaign reachability/card/security progression and transport
  persistence. The table/predicate is recovered, not a claim that every edge
  has been played in DOS or in the port.
- Host policies for quit, focus pause and missing resources are implementation
  choices, not invented original controls. No unresolved resource identity,
  elevator motion formula, ANM order or arrival coordinate needs re-analysis.

### Graphic-file identities

These hashes identify the decoded local assets; none are included here.

```text
A_LEVEL.VGA  067fd8d74bd2127da25bf9d88bc715182b173d1f6690082409ba0b06f20c6311
A_LEVEL.SPR  3b7f37136c5b1b7bc9e292d62ede31813feed1e7637b5bf64c9eb56825a7de47
NAV_HANG.VGA e863f5532e31abafed412543cc20b8decab6c6070ce15755008fa091997ffc62
NAV_INTE.VGA 5a474a8eb174793034408659a9e06a6f003c3438047589518109d810ea8aaaff
NAV_INST.SPR 064e8a26a2a7d6458577a5658991534b897cef365771bcfb323f64cc8965b0e0
NAV_INS2.SPR 7fa54e307217852f2e478fec09e5871248bb47258ff2988d6f0481cec111f402
NAV_SEQ2.SPR 08f482e200a669d527b01ffde3c26dd6e941f90c7a76f53866da3dafa628e1d5
NAV_SEQ3.ANM c59cc8488a448adc99167a0c7508776b7fa76bf56d0ad31022cb96ea97315002
NAV_SEQ4.ANM ba57f3257563dad29b5e4e7fac6855100a4b19e7878570cb765f0cfba6cdefc1
VAIS_EXP.ANM a5aa6e1a9330116c54cd96946033427b16f093069706e3f99bdb432ac7c73291
NAV_SEQ7.ANM ecdfd33de1db8a3a786f95f243114704a8eee9af640ec72238d529abc55d991c
NAV_SEQ8.ANM 760263c0c6c9f70f379bbc40a8a491b97fef82069676eed24c5bad173166f452
ARR_BASE.ANM ba01c0d0230b5368f22e2f6a571da3b6b17441b03843df2a1354536a5ea57eb8
FSEQ2_1.VGA  47aa04dc56db917ddf6e1405a069340a3fa52189b6be360a25d454d87d36b645
INBASE.SPR   3867e29db63dcaaabd61a78b779aedff7b5e18b47ada04c1e71b2469764d458d
```

Sound-file SHA-256 identities for the header checks above:

```text
ALARME3.DSS e5312739a0538b76668bd410fbd287440c978f05bc99bb663c985bf5277c0de3
ALARME4.DSS eb8d272c2d8a0a5e951a85d835f46c5c2b8f9342c232f5ee9233181c1452b6db
DEPART.DSS  dc159103ad17ece2b8a6f6f7ca5ba554b035bf7513f4cffd73be8597d85e0561
BARGE1.DSS  556e543add6b20bc82357e9ed408fb416f491efb0cac68d791cc277a33faf8d5
BARGE2.DSS  b1a5431a2b37d1f9a5e47131f0bba27453ebb656d995b830f114f878491c3de3
EXPLO.DSS   4a93eda1cebf817327fdecfca19f045420940f38447c698dde51e75c9d0c46b7
CABINE.DSS  46faa20199245749dc32fee2be2a2c3708cf2cb16b0a319ea34d6fab9d972f04
VENT.DSS    80ca7a6ed0e79a95a398af48a8b81c8e375ca759bda95abf0ebf7126f8f42529
BANK.DSS    f83ae14e969b1e7658d0fdf4ffc63565b3c5538cf84a6fee62c52db1541253b8
```


### Campaign shuttle integration (2026-11-16)

**Port implementation; inherited EXE evidence, no new disassembly.**
`shuttle_session.h/.cpp` and `main.cpp` now connect level22/sector38,
connection45, nonzero high byte and signed low byte >60 to the two original
questions. Phases 1..60 darken the gameplay frame; entry zeroes the connection
before the blocking screens. The original VGA/SPR decoders and six bounded
NUL-terminated EXE text reads at the offsets above supply the prelude; no
copyrighted prose is embedded. E/Space confirm, W/S or arrows choose, matching
the existing item interfaces. Escape does not reject or skip a question.
The host uses nominal 70-Hz fades and 160-unit blinking; focus suspends the
presentation. These remain host timing adaptations, not DOS measurements.

Card eligibility reads the existing three inventory slots for exactly 0x2005
(the existing pickup API encodes that card as world item 0x5005). Card failure
has priority. `ItemSession::completedSecurities()` records distinct accepted
security readers alongside their existing flags. Departure requires exactly
16. Neither refusal nor an error consumes a card or changes vitals, map
permissions, security flags/count, or the remaining countdown.

Both NO paths and acknowledged errors restore sector38/local (300,900), angle0;
connection45 remains closed/unlocked and can be approached again. Common cleanup
clears the port's player impact reaction and movement/input context. The six DOS
HUD damage-marker records have no separate implemented counterpart to clear.
On success the **unchanged** `showOriginalShuttle` runs to completion before any
destination load. Only Continue queues the existing level-transfer path to
level23/sector1/local (320,300), angle0. Floor8 is represented by the existing
level23/sector1 route (lift19); the port derives floor from routes, not a second
global floor variable. No conventional elevator movie is requested.

After the movie `finishEvacuation()` zeroes the completed count and disables the
existing countdown, retaining all security flags and the unused time. Messages
are cleared; health, oxygen, inventory and map permissions retain their values.
World simulation does not run during either question, acknowledgement or movie;
clock/input boundaries discard elapsed modal time and queued gameplay keys.
The existing expiry latch is deliberately **not cleared**. The last-update
expiry/boarding anomaly remains unverified and has no new rescue, deadline or
special death policy in this integration; existing death dispatch is unchanged.
This task does not certify the behavior of that boundary.

Debug-only `--shuttle-debug` starts at level22/sector38/local (300,900), facing
the actual door (angle180 for this diagnostic spawn only), without the required
card or completed securities. Ctrl+Shift+L uses the existing inventory pickup/replacement
and security diagnostic APIs to prepare the successful case; it neither opens
the door nor directly invokes a cinematic. After a refusal the original angle0
is restored, so turn left toward the door before approaching again. No Release
shortcut is added.

Manual check from the repository root:

```sh
./build/debug/InExtremisPort /Users/kurlos/Desktop/In_Extremis/GAME.LAB --shuttle-debug
```

1. Finish/skip startup as usual. Press W briefly toward the door and wait for
   its normal opening cycle. The first question must appear without E.
2. Choose NO with Down/S, then press E. Verify return to sector38, (300,900),
   angle0; turn left approximately 90 degrees toward the same door and approach.
3. Accept both questions with separate E presses. Without Ctrl+Shift+L the missing-card
   text must appear. Acknowledge with E and verify the same reusable return.
4. Press Ctrl+Shift+L in gameplay. Turn left toward the door and approach again. Accept
   both questions. Wait for the entire validated shuttle presentation.
5. The log reports `Shuttle complete: destination level 23, sector 1, local
   (320,300), angle 0, floor 8`, followed by the existing loader's level23 arrival.
   TAB's level indicator also shows 23. Inspect inventory with I: the card
   remains. The destruction warning stops; gameplay resumes at the base.

Automated checks: `shuttle_session_tests` covers both NO paths, card precedence,
wrong card, security rejection, acknowledgement/retry, trigger phase/sector/zone
and duplicate entry, deferred countdown disable, preserved time/flags/cards,
arrival coordinates and the existing floor8 route. Its optional original-data
probe checks normal DoorControl approach/retry on LAB connection45 and runs
four SDL dummy-driver prompt routes with the actual graphics/font/EXE text:

```sh
./build/debug/shuttle_session_tests /Users/kurlos/Desktop/In_Extremis/DATA
```

Manual in-game review of the new prelude/arrival remains pending; the previously
validated shuttle animation and destruction/death presentations were not edited.


**Current validation:** Debug configure/build succeeded; full CTest **34/34
passed**. The optional original-resource/LAB/SDL probe above passed all four
routes. Movie Error/Quit do not produce arrival or disable the countdown;
Continue completes once. Escape/inventory/heal keys injected into the prelude
are consumed without changing its route. `git diff --check` passed. These are
Port-tested results, not a manual DOS comparison or a claim about the unresolved
last-update expiry edge. No commit or push was made.

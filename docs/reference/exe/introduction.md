# Original introduction — bounded reconstruction (2026-11-15)

**EXE-confirmed**, unless explicitly labeled otherwise. Independently read from
local `EXTREMIS.EXE`, SHA-256
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
LLVM disassembly used **32-bit decoding** for the instruction ranges below;
the real-mode characterization in the general handbook is insufficient for
these operand/address sizes. Offsets are absolute file offsets; `[address]`
means runtime storage. No running-DOS observation or timing calibration.
The original investigation made no engine changes. The implementation status
below supersedes that historical boundary; no final-cinematic investigation
or original assets are included.

## Entry, references and exit

`0x68a5` calls the blocking narrative routine **`0x1f4ba–0x20cd6`**.
Its text callback is **`0x20cd7–0x20d72`**. It stops game audio on entry
(`0x13090`), blacks the palette and clears both framebuffers. Four allocations
at `0x1f4d8–0x1f50f` provide two 64800-byte buffers, a 120000-byte buffer
and a 62000-byte sound buffer. No world updates occur in this routine.

The supplied name offsets point inside `DATA\…` strings. For this particular
static-data mapping, the immediate pointer is **full path file offset −0x200**;
the path starts five bytes before the bare filename. For example, `0x1f58e`
loads `0x20cdf`, referencing the path at file `0x20edf`, whose PSEQ1 name is
at `0x20ee4`. This is not a universal pointer conversion.

| Resource | Instruction loading its path pointer |
| --- | --- |
| PSEQ1.ANM | `0x1f58e` |
| PSEQ2_1.VGA | `0x1f5f3`, `0x1fa1b` |
| PSEQ2_2.VGA | `0x1fa64` |
| PSEQ2_3.SPR | `0x1f63c`, `0x1faad` |
| PSEQ3.ANM | `0x1f866` (load), `0x1f8eb` (memory-player argument) |
| PSEQ4.ANM | `0x1f9b6` |
| PSEQ6_1.ANM / PSEQ6_2.SPR | `0x1fd3b` / `0x1fd57` |
| PSEQ7_1.VGA / PSEQ7_2.SPR | `0x1fe85` / `0x1fece` |
| PSEQ8.ANM | `0x20157` |
| PSEQ10_1.VGA / PSEQ10_2.SPR / PSEQ10_3.SPR | `0x201d7` / `0x20220` / `0x20242` |
| PSEQ11_1.VGA / PSEQ11_2.SPR | `0x204f3` / `0x2053c` |
| PSEQ12_1.VGA / PSEQ12_2.SPR | `0x2073d` / `0x20786` |
| PSEQ14_1.VGA / PSEQ14_2.VGA / PSEQ14_3.SPR | `0x2095e` / `0x209a7` / `0x209f0` |

Normal completion and Escape converge at `0x20c79`: stop voices 1 and 0,
free buffers, black palette, clear screens, return. Normal completion first
fades brightness 100,96,…,0 (`0x20c5a–0x20c6c`) and waits for voice 0's
active word `[0x3d76]` to become zero (`0x20c6e–0x20c77`); that wait does
not poll Escape. It is **not** a fixed post-movie pause.

After return, `0x68aa–0x6a06` initializes gameplay resources; an optional
configuration branch at `0x6a0b–0x6a3b` calls `0x1e80a` (not reconstructed
here). Normal flow reaches menu **`0x6a4e → 0x1d92d`**, not automatic play.
Menu/new-party behavior is covered in [Death sequences](player-death.md).

## Presentation before PSEQ

The earlier startup path is separate, but precedes the narrative:

- `0x63d2–0x63fd`: load CONCEPT.PAL and CONCEPT.SPR. `0x642f–0x650b`
  draws scaled sprite 1 with counter `t=[0x1428]`, until `t>=160`:
  x=−106+trunc(195t/160), y=−64+trunc(118t/160),
  width=532+trunc(−399t/160), height=328+trunc(−246t/160).
  Brightness is min(100,trunc(100t/160)).
- `0x6511–0x65ba`: CONCEPT.ANM **twice**, interval argument 6, mode −3,
  ANM Escape disabled. After each movie, draw CONCEPT sprite 1 at (89,54)
  and call `0x170a` with 70 (70 retrace waits, not 70 PIT ticks).
- `0x65c0–0x66ae`: reverse the same scale/brightness formulas using the
  decreasing counter `[0x142a]`, initially 160; finish when it is <=0.
- `0x66b9`: PRESENTE.VGA through `0x676a`; `0x66c3–0x670f`: TITRE.ANM
  **twice**, interval 8, mode 0, ANM Escape disabled.
- `0x6711–0x6742`: WHO1.VGA, WHO2.VGA, WHO3.VGA, WHO4.VGA in order.
  Still helper `0x676a–0x67fd` decodes, displays, fades 0,4,…,100,
  then waits for any nonzero key result or `[0x1428]>800`; with
  `[0x14958]==0` it fades 100,96,…,0 afterward.
- Configuration/audio setup `0x67fe–0x68a0` precedes the PSEQ call.

These steps and reused scenes below explain why numeric gaps in PSEQ names
must not be filled with invented files or transitions.

## Narrative order and composition

Sprite numbers in this note are **DOS one-based**; subtract one for
`spr_format::Image` vector indices. `t` is the accumulated word counter
`[0x20b83]`; `p` is the secondary phase `[0x20b85]`. Integer division uses
the signed/unsigned instructions described below, not floating interpolation.

| Order | Evidence / resources | Behavior and completion |
| --- | --- | --- |
| 1 | `0x1f521–0x1f5b2`, PSEQ1 | ANM interval 15, mode 0 (fade in); BARGE2 voice 0, volume 32 |
| 2 | `0x1f5bd–0x1f820`, PSEQ2_1 + 2_3 | Cockpit composition below; finish when t>320; CABINE voice 0, volume 48 |
| 3 | `0x1f82a–0x1f944`, PSEQ3 | Memory ANM player, interval 8, mode −3, text callback; repeat whole movie until text terminator; restart CABINE voice 0 once on entry; restart RADAR voice 1, volume 48 on each pass |
| 4 | `0x1f949–0x1f9db`, PSEQ4 | ANM interval 10, mode −2; LABO voice 0, volume 48 |
| 5 | `0x1f9e0–0x1fcb1`, reused PSEQ2_1 + 2_2 + 2_3 | Same cockpit sprites; alternate backgrounds using t%40<20; finish t>320; ALARME1 voice 0, volume 48 |
| 6 | `0x1fcbb–0x1fe45`, PSEQ6_1 then 6_2 | ANM interval 15, mode −3 and text callback; then clear canvas each iteration, draw sprite 1+(p&3) at (169,113), callback, copy; wait counter>=15, reset and advance p; finish on text terminator; ALARME4 voice 0, volume 48 |
| 7 | `0x1fe4f–0x200e5`, PSEQ7_1 + 7_2 | Sprite at (142,67), number 1 before t=160, then 2+floor((t−160)/20); procedural stars below; finish t>=280; ALARME1 voice 0, volume 16 |
| 8 | `0x200ea–0x2017c`, PSEQ8 | ANM interval 20, mode −2; ALARME4 voice 0, volume 48 |
| 9 | `0x20186–0x20472`, PSEQ10_1 + 10_2 + 10_3 | 13 motion records below; ALARME4 voice 0, volume 48; TOUCHE voice 1, volume 48 on specified records |
| 10 | `0x2047c–0x206ec`, PSEQ11_1 + 11_2 | Sprite 1: x=72+floor(58t/320), y=30+floor(35t/320), w=82+trunc(−53t/320), h=103+trunc(−66t/320); finish t>=320; ALARME1 voice 0 volume 16 + OXYGENE voice 1 volume 48 |
| 11 | `0x206f1–0x2090d`, PSEQ12_1 + 12_2 | Nine motion records; **buffer mismatch described below**; restart preceding two sounds, same volumes |
| 12 | `0x20912–0x20c77`, PSEQ14_1 + 14_2 + 14_3 | Moving/scaling sprite, flashing background and explosion below; finish t>=480, fade, wait voice 0 |

**Cockpit**, `0x1f65e–0x1f817` and `0x1facf–0x1fca8`: restore background
before drawing each iteration. Sprite at (129,70): let q=p>>3;
number=17+q when q<15, otherwise 17+(29−q). Phase adds elapsed PIT ticks
and subtracts 232 **once only if >232**. Five indicators at x=192,
y={73,80,87,94,101}, use sprite 33 if t & {512,256,128,64,32}, else 32
(table file `0x20d8b–0x20d9e`). At (136,74), number=1+floor(t/20).
At (91,68), q=floor(t/40); use 34+q for q<4, 34+((~q)&3) for 4<=q<8,
otherwise 34. Copy complete canvas and set palette on t==0.

**Stars**, `0x1fef0–0x2007a`: 400 records, RNG `0x3dac`; X magnitude
random&2047 with sign from loop-counter bit 0; Z=1000+(random&2047);
Y magnitude random&1023 with sign from bit 1. Project using signed integer
x=160+trunc(256X/Z), y=71+trunc(256Y/Z). Draw only inside 320×200 and
only over index zero, after the sprite; color=clamp(floor((Z−1000)/136),1,15).
Each iteration adds 10 to Z; if Z>=3047 subtract 2047. **Star movement is
per render-loop iteration, not multiplied by PIT elapsed ticks.** RNG algorithm
and DOS loop cadence remain unverified here.

**PSEQ10 records**, file `0x20d9f–0x20e70`: count word 13 followed by
eight signed words per record `(duration,x0,y0,x1,y1,movingSprite,panel,blink)`:

```text
40,297,112,189,83,1,-1,-1
40,187,83,187,83,2,1,-1
40,189,83,166,54,1,-1,-1
40,164,54,164,54,2,2,-1
40,166,54,166,87,1,-1,-1
40,164,87,164,87,2,3,-1
40,166,87,189,150,1,-1,-1
40,187,150,187,150,2,4,-1
40,189,150,189,150,1,-1,-1
40,187,150,187,150,2,4,-1
40,189,150,166,150,1,-1,-1
40,164,150,164,150,2,5,1
160,166,150,400,199,1,-1,1
```

Restore VGA; if panel!=-1 draw that PSEQ10_2 sprite at (151,45), and on
t==0 start TOUCHE. Draw PSEQ10_2 sprite 6 at (56,95) if blink!=-1 and
(t&32)!=0, otherwise sprite 7. Draw PSEQ10_3 movingSprite last, at
`start+trunc((end−start)*t/duration)` per axis; clipping 0..319,0..199.
After drawing add elapsed ticks, reset counter, repeat while t<duration.
The exact endpoint need not be rendered. Escape is checked **between records**.

**PSEQ12**, `0x207a8–0x208f0`: table file `0x20e71–0x20ede`, count 9,
six words per record `(duration,x0,y0,x1,y1,sprite)`. For i=0..8:
`(20,6i,5i,6(i+1),5(i+1),i+1)`. Same interpolation and record-boundary
Escape check as PSEQ10. However, `0x20786–0x20796` loads the SPR into
`[0x20b77]`, while **`0x207fa–0x207ff` assigns `[0x721]` to `[0x172c]`**.
The latter was the VGA input buffer; `0x146c3` does not redirect this global.
Thus the recovered code does not select the newly loaded SPR. Do not silently
fix this as if it were observed DOS behavior; inspect the running buffer and
screen before choosing fidelity versus an explicit correction.

**PSEQ14**, `0x20a12–0x20c54`: p wraps by subtracting 320 once at >=320.
q=floor(p/40); sprite=1+q if q<=4, else 1+((~q)&3).
x=118+floor(65t/480), y=65+trunc(−53t/480),
w=18+trunc(75t/480), h=18+trunc(73t/480).
Before t=320 use VGA1. At first t>=320 load/start EXPLO voice 0 volume 64,
stop voice 1. For 320<=t<400 select VGA1/VGA2 by floor(t/10)&1;
at t>=400 use VGA2. The palette remains VGA1's, including the final fade.

## Text, players, transitions and input

`0x20cd7–0x20d72` draws a prefix on the decoded canvas at (10,10) with
CON_VUET.SPR (`[0x1d711]`, loaded at `0x636d–0x637d`). CR resets x=10
and adds 11 to y; LF is ignored. Glyph helper `0x1f1f1–0x1f2db` handles
digits, upper/lowercase, punctuation and DOS accented bytes; advance is glyph
width, unknown/space width 4. Prefix length increases by **two bytes per
callback**, after drawing, unless already at NUL; it is not a character timer.
Callback is invoked before the ANM frame is copied to screen.

The two PSEQ3 path references are a preload and a player argument, **not two
fixed playbacks**. From the 36-frame resource and two-byte callback progress,
the uninterrupted path requires five complete passes to reach the 350-byte
terminator (a deduction from the verified loop and resource, not DOS observation).

Text is embedded in the EXE, not the ANM: file `0x23906` (350 bytes before
NUL, 9 CRs) and `0x23a68` (316 bytes before NUL, 8 CRs). Their SHA-256,
including NUL, are respectively
`0f87f4368670e3206298b52bba39503be8a9dddb8a317fdea68b81ea193120ca` and
`b38f97a217955815e0a2b8318420a980d5f0ddf501a14f9e58c581b160b21ed9`.
Read these bounded strings from the user's original EXE rather than committing
the original prose. PSEQ3's outer loop tests the terminator **after a whole
movie**, not after each text callback. PSEQ6 tests after each post-ANM sprite
iteration; preserve the draw-before-increment ordering.

Shared routines independently inspected:

- `0x1470f–0x14959`: file-streamed ANM; `0x1495a–0x14b51`: memory ANM.
  Both decode frame 0 fully, then deltas into the persistent indexed canvas;
  force palette index 0 black; call optional callback, wait retrace, copy.
  Wait while `[0x1428]<=interval`, then reset it; no catch-up skipping.
  Final frame also gets this wait. First-frame reveal consumes this same
  counter interval. `0x14958` is **both** code file offset and a distinct
  runtime status address: `[0x14958]` starts −1 for PSEQ, becomes 0 on normal
  completion, stays −1 on Escape. Do not confuse these address spaces.
- Mode 0 blacks palette before decoding, then brightness 0,4,…,100;
  mode −2 applies a 100% target-color blend before playback (here RGB=0)
  and installs palette directly on frame 0; mode −3 skips the preliminary
  palette operation and directly installs palette on frame 0.
- `0x146c3–0x1470e`: VGA decoding into palette+canvas, no scene timing.
  `0x1819` blacks DAC immediately; `0x1796` installs palette immediately;
  `0x17d2–0x1818` scales each 6-bit channel by integer brightness/100 and
  waits retrace. Most scene boundaries above are explicit palette cuts,
  **not crossfades** (`0x1f825`, `0x1f9e0`, `0x1fcb6`, `0x1fe4a`,
  `0x20181`, `0x20477`, `0x206f1`, `0x20912`).
- `0x14092–0x140e7`: one-based transparent SPR blit, zero transparent,
  no clipping/anchor subtraction. `0x22ee` checks number, subtracts anchors
  (`0x233f–0x2346`), then clips. `0x2055–0x22c9` uses width/height in
  `[0x171c]/[0x171e]`, 16.16 nearest-source stepping, clipping and transparent
  zero; no anchor subtraction in this scaled path. PSEQ assets have zero anchors.
- PIT setup `0x184d–0x1857`: divisor 7457. ISR `0x18bd–0x18df` increments
  `[0x1428]` and decrements `[0x142a]` only when `[0x142c]==0`.
  `0x16f3–0x1709` polls VGA 0x3DA bit 3 through retrace then its end.
  A nominal PIT frequency is not an observed elapsed duration.
- Custom compositions usually draw first, add `[0x1428]` to t/p, then
  zero it. No explicit retrace wait in their main composition loops.
  Word arithmetic, one-subtraction phase wraps and exact comparisons matter.
- Input reader `0x2a27` consumes the pending key value. PSEQ compares AL
  with 27; other keys do not skip it. Checks surround loads and occur in
  ANM waits / most scene iterations, but not inside PSEQ10/12 motion records,
  palette-fade loops, the PSEQ6 >=15 busy wait, or the final audio wait.

## Sound and resource verification

Direct DSS start is **`0x45ab–0x4649`**, stop selected voice is
**`0x464a–0x4680`**. Inputs: `[0x3de0]` voice, `[0x3de2]` sample pointer,
`[0x3de6]` volume; start copies sample count, loop fields and volume into
the 26-byte voice record at `[0x3d76]+26*voice`. PSEQ uses voices 0 and 1;
its second sample is stored at sound-buffer+31000. Stops and replacements
are explicit in the scene ranges above. No music/sequencer call was found
in this complete PSEQ control path; that is not a claim about all startup audio.

**DATA-confirmed / decoder-tested:** a temporary C++17 harness linked the
unchanged `src/anm_format.cpp` and `src/spr_format.cpp` and successfully
decoded all 21 local PSEQ files. ANM frame counts: PSEQ1=16, PSEQ3=36,
PSEQ4=61, PSEQ6_1=21, PSEQ8=12. All eight VGAs decode one 320×200 frame.
SPR counts: PSEQ2_3=37, 6_2=4, 7_2=7, 10_2=7, 10_3=2, 11_2=1,
12_2=9, 14_3=5; all anchors zero. This validates decoding, not presentation.
Original DSS headers all report 5000 Hz; CABINE, ALARME1, ALARME4 and
OXYGENE have loop flag 65535; BARGE2, RADAR, LABO, TOUCHE and EXPLO have
loop flag zero. Resource counts and loop flags were read locally this task.

## Port implementation and validation (2026-11-15)

**Implemented / Port-tested:** `src/original_intro.cpp` owns the twelve PSEQ
stages, indexed 320×200 composition, ANM frame waits, text callback, motion
records, palette changes, sprite scaling/clipping, pending Escape and explicit
two-voice sound cues. `src/original_intro_player.cpp` presents through SDL and
uses the existing DSS loader and `audio::Mixer` with separate ownership for
voices 0 and 1. `src/main.cpp` calls it after the unchanged customized credits
and before creating gameplay input/audio and entering the game loop.

All PSEQ stages run in the documented order. PSEQ3 repeats complete movies
until the text ends (five passes with these resources); PSEQ6 continues text
with its four sprites. PSEQ10/12 consume pending Escape only at motion-record
boundaries. Reveal/final fades and the final voice wait do not poll Escape.
Movie playback retains the strict PIT threshold and reset/no-catch-up rule;
the PSEQ6 sprite wait uses >=15. Original localized text is read with fixed
length/terminator bounds from the user's EXE. No original prose is bundled.

**PSEQ12 safety limitation:** its VGA background, nine timed records, audio
and input boundaries run, but the mismatched-buffer sprite draw is suppressed.
The port validates/loads PSEQ12_2.SPR without redirecting the draw to it or
interpreting compressed VGA bytes as host sprite pointers. A startup log
identifies the limitation. This is neither a claimed DOS visual match nor a
speculative repair of the original code.

**Adapted:** nominal PIT rate is 1193182/7457 with integer counter comparisons
and retained fractional interrupt phase. Retrace is serviced by nominal 70 Hz
host presentation pulses; DOS CPU-driven compositions use nominal **60 Hz**
host pulses. They remain separate from PIT accumulation. Star depth advances
once per such composition; its deterministic xorshift RNG is a substitute for
the unrecovered DOS generator/state. Resource loading is eager, so original
I/O stalls and palette-black time during loading are not reproduced. Pausing
focus pauses presentation/audio. The existing mixer's master gain, anti-click
ramp and SDL buffering remain host audio adaptations. If audio is unavailable,
visuals continue silently and the final voice wait does not block indefinitely.
Missing/invalid visual or text resources return an explicit startup error.

The initial PSEQ-only integration went **customized credits → PSEQ → game**.
The later pre-PSEQ implementation below supersedes the omission of
CONCEPT/PRESENTE/TITRE/WHO. The original main menu and final cinematic remain
outside this implementation. Gameplay and death presentation are unchanged.

**Validation:** Debug build succeeded; complete CTest suite **28/28 passed**.
`tests/original_intro_tests.cpp` covers scene order, all motion records, five
PSEQ3 passes, callback progress/CR/LF/font placement, PSEQ6 continuation, exact
wait boundaries, deferred Escape, final audio wait/cleanup, transparent and
offscreen sprites, scaled bounds, flashing-background palette retention,
explosion cue ownership and malformed resource rejection. Running
`build/debug/original_intro_tests /path/to/DATA` additionally exercises the
whole sequence with the original resource files; this passed locally.

A temporary SDL dummy-driver harness verified Escape/quit, renderer-state
restoration, missing-resource error handling and audio subsystem cleanup with
both available and unavailable audio devices. A temporary contact sheet of
all twelve stages from original resources was visually inspected; neither
harness output nor original-asset captures are committed. These are port
checks, **not DOS-observed equivalence**.

Still **Unknown / needs DOS validation**: PSEQ12's actual buffer contents and
visible result; DOS RNG/state; wall-clock cadence of unpaced loops and
retrace/display/audio latency; complete original enabled/disabled-audio
lifecycle. Manual checks should cover the customized-credit handoff, radar
and warning text, sound replacements, Escape latency during motions and final
fade/audio completion, then normal game input. The optional original post-intro
configuration branch at `0x1e80a` remains outside this implementation.


## Startup fidelity audit addendum (2026-11-15)

Historical audit snapshot, before the pre-PSEQ implementation below. Its
absence claims describe that snapshot, not the current prelude status.
Scope: startup through the original menu, plus comparison of the twelve local
PSEQ stages. That audit made no implementation changes, final-cinematic
analysis, or running-DOS observation. The EXE hash at the head of this note was independently rechecked.
Earlier PSEQ reconstruction above is reused except where explicitly reverified
below; passing port tests does not independently verify that reconstruction.

### Executable entry and pre-narrative presentation

**EXE-confirmed, newly traced:** MZ header fields give entry file `0x23c44`
(header paragraphs 0x20, CS 0x23a4, IP 4). The 16-bit bootstrap
`0x23c44–0x23dd6` prepares runtime state; `0x23dc2–0x23dd6` loads the GDT,
sets CR0.PE and jumps to selector 8:0x829. In this image the destination is
file `0x0a29`, decoded as 32-bit code. `0x0a29–0x0afa` initializes selectors,
stack, allocation tables and the shared 64800-byte buffer `[0x721]`, then
`0x0af6` jumps to `0x62e2`. Thus the narrative entry is not the EXE entry.
`0x62e2–0x6397` initializes input/video/timers/audio and loads MENU2.SPR,
CON_VUET.SPR and PAL_MENU.PAL; these menu/font loads are not additional
credit screens. The presentation then falls through into CONCEPT at `0x63d2`.
This is the successful initialization path, not an audit of bootstrap error UIs.

**DATA-confirmed, newly decoded and visually inspected:** a temporary harness
linked the unchanged ANM/VGA decoder and decoded CONCEPT.ANM (24 frames),
TITRE.ANM (18 frames), PRESENTE.VGA and WHO1–4.VGA (one 320×200 frame each).
A local contact sheet of final frames identifies the content below. These are
resource observations, not captures of running DOS. Captures and original
assets are not included in the repository.

| Order before PSEQ | Resource / content | Reverified EXE evidence and missing local behavior |
| --- | --- | --- |
| 1 | CONCEPT.PAL + CONCEPT.SPR + CONCEPT.ANM: Blue Sphere logo | `0x63d2–0x66b8`: scaled entrance with brightness increase, two 24-frame ANM plays (interval 6, mode −3), sprite hold at (89,54) for 70 retrace waits after **each** play, reverse scale/fade and black DAC. Entire block absent. Formulas remain in the earlier section. |
| 2 | PRESENTE.VGA: “Présente” | Path pointer loaded at `0x66b9`, call `0x676a` at `0x66be`. Static fade-in, key/counter hold, fade-out; entire card absent. |
| 3 | TITRE.ANM: IN EXTREMIS title | `0x66c3–0x670f`: two complete 18-frame plays, interval 8, mode 0; each play begins with black palette and first-frame fade-in. `0x6711` blacks the DAC after the second play. Entire title animation absent. |
| 4 | WHO1.VGA: PROGRAMMATION / CYRIL TREVOAN | Pointer/call `0x671f/0x6724`, same still helper; entire card absent. |
| 5 | WHO2.VGA: GRAPHISME / OROU MAMA | Pointer/call `0x6729/0x672e`, same still helper; entire card absent. |
| 6 | WHO3.VGA: TRAVAIL ADDITIONNEL / ERWAN BODENAN | Pointer/call `0x6733/0x6738`, same still helper; entire card absent. |
| 7 | WHO4.VGA: BANDE SONORE / OROU MAMA / CYRIL TREVOAN | Pointer/call `0x673d/0x6742`, same still helper; entire card absent. |

These credit strings are baked into the VGA images, not PSEQ's typewriter
callback. Static path strings at file `0x4c37–0x4cc2` confirm the resource
names; their pointer mapping here is file offset minus 0x200.
At `0x676a–0x67fd`, all five still cards fade 0..100 in steps of 4, wait for
any nonzero consumed key or counter **>800**, and fade 100..0 in steps of 4
when `[0x14958]==0`. The caller sets that status to zero; CONCEPT and TITRE
also run with it zero. `0x14904–0x14917` therefore bypasses ANM Escape
polling. A still-card key advances that card, not the entire presentation.
No fixed number of wall-clock seconds is established by these thresholds.

**Audio evidence:** `0x62fb–0x6311` selects audio mode zero and rate 5000,
then initializes `0x420c`; `0x4417–0x4429` clears all four voice-active words.
The inspected CONCEPT/PRESENTE/TITRE/WHO control path and shared still/ANM
players contain no sample-start or music-start call; the animation callback
argument is −1 (none). `0x67fe–0x68a0` subsequently reads the configuration,
shuts down/reinitializes audio, and only then calls PSEQ at `0x68a5`.
There is no verified missing soundtrack to add to the pre-PSEQ logos/credits.
Actual hardware output and audible transients have not been DOS-observed.

Resource identities for this newly inspected set (SHA-256):

```text
CONCEPT.PAL  ea7d708809dc1c463907555c16078850c773dde27f13acc648c55c0f881c6bb4
CONCEPT.SPR  da692b0dcad0f8dcbf545f5ed94e132bab1fa56e31e541c6a3b288a3a7762ecd
CONCEPT.ANM  f532be1c7b350d6b03dfc78722db902c43df277ce51b636d3996dc2036d07dcb
PRESENTE.VGA 55dff997e21d8bd82ee1af564971dfb7460a15929fd0aca14355a6fd5df3933a
TITRE.ANM    54ce3762cd907cfc2458c13258fb356860333b4e028433e5e0ea84b064be0553
WHO1.VGA     d5df57824c6b599a8fbf9138e552e9aa5267dda7e27684cc4180eaf2959a94ce
WHO2.VGA     3fea8ad98604124dd180eaee5274c2ad94dd16715506840be58525418e7f80c9
WHO3.VGA     0f039b8d3eaa3cc135d864d4c143e808f8a985af067ea1f6d952b47db7240b51
WHO4.VGA     67bd8976aab9eba9aa1ecfae5e1dd50a2e920807b18b51086b8a99df750086a1
```

### Concrete PSEQ discrepancies and limits of the comparison

**New EXE-confirmed correction: CABINE restarts at the cockpit/radar boundary.**
`0x1f817–0x1f824` selects voice 0 and stops it; `0x1f825` blacks the DAC.
`0x1f82a–0x1f840` loads RADAR into sound-buffer+31000, leaving CABINE in
its first half. `0x1f845–0x1f865` selects that first half, voice 0, volume 48,
and calls `0x45ab`. The start routine resets the sample pointer
(`0x45e9–0x460a`), so this is a fresh CABINE start, not continued playback.
`0x1f88e–0x1f8af` starts RADAR on voice 1, and the repeat branch at
`0x1f927` returns there, not to the CABINE start. The earlier table's
“retain CABINE” claim is superseded by this instruction-level evidence.

**Port-tested discrepancy:** `Presentation::enter(Scene::Radar)` deliberately
preserves voice 0 and only cues RADAR. A temporary harness compiled against
current source and original resources counted **one CABINE start total**
across cockpit plus radar, versus the two required by the traced path; RADAR
started five times. No code or test expectation was changed in this audit.
The perceptual size of the restart difference still needs listening in DOS.

**Additional entry-input discrepancy:** `0x1f514–0x1f51b` consumes and tests
Escape before the first sound/movie setup. The port constructs Ship and starts
its reveal before processing SDL events, then retains Escape until reveal
completion. The existing skip tests verify that deferred port behavior, not
the DOS pre-reveal check. Key carryover across startup screens remains a
running-DOS verification item.

**Source comparison with inherited reconstruction:** all twelve stages are
present in `src/original_intro.cpp`: PSEQ1 movie; PSEQ2 cockpit; PSEQ3 repeated
radar/text; PSEQ4 laboratory; reused PSEQ2 alarm; PSEQ6 movie/text/sprite tail;
PSEQ7 stars; PSEQ8 movie; PSEQ10 thirteen motion records and panel sounds;
PSEQ11 scaled departure; PSEQ12 nine records; PSEQ14 flash/explosion/fade and
voice wait. No additional missing whole PSEQ scene or credit/title insertion
was established. The existing PSEQ12 omitted sprite draw remains a confirmed
port omission with **unknown DOS-visible result**, not permission to redirect
the buffer to PSEQ12_2.SPR.

Existing adaptations remain material: substituted star RNG and 60 Hz
composition cadence, nominal retrace/PIT service, eager resource loading and
host audio. In particular `Presentation::enter` directly composes the next
scene: DOS's explicit black-DAC operations around loading have no distinct
host presentation interval. The presence of those DOS operations is verified;
the visible length of those black intervals is not. Do not replace them with
invented fades or infer missing artwork from timing differences. PSEQ14's
palette fade and final voice wait are implemented; they are not missing scenes.

### Return to the original menu and validation boundary

**EXE-confirmed, rechecked:** after PSEQ, `0x68aa–0x6a06` loads/initializes
runtime resources. The optional call `0x6a3b → 0x1e80a` executes only if
`[0x5431]==1` and `[0x242c]!=0` (`0x6a0b–0x6a3b`); its complete UI behavior
is not reconstructed here and is not classified as another credit screen.
`0x6a40–0x6a52` stops audio, suspends the counters and calls `0x1d92d`.
`0x1d95f–0x1d98b` loads/decodes MENU_F.VGA; `0x1d9be–0x1d9d4` reveals
its rendered menu with brightness steps of 2. Menu foreground uses the earlier
MENU2/CON_VUET resources (existing death/menu evidence). The port's
`src/main.cpp` instead goes from customized credits to PSEQ to gameplay.
The absent original menu is a separate startup-flow difference, not a missing
PSEQ scene. The customized presentation was not modified.

Current verification: existing Debug `original_introduction` CTest passed;
its resource-backed executable also passed with local DATA. The temporary
resource-decoding and CABINE-cue harnesses were compiled from current source
and passed their audit checks. These checks establish current port behavior,
not DOS equivalence. No new full-suite result or manual DOS comparison is
claimed. Remaining visual verification is chiefly PSEQ12's actual buffer/draw,
unpaced motion/star speed, title/logo animation cadence, black intervals,
input carryover, and hardware audio timing.


## Pre-PSEQ block implemented (2026-11-15, after the audit)

**Implemented / Port-tested:** `src/original_presentation.cpp` and
`include/original_presentation.h` provide a separate silent controller.
`showOriginalPresentation` in `src/original_intro_player.cpp` reuses the SDL
texture format, nearest scaling, letterboxing and renderer-state guard.
`src/main.cpp` now executes **customized Extremis RE credits → original
Blue Sphere / Présente / title / four credits → existing PSEQ → gameplay**.
The customized credits and the twelve PSEQ stages were not changed. In
particular the audit's CABINE, PSEQ12 and PSEQ entry-input discrepancies are
not fixed by this task. No menu or final cinematic is added.

The prelude uses the existing PAL, SPR, VGA and ANM decoders and external
resources only. It validates 24 CONCEPT frames, 18 TITRE frames, five complete
320×200 card canvases and the first CONCEPT sprite's bounds. Missing/invalid
resources produce a startup error. No soundtrack, DSS load or audio subsystem
initialization occurs in the prelude player.

Implemented behavior, from the existing EXE evidence (no new EXE analysis):

- Blue Sphere uses the documented integer scale/position/brightness formulas,
  clipped transparent sprite drawing and 16.16 source stepping. It plays all
  24 CONCEPT frames twice with interval threshold **>6**, then holds sprite 1
  at (89,54), subtracting its anchors, for **70 retrace pulses after each play**.
  The hold retains the movie palette; scale entrance/exit use CONCEPT.PAL.
- PRESENTE and WHO1–4 use fades 0,4,…,100 and 100,96,…,0. Their hold counter
  resets after reveal; any pending key or counter **>800** starts the exit
  fade. A single consumed key advances only one card.
- TITRE plays all 18 frames twice with threshold **>8**. Both passes reveal
  frame zero with a fade from black. The reveal shares the first-frame wait
  counter; later frames reset it, and the final frame also receives its wait.
- CONCEPT/TITRE, scale loops, retrace holds and fades do not poll keys for
  interruption. A key received there remains pending until a still-card hold.
  Escape is an any-key advance on a card, not a whole-block skip. Window close
  and application quit still terminate the host player immediately.
- The end of the logo exit, the boundary after the second title pass, and
  the final credit handoff are black. Title repeats start from black as well.

**Explicit host adaptations:** nominal PIT rate `1193182/7457` with retained
fractional phase and 16-bit counters, and nominal **70 Hz** retrace pulses.
Zoom sampling follows those pulses; no measured DOS wall-clock equivalence
is claimed. A late host update services at most one retrace/movie frame,
without catching up hidden frames. Zoom samples clamp the endpoint to 0..160
instead of extrapolating after a host stall. Resource loads are eager; DOS
load stalls and their black-screen durations are not reproduced. Focus loss
pauses elapsed time. Input is a single pending any-key flag from non-repeat
SDL key-down events, not a complete DOS keyboard-buffer/typematic emulator.
Pending prelude input is discarded at the PSEQ handoff and SDL keyboard state
is reset; cross-block key carryover is not claimed DOS-faithful. These choices
are isolated to the new block and do not change PSEQ's timing/input policy.

**Current validation:** Debug configure/build succeeded and the full CTest
suite passed **29/29**. `tests/original_presentation_tests.cpp` covers screen
order, both complete movie passes, both 70-retrace holds, card timeout at
800 versus >800, fade steps, repeat-title reveal, scale/brightness/clipping,
transparent sprite anchors, deferred/consumed keys, black final handoff,
no frame catch-up and malformed resources. Running this executable with the
local original DATA also passed. A temporary SDL dummy-driver harness ran
the complete block with repeated Escape events, verified normal return,
window/application quit, renderer logical-size/scale/color restoration,
missing-resource failure and absence of audio initialization. A temporary
contact sheet from the new controller was visually inspected; no original
asset or capture is bundled. The linker reports its existing duplicate
`libsprites.a` warning; the build succeeds.

Manual verification still needed: the complete customized/prelude/PSEQ
handoffs in a real window, perceived logo zoom and title cadence, retrace
holds and card readability, key behavior during fades, focus pause/resume,
and comparison of timing/black intervals with running DOS. Automated tests
and decoded-frame inspection are not DOS-observed equivalence.

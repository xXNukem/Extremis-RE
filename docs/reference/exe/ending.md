# Original ending — FSEQ reconstruction (2026-11-15)

**Original investigation:** independently inspected local
`EXTREMIS.EXE`, SHA-256
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`,
and `GAME.LAB`, SHA-256
`acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`.
Instruction evidence below is **EXE-confirmed**, using bounded LLVM **32-bit**
decoding with `tools/inspect_original_code.py`. Addresses are absolute file
offsets; brackets denote DOS runtime memory. Static resource pointers in this
particular block map to file offset +0x200, not a universal address conversion.
No running-DOS observation, timing calibration, or elevator investigation.
The implementation addendum below supersedes the initial unimplemented status;
its code follows this evidence without repeating the disassembly.
Shared player/blitter knowledge is inherited from [Introduction](introduction.md)
unless a new range is explicitly given below.

## Entry and game-completion trigger

The complete blocking ending routine is **`0x210f4–0x21822`**, called at
**`0x769a`**. `0x21823–0x21856` is a coordinate table; `0x21857–0x218d6`
contains the eight `DATA\\FSEQ…` paths, not executable routines.

The decisive enemy branch is **`0xe22b–0xe27e`**:

1. Enemy state at record `+4` must be **5 (dying)**. Render sprite word
   `0x8005` on even phase, `0x8009` on odd phase (`+6`). Increment phase.
2. When the incremented phase reaches **>=8** (signed comparison), set health
   at `+0` to `0xffff`, the removed sentinel.
3. If current MONS bank **`[0x5192]==9`**, set both **`[0x5429]` and
   `[0x542b]` to `0xffff`**, then leave this enemy update. The ordinary sound
   17 / explosion setup at `0xe27f–0xe297` is bypassed.

This is completion of the death phase, not merely the hit that reduces health.
There is no level-number, sector, object, exit-door, countdown, or inventory
condition in this victory branch. It tests the bank, not an enemy index.
The level loader at `0x11336–0x11383` reads bank `+6` into `[0x5192]` and
resolves the enemy-table pointer from header `+0x0c`.

**DATA-confirmed:** scanning all 30 original LAB headers finds MONS9 only in
**level 30**, block file `0x3e3b2`. Its enemy table at `0x3e61a` contains
one record, at `0x3e61c`: health 5000, behavior 3, state 6, phase 0,
sector **4 (zero-based)**, local (300,300), polygon `0xffff`, angle 0,
reserved 0, sprite/flags `0x8000`, elevation 0, home (300,300).
Thus defeating this enemy is the original-data victory trigger; entering
level 30 alone is not sufficient. Behavior 3 is already active pursuit,
not a special FSEQ arming event. The existing enemy/navigation chapter covers
its general behavior; this task does not re-audit all boss combat or level puzzles.

Caller and flag lifecycle:

- `0x6acf–0x6ae0` initializes both completion words to zero.
- Main loop `0x7021` updates player logic; `0x7026` calls enemy update
  `0xe018`; immediately afterward **`0x702b–0x7038`** tests `[0x542b]==-1`
  and branches to `0x7686`. This avoids another normal gameplay/render cycle.
- General termination check `0x75bb–0x75c9` also branches to `0x7686`
  if `[0x5429]!=0`. That word alone is not victory: player termination also
  uses it. The separate `[0x542b]` test distinguishes the successful ending.
- `0x7686` calls level cleanup/save routine `0x7b83`, then
  `0x768b–0x7693` tests `[0x542b]!=0`. If true: `0x7695 → 0x12e96`
  releases the gameplay audio bank, `0x769a → 0x210f4` plays FSEQ,
  `0x769f → 0x12de4` reloads that bank, and `0x76a4` jumps to **`0x6a40`**.
- `0x6a40` stops game audio; `0x6a45` suspends the gameplay PIT counters;
  **`0x6a4e → 0x1d92d` opens the original menu**. The non-victory route at
  `0x76a9` checks health/oxygen/ship expiry for death presentation instead.

A bounded direct-reference scan of `[0x542b]` found initialization, these two
main-loop tests, and the MONS9 death write (`0x6ad8`, `0x702b`, `0x768b`,
`0xe271`); no additional direct writer. This is static evidence, not a claim
that every possible indirect memory write was dynamically ruled out.

## Playback order and transitions

All sprite numbers below are **DOS one-based**. Canvas is indexed 320×200.
ANM callbacks are **-1 (none)** throughout: no narrative font/text callback.
All four movie resources use the same full-first-frame/delta decoding as PSEQ.
Each movie frame, including the last, waits until the signed word counter
`[0x1428] > interval`, then resets the counter; no catch-up skipping.

| Order | Code / resource | Exact presentation |
| --- | --- | --- |
| Entry | `0x210f4–0x21128` | Tint the existing gameplay palette `[0x50f4]` toward DAC RGB **(63,48,0)**, amounts 0,4,…,100, one retrace before each blend. Keep the current screen pixels. |
| 1 | `0x2116f–0x21241`, FSEQ1.ANM | Load once, play from memory **four full passes**, 16 frames each, interval **10**. First pass mode **-1**, target (63,48,0); three more passes mode **-3**. No pause or blackout between repeats. MONSTRE voice 0, volume 64 starts before load/play. |
| 2 | `0x21243–0x213bf`, FSEQ2_1.VGA + FSEQ2_2.SPR | Stop voice 0; start VENT volume 48. Clear display and black DAC before loading. Restore background and draw the moving 7-frame sprite as specified below; install VGA palette on first composition. |
| 3 | `0x213c1–0x21438`, FSEQ3.ANM | Stop/restart the same VENT sample on voice 0, volume **32**. Clear display, black DAC; stream 12 frames, interval **13**, mode **-3** (direct first-frame palette). |
| 4 | `0x2143b–0x21668`, FSEQ4_1.VGA + FSEQ4_2.SPR | Stop voice 0, load/start LABO volume 48. Clear display, black DAC. Sprite 1 translates from the right, then sprites 1–13 play at the coordinate table below. |
| 5 | `0x2166a–0x216f6`, FSEQ5.ANM | Stop voice 0; load/start BARGE2 volume 48. Clear display, black DAC; stream 52 frames, interval **12**, mode **-3**. |
| 6 | `0x216f9–0x21785`, FSEQ6.ANM | Stop voice 0; load/start RADAR volume 48. Clear display, black DAC; stream 12 frames, interval **13**, mode **-1**, target **(0,0,0)**. |
| Hold / exit | `0x21788–0x21822` | Start the same RADAR sample on **voice 1**, volume 48; leave voice 0 alone. Hold the final image for **350 retrace waits**, then fade brightness **100,99,…,0**. Stop voice 1 then 0; release four buffers, black DAC, clear both screens, return. |

The first FSEQ1 call is at `0x211f6 → 0x1495a`; the repeat call is
`0x21238 → 0x1495a`, controlled by ECX=3 at `0x211fe`.
The streamed calls are `0x21433`, `0x216f1`, `0x21780 → 0x1470f`.
FSEQ1's path pointer at `0x211a5` is a preload; the two subsequent pointer
uses are player arguments, not three independent file loads.

**Mode -1**, newly needed relative to PSEQ: before decoding, apply a 100%
target-color palette; after copying frame 0, blend target amount
**100,96,…,0**, with retrace before every blend. It reveals from gold for
FSEQ1 and black for FSEQ6. The reveal shares the first-frame PIT interval.
Verified streamed branches `0x14791–0x147b9`, `0x14856–0x148b1`, and memory
branches `0x149c7–0x149ef`, `0x14a8b–0x14ae6`.
Mode -3 skips preliminary tint and installs the movie palette directly on
frame 0. These boundaries are palette cuts/reveals, not image crossfades.

For exact six-bit blending, helper **`0x14611–0x146c2`** computes target
channel `floor(target*amount/100)` separately, and source contribution via
its 64-entry table: at amount 0 the table is the identity; otherwise use
`step=floor(65536*(100-amount)/100)` and `table[c]=floor(c*step/65536)`.
Add the target contribution to this lookup. This fixed-point table can differ
from a single floating-point lerp. Brightness helper `0x17d2` is separate.

Allocations at `0x2112a–0x21165`: 64800 bytes at `[0x20b73]` (palette/canvas),
208000 at `[0x20b77]` (FSEQ1 preload / decoding scratch / FSEQ4 sprites),
5000 at `[0x20b7b]` (FSEQ1 decoder scratch), 62000 at `[0x20b7f]` (sound).
FSEQ2 sprites use existing `[0x721]`; unlike the documented PSEQ12 mismatch,
FSEQ2 and FSEQ4 select their actually loaded sprite buffers for drawing.
No world update is called inside FSEQ.

## Sprite compositions and timing

### FSEQ2: descending figure

`0x212da–0x213bf` initializes y=102, sprite=1, direction=+1, PIT counter=0.
For **y=102 through 149 inclusive (48 draws)**:

1. Restore VGA pixels to the composition canvas.
2. Draw the current sprite at **(154,y)** with transparent unscaled blitter
   **`0x14092`** (no clipping/anchor subtraction).
3. Copy the full canvas to display; install the VGA palette only when y=102.
4. Busy-wait until `[0x1428]>10`, then reset it.
5. Add direction to sprite; if the result is 7 set direction=-1, if it is 1
   set direction=+1. Increment y; continue while y<150.

Rendered sequence is `1,2,3,4,5,6,7,6,5,4,3,2` repeated four times.
The last displayed pose is sprite 2 at (154,149); y=150 is not rendered.
This is one-pixel movement per completed wait, not interpolation by elapsed PIT.
No explicit retrace wait in this composition loop.

### FSEQ4: translation, then 13 poses

**Translation `0x214d2–0x215bd`:** accumulated word `t=[0x20b83]` starts at
zero; reset `[0x1428]`. Each iteration restores VGA, draws sprite **1** at

```text
x = 274 + trunc_toward_zero(-197*t/320)
y = 24
```

Use **`0x22ee`**, transparent with clipping rectangle x=0..319, y=0..199
(and its usual anchor subtraction; all resource anchors are zero).
Then wait retrace **`0x2156a → 0x16f3`**, copy full canvas, install the VGA
palette if t==0, add elapsed `[0x1428]` to t and reset it. Continue while
signed t<320. The endpoint x=77 need not be sampled in this loop.
Unlike many PSEQ custom compositions, this translation explicitly waits retrace.

**Poses `0x215be–0x21668`:** reset PIT, then draw sprites **1 through 13**,
restoring VGA each time and using unscaled/unclipped `0x14092`. After each
copy, busy-wait until `[0x1428]>=32`, reset and advance. There is no explicit
retrace wait. Coordinate table **file `0x21823–0x21856`**, 13 `(x,y)` pairs:

```text
1:(77,24)   2:(78,24)   3:(78,24)   4:(79,23)   5:(81,23)
6:(82,23)   7:(85,23)   8:(87,22)   9:(90,22)  10:(84,22)
11:(77,21) 12:(69,21)  13:(62,21)
```

Sprite 1 therefore appears both during translation and as the first held pose.
The >=32 threshold is distinct from the strict >10 / >13 movie-style waits.
All coordinates plus decoded extents fit the canvas in this unscaled phase.

## Sound, text, interruption and postlude

Direct start **`0x45ab`**, stop selected voice **`0x464a`**, with
`[0x3de0]` voice, `[0x3de2]` sample address and `[0x3de6]` volume, reuse the
introductory audio ABI. All five sample paths are reused from the nearby PSEQ
string pool: MONSTRE file `0x210d5`, VENT `0x210e6`, LABO `0x21065`,
BARGE2 `0x21073`, RADAR `0x21056`. They are additional **audio** resources
outside FSEQ; no additional ending visual resource is loaded by this path.
The routine does not call the PSEQ text helper, credits routines, GAMEOVER,
or a music sequencer. Do not add music or narrative text based on filenames.

**DATA-confirmed DSS headers:** all five report 5000 Hz. MONSTRE has loop
flag 65535, loop range (0,9621), 9622 samples; VENT flag 65535, (0,25394),
25395 samples. LABO (27043), BARGE2 (17954), RADAR (5867) have loop flag zero.
Voice 0 is explicitly stopped/replaced at every scene boundary except within
FSEQ4 and between FSEQ1's repeats. RADAR's voice-1 replay after FSEQ6 is
intentional; there is **no wait for audio completion**, unlike PSEQ's exit.
The startup block itself does not issue a global stop-all; the caller releases
the gameplay bank before entry. Complete interrupt/audio-device lifecycle and
any inherited voice state remain a running-DOS verification item.

**No Escape skip:** `0x21166` writes **`[0x14958]=0`** before FSEQ1.
Both ANM players gate their keyboard polling on this word being nonzero
(`0x14904–0x14915`, `0x14b07–0x14b18`); normal completion writes zero again.
No direct key read, key comparison, or interruption branch occurs anywhere
in `0x210f4–0x21822`. Sprite waits, fades and final hold do not poll input.
Thus neither Escape nor another ordinary key advances/skips a scene. FSEQ
also does not explicitly drain pending input; its effect after returning to
the menu should be verified in DOS. Host window-close behavior is a separate
implementation policy, not an original cinematic key binding.

**Postlude:** final hold `0x217a9–0x217b2` passes AX=350 to **`0x170a`**,
whose loop calls retrace helper `0x16f3` then decrements AX (`0x170a–0x1713`).
It is **350 retraces, not 350 PIT ticks**. Final fade `0x217b3–0x217c3` uses
101 brightness samples. The last FSEQ6 frame contains the rasterized word
**FIN** (decoded-frame inspection, **not DOS-observed**). It is not a separate
screen or dynamic text. Return goes straight through audio-bank reload to
the menu described above: no credits, extra movie, GAMEOVER, forced exit,
or automatic new game on this route.

## Initial decoder checks and port boundary (before implementation)

A temporary C++17 harness linked the **unchanged** `src/anm_format.cpp` and
`src/spr_format.cpp`, decoded all eight local FSEQ resources and checked
64000 pixels for every movie/still frame. Results:

| Resource | Decoded data |
| --- | --- |
| FSEQ1 / FSEQ3 / FSEQ5 / FSEQ6 | 16 / 12 / 52 / 12 full decoded frames |
| FSEQ2_1 / FSEQ4_1 | one 320×200 frame each |
| FSEQ2_2 | 7 sprites, widths 17, heights 22–24 |
| FSEQ4_2 | 13 sprites, widths 228–253, heights 49–60 |

All SPR anchors are (0,0). A temporary sheet of each movie's final decoded
frame and both VGA backgrounds was inspected: burning monster, exterior,
helmet-removing figure, landscape, space, FIN. This describes resource images,
not an observed DOS presentation. No original assets, captures, prose or raw
disassembly are committed. Decoder success does not establish timing fidelity.

**Historical source status at investigation:** `src/enemy_system.cpp::System::step` already
sets `terminalDefeated_` at the MONS9 death boundary, and
`tests/enemy_tests.cpp` checks the bank-dependent flag. `include/enemy_system.h`
exposes `terminalDefeated()`, but there is **no gameplay consumer** in current
`src/` and **no FSEQ player**. Existing flag logic is not a completed ending.
No gameplay/cinematic code or test was changed or run as a new regression
suite in this documentation-only task; the resource-decoder harness passed.

Reuse candidates, checked against current code rather than documentation alone:

- ANM/VGA and SPR decoders, indexed canvas/palette conversion, transparency,
  clipped sprite composition, two-voice DSS playback and SDL renderer-state
  management in `original_intro.cpp` / `original_intro_player.cpp`.
- Strict movie PIT thresholds, counter reset and no catch-up behavior;
  separate retrace waits, repeated complete movies and brightness transitions
  from `original_presentation.cpp` (CONCEPT/TITRE).
- Existing native menu integration from player-death/main flow for the return.

Required FSEQ-specific work in a later implementation: consume the victory
flag once and suspend gameplay at the correct boundary; capture the current
screen/palette for gold tint; implement mode -1 color reveal and its integer
blend semantics; four FSEQ1 passes; FSEQ2's step/ping-pong movement; FSEQ4's
retrace-paced translation and >=32 pose waits; MONSTRE/VENT cues and final
RADAR replay; unskippable cinematic input; 350-retrace hold and unit-step fade;
cleanup and menu return. The current PSEQ controller is scene-specific,
not a parameterized player into which FSEQ can simply be inserted.

**Remaining verification:** DOS wall-clock cadence, I/O black intervals,
retrace/audio latency, incoming/pending keyboard state and complete audio
lifecycle. The intro's nominal PIT/retrace scheduling and host pulse policies
are adaptations, not measured DOS equivalence. The recovered control flow,
resources, transitions, coordinates and integer wait conditions are sufficient
to implement the sequence without inventing missing scenes; these timing and
handoff observations should remain explicit validation limits.


## Standalone implementation (2026-11-15, historical boundary)

**Implemented / Port-tested:** `include/original_ending.h` and
`src/original_ending.cpp` provide an independent controller; the SDL adapter is
`original_ending_player.h/.cpp`. It uses the existing ANM/VGA/SPR decoders and
DSS reader/mixer, following the existing presentation texture, nearest scaling,
letterbox and renderer-state restoration conventions. Intro/death controllers,
main menu, gameplay and campaign entry points are unchanged. The ending is
**not connected to `terminalDefeated()`**, which still has no gameplay consumer.
No original executable analysis was repeated and no new DOS facts are claimed.

The controller implements entry tint toward (63,48,0), all four complete FSEQ1
passes, the two sprite compositions and original coordinate table, FSEQ3/5/6,
mode -1 reveals with the documented fixed-point palette lookup, final RADAR
replay, 350 retraces and 101 fade samples. Explicit stop/start cues retain voice
ownership, sample looping and scene volumes. There is no audio-completion gate.
Escape and other keys cannot skip any scene. Application/window close returns
`Quit` to the caller; completion returns `Continue` with black output, and
missing/malformed visuals or sounds return `Error`. No menu transition is added.

`EntryImage` accepts the caller's indexed 320×200 screen and RGBA palette;
the tint preserves its indices and quantizes palette channels to the six-bit
DAC. This prepares the original entry effect without connecting it to gameplay.
The isolated Debug preview has **no gameplay screenshot** and defaults to a
black entry image. Its gold fill and subsequent FSEQ1 reveal run normally; a
real frozen gameplay image must be supplied by the later campaign integration.

**Timing and host adaptations:**

- Nominal PIT `1193182/7457`, fractional interrupt phase and wrapping 16-bit
  counters; signed wait comparisons, strict ANM thresholds, last-frame waits
  and reset/discard-excess behavior. First-frame reveal shares its PIT counter.
- Retraces are nominal **70 Hz host pulses**, separate from PIT ticks.
  ANM copies, mode -1 reveal steps, gold tint, FSEQ4 translation, final hold
  and fade use these pulses. A delayed host update performs at most one
  retrace/frame operation; hidden catch-up frames are not synthesized.
- FSEQ2 and the 13 FSEQ4 poses have CPU busy waits in DOS, but are **not free
  running animation loops**: their >10 and >=32 PIT thresholds are preserved.
  The SDL loop services them on the first update meeting the threshold,
  without adding the intro's 60 Hz composition timer or a retrace requirement.
  Host render scheduling and the loop's 1 ms yield add scheduling latency;
  exact DOS CPU/draw duration is not emulated. FSEQ4 translation uses signed
  integer interpolation and its documented draw-before-accumulate ordering.
- Visuals and sounds are loaded eagerly before playback. Explicit black scene
  boundaries are retained, but original disk stalls and their black-screen
  durations are not recreated. A black boundary is serviced by the next host
  update, not an invented timed pause.
- Focus loss pauses the controller and audio device. The SDL adapter consumes
  key events and resets keyboard state on normal return; original pending-key
  carryover to a menu is not emulated. The Debug window exits after completion,
  whereas original DOS returned to its menu. Campaign/menu handoff is deferred.
- Two independently owned instances of the existing mixer implement DOS voices
  0 and 1. Mixer gain, anti-click ramps, SDL buffering and device latency remain
  host adaptations. All five external DSS resources are required and validated;
  failure to open an audio device logs the error and permits silent playback.
  No incoming gameplay voices are inherited in this standalone adapter.

**Current validation:** Debug configure/build succeeded; full CTest **30/30
passed**. `tests/original_ending_tests.cpp` tests complete scene/frame order,
64 FSEQ1 frames across four passes, direct-palette repeats, gold/black reveals,
integer tint and brightness, clipping/transparency/background restoration,
all 48 descent draws and all 13 pose coordinates, strict/non-strict timing
boundaries, final-frame waits, no catch-up, sound cue order/volumes/voices,
350-retrace hold, unit-step fade, Escape immunity, cleanup and malformed data.
Its resource-backed invocation also passed with all eight original visual
files and all five DSS files (including their loop flags).

A temporary SDL dummy-video/audio harness ran the complete sequence while
injecting Escape throughout, and checked normal return, quit handling,
missing-resource failure, renderer logical-size/scale/color restoration and
audio subsystem cleanup. A temporary six-scene contact sheet rendered by the
new controller was visually inspected. Neither harness output nor original
asset captures are bundled. A Release build of the test executable succeeded
and rejected `--play`; the preview branch is compiled only with the Debug
configuration. These are port checks, **not DOS-observed equivalence**.

### Manual Debug playback

From the repository root, with the original external DATA directory:

```sh
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug -j4
./build/debug/original_ending_tests --play /Users/kurlos/Desktop/In_Extremis/DATA
```

Keep the window focused to play the whole sequence, including FIN, hold and
fade. Escape does nothing; closing the window stops the preview. At normal
completion the standalone preview window closes; it does not open a menu or
start a campaign. `--play` is absent in Release. Automated external-resource
validation (no window) uses the same executable without `--play`:

```sh
./build/debug/original_ending_tests /Users/kurlos/Desktop/In_Extremis/DATA
ctest --test-dir build/debug --output-on-failure
```

Remaining verification is manual viewing/listening in a real window, followed
by comparison with running DOS for wall-clock/retrace/audio behavior and
loading intervals. Victory dispatch, frozen gameplay capture and original
menu return belong to the later campaign-integration task.


## Campaign integration (2026-11-15)

**Implemented / Port-tested; supersedes the standalone-only status above.**
No EXE analysis repeated and no new DOS facts claimed. The trigger evidence
at `0xe25e–0xe27e` and caller at `0x702b–0x7038` is inherited from this note.
`main.cpp` now consumes `System::terminalDefeated()` immediately after enemy
update. `VictorySession` latches before blocking playback; enemy update stops
remaining fixed substeps when the terminal flag becomes true. Neither lethal
hit nor phases 1–7 trigger FSEQ. AI, damage rules and death poses are unchanged.

The handoff stops gameplay voices, drains enemy cues, clears item/fire input,
disables the door frame and suspends all remaining gameplay work. The last
presented 320×200 framebuffer supplies the gold entry tint. `EntryImage::rgba`
accepts the port's composed colors (potentially more than 256); the existing
six-bit conversion and tint arithmetic apply per pixel, without palette
reduction. Subsequent scenes and all FSEQ timing/content remain unchanged.
This framebuffer representation and input reset are host adaptations.

Normal completion enters `DeathPresentation::beginMenu()`, reusing the exact
menu reveal, RUN submenu, new-party loading and EXIT handling after GAME OVER.
The essential adaptation is allowing this menu to remain active while the
victorious player is alive; the diagnostic F10 death recovery cannot dismiss
it. New party resets the victory latch and reloads the world even from level 1;
Debug menu state is reset too. Missing menu/FSEQ resources fail explicitly;
window close exits without entering the menu. OPTIONS/password continuation
remain unsupported, as before; they are not needed for new party or EXIT.

Validation: Debug build and full CTest 30/30 passed. Existing enemy tests now
check the lethal-hit and every pre-completion death boundary. FSEQ tests run
the full controller through the same victory latch into the real menu, assert
single playback and compare RGBA/indexed entry tint. Menu tests cover restart,
Quit/Error and latch reset. These are automated port checks, not an SDL-window
playthrough or DOS comparison. The integrated manual cycle remains to be
checked in a real window; inherited DOS timing/audio limits still apply.

### Manual campaign victory in Debug

Run `./build/debug/InExtremisPort /Users/kurlos/Desktop/In_Extremis/GAME.LAB`.
After startup, press Tab for the existing Debug menu. In its level category,
select a `NIVEL 30` arrival with arrows and Enter. Keep enemies enabled and
close the menu with Tab. Press **F9**: the Debug-only level-30 shortcut applies
ordinary weapon-4 hits until MONS9 enters its normal dying state, discarding
the synthetic hit sounds. It does not write the terminal flag or skip death
phases. Keep the window focused: normal death completes, then all of FSEQ
plays, including FIN, hold and fade. Escape must do nothing during FSEQ.
The original menu follows. Use Enter on RUN, then Enter on Make new party to
verify a fresh level 1; alternatively select EXIT. F9 is absent in Release.

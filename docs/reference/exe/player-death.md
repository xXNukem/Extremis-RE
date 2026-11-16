# Player death presentation (2026-11-14)

Reference EXE SHA-256, independently checked this task:
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Offsets are absolute file offsets; brackets indicate runtime memory.
Death/ANM instructions were reused from `build/audio-debug/executable-disassembly.txt`
and vitals evidence. New bounded LLVM reads covered the retrace/palette/blit
helpers (`0x16f3–0x183b`), sprite entry (`0x22ee–0x237e`), stop-audio entry
(`0x13090–0x130c4`), menu caller/selection/drawing/font (`0x6a40–0x6a8a`,
`0x7700–0x772c`, `0x1d92d–0x1daa1`, `0x1e18a–0x1e356`,
`0x1f1a5–0x1f2bd`, resource loads `0x6330–0x637c`).
The following is **EXE-confirmed**, not a running-DOS observation.

## Branches and graphics

- `0x76a9–0x76db`: health `[0x5491]==0` calls `0x1723a` with AX=0;
  otherwise oxygen dword `[0x548d]==0` calls with AX=1. Health has priority
  if both are zero at dispatch. AX=2 is a separate ship-expiry sequence and
  is not implemented by this change. Existing Vitals retains its first cause.
- Health `0x17254–0x1728d`: stop all voices, tint the current palette toward
  (63,0,0), amounts 0,4,...100, with `0x16f3` retrace waits.
  `0x172bc–0x1731d` loads MORT0.VGA / MORT0.SPR, requests BANK[25] on voice 0,
  and copies the original full-screen background. No GOUTTE call.
- `0x17322–0x1744e`: six sprites cyclically, x={55,75,95,95,90,55},
  y=26 down to −148 inclusive (175 iterations), clipping y=26..199.
  Only rectangle x=55..264,y=26..173 is copied to display each time.
  Tint of the MORT0 palette retreats 100,96,...0 during the first 26 iterations.
  **No PIT/retrace wait in this loop**: sprite `0x22ee` and rectangle copy
  `0x175b` are unpaced. Do not claim a recovered duration for the scrolling.
  Fade brightness 100,95,...0 at `0x17454–0x17466`, then stop voices.
- Oxygen `0x174a7–0x17555`: stop voices; white tint 75,76,...100 with retrace
  waits; BANK[26] voice 0; play MORT1.ANM, argument 25, initial white reveal.
  ANM `0x1470f–0x14959` forces palette entry zero black, reveals first frame
  with white blend 100,96,...0, and advances after counter `[0x1428] > 25`,
  resetting that counter (no catch-up frame skipping). The reveal time is
  included in the first frame's counter interval. There are **32 original
  frames**, not frames synthesized from GOUTTE. Post-movie wait >160 ticks
  (`0x17558–0x1756a`), brightness 100,98,...0, stop voices (`0x1759e`).
- VGA1 `0x146c3–0x1470b` shares LZSS with ANM: 4-byte signature, 768-byte
  six-bit palette, length word then compressed 64000 pixels; entry zero black.
  Palette blend `0x14611–0x146c2` truncates target and source contributions
  separately; brightness `0x17d2–0x1818` scales DAC channels and waits retrace.

## Common ending and restart

`0x176db–0x1785f` loads GAMEOVER.VGA / TYPOOVER.SPR. Fade in brightness
0,4,...100. Read eight zero-terminated lines at EXE `0x21b96` (count byte),
center each using glyph widths+1 (unknown glyph/space width 4), first y=60,
line stride 10. `0x17874–0x178c3` displays each character and waits >8 PIT
ticks. `0x17805–0x1782b` drains held input, resets the counter, then waits for
input or >1600 ticks. Fade 100,99,...0. Return AX=0 goes via `0x7700` to
`0x6a40`, then menu `0x1d92d`; **no automatic resurrection**.
Menu uses MENU_F.VGA, MENU2.SPR and CON_VUET.SPR (load `0x6358–0x637c`).
Text pointer tables are at file `0x2369e` and `0x236be`; native RUN opens
Make new party / Continue with party / Return to menu (`0x1da20–0x1da98`).
New party uses the recovered initial-player template (`0x1de72` onward).
Menu text x=39+(149−width)/2, y=84+40*row; selected row blinks 4 of 8
iterations. MENU2 image 2 rotates rows two pixels left at (251,9).

The death routine is blocking: no world, weapon, HUD or interactive-modal
updates are called inside it; PIT/audio interrupts remain available. All
voices are stopped on entry and before GAMEOVER by `0x13090`. Oxygen's
native sample loop ends at that stop, not on an invented audio timer.

## Port implementation and limits

`player_death.cpp` owns presentation state and indexed resource composition;
`anm_format.cpp` reuses the ANM decoder for VGA1. `SurvivalSession` still owns
cause detection and single notification. `GameAudio` owns voices; presentation
emits start/stop events. `FirstPersonRenderer::drawFrame` presents the full
320×200 canvas. `main.cpp` gates simulation/UI, consumes death input and
uses the existing staged level-loading path for a **full** new game, even if
already in level 1. Menu RUN/new party, return and EXIT are integrated.
OPTIONS and password continuation are **not implemented**; their original
entries remain visible and report that limitation to the log when selected.
No alternative modern screens are substituted.

**Adapted, not exact DOS timing:** retrace loops and unpaced body/menu loops
advance one iteration per host presentation. Body/menu iterations add no
sleep or fixed-duration timer; their speed still depends on host rendering,
not emulated DOS hardware. Other phases use the existing host render pacing.
This preserves iterations and source graphics but does **not** recover DOS
scroll speed or retrace cadence.
PIT waits use nominal 1193182/7457 ticks/s, integer thresholds and reset/no
catch-up semantics; scheduling/interrupt latency is not emulated. The tint
of the frozen port framebuffer quantizes RGBA back to six-bit channels,
not a live emulated VGA DAC. Focus loss pauses presentation. Menu submenu
transition delays and the complete native options/password systems remain
outside this implementation. No DOS-observed visual/timing equivalence claimed.

**DATA-confirmed:** decoded health background is red; oxygen frames are gray.
No green screen-covering effect occurs in these two resource paths. GOUTTE
near-enemy slime remains separate (`0xfdd4–0xfdfd`, `0x10013–0x1003c`).
Original asset SHA-256:

- MORT0.VGA: `c8535c08bf3de9717533a493cf67f30c167264f61101b07368a9b9d3e290ad88`
- MORT0.SPR: `65f53852301a67f21c4c03d75c99114e973cb3d3c66d2258acd6d0582686e7f1`
- MORT1.ANM: `7eb4c676405ad04c0b31467f5579db0adf9ce29491301e022f1c685642164131`
- GAMEOVER.VGA: `c4577f9c887ee982aa7be9ee3b9ba36c4a1c3fbb23d38aa31818c7714c29e75b`

**Port-tested:** synthetic and actual-resource sequence tests cover both causes,
single activation/audio events, 175 body steps, movie/hold thresholds, text,
held-key drain, final fade, menu restart/exit requests and reset. VGA decoding
and truncated-stream rejection are in `anm_tests`; deferred audio and stop
ownership in `survival_session_tests`. Debug full suite: 27/27. Resource-backed test decoded 6 body sprites, 32 movie frames,
76 glyphs; reconstructed GAMEOVER image visually inspected. Manual in-window
DOS comparison and full menu-driven world-restart playthrough remain pending.

## Health-speed audit after manual testing (2026-11-14)

**Port-observed (user report):** MORT0 passes too quickly to distinguish the
animation; MORT1's 32 frames and the final text screen appear correct. This
is not a timing measurement of running DOS.

**New EXE-confirmed evidence:** reference SHA-256 rechecked above. Targeted
LLVM reads extended the previously inspected sprite entry through
`0x237f–0x259c`: clipping/setup leads, with `[0x1734]==0` (set at `0x17346`),
from `0x24ce–0x24d7` to the transparent blit at `0x256c`. Its inner loop
`0x257d–0x2585` copies nonzero bytes only; row stepping at `0x2587–0x259b`
returns through `0x256a–0x256b`. No timing call, PIT read or retrace wait is
hidden in that path. The inherited full-screen restore `0x17371→0x1745`,
rectangle transfer `0x17407→0x175b`, and blend `0x1742c→0x14611` also do not
wait. The next explicit retrace wait is in the subsequent brightness fade.

**Code comparison:** Body increments once per `advance` call regardless of
elapsed seconds. Main makes one such call then one `SDL_RenderPresent`; it
has no catch-up loop consuming multiple body steps before that API call.
Body explicitly bypasses `SDL_Delay`, and the renderer is not configured
with VSync. Thus progress is driven by host CPU/render submission throughput;
a submitted present is not proof that the compositor displayed every step.
The six-image cycle, x offsets, background restoration, vertical decrement,
transfer rectangle, transparent zero and red reveal match the recovered path.

**Port-tested:** `healthSequence` now checks all 175 steps with distinct sprite
colours, y movement, clipping, erasure and palette endpoints. Zero elapsed
and ten-second calls advance identically by one step; neither invalid time
nor framebuffer reads advances the state. These are instruction-sequence
regressions, **not a validated DOS clock model**. Runtime is unchanged.

**Unresolved correction:** there is no recovered constant frame interval for
MORT0. A new 16 ms sleep, 25-PIT-tick ANM interval or one-step-per-vblank rule
would invent a wait absent from this branch. Faithful wall-clock calibration
requires a DOS recording/trace with specified hardware or emulator CPU/cycles
and display settings. No such reference or installed DOSBox configuration was
found in the checked local game/application/preferences paths. Do not claim
this audit fixes the manual speed defect or establishes temporal fidelity.

## Manual MORT0 timing adjustment

Manual testing confirmed that the unpaced health-death animation was
too fast to distinguish on modern hardware. A 16 ms delay improved it,
but the final visually validated setting is 32 ms per body step.

The original 175 iterations, six sprites, positions, clipping and palette
transitions remain unchanged. The 32 ms delay is a deliberate port
adaptation, NOT a recovered DOS timing constant.

Both death sequences were manually tested. Debug CTest: 27/27 passed
after correcting the synthetic palette expectation in the death test.
Temporal comparison with the original DOS game remains pending.


## Shared victory menu entry (2026-11-15)

`DeathPresentation::beginMenu()` now opens the existing menu reveal directly
for successful FSEQ completion. The main loop retains that menu for a living
victor using `VictorySession`, rather than applying the diagnostic death-reset
path. Death presentation, MORT timing and GAME OVER flow are unchanged.
RUN/new party and EXIT are shared; OPTIONS/password limitations remain.
Debug CTest 30/30 passed, including direct victory menu/restart and Quit/Error
checks. See [Ending integration](ending.md#campaign-integration-2026-11-15).
No new original-game evidence was gathered.

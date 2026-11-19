# Weapons: one in your hands, twenty shots in flight

Forget a modern weapon wheel. The original *In Extremis* inventory revolves around **one equipped weapon at a time**, with ammunition, weapon exchanges and a small fixed pool of active projectiles. That design shapes almost everything about combat.

## Six weapon descriptors

The executable contains **six 112-byte descriptors** (starting at file `0x56d7`). The port preserves a typed version in [`include/extremis/gameplay/player/weapon_fire.h`](../../include/extremis/gameplay/player/weapon_fire.h).

| Weapon index | Fire input | Original `wait` field | Notes |
| --- | --- | ---: | --- |
| 0 | Single press | 4 | A new trigger press is required |
| 1 | Hold to repeat | 3 | Automatic |
| 2 | Hold to repeat | 2 | Uses alternating muzzle positions |
| 3 | Hold to repeat | 3 | Automatic |
| 4 | Single press | 6 | A new trigger press is required |
| 5 | Hold to repeat | 1 | Longer visual stage sequence |

These numbers are **descriptor fields**, not universal milliseconds or measured rounds per second. The DOS uses its iteration/FPS estimator when interpreting waits. Converting them straight into wall-clock delay would be inventing a new mechanic.

Each descriptor also gives firing geometry and projectile frame data: speed, aiming reference, muzzle offsets, lateral position and animation stages. That explains why the visible gun, the projectile sprite and the actual hit test need to be treated as related but distinct things.

## Pulling the trigger

The important sequence is:

1. Confirm a real press or a held trigger on an automatic weapon.
2. Check the equipped weapon and available ammunition.
3. Find a free slot in the **20-projectile pool**.
4. Accept the shot and spend one unit of ammunition.
5. Update the projectile through the game simulation, not from the draw call.

A button press that can't create a valid shot shouldn't quietly consume ammunition. The reconstructed path lives in [`src/extremis/gameplay/player/weapon_fire.cpp`](../../src/extremis/gameplay/player/weapon_fire.cpp); the item slots and ammo belong to [`player_inventory.cpp`](../../src/extremis/gameplay/player/player_inventory.cpp).

The currently equipped weapon is also represented in the original HUD, which uses its own graphics and selection rules. The code does not pretend the character is carrying a modern arsenal behind the scenes.

## Hits: a mask, not a guess

When a shot reaches an enemy, the hit test can use the enemy's per-image **8×8 SPT mask**. **Bit 1 rejects**, **bit 0 accepts**. This avoids the trap of treating a huge transparent area of a sprite as a solid rectangle.

The same rules apply when sprites are mirrored: the recovered DOS code does not automatically mirror the SPT mask bits a second time. Details and enemy reaction states are covered in [Enemies](enemies.md).

The projectile ray tests the moving leaf from the camera-facing side. Rendering and shots share the retracted span, so a shot can pass through a clear part of the opening before the actor passage branch is enabled. [DOS projectile and wall records](../reference/exe/doors.md#projectiles-and-visible-faces).

## Weapon exchange and empty magazines

Supplies and weapons are taken through the original panel/inventory flow; a weapon exchange is not the same thing as adding another gun to a hidden collection. The DOS behaviour when ammunition runs out has been investigated, but precise selection and edge cases still need validation across the full campaign.

We also still need to compare the **visible shot origin** to the original weapon's muzzle and the on-screen crosshair. A projectile that hits correctly but appears to fly out of the screen centre can still feel plainly wrong.

## Where to look

[`weapon_fire.h`](../../include/extremis/gameplay/player/weapon_fire.h) is the compact descriptor catalogue; [`weapon_fire.cpp`](../../src/extremis/gameplay/player/weapon_fire.cpp) advances shots; [`player_inventory.h`](../../include/extremis/gameplay/player/player_inventory.h) owns the equipped weapon and ammunition; [`enemy_system.cpp`](../../src/extremis/gameplay/enemies/enemy_system.cpp) handles enemy damage and reaction. The original bank graphics are read at runtime — they are not distributed here.

**Original evidence:** weapon table near file `0x56d7`; 20-slot projectile handling and firing cadence traced in the executable; the FPS estimator at `[0x1422]` is distinct from the PIT's 160 Hz clock. Timing fidelity is still an explicit area of work.

## Enemy impact visibility

The port's enemy hit test projects the shot onto the targeted sprite plane. The enemy is drawn first, so the projectile's impact used to sit at the same depth as opaque enemy texels and fail the strict sprite depth test. With level 1's original MONS1 at (4100, 2100), a shot from (4100, 2050) along +Y reduced health from 4 to 3 and emitted all five BALLE0 impact stages, but contributed zero pixels in each presented frame. This was reproduced before the door geometry correction and at `48d7f48`.

The projectile scene now marks enemy impacts as eligible to overlay sprite texels at coincident depth. Vortex uses the wall-only column depth to exclude wall ties and leaves the live depth intact when overlaying a sprite. The trajectory, collision, damage and stage progression use the existing weapon update. The five-stage lifetime comes from the previously traced `EXTREMIS.EXE` file range `0x180e9–0x180fe` (the executable hash is recorded in the reference index).

**Port-tested:** the combat regression renders two flight frames, all five impact phases and the subsequent empty scene at three viewport sizes. It checks the projectile position, damage, ammunition, and occlusion by a closer sprite and a coincident wall. With original resources, consecutive SDL software presents at 320×200, 960×600 and 1260×600 showed the five enemy impact frames again. Flight, ordinary wall and door sequences retained the same coverage after the fix. Software captures establish visibility per frame. Interactive display cadence and DOS elapsed-time equivalence remain unverified.

## Live gameplay shot cadence

**EXE-confirmed, 2026-11-19.** Rechecked the original `EXTREMIS.EXE` with SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`. In the normal gameplay iteration, file offset `0x727a` calls projectile processing at `0x17bdd`. It draws at `0x180db` and, for slot state 2, increments the phase word `[esi+8]` at `0x180e9–0x18101`. A result of 6 clears the slot word `[esi]`; the phase-five draw has already occurred. This branch reads neither the PIT counter nor the FPS estimate. The gameplay iteration increments `[0x1424]` at `0x738e`, then blits 12,000 dwords from the buffer pointed to by `[0x1410]` to the buffer pointed to by `[0x140c]` at `0x7395–0x73a7`. That blit has no retrace wait at the copy site. The normal back edge at `0x7681` returns to `0x6c25`. The cinematic copy uses an explicit retrace helper at `0x16f3`. These offsets establish five gameplay passes. DOS wall-clock duration and display cadence remain unmeasured.

**Port before the timing correction.** The startup sequence enters `AppSession::run`, which polls SDL, handles focus and modal state, advances the world, calls `WeaponFire::update` once and calls `AppSession::presentFrame`. Enemy and posture updates subdivide elapsed time around an adapted 18 Hz step. Weapons run outside those substeps. Each active weapon update advances a flying shot by the descriptor's distance and advances an impact by one phase. The SDL millisecond delta feeds the rate estimator used for cooldown. The nominal 160 Hz gameplay counter used by doors and self-destruction is separate and resets to 1. The trace showed one weapon advance at both 0 ms and 16 ms of SDL delta. There is no projectile accumulator or render interpolation. Focus and modal blocks stop weapon progression. Menus and level transfers reset the elapsed-time baseline. Regular 3D draws can still use the last cached projectile scene while simulation is blocked.

After walls, decoration, enemies and enemy effects, the renderer draws projectiles with depth, applies optional fisheye, draws the HUD and composes the texture. SDL uploads and scales that texture before the session presents it. Debug profiles the same present call and can draw diagnostics. Release calls it directly. Neither session imposes a regular gameplay delay.

**Port-tested with the original GAME.LAB** (SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`). Injected SDL input into the normal executable on Metal with a 60 Hz display mode. Release at 854×300 internally, 16:9 and 2×, returned from SDL for all five impact phases in a total phase-one-to-empty span of 7.856 ms. A separate Release run at 439×150 took 19.731 ms for the same door impact. MAX at 3419×1201 took 40.929 ms. The Debug level-18 shot reduced enemy health from 17 to 16. Its four flight updates took 4.730 ms until the hit, and all five impact phases returned successfully to SDL over 5.639 ms. The corresponding coverage capture retained color in all four flight and five impact frames. An external control with Metal VSync 1 yielded about 16.7 ms per phase, about 83.4 ms through the five impact phases. A fixed-wall control kept the same three flight updates and collision position. Creation to hit took 4.797 ms in the unpaced capture and 49.891 ms with VSync. A Debug pause after impact phase one recorded two inactive updates, including an SDL delta of 116 ms on resume. The next active update produced phase two.

In the captured door, ordinary wall, enemy and open-flight frames, some projectile color reached composition and each impact phase received one successful SDL present. At 1×, the first door flight frame changed 282 world pixels and 58 final texture pixels. At 2× 16:9 the same comparison was 1174 and 267. The Debug enemy capture retained 65 of 295 first-flight pixels and 47–212 texture pixels per impact phase. The ordinary wall sample's last unobstructed flight frame contributed 28 pixels to the texture. A separate four-step open flight ended at another wall. These counts are texture-input differences with and without the projectile layer, with the same HUD state. They precede SDL scaling, debug overlays and scan-out. They establish partial composition loss and short presentation intervals in the actual executable. They are not a monitor capture or a measurement of which phase the player saw.

The earlier targeted tests advanced phases manually. The software probe supplied a fixed 1/60 s update interval, gave the HUD no equipped weapon and exported consecutive images for fixed-rate playback. It tested whether individual frames could be drawn at chosen depths. It did not measure real cadence or include the gun and reticle that can cover flight pixels. This audit reproduced the visibility incident before the timing correction below.

### Capturing a shot

Build Debug or Release and launch the game from the repository directory. Enter gameplay, take a single shot, wait for the effect to disappear, then exit. Repeat in a fresh process for each shot. With the initial view of level 1, aim straight ahead for the moving door or turn a quarter turn right for the side wall.

```sh
EXTREMIS_SHOT_TRACE=timing ./build/shot-live-release/InExtremisPort \
  /Users/kurlos/Desktop/In_Extremis/GAME.LAB 2> shot-time.log
python3 tools/check_shot_trace.py shot-time.log --target door --require-flight
```

Build directory used in the audit: `build/shot-live-release` (`cmake -S . -B build/shot-live-release -DCMAKE_BUILD_TYPE=Release` and `cmake --build build/shot-live-release`). Repeat with `EXTREMIS_SHOT_TRACE=pixels` and pass `--require-color` to check the composed texture. The comparisons in pixel mode add CPU work, so use timing mode to measure cadence. The analyzer expects an uninterrupted shot with one impact. It reports simulation durations and the intervals from SDL returns to subsequent returns independently. `--min-ms` and `--min-presents` can validate externally specified limits. The audit sets no DOS millisecond limit.

### DOS clock audit and timing correction gate

**EXE-confirmed.** A bounded inspection of the same executable separates three mechanisms.

- In flight (`0x17c4d–0x17c6d`), a state-one slot supplies the angle from `[esi+6]` and the descriptor supplies the distance from `[edi+0x20]`. The call at `0x17c61` projects the distance into X/Y with the fixed-point trigonometry at `0x3ebb–0x3ee7`. The caller adds both components to the slot's position once per pass. This path does not scale the distance by elapsed time.
- On impact (`0x180db–0x18101`), the draw precedes one phase increment per pass and the slot is released after five phases. The normal gameplay blit has no retrace wait at its copy site.
- The PIT handler at `0x18c6–0x1910` advances `[0x1428]` and the sampling counter when `[0x142c]` permits it. Every 80 IRQs it writes twice the iteration count in `[0x1424]` to the FPS estimate `[0x1422]` and resets `[0x1424]` to 1. The loop floors the estimate at 9 at `0x6c25–0x6c39`. Code at `0x17b08–0x17b24` computes `floor(descriptor_wait × estimate / 18) + 1` and stores it in `[0x5215]`. That countdown is checked and decremented before allocating another shot (`0x17a4b–0x17a63`). It does not advance existing slots. The divisor of 18 therefore gives no projectile step period.

**Decision.** No fixed original period in milliseconds was established. A compatible engineering option is to calibrate a fixed simulation step against a declared DOS reference profile with recorded machine or emulator settings. Measure active iterations and elapsed time in repeatable shots at more than one load. If DOS cadence varies by profile, document the choice of reference as an adaptation. An independent monotonic accumulator could then run each due swept step in order, with the existing collision and damage functions and no paused time in the accumulator. Keep the first-shot and burst admission rules separate from this decision. At render rates below the resulting simulation cadence, an intermediate phase can elapse between presentations. Requiring every phase to appear would require a separate presentation policy and can extend apparent lifetimes.

The replay checker accepts `--compare-with` and an explicit `--tolerance-ms` to compare the per-phase SDL-return spans of two uninterrupted single-shot captures. The current Metal traces at 30 and 120 FPS fail a comparison with a 20 ms tolerance. That tolerance distinguishes this reproduction and does not define a DOS target. The comparison measures presentation cadence and includes renderer scheduling. A future fixed-step implementation also needs a regression on the simulation timestamps and swept steps, including several simulation steps between two renders.

**Port-tested timing baseline.** Release, Metal, VSync 0, one original level-1 wall shot with externally paced SDL presents. Every run took three flight updates, hit the same coordinates (3799.999900, 319.999799) and rendered each of the five impact phases once. Times are milliseconds.

| Requested limit | Effective present rate during impact | Creation to collision | Impact from first SDL return to empty frame |
| --- | ---: | ---: | ---: |
| 30 FPS | 30.0 | 99.996 | 166.657 |
| 60 FPS | 59.9 | 49.927 | 83.412 |
| 120 FPS | 120.2 | 26.144 | 41.613 |
| 144 FPS | 115.7 | 25.370 | 43.216 |

The 144 FPS request did not reach a steady 144 FPS in this Metal run. These measurements use the unmodified gameplay timing. See the fixed-step measurements below for the new behavior.

### Fixed-step shot timing in the port

**Adapted.** Extremis uses 60 projectile steps per active second as a reference. One step matches an iteration at the previously tested 60 FPS pacing and gives the five recovered impact phases a total of 83.333 ms. The EXE evidence above supplies the phase order and the distance per step. Absolute DOS elapsed time remains unknown.

`AppSession::run` supplies its existing monotonic SDL delta to `WeaponFire`. On an idle launch, the first sweep and flash are immediate. The next sweeps run every 16.667 ms. The ARME firing composition lasts 16.667 ms of active elapsed time from each accepted shot and uses the original barrel phase. Additional shots join the next projectile tick. The existing frame-rate estimator and cooldown formula still govern admission once per gameplay update. Admission follows the elapsed sweeps and can use a slot just freed by them. Paused time, including the first delta on reactivation, is excluded while the accumulated fraction is held.

Each update accepts up to eight steps' worth of elapsed time (133.333 ms). During longer stalls the excess time is discarded. Every due sweep calls the existing collision and damage code in order, using the camera, door leaves and enemies available to that gameplay update. Moving targets or leaves can therefore be in different poses on different replays. The renderer presents the most recent completed state, including the fifth impact image until the next tick. Faster presentations can repeat a phase and slow or uneven presentations can skip phases. These choices preserve step distance and collision order through catch-up and impose no global FPS limit.

**Port-tested, 2026-11-19.** The original GAME.LAB ran through the usual startup, SDL input, `AppSession::run` and Metal presentation in Debug and Release. The fixed-wall shot took three 80-unit flight steps before the collision step and hit (3799.999900, 319.999799) in every pacing run. The level-18 enemy shot flew for four steps (66.667 ms) and hit (5499.999871, 1400.000490), reducing health from 17 to 16. The door samples flew for two steps (33.333 ms), with contact at y≈500 and x varying with the moving leaf and the player's pose. The compact, 854×300 2× wide and 3419×1201 MAX layouts all ran the same scheduler.

In the table, millisecond values are host observations at update boundaries, in the order flash expiration, launch to collision, and collision to clear. Present counters come from successful SDL returns. The phase durations in simulation were 16.667 ms each in every row. External pacing targeted the requested FPS. The display reported 60 Hz and the 144 FPS requests averaged below 144 successful presents per second. The paused rows include the menu interval in wall time.

| Capture | Mean SDL presents/s | Host times (ms) | Flash / flight presents | Presents for impact phases 1–5 |
| --- | ---: | --- | --- | --- |
| debug-door | 520.4 | 18.182 / 36.610 / 80.615 | 3 / 13 | 12, 7, 10, 3, 17 |
| debug-enemy-pixels | 119.0 | 25.091 / 74.509 / 83.765 | 3 / 9 | 2, 1, 3, 2, 2 |
| debug-enemy | 540.9 | 17.477 / 67.491 / 86.509 | 15 / 59 | 15, 14, 15, 15, 5 |
| debug-impact-pause | 103.1 | 25.009 / 41.749 / 203.722 | 3 / 5 | 4, 2, 0, 2, 2 |
| debug-pause | 96.7 | 16.667 / 164.876 / 79.462 | 2 / 6 | 2, 2, 2, 2, 2 |
| debug-wall-120fps | 116.9 | 24.961 / 49.986 / 84.607 | 3 / 6 | 3, 2, 1, 2, 2 |
| debug-wall-144fps | 135.6 | 20.799 / 55.550 / 83.317 | 3 / 8 | 2, 3, 2, 2, 3 |
| debug-wall-30fps | 29.9 | 33.123 / 66.213 / 66.532 | 1 / 2 | 0, 1, 0, 1, 0 |
| debug-wall-60fps | 59.9 | 33.406 / 50.127 / 99.776 | 2 / 3 | 2, 1, 0, 2, 1 |
| debug-wall-unlimited | 119.9 | 16.671 / 50.242 / 91.090 | 2 / 6 | 3, 1, 2, 2, 3 |
| debug-wall-vsync | 119.9 | 24.129 / 57.851 / 82.840 | 3 / 7 | 2, 2, 2, 2, 2 |
| release-door-pixels | 409.7 | 28.535 / 44.572 / 72.167 | 3 / 4 | 4, 5, 11, 7, 8 |
| release-door | 423.0 | 17.913 / 34.136 / 82.484 | 11 / 19 | 5, 7, 8, 8, 8 |
| release-max | 142.6 | 18.589 / 38.138 / 78.886 | 3 / 6 | 2, 2, 3, 1, 2 |
| release-wall-120fps | 119.2 | 25.162 / 58.620 / 83.165 | 3 / 7 | 2, 2, 1, 3, 2 |
| release-wall-144fps | 130.3 | 33.178 / 55.515 / 83.261 | 3 / 7 | 2, 3, 2, 2, 3 |
| release-wall-30fps | 30.0 | 33.379 / 66.676 / 66.643 | 1 / 2 | 0, 1, 0, 1, 0 |
| release-wall-60fps | 59.4 | 33.371 / 50.094 / 99.801 | 2 / 3 | 2, 1, 0, 2, 1 |
| release-wall-pixels | 119.8 | 24.843 / 49.853 / 91.110 | 3 / 6 | 3, 2, 1, 2, 3 |
| release-wall-unlimited | 392.6 | 17.118 / 49.999 / 83.998 | 11 / 28 | 9, 9, 8, 9, 9 |
| release-wall-vsync | 60.5 | 33.316 / 49.983 / 100.072 | 2 / 3 | 2, 1, 0, 2, 1 |
| release-wide | 134.4 | 17.030 / 36.490 / 88.043 | 1 / 3 | 2, 2, 0, 3, 3 |

The coverage passes found composed projectile pixels in flight and in all five phases against the wall, door and enemy. The flash counters indicate the original firing composition with successful SDL presents. The HUD tests separately exercise the firing layers, offsets and reticle. These checks measure composition and SDL return times. Physical display scan-out and subjective visibility remain unmeasured.

The Debug menu was opened in flight and again in a separate shot after impact phase one. The first update after closing each menu carried 111 ms of elapsed host time. The weapon discarded the reactivation delta and resumed from the saved phase and fractional time. Both runs completed all five simulated phases.

The automated `equipped_weapon_fire` test failed on the old code with “Flash did not last one simulation step” and passed after the change. It checks 30, 60, 120, 144 and 2400 updates/s, repeated zero deltas, paused flash and impact timing, catch-up bounds and overlapping shots. All 40 CTest targets passed in each build. The live checker runs as `python3 tools/check_shot_trace.py <log> --fixed-step --target wall` (also accepts `door` and `enemy`). It checks the phase sequence and the deadlines against the actual gameplay deltas, allowing zero presentations for an intermediate phase. The logs and summaries are in `build/shot-timing-live/`; the local injection harness is in `build/shot-live-evidence/`.

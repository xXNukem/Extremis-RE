# Original executable — reverse-engineering knowledge index

**Purpose:** find and reuse established reverse engineering before reopening `EXTREMIS.EXE`. This index is a lookup map, **not** a claim that every area is fully reconstructed. The accessible system chapters already contain substantial evidence; do not duplicate them indiscriminately.

Original reference: `EXTREMIS.EXE` SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`; `GAME.LAB` SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`.

For address conventions and evidence labels, consult [Reverse-engineering notes](reverse-engineering.md). Offsets below refer to **file** positions, whereas `[address]` denotes runtime memory. System chapters remain the canonical readable explanations; detailed EXE entries record instruction-level evidence.

## Where to look first

| Subsystem | Detailed EXE evidence | Existing system knowledge | Representative starting points / status |
| --- | --- | --- | --- |
| Doors, connectivity and passage | [Doors: original state word and automatic cycle](exe/doors.md) | [World](../systems/world.md) | `0x1074b`, `0x10a3c`, `0x1146d–0x114b9`, `0x1184f` — integrated; activation/stopping boundary 180 compared with inherited EXE evidence; DOS wall-clock timing still unverified |
| Level structure and bank selection | — | [Levels](../systems/levels.md), [File formats](file-formats.md) | `GAME.LAB` 30 level records, sector/connection/enemy tables — parser implemented |
| Elevator cinematics and shuttle level transfer | [Elevator and shuttle cinematics](exe/elevator-cinematics.md) | [World](../systems/world.md), [Levels](../systems/levels.md), [File formats](file-formats.md) | `0xce1a–0xd63a`, composer `0xd96d`: A_LEVEL VGA/SPR, four floor-delta variants; `0x7051 → 0xb4b2–0xc86b`: separate level 22→23 shuttle, six NAV/VAIS_EXP/ARR_BASE ANMs plus composites. Trigger, routes, input, audio, motion and load boundary recovered; conventional A_LEVEL presentation integrated (31/31 Debug tests), preview target elevator_preview; standalone post-confirmation shuttle implemented with Debug shuttle_preview (33/33 tests), campaign hookup integrated via shuttle_session (two prompts, exact prerequisites, reusable door, deferred level23 arrival; Debug 34/34 plus original-resource prompt probe); CPU/retrace cadence adapted, DOS timing comparison pending |
| Enemy AI, animation and hit masks | [Per-record comparison and MONS5 timeout](exe/enemy-record-comparison.md) | [Enemies](../systems/enemies.md) | `0xe11b–0xe127` mode 2 activation; `0xe696–0xe703` walking; `MONS*.SPT` 8-byte mask entries |
| Enemy motion detector | [Acquisition, projection and CASQUE](exe/motion-detector.md) | [Enemies](../systems/enemies.md#motion-detector) | `0x100f6–0x101e3`, `0x1117a–0x111ee`, `0x13e00–0x13eef`. Lateral sign and marker anchors corrected. Live level-2 pixel regression added. DOS sweep cadence pending |
| Level-22 recorded communication | [Recorded communication](exe/level22-communication.md) | [Interfaces](../systems/interfaces.md), [Levels](../systems/levels.md) | `0x704c → 0xa937–0xafb4`: sector16, connection44 toward room44; YES/NO, seven pages, delayed Escape; repeatable door trigger, no campaign/security award; documentation only, DOS observation pending |
| Ship self-destruction / refused evacuation | [Self-destruction](exe/self-destruction.md), [Shuttle state follow-up](exe/elevator-cinematics.md#2-shuttle-level-22--23) | [Items](../systems/items.md), [Player](../systems/player.md) | `0x95e4–0x95f9`, `0x16c57–0x16cc6`, `0x76dd–0x76f4`: 16 distinct securities activate 48000-unit countdown; refusal permits boarding retry; expiry AX=2 → explosion/GAME OVER/menu. Pauses, password reset, global level persistence and late-boarding edge documented; countdown/expiry already integrated; shuttle success now disables counting while preserving flags/time; DOS timing and final-tick boarding still pending |
| Weapons and projectiles | — | [Weapons](../systems/weapons.md) | `0x56d7` six descriptors; 20 projectile slots; [targeted depth regression](../systems/weapons.md#enemy-impact-visibility); [live-loop cadence audit](../systems/weapons.md#live-gameplay-shot-cadence) at `0x727a`, `0x738e–0x73a7`, `0x180db–0x18101`; port uses an adapted 60 Hz shot cadence; see [fixed-step timing and validation](../systems/weapons.md#fixed-step-shot-timing-in-the-port) |
| Password checkpoints and level persistence | [Password continuation](exe/password-continuation.md) | [Levels](../systems/levels.md), [Interfaces](../systems/interfaces.md) | `0x74c1 → 0x1eede` arrival modal, `0x1dbc6–0x1defc` fixed templates, `0x78a6–0x7c40` working LAB. Custom LOAD GAME and one-time arrival notices implemented |
| Colored proximity lighting and optics | [Colored lights](exe/colored-lights.md) | [Rendering](../systems/rendering.md), [Items](../systems/items.md) | `0x10ea3–0x10f36`, `0x74e2–0x75a4`, `0x16b5b–0x16bca`, `0x1bc24–0x1bd1b`; 30 lamps, optics, shared battery and cabinet exchange integrated |
| Nests and bomb | [Nests and bomb](exe/nests-and-bomb.md) | [Enemies](../systems/enemies.md), [Items](../systems/items.md) | `0x1bfbc–0x1c027`, `0x101e4`, `0x10584`; 21 eligible generators among 24 entries and 39 bomb cabinets; live port integrated, DOS arming presentation and runtime comparisons pending |
| Power surge | [Power surge](exe/power-surging.md) | [Rendering](../systems/rendering.md) | `0x6aea`, `0x16be0–0x16c56`, `0x74e2–0x75a4`; global timer, warning, alarm and composed-frame darkening integrated. Port surge timing now uses fractional nominal PIT units independent of rendered FPS; shared gameplay clock unchanged. DOS uses the shared RNG and darkens the active optics palette. Windows and DOS observation pending |
| Player death presentation | [Death sequences](exe/player-death.md) | [Player](../systems/player.md), [Interfaces](../systems/interfaces.md) | `0x1723a–0x179b0`: MORT0 health speed fails manual review; CPU-driven cadence requires DOS calibration; MORT0/MORT1/explosion and GAMEOVER lead to the custom menu |
| Movement, posture, player survival | — | [Player](../systems/player.md) | `0x1656a–0x16a7d` movement/posture, `0x16a8e–0x16aab` bobbing, `0x165db–0x166bb` damage reaction |
| Keyboard bindings and turn direction | [DOS keyboard controls](exe/classic-controls.md) | [Player](../systems/player.md#controls-in-the-port) | `0x28b2–0x28d9` extended-key map, `0x16d1e–0x16e34` configurable gameplay input, `0x16707–0x16725` signed turn step. Classic uses the traced turn directions and the requested port bindings |
| Decoration, animation and collision | [OBJET collision and enemy sector transitions](exe/objet-collision.md) | [Decorations](../systems/decorations.md) | `0x10db7–0x11028` XY collision and actor caller differences, 30-level inventory, `0x11e79–0x11ea5` OBJET6 timing, `0x14d1f–0x1517e` pixel patches |
| Wall textures, floors, ceilings, sprites | — | [Rendering](../systems/rendering.md) | `0x15795–0x15925` DALLE addressing; `0x12ac4` onward planes; sprite projection `0x11f83–0x1207f` |
| Inventory, cards and level transfer | [30-level terminal inventory and progression](exe/code-terminals.md) | [Items](../systems/items.md), [Interfaces](../systems/interfaces.md) | `GAME.LAB` zone access offset `+0x0e`. Keypad authorization at `[0x6039+zone]` persists across travel and clears on new or continued games (implemented and tested). Door/card routines `0x937c–0x95d7`. Wider interactions incompletely verified |
| Audio and timebases | — | [Audio](../systems/audio.md), [Player](../systems/player.md) | PIT divisor 7457 (~160 Hz); main loop/frequency estimator are distinct; fixed 18 Hz in port is an adaptation |
| Original introduction / startup presentation | [Introduction sequence](exe/introduction.md) | [File formats](file-formats.md) | `0x63d2–0x6742` prelude + `0x68a5 → 0x1f4ba–0x20cd6` PSEQ integrated after custom credits; Blue Sphere/title/WHO fades, repeats and key tests pass (Debug 29/29); Space/E whole-startup skip added as host adaptation (Debug 35/35 plus local-resource SDL playback/skip checks); Escape/controllers unchanged, unsafe PSEQ12 draw suppressed; host timing/RNG adapted; DOS comparison pending |
| Original ending / victory (FSEQ) | [Ending sequence](exe/ending.md) | [Enemies](../systems/enemies.md), [Introduction](exe/introduction.md) | `0x769a → 0x210f4–0x21822`; MONS9 completed death at `0xe25e–0xe27e`; FSEQ1 ×4 → 2 → 3 → 4 → 5 → 6/FIN → menu; Escape disabled; campaign trigger after completed death opens the custom main menu; Debug level-30 F9 shortcut; eight visuals/five sounds checked; host timing adapted, DOS comparison pending |
| HUD, map and terminals | — | [Interfaces](../systems/interfaces.md) | `0xa608` dispatch, `0x114ba–0x1185c` connected-sector traversal |

## Current verification priorities (2026-11-14)

- **Door opening distance / momentary movement stall:** the 26-case Debug suite passes and the user confirmed automatic opening and closing in gameplay. Static comparison found the same 180-unit request/stopping boundary and passage after phase 60; no ordinary-approach activation discrepancy was found. Perceived waiting time still needs a running-DOS comparison. An optional early-opening quality-of-life mode must be labeled an adaptation, not silently applied to the faithful default. See [Door evidence and status](exe/doors.md).
- **Open-door traversal:** the port's separate loss of fractional movement in an already passable, unclamped doorway was reproduced before the refactor and corrected. See [the level-1 trace and port correction](exe/doors.md#open-passage-slowdown-in-the-port-2026-11-19).
- **Crossing, actor clearance, projectile leaves and backing:** swept movement now rejects geometrically crossed blocked faces. Approaching enemies use the recovered door-specific clearances. Projectiles share the directed visible leaf with rendering. The EXE confirms the ordinary facing gate and a separate action-panel path. [Evidence, regressions and remaining DOS observations](exe/doors.md#door-crossing-actor-clearance-shots-and-interaction-audit-2026-11-19).
- **Door timing:** the port runs a nominal PIT-derived counter, but the historical DOS main-loop cadence and all suspension paths are not yet dynamically verified. Do not reimplement the already integrated door state machine just to investigate this.
- **Elevator texture leakage:** keep as an unconfirmed/reproduction-dependent visual issue; avoid speculative fixes.

## When to update

For each **new** investigation, create or extend one scoped entry under `docs/reference/exe/` with:

1. Original file hash/version, investigation date/source, and address conventions.
2. Short answer and precise DOS instructions/data (bounded ranges, branch conditions and caller paths).
3. Evidence classification: confirmed disassembly vs observed game behavior vs deduction vs unknown.
4. Difference between DOS, current port, and the tests; do not label an unimplemented rule as fixed.
5. Unresolved details and conditions which would justify reopening the EXE.

Avoid creating empty subsystem pages or megabytes of raw disassembly. The objective is **reliable retrieval**, not archiving every investigation transcript. The existing system chapters may be expanded directly for small discoveries.

For continuation of the five-system audit, see [Research checkpoint](research-checkpoint.md).

See also [Known differences](known-differences.md) for the backlog (which can lag the latest local tests).

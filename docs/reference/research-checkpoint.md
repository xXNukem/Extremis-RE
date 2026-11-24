# DOS research checkpoint

2026-11-20. Both local reference hashes match the [EXE index](exe-index.md). The original files stayed external. The following offsets are EXE file offsets unless specified.

| System | Established in this session | Next verification |
| --- | --- | --- |
| Password continuation | [EXE + DATA + port](exe/password-continuation.md): 16 fixed code templates and 29 eligible arrivals, now live in the custom menu and session. | [Port validation and presentation differences](#password-continuation-port-implementation). |
| Colored lights and optics | [EXE + DATA + port](exe/colored-lights.md): 30 lamps in 17 sectors, original optics and palette priority, shared drain and exchange at 12 battery cabinets. | [Port validation and next DOS check](#optics-and-battery-port-implementation). |
| Death and endings | [EXE + port](exe/player-death.md#custom-menu-handoff-2026-11-20): native dispatch `0x768b–0x772c`; port now hands off from death fade or FSEQ success to its custom main menu. Debug and Release CTest 43/43 each, all four SDL app handoffs and level-1 restart verified with diagnostic inputs and original resources. | DOS MORT0 scroll duration and retrace pacing remain open. |
| Nests and bomb | [EXE + DATA](exe/nests-and-bomb.md): 24 entries, 21 initially eligible, 39 bomb cabinets, visible OBJET variants and the slot reuse/destruction rules. | [DOS checks for blast bounds, visibility and armed travel](#nests-and-bombs-2026-11-23). |
| Power surge | [EXE](exe/power-surging.md): `0x6aea`, `0x16be0–0x16c56`, and `0x74e2–0x75a4` establish an ungated campaign timer, strict boundaries, random-strength full DAC darkening, and the exact warning/alarm requests. | Run DOS through the first window and a level transfer near expiry. Trace writes to the DAC and audio request to measure visible cadence and alarm competition. |

No DOSBox executable was found on PATH or in the inspected application directories. The 30-level reports are static data scans. The original research baseline is summarized above. Current implementation status is recorded below and in [Known differences](known-differences.md).

## Optics, colored lights and Power Surging (2026-11-23)

- **EXE-confirmed.** `[0x608f]` has only three direct operands in the reference: party initialization, player update read and write. Surge presents a palette pulse, warning and alarm. Its draws at `0x16c17/0x16c43` also advance the RNG used by bomb shake at `0x167d1/0x167e5/0x167f9`. No direct gameplay gate or resource penalty was found. [Surge cross-references](exe/power-surging.md#state-cross-references-2026-11-23).
- **EXE-confirmed / DATA-confirmed.** `0x1bc24–0x1bd1b` selects radar or green optics with one shared battery. `0x16b5b–0x16bca` handles drain and cutoff. The lamp scan still runs with optics enabled, but the DAC path suppresses its tint. A surge darkens the selected base palette without changing mode or draw on the battery. Validated all ten stock PAL banks and found 12 battery cabinets in the pristine 30-level LAB. Corrected the battery and reserve columns in all 16 continuation templates. [Optics and cabinet table](exe/colored-lights.md#optic-system-and-battery), [template correction](exe/password-continuation.md#template-payload).
- **Next DOS check.** On a private copy of the original, equip a battery and enter level 11 sector 10 at local `(100,440)`. Compare radar and optics at the lamp overlap. With a DOS debugger, set `[0x608f]` to 0 just before a positive-delta update at `0x16be0` and compare the DAC output with optics off and on. Repeat with `[0x544b]` at or below the next delta to check the same-update palette reversion. Then trace the real PIT-to-display cadence, BANK[14] competition and ordinary pause/transfer timing without forced counters. No DOS run was available in this intervention.

## Radar position and contact composition (2026-11-23)

**EXE-confirmed / Port-tested.** The lateral projection now uses the DOS subtraction order. The HUD applies the original CASQUE marker anchors at all three fade levels. The level-2 regression checks the live scene through a real sector crossing and compares the final radar pixels at all four cardinal headings. DOS sweep cadence remains open. [Offsets, reproduction and validation command](exe/motion-detector.md).

## Optics and battery port implementation

- **Implemented.** The existing inventory and supply modals drive a single
  off/radar/optics state, with direct switching and charged-battery exchange.
  Both modes drain the same gameplay counter and share the HUD battery
  indicator. Radar alone shows contacts. The single compositor transforms
  the current RGBA world and HUD to the optical source before a transient
  blend. Cutoff restores the normal source on the same update, including
  during a surge. See [palette order](exe/colored-lights.md#discharge-and-interaction-with-lamps-and-surges).
- **Port-tested.** Debug and Release CTest passed 45/45 each. Original
  assets verified the 12 battery cabinets, the HUD, modal banks and the
  optical presentation across aspect and render-scale choices. A Debug app
  run used a diagnostic battery and normal inventory events to switch radar
  → optics → radar → off. A separate session probe used the real transfer
  path and found charge 143983 and optics still active on return, with the
  swapped battery stored at 449 units in level 2 sector 13 wall 15. A
  continuation restored its preset charge with both modes off and the
  cabinet back at 900 units. NEW GAME reset to no battery. Reference LAB
  and EXE hashes matched the index.
- **Adapted / next DOS check.** The host applies RGB6 arithmetic to
  composed RGBA output, and skips its first resumed gameplay interval.
  DOS programs the VGA DAC and seeds the first delta to 1 after ordinary
  inventory return. The port uses the existing modal layout and HUD
  message queue. Compare the original modal cadence and DAC ordering
  with the [DOS scenario above](#optics-colored-lights-and-power-surging-2026-11-23).

## Nests and bombs (2026-11-23)

- **DATA-confirmed.** The 30-level table joins generator anchors, initial eligibility, MONS bank and mode-4 pool to all 39 bomb cabinets. Level 5 also has one continuation template with a bomb. Decoded both OBJET variants in all seven enabled banks. The nest is a solid floor sprite with an oval mouth and a broken variant. Its anchor does not lie on a wall segment. Level 30's unrelated OBJET10 forms also use the image indices 0 and 1. [Campaign inventory and visual evidence](exe/nests-and-bomb.md#campaign-inventory-of-generators-and-bombs).
- **EXE-confirmed.** The ordinary shot handler at `0x17dd3–0x17e68` makes an impact without changing the Entity A variant. The bomb arms from inventory, runs a blocking `A_BOMB.SPR` sequence, then uses the live fuse of 800 units. The warning says five seconds. The blast disables eligible generators at max-axis distance 207 or less and stores the changed variant in the working LAB on departure. The traced travel and victory gates have no nest requirement. [Rules and port comparison](exe/nests-and-bomb.md#pickup-placement-and-explosion).
- **EXE-confirmed / DATA-confirmed transition.** A live fuse resumes in the destination with the original sector number and local XY. The projected sprite is skipped on a sector-list miss, while damage still uses the origin words at the computed address. Example lift 3, level 3 sector 22 to level 4 sector 9, uses stale level 4 LAB words at `0x58c2/0x58c4`. [Controlled transition case](exe/nests-and-bomb.md#port-comparison-and-remaining-checks).
- **Next DOS check.** Use separate snapshots of level 5 sector 12 with a bomb. Fire at the intact sprite, compare spawn eligibility at the connection boundary, then detonate at local `(407,190)` and `(408,190)`. Compare the live type at the two distances and after departure/return. Run the lift-3 case with a debugger and check the fuse, sprite and damage independently. Capture the arming animation, warning, alarm and explosion timing. No DOS run was available in this intervention.

## Password continuation and disk I/O (2026-11-23)

- **EXE-confirmed / DATA-confirmed.** Disclosure is triggered by a completed lift or shuttle arrival and looks up the zone. The 16 codes have 29 possible sectors across those arrivals. No terminal supplies them. All 16 template payloads, world/sector joins, and new-party defaults were checked against the indexed EXE and LAB. A successful code installs a fresh world and consumes the display marker. Used markers remain until the DOS process exits. [Arrival and template tables](exe/password-continuation.md#where-the-game-gives-the-code).
- **EXE-confirmed.** `RUN → Continue with party` accepts eight alphanumeric characters, folds lowercase, handles Backspace and ignores incomplete Enter. The result waits 140 VGA retrace cycles and dismisses itself. Refusal returns to the parent menu at `0x1d9e2`. Pause `Continue with party` resumes the live session through a different path. [Menu and validation](exe/password-continuation.md#original-run-choices-and-validation).
- **EXE-confirmed / DATA-confirmed.** The disk call audit identifies the working `GAME.LAB`, 13-byte `EXTREMIS.CFG` settings, and `SNAPIC*.PIC` captures as the write families. A fresh party always regenerates the work file from `DATA\DATA.LAB`. Enemies, doors, cabinets and generators start from stock data. Security, map/authorization, and other party globals start from the common initializer. No full-save path found. [Disk audit and state boundaries](exe/password-continuation.md#disk-operation-audit).
- **Prior port inspection.** At the time of the DOS audit LOAD GAME had no action. The [implementation below](#password-continuation-port-implementation) completes that work.
- **Next DOS check.** Reveal `STRADIUS` on an ordinary lift, verify suppression during a second party in the same process, then confirm it reappears on a cold launch. Check `CENTRAL7` after successful shuttle arrival and one accepted/refused menu response with an input trace. Trace `0xb32` during a successful continuation and a lift, then capture one screenshot and exit Options to verify the two auxiliary writes. No DOS execution in this intervention.

## Password continuation port implementation

- **Port-tested.** Debug and Release built successfully and each passed all 45 CTest cases. The new `continuation_tests` exercises the editor for all 16 codes, including invalid and short input, case folding, Backspace, repeated lookup and one-use disclosure. Running it with the original `EXTREMIS.EXE GAME.LAB` in both builds verified each field directly and checked all 29 destinations against the route table.
- **Port-tested with original resources.** A scripted SDL software run in both builds exercised the title menu with an invalid code, then accepted `cartoon0` using Backspace and reached level 10 at world `(1280,3700)`. NEW GAME from pause returned to level 1 at `(3500,320)`. A separate Debug probe through the actual transfer handler checked all 16 starts, the original lift 4 sequence and its level 5 notice, the successful shuttle arrival request for level 23 and its notice, and suppression on revisiting. It verified the enemy, door, cabinet and generator changes survive an ordinary round trip and revert on continuation. It also checked NEW GAME defaults, preserved notice history, and the run-loop reset of an armed bomb, generator clock and security state when a checkpoint supplies a new inventory bomb. The shuttle portion injected the success handoff rather than replaying the shuttle cinematic. The LAB hash remained the reference value.
- **Adapted.** The custom editor and arrival overlay render at host cadence. The menu result is held for two seconds. The notice pauses simulation and accepts a fresh SDL key. The DOS palette and animated modal box remain different. Native DOS observation checks stay in the previous section.

## Resource-layer update

The new `vga_palette` decoder shares RGB6-to-RGB8 expansion between PAL
texture banks and ANM. The loaders retain their existing output formats, size
checks and error messages. ANM retains its index-zero handling. The texture
fixture verifies the expansion of all 64 source values. Debug and Release
CTest passed 43/43 each.


## Gameplay presentation update

Colored lamps and power surges are wired into the active gameplay loop.
`ambient_effects.cpp` owns lamp selection, surge timing and the RGB lookup.
The app posts the warning through the HUD queue and shares BANK[14] with
self-destruction. `first_person_presentation.cpp` applies one tint to the
composed world and HUD while leaving the base framebuffer intact.

Next DOS comparison: reproduce the level 11 sector 10 overlap and cross
the first surge threshold with a lamp active. Compare the warning, alarm
and first restored color frame. Native DAC scanout, the shared DOS random
stream and separate DOS transient effects remain open. See the
[optics and battery update](#optics-and-battery-port-implementation) for the
current port validation.

Validation for this update: Debug and Release CTest 44/44 each.
`ambient_effects_tests GAME.LAB` passed in both builds against the local
reference, checking all 30 lamp centers and their 17 sectors. The synthetic
fixture also checks overlap priority, the 159/160 boundary, surge rearming,
warning deduplication and world/HUD color restoration.


## Session persistence update

`session_travel.cpp` stages returning levels through `prepareDestination`.
Departed levels retain enemy records and panel contents, with doors reduced
to their unlock bit. NEW GAME clears the in-memory cache. Same-zone travel
continues using the active level. Debug and Release CTest passed 44/44 each.
The inventory regression verifies a consumed cabinet after reconstruction
from a snapshot and its availability from the pristine source. The Debug
selector verified the original 30 levels and 55 arrivals. A subsequent SDL software-rendered probe linked the actual app objects
and called `AppSession::transferLevel` with original level resources.
The level 1 → 2 → 1 trip kept an enemy’s reduced health and the door
unlock bit, cleared the door phase, and NEW GAME restored the enemy and
cleared the cached levels. This bypassed the interactive elevator UI.

The directed bomb trace resolves the static stale-sector case. The direct
reads stay within every stock destination record and reinterpret bytes
beyond the logical directory. See [the bounded trace](exe/nests-and-bomb.md#port-comparison-and-remaining-checks).
The older pickup-only status for generators and bombs was superseded by the live integration
documented in [the current port comparison](exe/nests-and-bomb.md#port-comparison-and-remaining-checks).

## Release options and Classic keyboard (2026-11-23)

The title and pause menus update the same preferences. Changes are written to
`settings.ini` in the per-user directory returned by SDL. This is under
`~/Library/Application Support` on macOS and `%APPDATA%` on Windows.
Missing and malformed values fall back individually. The first-run
floor/ceiling default still reads byte 12 of a valid 13-byte original
`EXTREMIS.CFG`, if available. Every save replaces a complete file in the
user directory. The settings persist across party creation, continuation,
travel and application restarts. [SDL preference directory](https://wiki.libsdl.org/SDL3/SDL_GetPrefPath).

| Option | Previous behavior | Release behavior and validation |
| --- | --- | --- |
| Aspect Ratio | The renderer applied 4:3, 16:9, 21:9 and AUTO. | The selection persists. The composed resolution follows the selected aspect. The menu regression and stock HUD captures exercise aspect and scale. |
| Resolution | Real internal scaling lived in memory. Selector dimensions could differ from the composed frame. | Persists the integer scale or MAX. MAX resolves against the current display. An unavailable preset falls back to 1× and the selector displays the applied frame dimensions. Checked through the real pause menu, renderer creation and MAX restore. |
| Fisheye Effect | Worked through the existing compositor. | The flag persists and is applied before the first gameplay frame. Checked in the pause regression, the stock HUD matrix and the live renderer trace. |
| Floor and Ceiling | The menu row was cosmetic. F toggled the current level's material loader state. | F and the menu now change one saved renderer preference. A disabled or unavailable SOL bank draws flat colors. The plane regression compares rendered pixels before and after the toggle. |
| Colored Lights | The menu row was cosmetic and showed OFF while the original lamp tint was active. | The saved flag controls proximity lamps and defaults to ON. Checked at all 30 original lamp centers with ON and OFF, plus the optics and power surge regressions. |
| VSync | Changed the active SDL renderer and read back its state. | The preference is restored at renderer setup and reapplied on device reset. The menu shows the active interval. Tested OFF/ON, an unsupported renderer and destruction/recreation of the software renderer. |
| Master Volume | The menu displayed a percentage. Gameplay kept its fixed mixer gain and the menu had independent streams. | All output paths receive the saved gain. SDL disk output with original BANK.DSS measured game peaks `0, 0.0817304, 0.163461` and menu peaks `0, 0.181445, 0.362889` at 0/50/100. Effect completion and native loops are covered separately. |
| Control Scheme | The menu label changed and gameplay still used Modern input. | Modern remains the default. Classic handles keyboard movement, firing and contextual action without relative mouse mode. Both profiles share the existing menu inputs. See [bindings](../systems/player.md#controls-in-the-port) and [EXE trace](exe/classic-controls.md). |
| Language | The label moved between three choices and all text stayed English. | Superseded by the 2026-11-24 localization integration below: English, Spanish and French are selectable, applied to interactive port text and persisted. Original cinematic text and text painted in sprites remain pending. |

**Port-tested.** Debug and Release CTest each passed all 45 cases. With original
resources, a scripted SDL software run changed video and audio from the title
and pause menus, fired a Classic keyboard shot, switched to Modern, loaded
level 10 with a continuation code and returned to level 1 with NEW GAME. A
second process reused the saved settings, including 16:9, 1×, fisheye and
VSync. The original LAB, CFG and EXE retained their hashes and modification
times. Stock HUD pixels and original audio were also checked. Native Windows
execution and physical keyboard input on macOS remain unverified.

## Localización de interfaz (2026-11-24)

El catálogo y el inventario de `docs/localization-*.md` registran 130 IDs propuestos; 101 cadenas de menús, HUD, inventario y paneles están conectadas al idioma compartido en `settings.ini`. Los 29 IDs restantes son texto original de cinemáticas o etiquetas reservadas. Las tres lecturas narrativas dudosas (`comm.page.01`, `shuttle.script.02`, `shuttle.script.04`) no se activan en esta fase y conservan el original en los reproductores. `include/extremis/ui/localization.h` contiene la tabla usada por el port; `utf8_glyph.h` aporta el mapeo de glifos sobre los recursos originales sin editarlos. Quedan pendientes la revisión visual de compresión de texto, los rótulos pintados en sprites y la localización de cinemáticas. No se estudiaron de nuevo mecánicas del EXE.

## Localización de cinemáticas (2026-11-24)

**Port-tested / DATA-confirmed.** La Fase 3 enlaza los 26 IDs cinematográficos verificables de los 29 reservados con `UserSettings.language`, incluidos FIN por identidad y la línea vacía de GAME OVER. Siguen en inglés original del EXE `comm.page.01`, `shuttle.script.02` y `shuttle.script.04`, por dudas de sentido ya registradas; ninguna propuesta provisional se activa. La inspección de `PRESENTE.VGA` y `WHO1–4.VGA` identificó cinco rótulos rasterizados adicionales, localizados al español mediante máscara blanca y `CASQUE.SPR` en tiempo de ejecución; el francés usa la tarjeta francesa original. El catálogo ampliado tiene 135 IDs, 132 resueltos. No se modificaron recursos originales ni Vortex.

**Verificación.** `cinematic_text_tests` comprueba fuente DOS original, límites de todas las traducciones y exclusión de los tres pasajes ambiguos. `original_presentation_tests` coteja la tarjeta original con ES y FR y verifica que la fase y el brillo coinciden. Las suites originales con DATA confirman orden de PSEQ, siete páginas, shuttle, muerte, audio y relojes. Falta captura visual comparativa de cada fotograma en todos los aspectos y una ejecución DOS frente a port; la tipografía compuesta se verificó por límites y pruebas del port, no por observación en DOS.

**Resultado de compilación y suites:** Debug y Release compilan; CTest completo pasa 46/46 en cada configuración. Además, `cinematic_text_tests`, introducción, tarjetas, comunicación, shuttle y muerte pasan con `DATA` original en Debug; `cinematic_text_tests`, introducción y tarjetas también en Release. SHA-256 de `GAME.LAB` y `EXTREMIS.EXE` coincide con los hashes de referencia de `AGENTS.md`. La comparación visual fotograma a fotograma DOS/port continúa pendiente.

**Validation:** Debug and Release builds succeeded; full CTest passed 45/45 in each configuration. Original-resource `interaction_tests DATA` and `hud_tests DATA` passed with distinct EN/ES/FR pixel output for inventory and HUD. `port_menu_tests` covers language selection, persistence, catalog coverage, dynamic placeholders and UTF-8 glyph mapping. Exact visual legibility of every compressed panel row and every rasterized DOS label remains a manual review item.

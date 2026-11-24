# DOS quick access passwords and level persistence

Static verification against the reference [EXE and LAB hashes](../reverse-engineering.md) on 2026-11-20. The local `DATA/DATA.LAB` is byte-identical to the reference `GAME.LAB`, SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`. EXE offsets are absolute file offsets. Brackets denote runtime storage.

## The gameplay modal

**EXE-confirmed.** In the post-transition fade/entry path, `0x747b–0x74c6` tests the OR of `[0x5427]` and `[0x520a]`. Both zero bypass this block. After the fade, only `[0x5427]==0` reaches the password routine at call `0x74c1 → 0x1eede`. The branch sets `[0x520a]=1` and `[0x5425]=0xffff` before calling it. This is the gameplay entry path, separate from the menu's password input.

The routine scans a count and 16 zone words at file `0x1d55c` (runtime `[0x1d35c]`), comparing the current zero-based zone `[0x51f0]`. A miss returns at `0x1efd7`. A match overwrites that zone entry with `0xffff` at `0x1eeff` and uses its unchanged ordinal to copy eight bytes from file `0x1d580 + 8*i` to the modal buffer `[0x236e6]`. At `0x1ef1b–0x1ef39` the copy is XORed byte by byte with ASCII `BLUE1993`. Nothing in this calculation reads the live player inventory, position, vitals, PRNG or clock. The modal displays the decoded code under the French resource string at file `0x238d2` (`Code d'accès rapide`), draws its rectangle in steps of four percent, waits for a nonzero input byte, plays sound 12 and calls the restore routine before returning (`0x1ef3a–0x1efd8`, drawing `0x1efd9` onward).

The used marker suppresses another display while the in-memory list stays consumed. Search of the reference image for the direct list pointer identified the display and successful continuation paths. An accepted continuation also consumes its own zone entry if it is present. Validation reads the independent code table, so the used marker does not prevent typing the same code again.

### Where the game gives the code

**EXE-confirmed, 2026-11-23.** Ordinary lift travel copies a different `(zone,sector)` pair before reloading and sets `[0x520a]=-1` at `0xd29e–0xd2d8`. This includes a lift journey within the same zone to a different sector. Choosing the current pair leaves the flag at zero. The successful level 22 → 23 shuttle sets the same arrival flag at `0xc784` after loading level 23, sector 1. The post-fade handler at `0x74c1` attempts the disclosure on these arrivals. `0x74af` changes the flag to 1 and ordinary player input later clears that value at `0x166f8–0x16706`. Merely walking into a sector or operating a code terminal has no such trigger. The terminal code interface at `0x92f0` and enquiry path at `0x9ace` explicitly clear the arrival flag. Their zone/equipment codes have six characters and separate tables.

On the first gameplay fade after a new or continued party, `[0x5427]=-1` from `0x6ac6` and `0x74ad` skips the disclosure. `0x75b2` then clears the initial-entry flag. Disclosure after a later arrival compares the **current zone only**, so its sector can differ from the starting sector hardcoded in the template.

**DATA-confirmed / EXE-confirmed.** The table below joins the EXE lift descriptors at `0xdacf + 54*id` with the stock LAB lift walls (family `0x3000`, descriptor id in the contents word). The lift ids are zero-based, as are sector indices. Each ordinary destination listed has a matching lift wall, wall 6 in its sector. The shuttle has a separate caller and arrives in sector 1 of level 23. These are possible disclosures when the code is still unused and the trip satisfies its normal route/card or shuttle prerequisites.

| Level | Code | Possible disclosure sectors and trigger | Continuation sector |
| ---: | --- | --- | ---: |
| 3 | `ATILAHUN` | 0 via lift 2, 22 via lift 3 | 0 |
| 5 | `ZENDOR56` | 0 via lift 4 | 0 |
| 6 | `STRADIUS` | 0 via lift 6, 10 via lift 4, 21 via lift 5 | 10 |
| 8 | `TERRAC12` | 0 via lift 6 | 0 |
| 10 | `CARTOON0` | 0 via lift 7, 1 via lift 8 | 0 |
| 11 | `GOREXZ73` | 0 via lift 8 | 0 |
| 12 | `ALTEA117` | 0 via lift 8, 1 via lift 9 | 0 |
| 15 | `RAMDAC38` | 0 via lift 11, 1 via lift 12, 2 via lift 13 | 0 |
| 18 | `EPSILON4` | 0 via lift 16, 1 via lift 15 | 0 |
| 21 | `RUBISCOP` | 0 via lift 17, 1 via lift 18 | 0 |
| 22 | `NEAREND1` | 0 via lift 18 | 0 |
| 23 | `CENTRAL7` | 0 via lift 20, 1 via shuttle, 2 via lift 21 | 1 |
| 26 | `ARKETYPE` | 0 via lift 21, 1 via lift 22 | 0 |
| 27 | `PRACTIX4` | 0 via lift 22 | 0 |
| 29 | `ANTARES5` | 0 via lift 24, 1 via lift 23 | 1 |
| 30 | `CLEENEND` | 0 via lift 24 | 0 |

For example, reaching level 6 sector 0 can show `STRADIUS`, but entering it at the menu always starts level 6 sector 10. There is no terminal to find at the destination. A normal campaign first reaches level 23 through the shuttle, so that is the earliest opportunity to see `CENTRAL7`. Its other destinations use the same code only if the process has not already consumed it. The same-zone different-sector case is supported by the EXE branch, but the shipped 25 descriptors contain no repeated zone in a single lift, and their wall panels match the descriptor destinations. It is not a distinct stock route.

**EXE-confirmed marker lifetime.** The two direct references to the count/list base `[0x1d35c]` are `0x1dc7d` and `0x1eedf`. Each only scans and replaces a matching word with `0xffff`. Menu return after a party at `0x7665` or `0x7700` branches back to the startup/menu code in the loaded image at `0x6a40`. The common party initializer at `0x6a4e–0x6b97` reinstalls the world file but does not restore this list. A new party in the same DOS process therefore keeps previously consumed markers. Relaunching the executable loads the original list again. Marker updates are outside all disk-write buffers.

## Original RUN choices and validation

**EXE-confirmed.** Select RUN from the parent menu (`RUN / OPTIONS / EXIT` at file `0x2369e`). RUN opens the party submenu at `0x1da3d`. Navigate with the configured up/down controls and confirm with the configured action (`0x1e285–0x1e359`). Each menu starts at row 0. Selection zero (Make new party) loads the template at file `0x1d8a0`, selection one (Continue with party) opens the code form, and Return to menu goes back to RUN/OPTIONS/EXIT (`0x1da7b–0x1da9d`).

The form starts with eight dots at `[0x2369c]`. `0x1db29–0x1dbc5` polls the keyboard character translator at `0x2a27`, plays BANK[12] for each nonzero character received, accepts ASCII digits and letters, uppercases lowercase letters, and sets a removed character back to a dot on Backspace. Enter validates only once all eight positions are filled. Extra characters and other keys are ignored. The loop has no Escape cancellation branch. The strings at file `0x23883`, `0x238a5`, and `0x238bc` call the input a Save Game filename and report acceptance or refusal.

`0x1dbc6–0x1dc26` XORs the eight input bytes with the same mask and compares all eight bytes against the count-prefixed table at file `0x1d57e` (runtime `[0x1d37e]`). A match records the ordinal in `[0x1d352]`, otherwise writes `0xffff`. The response modal displays Accepted or Refused. Call `0x1dc4c → 0x170a` waits for **140 VGA retrace cycles** through `0x16f3`. Call `0x1dc51 → 0x1f3b3` then shrinks and restores the modal without waiting for a key. The refusal branch at `0x1dc5e` returns to **the parent RUN/OPTIONS/EXIT menu** at `0x1d9e2`, correcting the old submenu description. On success, select the 42-byte template at file `0x1d600 + 42*i`, consume any matching zone entry in the display list, then install that template (`0x1dc64–0x1dca5 → 0x1de72`). `0x1defc` returns success, and `0x6a4e–0x6b97` proceeds to the common party initializer. The password text never goes to a DOS filename parameter.

The **gameplay disclosure** has a different dismissal rule. It polls `0x2a27` until a nonzero translated character at `0x1efb4–0x1efbb`, then plays BANK[12] and restores the scene. The **pause** menu is also separate. Its `Continue with party` row at file `0x2365c` returns to the live party via `0x7659 → 0x766a`; choosing the other row writes the active level at `0x7660` and returns to the title menu. It does not invoke the password form.

**Fixed codes.** The reveal and validation paths both consume the 16 embedded strings. Reveal looks up the current zone and decodes the corresponding string. Validation looks up the string and selects the corresponding static template. The only list mutation is an in-memory display marker. These paths have no writer of a new password, no derivation from player state and no progress gate on accepting a known valid code. A code may be used before reaching its level and can be used repeatedly.

## All continuation points

**EXE-confirmed** codes and starts. **DATA-confirmed** sector origin join against all 30 level records and bounds of every listed sector. Levels are one-based. Sector and floor are raw zero-based words. Heading uses 720 units per turn.

| Code | Template file offset | Level | Sector | Local XY | World XY | Heading | Floor |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `ATILAHUN` | `0x1d600` | 3 | 0 | 300, 280 | 2700, 3280 | 540 | 0 |
| `ZENDOR56` | `0x1d62a` | 5 | 0 | 300, 280 | 500, 3880 | 540 | 2 |
| `STRADIUS` | `0x1d654` | 6 | 10 | 300, 280 | 300, 2280 | 540 | 3 |
| `TERRAC12` | `0x1d67e` | 8 | 0 | 300, 280 | 2700, 4480 | 540 | 2 |
| `CARTOON0` | `0x1d6a8` | 10 | 0 | 280, 300 | 1280, 3700 | 360 | 0 |
| `GOREXZ73` | `0x1d6d2` | 11 | 0 | 320, 300 | 2920, 300 | 0 | 2 |
| `ALTEA117` | `0x1d6fc` | 12 | 0 | 320, 300 | 320, 300 | 0 | 1 |
| `RAMDAC38` | `0x1d726` | 15 | 0 | 320, 300 | 1520, 3700 | 0 | 2 |
| `EPSILON4` | `0x1d750` | 18 | 0 | 300, 280 | 1300, 1880 | 540 | 3 |
| `RUBISCOP` | `0x1d77a` | 21 | 0 | 280, 300 | 5080, 300 | 360 | 5 |
| `NEAREND1` | `0x1d7a4` | 22 | 0 | 280, 300 | 7280, 300 | 360 | 7 |
| `CENTRAL7` | `0x1d7ce` | 23 | 1 | 320, 300 | 320, 3100 | 0 | 8 |
| `ARKETYPE` | `0x1d7f8` | 26 | 0 | 300, 320 | 500, 3120 | 180 | 7 |
| `PRACTIX4` | `0x1d822` | 27 | 0 | 280, 300 | 2280, 1100 | 360 | 5 |
| `ANTARES5` | `0x1d84c` | 29 | 1 | 300, 320 | 1900, 2120 | 180 | 5 |
| `CLEENEND` | `0x1d876` | 30 | 0 | 300, 320 | 2300, 320 | 180 | 6 |

### Template payload

`0x1de72–0x1defb` consumes 15 little-endian words followed by three dwords, i.e. Python `struct.unpack_from("<15H3i", exe, 0x1d600 + 42*i)`. The new-party template has the same layout. Decode only from a local copy of the reference EXE.

| Offset | Runtime destination | Meaning |
| --- | --- | --- |
| `+0x00, 02, 04` | `[0x51da/51dc/51de]` | Local X, local Y, heading |
| `+0x06, 08, 0a` | `[0x51ee/51f0/51f2]` | Sector, zone, floor |
| `+0x0c, 0e, 10` | `[0x5453/5455/5457]` | Three card slots, `0xffff` empty |
| `+0x12, 14` | `[0x5459/545b]` | Two injector slots |
| `+0x16` | `[0x545d]` | Mine slot |
| `+0x18, 1a` | `[0x54cf/54d1]` | Ammunition, weapon index |
| `+0x1c` | `[0x5491]` | Health |
| `+0x1e, 22, 26` | `[0x544b/544f/548d]` | Battery, oxygen reserve, main oxygen |

Cross-references for item meanings include the three-card check in [shuttle prerequisites](elevator-cinematics.md#2-shuttle-level-22--23), injector tests `0x190ba–0x190eb`, and mine pickup `0x191a8–0x191da`. Stored `0xffff` or dword `-1` is the empty sentinel. These are fixed checkpoint values. There is no serialization of the current player's loadout into a newly issued code.

**EXE-confirmed correction (2026-11-23).** The old labels for the two equipment dwords were reversed. `0x18fc4–0x1900c` takes a battery from a cabinet into `[0x544b]`, and `0x16b5b–0x16bca` drains that slot when radar or optics is on. `0x1bd59–0x1bd6e` swaps `[0x544f]` with main oxygen `[0x548d]`. The table below has been checked against all 16 templates with these corrected labels. Of the continuation templates, only level 3 starts without a battery. Only level 10 has an oxygen reserve. Both powered modes start off, including on continuation (`0x6ab4–0x6ac5`). See [battery and optics evidence](colored-lights.md#optic-system-and-battery).

All loadouts are shown in raw game units. A dash denotes the empty sentinel.

The new-party record at `0x1d8a0` is level 1, sector 0, local `(300,320)`, heading 180 and floor 2. It has health 1000, main oxygen 288000, weapon index 0 with 500 ammunition, and all cards, injectors, bomb, oxygen reserve and battery absent. This record is separate from the 16 password ordinals and has no code.

| Level | Cards | Injectors | Mine | Weapon index | Ammunition | Health | Reserve | Battery | Main oxygen |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 3 | `0x0001`, `0x1002`, — | 100, — | — | 1 | 400 | 900 | — | — | 216000 |
| 5 | `0x0002`, `0x1002`, — | 100, 100 | 100 | 2 | 400 | 850 | — | 72000 | 288000 |
| 6 | `0x0003`, `0x1003`, — | 100, 100 | — | 2 | 500 | 700 | — | 57600 | 216000 |
| 8 | `0x0003`, `0x1003`, — | 100, 100 | — | 3 | 480 | 500 | — | 115200 | 115200 |
| 10 | `0x0004`, `0x1003`, — | 100, 100 | — | 3 | 350 | 900 | 288000 | 50400 | 72000 |
| 11 | `0x0004`, `0x1003`, — | 100, 100 | — | 3 | 380 | 750 | — | 14400 | 288000 |
| 12 | `0x0005`, `0x1003`, — | 100, — | — | 3 | 500 | 1000 | — | 144000 | 172800 |
| 15 | `0x0006`, `0x1004`, — | 100, 100 | — | 2 | 500 | 500 | — | 28800 | 288000 |
| 18 | `0x0007`, `0x1006`, — | 100, 100 | — | 4 | 460 | 500 | — | 100800 | 57600 |
| 21 | `0x0008`, `0x1006`, — | 100, 100 | — | 5 | 500 | 750 | — | 144000 | 288000 |
| 22 | `0x0008`, `0x1008`, — | 100, — | — | 4 | 500 | 1000 | — | 144000 | 288000 |
| 23 | `0x100b`, —, — | 100, — | — | 4 | 400 | 1000 | — | 144000 | 288000 |
| 26 | `0x0009`, `0x100d`, — | —, — | — | 4 | 300 | 800 | — | 72000 | 144000 |
| 27 | `0x0009`, `0x100d`, — | 100, — | — | 4 | 150 | 1000 | — | 14400 | 288000 |
| 29 | `0x0009`, `0x100d`, — | —, — | — | 3 | 500 | 1000 | — | 144000 | 144000 |
| 30 | `0x0009`, `0x100d`, — | 100, — | — | 4 | 400 | 500 | — | 28800 | 172800 |

## GAME.LAB during and between sessions

**EXE-confirmed.** New and accepted continued parties both reach the common initializer at `0x6a4e–0x6b97`. Security flags/count and the self-destruction timer reset, as detailed in [self-destruction](self-destruction.md). The six HUD damage markers at `[0x5493 + 10*i]` have their active and time words cleared at `0x6b55–0x6b8c`. Before loading the selected level, `0x6b92 → 0x78a6–0x7919` opens `data\data.lab` read-only, creates/truncates `game.lab`, copies it in chunks of up to 32000 bytes, and closes both. File-name strings are at file `0x4b5e` and `0x4b6c`. Consequently, an accepted continuation starts with pristine world records, including the current and previously visited levels' doors, pickups and enemies. It starts with the checkpoint's spawn and loadout. It does not recover live map/mission progress, security or countdown state, or the inventory and damage values immediately before death.

- **Load** (`0x791e–0x7b82`). Open working `GAME.LAB` read-only. Seek to `2 + 4*zone`. Read eight bytes as the start and next offset, seek to start and read exactly their difference into the level buffer. Resolve sector and load/setup associated tables and resource banks. All 30 pristine spans are between 890 and 22504 bytes.
- **Write/release** (`0x7b83–0x7c40`). Resolve current sector first. For each connection word, preserve only `0x0400` (unlock bit) at `0x7b98–0x7ba5`. Open working `GAME.LAB` read/write, read the same offsets, seek back to the selected record and overwrite its original byte span from the live buffer, then close and release the resource banks. This retains sector record edits in the current party while discarding the open bit and motion phase of doors. Global player state is outside that write. Call sites include ordinary lift `0xd3f4`, shuttle `0xc732`, and session exits `0x7620`, `0x7660` and `0x7686`. Another party overwrites this working file from the pristine copy.

### Disk operation audit

**EXE-confirmed, 2026-11-23.** The native disk wrappers pass DOS service numbers through the real-mode bridge at `0xb32`. Open `3dh` is at `0x12a8` (mode 0 for read, 2 for work-file update), close `3eh` at `0x12f8`, read `3fh` at `0x130f`, create/truncate `3ch` at `0x13e5`, write `40h` at `0x1436`, and absolute seek `42h` at `0x14be`. Rechecked the immediate callers of create/write and of the whole-file writer `0x1580`. The alternate empty-file helper at `0x1241` has no direct caller in the reference. The raw `CD 21` sites at the end of the executable belong to DOS loader/interrupt support (memory, termination, vector work, and file/directory enumeration). The gameplay file services use the bridge. No save-file rename/delete, slot enumeration, or additional player-state writer was found in the traced gameplay/menu paths.

| File or family | Read, create or change | Evidence |
| --- | --- | --- |
| `DATA\DATA.LAB` | Read at each new or accepted continued party. Remains pristine. | `0x78a7–0x78db` |
| `GAME.LAB` | Create/truncate then copy the complete source. Read directory offsets and one level span at load. Overwrite one span when leaving a level or the party. Directory and file length stay fixed for in-party writes. | `0x78b9–0x7919`, `0x791e–0x7995`, `0x7ba6–0x7c0b` |
| `EXTREMIS.CFG` | Probe/read 13 bytes on startup when available. Create/truncate and write 13 settings bytes on returning from Options. The buffer contains configuration fields such as sound/keyboard selection and bindings. | `0x67fe–0x6826`, `0x1df5e–0x1dfd1 → 0x1580` |
| `SNAPIC00.PIC` and subsequent indexed capture names | Optional screen capture from the ordinary loop at `0x75ad → 0x77c8`, gated by key-state indices `0x1d` and `0x68`. Create/truncate and write 64800 bytes: 32-byte PIC header, 768 DAC bytes and 64000 indexed screen bytes. Advance the two filename characters in memory. It contains no simulation state. | Filename at `0x7879`, capture `0x77c8–0x7878`, write call `0x7848 → 0x1580` |
| `DATA\` presentation and level assets | Read-only resource I/O. The whole-file reader `0x1504` serves original palettes, textures/sprites, masks, screen art and sounds. Streaming/indexed readers include `DATA\MONS<n>.SPR` and enemy sprites at `0x13570`, ANMs at `0x1471a`, and `DATA\MAP.SPR` at `0x1a6d2`. The disclosure/validation strings and templates are already in the loaded executable. | Whole-file reads use `0x1524`. Direct stream opens also pass mode 0. |

The write wrapper `0x1436` has three direct call sites: the `DATA.LAB` copy at `0x78f5`, the working-record write at `0x7c00`, and the generic whole-file write at `0x15ca`. The latter has just the capture and settings callers listed above. The code never opens a path supplied from the password field. Normal successful I/O is traced statically. Filesystem failure and interrupted physical writes were not exercised in DOS.

### State boundaries

| State | Elevator/shuttle or pause in the same party | New party or accepted password |
| --- | --- | --- |
| Enemies and generators | Travel writes the live 28-byte enemy records and mutated Entity A variants. Pause resume retains the live buffer. | The pristine LAB replaces all edited records in every level. No previous kills, reused records, or destroyed generator variants carry over. |
| Doors | Restore only unlock bit `0x0400` after travel. Pause resume leaves the live motion state. | Start from stock bits, normalized again by `0x7b23–0x7b37`. The stock LAB has 660 zero words and 14 words already set to `0x0400`. |
| Cabinets and weapon lockers | Live items or exchanged/empty contents remain in the working level. | All contents revert to the pristine data, regardless of what the template gives the player. |
| Player spawn and loadout | Travel sets the arrival coordinates/heading/floor. It retains carried items and current vitals. | Use the selected 42-byte record. There is no inherited live inventory, position, or vitals. |
| Radar/optics and campaign timers | Live globals outside the level record. Modal time uses the suspension logic. Successful shuttle has its own countdown exception. | `0x6ab4–0x6b8c` turns off radar/optics, resets victory/termination and text, starts surge at 9600, clears bomb shake, sets bomb state idle and clears the generator clock. |
| Firing state and HUD damage markers | Level load clears firing words `[0x5213/0x5215]` at `0x7b6f–0x7b80`. The shuttle clears the six HUD damage markers. | Templates contain weapon and ammunition only. Common initialization clears the HUD damage markers and the level loader clears the firing words. The separate player projectile pool `[0x5219]` is cleared on an interaction path at `0x1699d–0x169b0`. An unconditional fresh-party clear of that pool was not identified in the bounded trace. |
| Security, map and authorization | Separate party globals. Shuttle sets the completed security count to zero on success as documented. | Security flags/count clear and countdown returns to 48000. Writes at `0x6b2e–0x6b4b` clear map acquisition state (indexed from `[0x601b]`, see `0x1a217–0x1a225`) and the zone authorization state indexed from `[0x6039]`. Neither comes from the template. |
| Used password display markers and user settings | Process state and configuration. | Display markers survive a new/continued party in the same process. Settings are separate in `EXTREMIS.CFG`. |

**Result.** A password selects a preset starting point for a fresh party. The working `GAME.LAB` provides temporary world persistence while that party runs. The pause menu resumes live memory. The executable exposes no path that reloads an old working LAB as a full save or serializes a complete live party. Even after a party exit writes the current record, the next party replaces the working LAB before loading its first level.

## Port integration and open checks

**Port implementation (2026-11-23).** `LOAD GAME` opens an eight-character editor on the custom title and pause menus. `port_menu_state.cpp` handles input and validation. `port_menu.cpp` holds acceptance or refusal for two seconds. The result returns a typed checkpoint on success or returns to the main page on failure. Continuation skips the startup story and uses the fresh-party path in `app_session.cpp` and `session_travel.cpp`. `continuation.cpp` contains the 16 documented records and eligible arrival sectors. Inventory and vitals install raw values without the pickup conversion.

The per-process disclosure history belongs to `AppSession`, outside the party reset. A committed elevator or shuttle arrival can set a pending notice based on the destination zone and sector. `session_frame.cpp` draws it over the destination. The loop pauses simulation while it is pending and discards carried input and elapsed time when dismissed. Acceptance consumes only that zone's notice, and lookup remains available. `BACK TO GAME` still resumes live state.

**Port-tested, session persistence.** On cross-zone travel, the app
keeps the departed level in memory with its live enemy records, panel
contents and door unlock bits. Returning rehydrates those records before
building destination resources, and NEW GAME clears the cache. The current
snapshot also copies the live Entity A list, including any changed generator.
Same-zone travel uses the live world. An interrupted transition leaves the
saved records available. This uses the external LAB as an immutable resource.
Continuation and NEW GAME both reload the pristine data and clear the
previous in-memory snapshots.

### Implementation contract for the custom menu

1. Define a validated, ordered table of the 16 codes and their 42-byte payloads with the field types above. Keep codes/ordinals, initial disclosure zones and spawn sectors explicit. A separate record supplies new-party defaults. Keep DOS resource decoding in the game code. The engine does not participate.
2. Route the custom menu's LOAD GAME through a separate eight-character editor. Keep the terminal interface on its own six-character input and state. Implement the native accept/refuse rules and an explicit accepted-checkpoint result. Validate before changing the active party or its cache. The current custom menu can present the outcome in its own style. Divergences such as adding an Escape action to the code editor can be documented as adaptations.
3. Stage the accepted start from the pristine LAB using the specified sector, local coordinates and heading. Preserve the floor in checkpoint metadata for any logic that needs it. An arrival through this route has no elevator/shuttle origin and plays no travel cinematic. Apply health, main and reserve oxygen, cards, injectors, battery, bomb slot, weapon and ammunition as exact template values. Convert raw empty sentinels to the inventory's absent representation. Do not reapply cabinet pickup conversion or replacement rules to template dwords.
4. Commit as a fresh party. Clear the level snapshots, reset the party globals listed above, and make the new destination and loadout live together. The installed charge does not enable radar or optics. A bomb in inventory starts unarmed. The loaded code consumes its zone's disclosure marker without displaying a password on the first frame. Resume remains a separate action that preserves the live party.
5. Track the 16 disclosure markers for the executable lifetime, outside the fresh-party state. After a committed gameplay lift or shuttle arrival, look up the destination zone. For an unused hit, consume its marker, show the code in a blocking modal and dismiss with translated input. Clear any carried input and suppress simulation catch-up on return. A canceled transfer, first-party entry, RESUME, walking, and the terminals do not trigger a disclosure. Keep cross-zone snapshots in memory. The DOS work file, configuration and screen capture have no role in the password path of the custom menu.

**Port-tested.** Debug and Release passed CTest 45/45 each. `continuation_tests EXTREMIS.EXE GAME.LAB` in both builds matched the original fields and all 29 destinations. A scripted SDL software run exercised the title menu, an invalid submission, lowercase entry with Backspace, `CARTOON0` at level 10 sector 0, and NEW GAME from pause. A Debug probe linked the app objects and exercised all 16 committed starts with original level resources, an original elevator sequence, the successful shuttle arrival handoff and repeated arrivals. It verified that a round trip keeps edits and a continuation discards them. See the [implementation checkpoint](../research-checkpoint.md#password-continuation-port-implementation).

**Adapted.** The custom editor uses the port's menu graphics and a two-second response hold in place of the VGA retrace wait. The arrival notice uses a static overlay and a fresh non-repeat SDL key for dismissal. The DOS rectangle animation and palette transition are not reproduced. Port-level temporary persistence stays in memory.

**Unknown / DOS observation pending.** Measured fade and modal durations, VGA presentation order and disk-failure behavior remain unobserved. The targeted DOS comparison is to reveal `STRADIUS` on lift 4 or another listed arrival, leave and start a new party in the same process, and compare with a fresh launch. Enter one refused and one accepted string and observe automatic response dismissal. Trace DOS file services at bridge `0xb32` through one of those starts and one lift, then separately take a capture and exit Options. To check a possible transient carryover, leave a party with a live shot and watch `[0x5219]` through the next party's first update. No running DOS session was available in this intervention. A same-zone transfer between different sectors requires a controlled fixture, since stock lift data has no such route.

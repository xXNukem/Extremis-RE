# Enemy generators and inventory bomb

Static trace and all 30 LAB records examined on 2026-11-20. Reference hashes are in the [EXE index](../exe-index.md). EXE and LAB offsets are absolute file positions. Brackets denote DOS runtime addresses. Levels are one-based, sectors and walls zero-based.

## Generator table and original records

**EXE-confirmed.** `0x11393–0x1139d` sets `[0x51a2]` to level base plus header word `+0x10`. Its payload is three fields in order: a word count, a word enemy health, and that many words of sector indices. Both consumers take **the first Entity A record** from each listed sector, via `0x11335 → [0x51c2]`. Neither consumer searches for an OBJET type or checks the entity count. Both test the low byte of the first record's type for zero. They use its vertex index to locate the generator. A bomb changes the low byte to one while preserving the upper byte.

**DATA-confirmed.** List order is significant to slot allocation. Below, entries have the form sector at absolute LAB Entity A offset. Unless stated otherwise, the first entity is type `0x5000`, with both parameters zero. All unlisted levels have a zero count. There are 24 table entries across 12 levels, 21 matching the active-entity predicate in the shipped data.

| Level | LAB table | OBJET / MONS | Spawn health | Sectors in table order |
| --- | --- | --- | --- | --- |
| 5 | `0x698e` | 1 / 1 | 2 | 12 @ `0x792e` |
| 10 | `0x104b8` | 6 / 4 | 10 | 29 @ `0x12cc2` |
| 12 | `0x145e6` | 2 / 3 | 9 | 22 @ `0x165e8`, 29 @ `0x1759e` (`0x5008`, skipped) |
| 16 | `0x1f824` | 8 / 3 | 12 | 25 @ `0x221c6` |
| 18 | `0x23f0e` | 8 / 7 | 15 | 4 @ `0x24436` |
| 19 | `0x25dd8` | 5 / 8 | 5 | 3 @ `0x2609c`, 9 @ `0x269da` |
| 21 | `0x27de8` | 5 / 4 | 5 | 9 @ `0x289dc`, 16 @ `0x29344`, 21 @ `0x299a0` |
| 22 | `0x2a33c` | 5 / 5 | 10 | 34 @ `0x2d134`, 30 @ `0x2cae6`, 11 @ `0x2b1ba`, 38 @ `0x2d99a`, 8 @ `0x2ae32` |
| 23 | `0x2efa6` | 4 / 7 | 10 | 10 @ `0x2fe72`, 16 @ `0x30726`, 42 @ `0x32e64` |
| 26 | `0x360fc` | 7 / 8 | 10 | 15 @ `0x376c8` |
| 27 | `0x37ddc` | 7 / 5 | 10 | 4 @ `0x383e6`, 27 @ `0x3a03e` |
| 29 | `0x3d752` | 9 / 8 | 10 | 2 @ `0x3ddfa` (`0x5017`, parameters 256 and 0, skipped); 5 has zero entities at `0x3e2f6` |

In level 29 sector 5 the unchecked pointer reaches the polygon count (7), then the first polygon offset (16). The read at pointer `+2` therefore sees low byte 16 and skips the sector in the shipped map. Level 29 also has zero mode-4 records, so none can be recycled there. No correction to the LAB table is implied.

Reproduction: read the count and directory offsets as `u16(0)` and `u32(2+4*i)`. For each level bound the table at `base+u16(base+0x10)` by the next level offset. Read the sector directory at `base+0x14+6*s`. At the sector record, skip the leading neighbour array (2 bytes per entry), vertex array (4), and wall array (8), including their counts. Read the Entity A count and the following 8-byte record if present. Check `u16(record+2)&0xff` against zero. Inspect all `u16(level+0x0c)`-indexed enemy tables for health and mode (first two words; records start after the count and are 28 bytes).

## Campaign inventory of generators and bombs

**DATA-confirmed, 2026-11-23.** Joined the existing generator list with the local coordinates of each first Entity A vertex and the level's mode-4 pool. Independently walked the wall records in all 30 pristine level spans for supply cabinets (wall family `0x3200`, contents high nibble 6). All 39 matches are `0x3205` with contents `0x6064`. The table uses sector and wall indices from the LAB, both zero-based. Each unqualified sector in the third column starts as `0x5000` and is eligible after the visibility and slot checks. The slot count is the total number of mode-4 records in the level, including living ones. At pristine load none of those slots is free.

| Level | Registered / eligible | Sector (local X,Y) in order | Spawn bank, health, mode-4 pool | Bomb cabinets (sector:wall) |
| ---: | ---: | --- | --- | --- |
| 1 | 0 / 0 | — | — | — |
| 2 | 0 / 0 | — | — | — |
| 3 | 0 / 0 | — | — | — |
| 4 | 0 / 0 | — | — | 4:15 |
| 5 | 1 / 1 | 12 (200,200) | MONS1, 2 HP, 4 slots | — |
| 6 | 0 / 0 | — | — | — |
| 7 | 0 / 0 | — | — | — |
| 8 | 0 / 0 | — | — | — |
| 9 | 0 / 0 | — | — | — |
| 10 | 1 / 1 | 29 (200,400) | MONS4, 10 HP, 4 slots | 9:11 |
| 11 | 0 / 0 | — | — | — |
| 12 | 2 / 1 | 22 (100,500), 29 (400,800) inactive 0x5008 | MONS3, 9 HP, 4 slots | 32:15 |
| 13 | 0 / 0 | — | — | — |
| 14 | 0 / 0 | — | — | — |
| 15 | 0 / 0 | — | — | — |
| 16 | 1 / 1 | 25 (600,200) | MONS3, 12 HP, 5 slots | 12:56 |
| 17 | 0 / 0 | — | — | — |
| 18 | 1 / 1 | 4 (200,500) | MONS7, 15 HP, 2 slots | 8:15 |
| 19 | 2 / 2 | 3 (700,400), 9 (200,400) | MONS8, 5 HP, 4 slots | 7:5, 7:28 |
| 20 | 0 / 0 | — | — | — |
| 21 | 3 / 3 | 9 (100,500), 16 (300,300), 21 (600,200) | MONS4, 5 HP, 9 slots | 17:11, 17:12, 17:13 |
| 22 | 5 / 5 | 34 (300,900), 30 (1400,600), 11 (100,300), 38 (1500,800), 8 (700,500) | MONS5, 10 HP, 6 slots | 2:24, 39:4, 41:5, 43:8, 43:23 |
| 23 | 3 / 3 | 10 (900,500), 16 (450,650), 42 (200,400) | MONS7, 10 HP, 3 slots | 32:5 |
| 24 | 0 / 0 | — | — | 6:24 |
| 25 | 0 / 0 | — | — | — |
| 26 | 1 / 1 | 15 (1100,900) | MONS8, 10 HP, 2 slots | 5:3 |
| 27 | 2 / 2 | 4 (100,700), 27 (200,400) | MONS5, 10 HP, 3 slots | 10:6, 12:18 |
| 28 | 0 / 0 | — | — | — |
| 29 | 2 / 0 | 2 (2300,3300) inactive 0x5017, 5 no Entity A | — | — |
| 30 | 0 / 0 | — | — | 1:16, 2:41, 3:8, 4:24, 5:14, 6:9, 6:11, 7:5, 7:17, 8:5, 8:17, 9:9, 9:11, 10:17, 10:19, 11:13, 11:22, 12:18, 13:16 |

The 39 cabinets are separate from the level 5 continuation template at EXE file `0x1d62a+0x16`, which starts with a bomb already in inventory (word 100). A new party starts with an empty slot. Examples of cabinet offsets are level 4 sector 4 wall 15 at LAB `0x5e3a`, level 10 sector 9 wall 11 at `0x110ee` and level 30 sector 13 wall 16 at `0x4023e`. To reproduce all offsets, skip the sector's neighbour and vertex arrays, then take `wallCountOffset + 2 + 8*wall`. The contents word is at record offset `+6`.

## What the player sees

**EXE-confirmed / DATA-confirmed.** The live generator is a solid floor sprite. The sector's Entity A record supplies its vertex and variant. All 21 eligible anchors have `Param1=0` and no anchor is on a segment of its sector's wall geometry. For example, level 5 sector 12 uses vertex 10 at local `(200,200)`, world `(1200,3800)`, from record `0x792e`. It shares neither wall geometry nor a separate hole record with the spawn rule. The dark opening is part of the sprite.

The Entity A projector reads the low type byte at `0x11f73–0x11f89`. The OBJET draw path at `0x129f1–0x12a1e` uses that byte plus one in the DOS drawing API (images 0 and 1 in a zero-based decoder). Decoding the first two images in original `OBJET1/2/4/5/6/7/8.SPR` with their level palettes shows the same design across all seven banks. Image 0 is a ribbed low ring with a dark oval mouth (84×34). Image 1 shows broken, raised pieces around the same base (84×31). Its collision extent and solid flag remain after the visual change. These are observations of decoded resources. Appearance from the running DOS camera remains unobserved.

The type word alone does not identify a nest. Level 30 sector 4 uses `0x5001` in OBJET10 (LAB `0x3f0f8`) and sector 18 uses `0x5000` (`0x40950`). OBJET10 images 0 and 1 are tall organic forms (60×119 and 65×86), and level 30 has no registered generators. Likewise level 12 sector 29 and level 29 sector 2 have different initial sprite indices and are skipped by the generator routine.

**EXE-confirmed ordinary shots.** The projected OBJET hit branch at `0x17dd3–0x17e68` accepts the solid bit `0x1000` and a matching projected rectangle/depth, sets projectile state 2 and phase 1, and plays sound 8. It makes no write to the source Entity A. An ordinary shot can hit the solid nest without changing its variant. The enemy damage branch at `0x17f77` instead resolves a 28-byte enemy slot via `[0x519e]`. The OBJET animation writers at `0x11df4–0x11f3b` have bank/variant gates that exclude all intact and broken generator indices. The confirmed mutator of the nest image is the bomb path at `0x103ef–0x103f5`.

## Reusing enemy slots

**EXE-confirmed.** Gameplay calls `0x10584` at `0x7021` immediately before enemy update. With a nonempty table, it adds word `[0x1428]` to word `[0x60a5]` and uses a signed comparison to wait for 800 units. At threshold, resets the accumulator to zero, discarding excess. Starting a party clears it at `0x6b07`. It walks all listed sectors once per pass, skipping first record variants other than zero and sectors already listed in `[0x5128]` (eight-byte entries). This is the previous connected-sector set consumed by enemy activation. The current frame rebuilds it after this call at `0x706a–0x70c5`.

`0x10603–0x10673` selects the **first** enemy slot with health `0xffff` and **behaviour** `+2 == 4`. It sets health to the generator table's shared health, state `+4` to **6** (the normal waiting state), phase to zero, sector and local X/Y to the generator vertex, and polygon to `0xffff` for subsequent lookup. Remaining record fields are retained, including home position, angle, and behaviour. The level's MONS bank still determines the enemy family. No eligible slot means no spawn. A pass can reuse one slot per eligible generator, in table order, and can repeat indefinitely as mode-4 records die again. The shipped LAB has no already-removed mode-4 slots at load. This is bounded by the original enemy record pool.

**EXE-confirmed limits.** `0x1058b–0x105a5` skips incrementing `[0x60a5]` if the table count is zero. A nonempty table still accumulates time when all its entries are disabled or in the connected set. After reaching 800 it discards the excess even when no spawn succeeds. An already removed mode-4 slot is required. A living enemy or one still in its death state with health zero is not available. There is no separate spawn radius, per-nest timer, random choice, or spawn quota in this pass. A connected sector excludes spawning regardless of whether the sprite is within the camera frustum. The accumulator is common across levels and resumes from its previous value when a level with a nonempty table is loaded.

## Pickup, placement, and explosion

**EXE-confirmed.** Interact with the supply cabinet and confirm the transfer. The cabinet dispatch at `0xa52f–0xa55d` checks for empty contents before opening the supply modal. It has no card or generator check. Inventory transfer `0x191a8–0x191db` tests mine slot word `[0x545d]` against `-1`. On pickup it sets the cabinet contents `[wall+6]` to `0xffff` and stores 100 in the slot. If already occupied it leaves the wall intact and sets inventory notice 4. The other write that can fill that slot is the template install at `0x1deca–0x1ded1`. The direct operands of `[0x545d]` resolve to these paths, inventory placement/inspection, and the HUD check at `0x1430c`. No separate floor pickup or drop was found.

See the [campaign table](#campaign-inventory-of-generators-and-bombs) for cabinet locations.

The inventory panel displays the slot through `0x1c848–0x1c8b6` (`Pas de bombe` or `Bombe opérationnelle`). Its bomb action at `0x1bfbc–0x1c027` checks availability, opens a choice with `Utiliser bombe` and `Ne pas utiliser`, then on acceptance clears the slot to `-1` and sets state word `[0x6097]=0`, timer dword `[0x6099]=0`, and position words `[0x609d,0x609f,0x60a1]` to player sector and local XY. There is no weapon equip action or geometry/nest proximity check in placement itself. Opening inventory is suppressed when `[0x5200]` indicates an enemy in the connected set (`0x6efd–0x6f17`, set by `0xe05e–0xe0c0`). Only one placed bomb state exists. This acceptance branch does not guard against overwriting it. Post-inventory continuation queues the explosion warning at file `0x21ac5` if state is zero.

**EXE-confirmed arming presentation and fuse.** After the choice, the armed path at `0x1c029–0x1c124` closes the inventory presentation and jumps to the blocking `A_BOMB.SPR` sequence at `0x1d0cc–0x1d356`. It plays direct BANK[28] and BANK[14] at `0x1d226` and `0x1d23c`, then returns through the inventory cleanup. This sequence does not advance the live fuse. Main-loop return at `0x6f60–0x6f86` replaces queued text with the armed warning (file `0x21ac5`, **5 SECONDS BEFORE EXPLOSION!**) and resets the gameplay delta to 1. The actual live compare at `0x10217` is **800 units**, nominally five seconds using the roughly 160 Hz PIT. The manual's stated ten seconds differs from both the warning and the threshold in this EXE. No elapsed wall-clock measurement is implied. The separate shock warning is at file `0x21ab0`, requested at `0x167c7–0x167cc`.

`0x712f → 0x101e4–0x104f1` uses the following states. Timers use zero-extended word `[0x1428]`, and elapsed excess is discarded at boundaries. On phase transitions execution falls through into the next phase in the same invocation, which can add the same frame delta again to the freshly reset counter.

| State | Timed rule | Effect |
| --- | --- | --- |
| `-1` | Idle | No bomb update. Initialized by `0x6afe`. |
| 0 | Until 800 | Requests BANK[14] each update. Places projected item `0x8000` at the saved coordinates with size word 50. |
| 1 | Until 100 | Starts with BANK[29]. Alternates `0x8001/0x8002` by timer bit 2, size word `50+4*t`, blend to VGA white of amount `min(100,30+t)`. |
| 2 | Until 160 | Damage applies once on entry. White blend decreases as `min(100,160-t)`. At threshold returns to idle. |

`BOMBE.SPR` loads at `0x695b–0x697b`; during drawing `0x12a6f–0x12a99` takes projected sprite ID low byte plus one. `A_BOMB.SPR` is the separate inventory presentation opened at `0x1d0cc`. While in phase 1, `0x10370–0x1037e` sets dword `[0x6093]` to `-1` if zero. The player update at `0x167bd–0x16842` uses that flag to post the shock warning, perturb two view offsets by `(rng & 31)-16`, vary the height target by `(rng & 47)-32` clamped to 100–165, and reduce the signed timer until it drops below `-320`. The flash and shake paths are independent of damage distance. **DATA-confirmed:** the three `BOMBE.SPR` images measure 19×33, 49×53 and 66×62 in order. The separate `A_BOMB.SPR` contains eight images, starting with a 196×100 panel.

**EXE-confirmed.** `0x10513–0x10581` adds sector origins then uses the maximum of the two absolute XY deltas, with 16-bit arithmetic. For ordinary coordinates: strength = `100-floor(distance*100/800)` for distance <800, otherwise zero. No wall, Z, facing, or same-sector check participates. `0x10398–0x103fe` changes generator variant at strength ≥75 (distance ≤207). `0x10400–0x1043f` subtracts `floor(strength*500/100)` from player health. `0x1043f–0x1049c` subtracts `floor(strength*20/100)` directly from each nonremoved enemy, writes `0xffff` for a negative result, and leaves a result of exactly zero as zero. This blast path does not enter the ordinary hit reaction. The nest mutation immediately excludes that record from the periodic spawn pass. The damage routine writes no progress counter or door state.

| Horizontal distance in map units | Strength | Nest eligible for destruction | Player damage | Enemy damage |
| ---: | ---: | --- | ---: | ---: |
| 0 | 100 | Yes | 500 | 20 |
| 207 | 75 | Yes | 375 | 15 |
| 208 | 74 | No | 370 | 14 |
| 799 | 1 | No | 5 | 0 |
| 800 or more | 0 | No | 0 | 0 |

Place anywhere the inventory permits. There is no particular interactable point on the sprite. At detonation a nest can be hit from another sector or across a wall if both world XY differences are within 207. Too far from the nest still consumes the bomb and plays the effects, with any player and enemy damage calculated independently. **DATA-confirmed:** the closest pair of eligible nests in the same stock level is 900 units apart (level 21 sectors 9 and 21), so no regular blast position overlaps two eligible nests. The blast changes the sprite in the level buffer. It neither removes the remaining obstacle nor despawns previously generated enemies.

## Persistence and progression

**EXE-confirmed.** The blast mutates the loaded level buffer. The same whole level record is written by `0x7bf7–0x7c04`, after only door-state words are normalized at `0x7b90–0x7ba5`. Elevator travel calls this writer at `0xd3f4` and reloads at `0xd62f`, including a same-level transfer with a different sector. Successful shuttle calls write at `0xc732` and read at `0xc77f`. Within the DOS party, the disabled generator and the recycled/removed enemy records therefore survive a write/reload of their level. This persists map mutations in the working LAB, as described in the [travel trace](elevator-cinematics.md#1-conventional-elevator-a_level).

An active bomb is separate from that map buffer. The placement stores only sector and local XY, and the blast resolves the sector origin against the *current* level buffer `[0x4975]` at `0x10515–0x10543`. Neither `0x791e–0x7b82` nor the elevator/shuttle caller resets the bomb or the shared spawn accumulator. Direct operand scans for `[0x6097]`, `[0x6099]`, `[0x609d]` and `[0x60a5]` corroborate their lifetime. The new-party entry resets the two states at `0x6afe–0x6b0f`. Consequently, travelling while a bomb is armed leaves its timer and sector index active against the destination level; what the original does when that index is outside the destination directory still needs a DOS test. It would be unsafe to turn this into a deliberate sector remapping rule.

The only direct operand occurrences of generator-table pointer `[0x51a2]` are the assignment at `0x11399` and its reads at `0x10398` and `0x10585`. There is no independent generator counter or completion gate in these paths. Rendering uses the new image variant. Physical blocking retains the original half-extent of 200: the first two extent words match in every enabled generator bank (file offsets `0x5d4f`, `0x5da5`, `0x5e7f`, `0x5ed7`, `0x5f31`, `0x5fa5`, `0x6009`), and the image change preserves the blocking flag. A mode-4 enemy removed by the blast still leaves a recyclable slot for any other intact generator. Level 30 has 19 bomb cabinets but no generators. Its sole MONS9 enemy starts at 5000 health (LAB `0x3e61c`), so the 20-point blast damage remains relevant there without any generator requirement.

The traced campaign gates do not require nest destruction. Conventional elevator admission uses its destinations and card/route checks. The shuttle reads the completed security count and the three card slots. Victory is latched by the MONS9 enemy finishing its death phase at `0xe25e–0xe27e`. Nest destruction can prevent replacement enemies in that area, and connected enemies can block opening the inventory, but these are combat and access effects rather than nest objectives. [Elevator and shuttle gates](elevator-cinematics.md), [victory trigger](ending.md).

## Port comparison and remaining checks

**Current port inspected (2026-11-23).** The old pickup-only status is superseded. `game_lab.cpp` reads header `+0x10`. `enemy_generators.cpp` implements slot reuse, blast damage and the `0x5000 → 0x5001` change. `item_session.cpp` requests placement and `placed_bomb.cpp` advances fuse, explosion and afterglow. `session_bomb.cpp` updates the sprite, plays BANK[29] and applies the damage. `session_travel.cpp` copies the mutated level records and enemies to an in-memory snapshot on cross-zone travel. Same-zone travel keeps the live level instead of writing and reloading a normalized working LAB. The session retains the bomb and accumulator on ordinary travel and resets them for a new party.

The port closes its menu straight into gameplay after confirmation, so the blocking DOS `A_BOMB.SPR` animation is missing. It computes a current connected-sector set for spawning where DOS consumes the preceding set. Its shock uses a separate random stream and adjusts the render camera in `session_frame.cpp`, where DOS changes proposed movement and eye height. Its generic damage handler clamps player health at zero where DOS subtracts from a word. It rejects an empty Entity A list rather than reading past it. The cleared/skipped stock level 29 entry agrees in ordinary outcomes. These are source comparisons, not new live port results.

**Directed EXE/DATA check.** At `0x10515–0x10543` the two little-endian
sector origin words are read at destination record offsets `0x16+6*s`
and `0x18+6*s`, independent of the sector count. The largest source
sector index in the 30 records is 57 (level 23). The largest exclusive
read bound is therefore `0x170`, inside even the 890-byte level 20
record. A stale stock sector outside the directory reinterprets subsequent
bytes of that level record as coordinates. The placement projector separately
scans connected sectors and returns on a miss at `0xf8bf–0xf8da`.
Cross-level execution with an armed bomb remains DOS-unobserved.

**EXE-confirmed / DATA-confirmed example.** The placement stores no zone. While the elevator is blocking, the fuse is suspended. On resuming it continues with its saved sector/local XY, but the projection and damage run against the current zone. With a valid saved sector index, the sprite can appear at that index in the destination if it is in the connected list. With an invalid index the sprite has no matching sector and is skipped. The flash and alarm/state progression still run. Blast damage uses the out-of-directory origin words. The source level cannot receive the blast while unloaded. If the player returns before the fuse finishes, the routines again use the original zone.

The EXE lift descriptor 3 at `0xdb71` connects level 3 sector 22 to level 4 sector 9, with local arrival `(300,320)`. Level 4 has ten sectors. A controlled placement with saved sector 22 and local `(300,320)` would read level 4 LAB `0x58c2/0x58c4` as origin `(4,700)`, giving blast world `(304,1020)`. Sector 9's actual origin is `(800,0)`, so the stationary arrival at `(1100,320)` is at max-axis distance 796. If both positions are still those values when damage runs, the expected strength is 1 and player damage is 5. The data and instructions establish the branch and arithmetic. This example has not been played through in DOS.

The port implements the stock stale-index lookup by retaining the extra origin pairs from the source LAB bytes. DOS reads the current working buffer. Where an out-of-directory pair aliases mutable data, later writes can therefore produce a discrepancy. For example level 20 has two sectors. The stale index 3 reads relative `+0x28/+0x2a`, the connection count and first connection state. The port's retained pair does not follow live door mutations there. No ordinary player route to that exact alias was established.

**Unknown / DOS observation pending.** In level 5 sector 12, remove a mode-4 enemy, trace `[0x60a5]` while sector 12 enters/leaves the connected list, and watch the reused slot when the counter reaches 800. Fire at entity 0 and check the type stays `0x5000`. From a fresh snapshot place at local `(407,190)` and `(408,190)` on separate runs, or set the position directly in a debugger if the game cannot be stopped at those coordinates. Wait for phase 2, read the live type word corresponding to LAB offset `0x7930`, then travel out/back and read it again. Level 29 is a negative data control. For transition behaviour, exercise the lift case above while watching `[0x6097..0x60a1]` and the current level base `[0x4975]`. Capture the actual fuse cadence, animation and audio ordering during the same runs.

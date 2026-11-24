# OBJET1 proximity lights (2026-11-20)

**EXE-confirmed / DATA-confirmed.** OBJET1 images 2 and 3 drive red and green screen-wide VGA palette effects. Their position is tested by the player OBJET pass in the source sector. The native loop chooses the first qualifying record and applies its tint to the entire active gameplay palette. Code and asset results below were checked against the indexed reference EXE and LAB. No running-DOS timing measurement was made.

Reference SHA-256: `EXTREMIS.EXE` `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`; `GAME.LAB` `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`. `OBJET1.SPR` SHA-256 `7a3a5f1d544da7b361fbe94eec8cc9a5eeb99d8894dbf3e2e4d4ce98f60dd0f5`; `PAL1.PAL` SHA-256 `1e321d969c890ca86f339f9ab22a0002d938241fa4bc3c1f0988387c3f4d570b`. Offsets are absolute file positions. Brackets denote runtime addresses.

## Position and record selection

1. `0x7039 → 0x1656a` runs player movement. On the normal movement branch, `0x16848` clears lamp-active word `[0x520c]`. `0x16851–0x1688c` passes mode `0`, original sector `[0x51ee]`, old XY and proposed XY to the OBJET pass at `0x10db7`. This precedes the wall/connection pass at `0x168e8`. A stationary player still reaches the normal per-frame check. Health-hit recoil takes the branch at `0x166bb` around this pass and retains the previous lamp state until a normal update. Its own transient palette request takes priority while active.
2. `0x10dc1 → 0x11335` resolves sector data. The loop reads the eight-byte Entity A records in stored order. It obtains local XY by indexing the sector vertex array with the record's first word, excludes `type & 0x2000`, and uses the bank-specific collision half-extent from `[0x5b1f]`. For these two images both extents are `200`. The outer comparisons at `0x10e61–0x10e8f` implement half-open boxes using 16-bit subtraction and the sign flag. A set `0x1000` flag takes the physical collision branch at `0x10f3b` instead of the lamp branch.
3. `0x10ea3–0x10ed3` requires mode zero, no earlier winner (`[0x520c] == 0`), OBJET bank word `[0x5190] == 1`, and low type byte 2 or 3. In `0x10ed5–0x10f0b`, take the 16-bit absolute coordinate differences from the proposed XY as it stands when this record is reached, then the larger difference `d = max(abs(dx), abs(dy))`. The amount is `80 − (d >> 1)`. A result ≤0 is rejected and the code caps positive results at 80. The effective discrete box is `[-159,+159]` on both axes. Sample amounts at `d = 0, 1, 2, 80, 159, 160` are `80, 80, 79, 40, 1, inactive` respectively.
4. `0x10f0c–0x10f36` stores active `1` in `[0x520c]`, the amount word in `[0x520e]`, and three byte channels in `[0x5210..0x5212]`. Image 2 is `(63,0,0)` and image 3 is `(0,63,0)`. The flag disables selection by later records even if they are closer. The light branch writes no position, damage, inventory or timing state. It does not consult Z, view angle, sprite visibility, line of sight, `Param1` or `Param2`. Connectivity matters only through the sector supplied to the pass. The same function also serves enemies and fragments, whose nonzero mode excludes the light branch.

## Palette arbitration and exact arithmetic

`0x7ad4–0x7ae7` loads the level's 768-byte `PAL<n>.PAL` into `[0x4af4]`. `0x7ae8–0x7b13` builds a green optic-system palette at `[0x4df4]`, using `(0,min(63,2*G),0)` for each entry. `0x73e7–0x73ff` selects the current source `[0x50f4]` from those two using optic-system flag `[0x545f]`. The instrument panel and the rest of the composed indexed framebuffer share the VGA DAC.

- `0x747b–0x74dd` handles the special palette fade on initial entry or the related palette-transition path before normal arbitration.
- `0x74e2–0x7572` first checks the transient request word `[0x53f9]`. Any nonzero value selects its amount `[0x53fb]` and RGB `[0x53fd..0x53ff]` in preference to the lamp. This covers, for example, health-hit white flashes (`0x16665–0x1669b`) and the power-surge darkening request (`0x16c04–0x16c38`). `0x16584` clears the transient request on entry to player movement. Later effect writers can replace each other; no blend accumulates on the physical DAC state.
- With no transient request, `0x74ed–0x757e` checks optic-system flag `[0x545f]` and then lamp-active `[0x520c]`. If optics is on (`-1`), the check bypasses the lamp even when the lamp branch has set it. Otherwise a live lamp request reaches `0x7574` and supplies the RGB and amount described above.
- When a blend is applied, `0x75a4` marks `[0x5425] = -1`. When neither palette request applies, `0x7504–0x752c` sees `-1`, reinstalls the current source palette with `0x1796` and clears the dirty marker. In the normal movement path this restores the base colours on the first frame outside all lamp boxes. If `[0x5425] == -2` instead, `0x752e–0x7565` only refreshes palette entries 73–81 for the separate DALLE5 cycle. The DALLE5 palette edits at `0x7400–0x747a` occur before arbitration and thus flow through any transient blend.

Both full blends pass the source pointer, RGB words and unsigned amount `w` through `0x759c → 0x14611–0x146c2`. Per channel, using unsigned integer divisions and DAC inputs in 0…63:

```text
scale = floor(65536 * (100 - w) / 100)
sourceContribution = floor(source * scale / 65536)
targetContribution = floor(target * w / 100)
out = sourceContribution + targetContribution
```

At `w = 0` the code builds an identity lookup because `65536` does not fit the fractional word. Channel contributions are truncated separately. For a red lamp at full strength `w = 80`, palette index 0 with source RGB `(0,0,0)` becomes `(50,0,0)` and DAC white `(63,63,63)` becomes `(62,12,12)`. Each index from 0 through 255 is explicitly sent to `0x3c8` and its three channels to `0x3c9` (`0x1468e–0x146bb`). This affects the whole display, including walls, planes, all sprite classes, weapon, helmet, HUD and text already drawn with those indices. It writes no framebuffer bytes and does not require the lamp sprite to be visible. DAC programming and scanout were traced statically; any scanline artefacts and the DOS wall-clock cadence remain **Unknown**.

## Complete placement inventory

**DATA-confirmed.** All 30 level records were scanned with the original Entity A layout. Exactly 30 qualifying records occur in 17 sectors across levels 1, 5, 8, 11 and 15. They are evenly divided between `0x4002` and `0x4003`. Every lamp has ceiling anchor `Param1=256` and `Param2=0`. No other level has an eligible record. Independent decoding of original OBJET1 images 2 and 3 shows both as 54×37 sprites with anchors `(27,1)` and distinct warm/red and green source palette ramps.

Level numbers are human-facing. Sector and entry indices are zero-based. Entries are displayed in file order as `colour (local X,Y) @ absolute LAB offset`.

| Level | Sector | Entries |
| ---: | ---: | --- |
| 1 | 5 | red (1100,400) `0x9d6` |
| 1 | 6 | green (500,600) `0xbd2` |
| 1 | 7 | green (600,300) `0xd58` |
| 1 | 13 | green (1100,500) `0x1532` |
| 5 | 6 | red (400,800) `0x715e` |
| 5 | 9 | red (200,600) `0x7610` |
| 5 | 15 | green (200,1000) `0x7db8` |
| 8 | 1 | red (500,300) `0xcda0` |
| 8 | 9 | red (700,1300) `0xe000`, green (700,900) `0xe008` |
| 11 | 2 | green (700,900) `0x13342` |
| 11 | 3 | red (500,300) `0x134dc` |
| 11 | 8 | green (300,100) `0x13ba8` |
| 11 | 10 | red (100,300) `0x13e2e`, red (100,700) `0x13e36`, green (100,500) `0x13e3e`, green (100,900) `0x13e46` |
| 15 | 4 | green (100,300) `0x1da24`, green (100,500) `0x1da2c` |
| 15 | 5 | red (600,800) `0x1dc32`, red (600,400) `0x1dc3a`, red (600,1200) `0x1dc42`, red (1000,800) `0x1dc4a`, red (200,800) `0x1dc52` |
| 15 | 6 | green (1100,700) `0x1de2c`, green (700,700) `0x1de34` |
| 15 | 18 | red (100,300) `0x1ee6e`, red (100,900) `0x1ee76`, green (100,500) `0x1ee7e`, green (100,700) `0x1ee86` |

The positive boxes overlap in level 11 sector 10 and level 15 sectors 4 and 18. For example at local `(100,440)` in level 11 sector 10, red at `(100,300)` qualifies with `w = 10` and wins over green at `(100,500)`, which alone would produce `w = 50`. This offers a deterministic case for a future integration test.

## Reproduction

Use `tools/inspect_original_code.py` on the executable for `0x10db7–0x10ff7`, `0x16843–0x168a4`, `0x73e7–0x75ad`, `0x7ad4–0x7b14` and `0x14611–0x146c3`. `tools/extract_dalle.py` supplies `decode_spr` for verifying sprite IDs and anchors. This bounded LAB scan independently verifies the placement table:

```python
from collections import Counter
from hashlib import sha256
from pathlib import Path
from struct import unpack_from
from sys import argv

lab = Path(argv[1]).read_bytes()
assert sha256(lab).hexdigest() == 'acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c'

def word(at):
    return unpack_from('<H', lab, at)[0]

levels = word(0)
starts = unpack_from(f'<{levels + 1}I', lab, 2)
records = Counter()
for level, (start, end) in enumerate(zip(starts, starts[1:]), 1):
    if word(start + 4) != 1:
        continue
    for sector in range(word(start + 0x12)):
        pos = start + word(start + 0x14 + 6 * sector)
        pos += 2 + 2 * word(pos)
        vertices = pos + 2
        pos += 2 + 4 * word(pos)
        pos += 2 + 8 * word(pos)
        count = word(pos)
        assert pos + 2 + 8 * count <= end
        for obj in range(count):
            at = pos + 2 + 8 * obj
            vertex, kind, height, param = unpack_from('<4H', lab, at)
            if kind & 0x3000 or kind & 0xff not in (2, 3):
                continue
            x, y = unpack_from('<2H', lab, vertices + 4 * vertex)
            assert (height, param) == (256, 0)
            records[kind] += 1
            print(level, sector, obj, hex(at), hex(kind), x, y)
assert records == {0x4002: 15, 0x4003: 15}
```

## Optic system and battery

2026-11-23. The bounded traces below use the indexed EXE and LAB hashes above. Original wording at file `0x231a2`, `0x231e8` and `0x231fd` identifies an *optics system* and calls the device *lunettes* (goggles).

**EXE-confirmed.** The optics flag is the word `[0x545f]`. The radar flag is the word `[0x5461]`. The operand matches for optics resolve to party initialization (`0x6ab4`), inventory exit notice (`0x6f4c`), base palette selection (`0x73ec`), lamp veto (`0x74ed`), the common battery HUD icon (`0x14360`) and discharge (`0x16b60`, `0x16ba4`), inventory writes (`0x1bc80`, `0x1bcca`, `0x1bcfb`), and inventory display (`0x1c347`). The raw `5f 54` match at `0x1aaa3` is the text `_T` in a resource filename. Neither the optics references nor palette writer reveal any night geometry, object detection or illumination calculation. The optics mode doubles only the source green DAC component up to 63 and discards red and blue. It can brighten pixels with a green component, while pure black stays black and visible palette entries without green can become black. It provides a green image enhancement with these limits. Radar contacts are a separate subsystem, gated by `[0x5461]` at `0x13e04` and `0x13e4e` before its contact drawing path.

**DATA-confirmed.** Checked all 256 entries in each local `PAL1.PAL` through `PAL10.PAL`. Each is 768 bytes of RGB in the DAC range 0–63. For example in PAL1, index 56 maps from `(2,1,1)` to `(0,2,0)`, index 215 from `(16,0,0)` to black, and index 0 stays black. Depending on the bank, 2–6 nonblack entries become black and 67–99 green components saturate. Counts describe the loaded palette, before any separate DALLE5 palette cycling.

### Obtaining and switching modes

- **EXE-confirmed.** The optics menu is a use of the battery item, with no separate goggles pickup or equipment flag in this path. The new-party template at file `0x1d8a0+0x1e` supplies absent charge (`-1`), and `0x6ab4–0x6ac5` clears both modes after the party template loads. A supply cabinet dispatches through `0xa52f–0xa55d → 0x183ce`. The code splits the wall's contents word at `0x183d9–0x183fa`. High nibble 0 denotes a battery and the low 12 bits hold its amount. Confirming a pickup at `0x18fad–0x19010` puts `amount × 160` in dword `[0x544b]`. An absent battery leaves `0xffff` in the cabinet. An empty battery leaves zero. For a positive old charge, confirmed exchange at `0x19321–0x19362` leaves `floor(oldCharge / 160)` in the cabinet and installs the new charge. These writes leave the mode flags as they were. `[0x544f]` is the oxygen reserve, exchanged with `[0x548d]` at `0x1bd59–0x1bd6e`. See the corrected [continuation fields](password-continuation.md#template-payload).
- **EXE-confirmed.** The gameplay inventory action at `0x16df7–0x16e15` sets `[0x540d]` through the configured binding `[0x541f]`. The main loop at `0x6eda–0x6f29` opens the inventory modal `0x1ab2e` on the ordinary path. Select the battery slot and confirm. Its display paths at `0x1c2aa–0x1c429` distinguish absent, empty, idle, radar and optics. Only the last three allow a change. The charge display uses integer `floor(100 × charge / 144000)`, where 144000 is the charge from a stock 900-unit battery.

| Current mode, with charge | First option | Second option | Third option | Selection path |
| --- | --- | --- | --- | --- |
| Off | Turn on radar | Turn on optics | No change | `0x1bc43–0x1bc89` |
| Radar | Turn off radar | Switch to optics | No change | `0x1bc8e–0x1bcd3` |
| Optics | Turn off optics | Switch to radar | No change | `0x1bcd8–0x1bd1b` |

The three-choice modal at `0x1cbb1–0x1ccf6` starts on option 0, moves the selection between 0 and 2, and confirms it through the inventory action flag `[0x5407]`. Off paths and swaps clear the active mode before optionally enabling the other. No change preserves both flags. Switching modes preserves charge and uses the same consumption rate. Returning to gameplay forces a palette refresh at `0x6f8f`.

### Discharge and interaction with lamps and surges

**EXE-confirmed.** At `0x16b5b–0x16bca`, either nonzero mode flag enables a single subtraction of the unsigned gameplay delta `[0x1428]` from `[0x544b]`. With both off, the code skips discharge and the battery warning. While in use, a result below 4800 requests the battery warning at file `0x219be`, including on expiry. A result at or below zero stores zero and clears both modes. The expiry notice at `0x16b96` selects file `0x21a49` if radar was on (`-1`), or `0x21a73` otherwise. The wording says “activated” even though the flags are being cleared. The optics expiry branch also marks the palette dirty at `0x16b88`. The shared HUD battery indicator at `0x14336–0x14376` uses sprite 42 for either active mode and 41 otherwise when a battery is present. The ordinary modal return sets the first gameplay delta to 1 at `0x6f7d`. Time spent choosing a mode in the blocking inventory does not itself run the battery or surge update.

| Normal palette arbitration after the entry fade | Optics off | Optics on |
| --- | --- | --- |
| No transient, no qualifying lamp | Level palette | Green optics palette |
| No transient, qualifying lamp | Level palette blended toward the selected red/green lamp | Green optics palette. Lamp request not applied |
| Surge owns transient slot | Level palette blended toward black | Green optics palette blended toward black |
| Another transient owns slot | Level palette blended toward its target | Green optics palette blended toward its target |

The table follows `0x73e7–0x73ff` and `0x74e2–0x75a4` after the separate entry fade. In particular, the lamp scan still executes with optics on. A surge neither switches modes nor changes the battery cost. Optical gain happens when the green palette is built. On a surge frame, darkening happens afterward using that palette and can drive a faint index back to zero. For example, PAL1 index 56 starts the surge blend at green 2 with optics on and reaches zero at strength 93. When darkening ends the next ordinary choice is green optics if active, or the qualifying lamp/normal palette if inactive. See [surge state and shared RNG](power-surging.md#state-cross-references-2026-11-23).

### Battery cabinet placements

**DATA-confirmed.** Walked all 30 level spans in the indexed pristine LAB. For each sector, skipped neighbor and vertex arrays, then examined each 8-byte wall for family `0x3200` and contents high nibble 0. There are 12 matching cabinets in 11 levels, all type `0x3205`, contents `0x0384` (900, charge 144000 on pickup). Level numbers are human-facing. Sector and wall indices are zero-based. Offsets point to the beginning of the wall record.

| Level | Sector | Wall | LAB offset |
| ---: | ---: | ---: | --- |
| 2 | 13 | 15 | `0x32d4` |
| 7 | 7 | 8 | `0xb92c` |
| 11 | 7 | 3 | `0x13a08` |
| 13 | 6 | 9 | `0x190ce` |
| 15 | 17 | 6 | `0x1ecde` |
| 17 | 8 | 3 | `0x22f90` |
| 20 | 1 | 12 | `0x278ca` |
| 21 | 11 | 5 | `0x28c6e` |
| 22 | 1 | 12 | `0x2a494` |
| 27 | 13 | 16 | `0x38e54` |
| 30 | 15 | 11 | `0x40542` |
| 30 | 17 | 16 | `0x407c0` |

## Port status and verification boundary

**Port implementation.** Inventory uses a single mode for off, radar or
optics. The existing battery slot offers the three choices above. A cabinet
with a charged battery in the inventory asks whether to take the cabinet's
battery or keep the installed one. The confirmed transfer preserves the
cabinet's unit precision and leaves the current mode in place. Travel keeps
the installed charge and current mode, and revisiting a cabinet keeps its
exchanged contents. NEW GAME and password continuation reset both modes and
start with the respective template's charge and pristine cabinets.

`AppSession::run` samples the source sector and proposed movement point,
updates battery charge before palette selection and restricts radar contacts
to radar mode. The HUD battery indicator shows use in either mode.
`ambient_effects.cpp` selects optics, lamp and transient effects.
`FirstPersonRenderer::presentFrame` applies the selection to a scratch copy
of the newly composed world and HUD before upload. With optics enabled, it
doubles the green DAC component before any selected transient blend. At
cutoff the frame uses the regular source, still respecting transient
priority. Unfiltered frames use the direct upload path.

**Adapted.** The port quantizes composed RGB8 to RGB6, applies the optical
transform and integer blend, then expands to RGB8. DOS programs a shared VGA
DAC over an indexed framebuffer. RGBA colors already interpolated or
composited by the host enter the calculation as pixels. Blocking interfaces
retain their own palette handling. The port samples lamps at the input to
its object collision pass and uses its shared gameplay clock for charge.
Blocking freezes drain, and the first resumed interval is discarded here.
DOS writes a delta of 1 on ordinary inventory return.

**Port-tested.** Debug and Release CTest passed 45/45 each. The original LAB
verified all 12 battery cabinets and their 900-unit stock amounts. The
original HUD and modal banks passed the software presentation tests.
Composed output passed green-channel and restoration checks across all four
aspect selections, render scales 1×–6× and MAX, with fisheye and workers
enabled. A Debug SDL app run with the original LAB and a diagnostic battery
switched radar → optics → radar → off through normal inventory input.

**Unknown / DOS observation pending.** Compare level 11 sector 10 with
optics off and on, then repeat at surge expiry and at charge depletion.
Measure the visible VGA frame ordering and alarm cadence. Also verify
the native inventory key and notices during a full modal cycle. The
conditions and arithmetic above are static findings.

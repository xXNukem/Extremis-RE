# DALLE1.DAL and related graphics: investigated layout

Animation follow-up: [animated-textures.md](animated-textures.md) now traces
ADALLE composition onto DALLE materials 11–13. Statements below that the overlay
destinations are unknown describe the earlier investigation and are superseded
by that audit; original timing in seconds is still uncalibrated.

## Orientation correction (original executable audit)

The earlier visual interpretation below is retained as historical evidence, but
its **74x60 row-major orientation was incorrect**. The runtime layout is **60x74
column-major**, with address `record * 4440 + x * 74 + y`. The loader and PNG
extractor now convert this to row-major 60x74 images. ADALLE decoding is unchanged.

In the supplied EXTREMIS.EXE (hash documented in wall-texture-mapping.md), absolute
file offsets `0x15795..0x157a8` select a texture column by multiplying its index
by 74. At `0x15827` the vertical step numerator is `73 << 16`; the loop at
`0x1591e..0x15925` advances the framebuffer by 320 bytes per pixel and advances
the source within that column by the positive vertical step. This establishes
that consecutive source bytes run downward, not horizontally. The ADALLE copy
at `0x152ff..0x15309` corroborates it: source rows advance through destination
columns in steps of 74, and each next source row starts one destination byte down.
Together with the 4440-byte record this gives 60 columns of 74 pixels.

This is an axis transpose, not a quarter-turn with an extra reflection. No
`width - 1 - x` or `height - 1 - y` is applied. All four corners and every interior
pixel are checked against the source address in the C++ and Python tests.
The renderer already uses U along A-to-B and V downward, indexing `y * width + x`;
its sampling, clipping, geometry, camera, material indices and portals are unchanged.
The original game's complete per-wall horizontal direction and UV scale still
require comparison with original in-game captures. No new mirroring is introduced
in our A-to-B convention.

The black regions are retained byte-for-byte. Frame geometry/compositing is outside
this correction. Historical references below to black lower halves become black
right halves in the corrected orientation; no black pixels are removed.

## Historical extraction investigation (orientation superseded above)

Scope: the original files in `~/Desktop/In_Extremis/DATA/`. No game resources
are included in source control. This investigation does not change the engine,
assign textures to wall types, or reproduce the original rendering code.

## Results and confidence

**A reliable pixel extraction is possible for the investigated DALLE1 sample:**
54 distinct indexed images of 74 by 60 pixels, 4,440 bytes each. Reading bytes
directly as palette indices in row-major order produces coherent complete
panels, switches, machinery, lettering and lights throughout the entire file.
There is no decompression step.

Important distinction: **the file does not contain an identified dimension or
image-count field**. The 74x60 layout is a strong empirical conclusion from
whole-file alignment, image boundaries, repeated motifs and visual inspection,
not a width/height declaration recovered from the original executable. Its use
by the original renderer, including possible UV scaling or cropping, remains
unverified. The extractor deliberately supports the measured 239,760-byte sample,
not arbitrary DAL variants or guessed dimensions supplied by the user.

`ADALLE.SPR` independently contains 20 uncompressed indexed images with explicit
dimensions and record boundaries. They look like small animated display panels,
grids and light strips related visually to the DAL surfaces. An actual runtime
overlay relationship has not been traced.

## Reference files

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| DALLE1.DAL | 239760 | `3335fe9e16aff6e6a8d53ecbafe18dd07475b353c51153b8021e8ae6fd5180ca` |
| PAL1.PAL | 768 | `1e321d969c890ca86f339f9ab22a0002d938241fa4bc3c1f0988387c3f4d570b` |
| ADALLE.SPR | 9972 | `e6048c4fcf4a430bbdc171d6afec4d44784ceaf0c5871d6e8360d7a2c502465e` |

The existing `extract_spr_inteligente.py` research script was read as a reference,
not executed: it writes outside this repository and its PAL1 fallback explicitly
marks palette associations as provisional. Its ADALLE report lists the same
20 image dimensions. The new tool independently validates the bytes and bounds.

## DALLE1.DAL byte layout

Under the demonstrated extraction layout:

| Location | Content |
| --- | --- |
| File offset 0 | First pixel index of image 0; no preceding header |
| `i * 4440` | Start of image `i`, with `0 <= i < 54` |
| `i * 4440 + y * 74 + x` | One unsigned 8-bit palette index, `0 <= x < 74`, `0 <= y < 60` |
| 235320 (`0x39738`) | Start of image 53 |
| 239760 (`0x3a890`) | EOF; no trailing bytes |

The file begins `73 55 19 18 1b 1b f2 ec ...`; treating those bytes as pixels
includes the first complete panel. No magic, count, dimensions, offset table,
per-image metadata or compressed packet headers are needed to account for the
file: `54 * 74 * 60 == 239760`, exactly. This is not a claim that every file with
the `.DAL` extension must have the same layout.

Pixels are serialized left to right, then top to bottom. A transposed/column-major
interpretation does not give the same upright panels. All 54 pixel blocks differ;
this counts stored images, not necessarily 54 logically different materials.
Do not discard duplicate-looking or darker variants.

### Evidence supporting the dimensions

- Candidate row strides 32, 37, 40, 60, 64, 74, 80, 111, 120, 128, 148, 160,
  222, 256 and 296 were rendered as explicitly labeled experiments. A stride
  of 74 gives upright coherent features; multiples combine neighboring rows,
  while 37 interleaves their halves. This avoids choosing 64x64 by convention.
- Splitting at every 4,440 bytes gives aligned complete graphics throughout all
  54 records, including variations of matching machinery and circular lights.
  Alternative 37x40 and 37x60 partitions break coherent features across records.
- Image 3 and images 22, 32, 42 and 52 each end in 30 completely zero rows
  (2,220 bytes). At 74x60 these are rectangular half-height graphics with
  consistent boundaries. Black rows are retained rather than cropped.
- As a numerical corroboration, for the complete byte stream `b`, the equality
  fraction `b[i] == b[i+lag]`, sampled at `i = 0, 3, 6, ...`, is:

  | Lag | Equal fraction |
  | ---: | ---: |
  | 37 | 0.146268 |
  | 60 | 0.153630 |
  | 64 | 0.156785 |
  | 73 | 0.219110 |
  | 74 | 0.294370 |
  | 75 | 0.219000 |
  | 128 | 0.139513 |
  | 148 | 0.240100 |

  This statistic alone does not prove dimensions; it supports the visual and
  boundary evidence.
- Read-only size checks of all ten DAL banks found nine files of 239,760 bytes
  and DALLE5 of 155,400 bytes. Both sizes are exact multiples of 4,440 (54 and
  35 respectively). Their full pixel interpretation has not been validated here.

Images 14–23, 24–33, 34–43 and 44–53 visually repeat ten motifs with different
tones. These might be lighting/material variants, but their runtime meaning and
ordering are unknown. Corresponding pixels are not explained by a single
unambiguous palette-index replacement table; do not implement a lighting model
or deduplicate them on this basis.

## PAL1.PAL

Verified observations:

- Exactly 768 bytes: 256 consecutive RGB triplets, no header required.
- Every component lies in `[0, 63]`, consistent with VGA 6-bit DAC values.
- The preview uses `round(component * 255 / 63)` for each RGB component.
  Index 0 is black. The palette is external to the DAL pixel stream.
- This palette produces coherent metal, green indicators and orange/red lights
  in DALLE1 and ADALLE. Level 1's already-decoded bank fields also select PAL 1
  and DALLE 1. This supports the association without tracing runtime code.

Unverified: exact original DAC conversion, gamma, palette changes/cycling and
whether any pixel index is transparent in a particular rendering operation.
No transparency is invented: exported PNGs retain every index and have no
alpha or transparent-index declaration. A simple left shift by two, for example,
would give a different preview range (maximum 252 rather than 255).

## ADALLE.SPR byte layout

All integer fields below are unsigned little-endian. The observed structure is
independently validated against all 20 records and EOF, without decompression.

| Offset | Content |
| --- | --- |
| `0x00` | uint16, observed 0; meaning unknown |
| `0x02` | uint16 image count, 20 |
| `0x04` | 20 uint32 offsets, relative to absolute file offset 4 |
| `4 + offsets[i]` | uint16 width, uint16 height, four metadata bytes, then `width * height` pixel indices |

The first stored offset is 80, hence the first record starts at byte 84, exactly
after the header/table. For example, record 0 has 38x31 pixels: its size is
`8 + 38*31 = 1186`; it ends at byte 1270, which is `4 + 1266`, the next record's
start. Every record ends exactly where the next begins; the last ends at 9972.
The four metadata bytes are zero in every observed record, with unknown meaning.

| Zero-based indices | Count | Dimensions | Visual appearance with PAL1 |
| --- | ---: | --- | --- |
| 0–3 | 4 | 38x31 | Orange/red display, changing illuminated marks |
| 4–11 | 8 | 24x17 | Green circular display and bars |
| 12–17 | 6 | 15x17 | Green grid/display variations |
| 18–19 | 2 | 37x3 | Light strip variations |

Pixel data is row-major with the supplied width; upright display motifs provide
visual corroboration. These are supplementary images, not a required dictionary
or decompression table for DALLE1. Their grouping suggests animation frames and
surface overlays, but frame ordering, timing, destination surfaces, coordinates
and compositing rules are not established. No complete ADALLE image was found
as an exact rectangular pixel match inside the 54 DALLE1 images. Similar-looking
panels do not constitute proof of an exact association.

## Reproducing the extraction

Pillow was already available in the local Python environment; nothing was
installed. The tool reports a missing dependency and exits if Pillow is absent.

```sh
python3 tools/extract_dalle.py --data-dir "$HOME/Desktop/In_Extremis/DATA"
python3 -m unittest discover -s tests -p 'test_extract_dalle.py'
```

Output is confined to this repository's ignored `build/texture-debug/` directory
(or a subdirectory supplied with `--output`):

```text
build/texture-debug/
  dalle1/00.png ... 53.png    # Native 74x60, indexed PNGs
  adalle/00.png ... 19.png    # Native dimensions, indexed PNGs
  dalle1-catalog.png         # 3x nearest-neighbor previews with indices
  adalle-catalog.png         # 4x nearest-neighbor previews with indices
  manifest.json             # Input hashes, offsets, dimensions, metadata, pixel hashes
```

The PNG catalogs are presentation artifacts; the individual indexed PNGs
preserve native pixel indices. Exploratory candidate images also remain only
under `build/texture-debug/`, labeled as experiments. Original binaries are
never written or copied into the repository. No engine/CMake changes are needed.

## Remaining work before engine integration

Update: a deliberately provisional C++ wall preview now uses this decoded layout.
See [runtime implementation and mapping evidence](wall-texture-preview.md).
The questions below still apply to reproducing the original rendering behavior.

1. Trace the original executable's texture addressing or compare with original
   in-game captures to independently confirm runtime dimensions, orientation,
   sampling and palette behavior.
2. **Resolved for level 1 record selection:** tracing the original executable
   proves `type & 0xff` selects the zero-based DAL image. See
   [the mapping audit](wall-texture-mapping.md). Upper bits and `extra` have
   additional behavior; the type word as a whole is not an image index.
3. Trace ADALLE selection, frame timing, placement and compositing. Do not infer
   transparency from black or any other frequent index.
4. Determine the purpose of the repeated tone variants, black lower halves and
   SPR metadata before assigning gameplay/rendering semantics.
5. Validate other banks/versions separately. Matching sizes alone are insufficient
   to assert their graphics have the same meaning or usage.

# Wall textures and provisional preview

Orientation update: the previous 74x60 row-major assumption mentioned below is
superseded by 60x74 column-major decoding. See [the orientation audit](dalle-format.md#orientation-correction-original-executable-audit).
Record selection and renderer U/V are unchanged.

The C++ renderer now reads original DAL/PAL files at runtime. The decoded graphics
are genuine. The default selection **`type & 0xff` is now verified** against the
original executable; see [the complete mapping audit](wall-texture-mapping.md).
Orientation and scale on walls remain provisional. No wall-order or modulo
assignment is used. The original single-texture preview remains as an override.

## Runtime resources and controls

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/InExtremisPort "$HOME/Desktop/In_Extremis/GAME.LAB"
```

The application looks in `DATA/` beside the GAME.LAB path and passes the loaded
level's DALLE and PAL bank IDs to the independent texture loader. Level 1 selects
DALLE1 and PAL1. The supported DAL layout is exactly the researched 239,760-byte
DALLE1: 54 images, 74x60 indices, 4,440 bytes each, row-major without a header.
Other DAL banks remain explicitly unsupported until independently validated.
The requested PAL bank is read by its ID, with exact 768-byte length and all
components limited to 0–63. Conversion is `(component * 255 + 31) / 63`, equivalent
to the extractor's full-range rounding. The decoded bank holds row-major RGB
pixels; original files remain read-only. No PNG, Python or original resource is
required inside the repository or embedded into the executable.

- Tab switches 2D/3D, initially 2D.
- N selects the previous image, M the next; both wrap through indices 0–53.
- Default selection follows each wall's original low-byte material index.
- N/M enable the provisional override, starting from the previous index cursor
  (initially 0) and wrapping as before. The selected image is then applied uniformly.
- The title/log distinguish original materials from **PROVISIONAL** single-texture
  mode. Restart to restore the default per-wall selection.
- Left/Right rotate the unchanged stationary camera. P keeps its 2D polygon
  toggle. Escape and window close still exit.

A missing resource, invalid palette, unexpected size or unsupported DAL bank logs
the reason and leaves the flat-color renderer available. No fabricated replacement
texture is created. The 2D renderer and GAME.LAB loader are unchanged.

## Investigation of GAME.LAB wall fields

The existing C++ loader was used to inspect all 280 level 1 walls. No second LAB
parser was introduced. Results use zero-based texture/catalog indices:

| High type bits | Walls | Lower 12 bits | Extra field |
| --- | ---: | --- | --- |
| `0x0000` | 181 | Values between 1 and 51 | Always 0 |
| `0x1000` | 30 | 0, 23, 33, 43, 53 and 2057 (`0x809`) | Every value 0–14 occurs twice |
| `0x2000` | 60 | 3, 22, 32, 42, 52 | Always 0 |
| `0x3000` | 9 | 2, 260, 517, 776, 1288, 1542 | Includes values above 53 |

Observations and hypotheses must be separated:

- All ordinary-wall and frame lower-12-bit values fit the 54-image bank.
- The frame values match exactly the five DAL images whose lower 30 rows are
  zero. This is strong evidence of a relationship between some type bits and
  texture selection, but does not establish the full original addressing rules.
- Nine lower-12-bit values exceed index 53, so treating `type & 0x0fff` as an
  unconditional texture index would be invalid. Five `extra` values also exceed
  53; `extra` cannot simply be used as a universal direct image index either.
- The **low byte** fits the bank for every wall in this sample, including special
  types. It is a promising texture-index candidate. Bounds compatibility and the
  frame correlation alone did not establish the rule. Subsequent executable
  tracing now confirms this low-byte extraction and direct bank addressing.
  Animation behavior and applicability to other versions remain unverified.
- Portal `extra` values coincide numerically with the sector range, suggesting
  connectivity metadata. That interpretation has not been traced and is unused.

The independent executable trace now establishes this mapping, including the
intermediate +1/-1 cancellation. The renderer uses the verified low-byte rule
by default. Full instructions, offsets and the complete identifier census are
in [wall-texture-mapping.md](wall-texture-mapping.md). Portal omission and the
existing flat fallback remain unchanged.

## Projection and sampling

1. Rays use the existing camera plane and segment-intersection algorithm. The
   nearest non-portal hit now also returns the segment fraction `u`, measured
   from vertex A to B and clamped to `[0,1]`. Geometry is not duplicated.
2. `texX = min(width - 1, floor(u * width))`. This provisional rule maps one whole
   image across each segment, irrespective of segment length. Reversing the
   segment's vertex ordering reverses its horizontal texture orientation.
3. Perpendicular depth remains the ray parameter because rays are not normalized
   and their dot product with the camera forward vector is one. FOV remains 70°,
   wall height 256 and eye height 128. Camera placement/rotation is unchanged.
4. The full, **unclipped** projected top and bottom are retained. Each visible
   pixel is sampled at its center: `v = (y + 0.5 - top) / (bottom - top)` and
   `texY = clamp(floor(v * height), 0, height - 1)`. Clipping crops the image;
   it does not remap the full image into the visible portion of a nearby wall.
5. Nearest-neighbor RGB samples use the existing distance shade
   `0.2 + 0.8 / (1 + depth / 700)`. All image pixels are opaque, including black;
   no transparency or original lighting behavior is inferred.
6. The CPU writes a contiguous RGBA framebuffer, uploads it once per frame to an
   SDL streaming texture, then draws that texture at the current output size.
   The upload texture and framebuffer are resized when the window output changes.
   Integer `RGBA8888` packing specifies channel bit positions across byte orders.

Portals with high type bits `0x1000` are skipped before hit selection and never
become opaque. Floor and ceiling retain their original prototype colors. The
ray/projection pipeline is shared with flat rendering to prevent logic drift.

## Responsibilities

- `texture_bank.h/.cpp`: validated direct file reads, palette expansion, RGB bank
  storage; no SDL or GAME.LAB dependency.
- `texture_preview.h/.cpp`: provisional selected index, wraparound, title and
  loading/fallback diagnostics; no rendering or binary decoding.
- `first_person_view.h/.cpp`: hits, perspective sampling and software drawing.
  `FirstPersonRenderer` owns its upload texture with RAII and borrows SDL_Renderer;
  it is destroyed before that renderer. Flat rendering remains available without
  textures.
- `main.cpp`: supplies bank IDs and resource location, connects N/M and invokes
  the selected view. It contains no new sampling or decoding algorithm.

## Verification and remaining limits

Synthetic CTest coverage includes full bank RGB conversion, palette-bank selection,
malformed/missing files, unsupported banks, N/M wraparound, endpoint `u` values,
pixel-center UV sampling including vertical clipping, background colors, invalid
texture rejection and flat fallback. Existing geometry and reader tests remain.
The level 1 bank is also tested directly without copying the original resources.

UV repeat scale/offset/mirroring, palette dynamics, ADALLE
overlays, original light/shade variants, sprites, floor/ceiling textures, actual
sector heights, movement and collisions remain unimplemented. All opaque surfaces
still have experimental uniform height. Every ray checks all wall segments; there
is no spatial acceleration, mipmapping or texture filtering beyond nearest sampling.

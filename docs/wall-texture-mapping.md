# Original wall-to-DAL texture mapping (level 1)

Orientation update: the previous 74x60 row-major assumption mentioned below is
superseded by 60x74 column-major decoding. See [the orientation audit](dalle-format.md#orientation-correction-original-executable-audit).
Record selection and renderer U/V are unchanged.

## Conclusion

**Verified by static tracing of the supplied original executable:** the texture
record index is `wall.type & 0x00ff`, zero-based. The original drawing API adds
one to this value, and the rasterizer subtracts one before computing the byte
offset, so there is no net one-based adjustment in our catalog.

This is no longer a range-check/visual hypothesis. The wall-table read, projected
wall record, material selection and DAL byte addressing were traced in sequence.
The implementation enables this mapping for level 1 while retaining existing
portal omission and the manual N/M single-texture preview.

Only **texture record selection** is established here. Original UV orientation,
scale, mirroring, frame heights, animation and light handling are not reproduced
by this change. The working 74x60 loader, palette and projection are unchanged.

## Inputs and texture identification

- GAME.LAB: 264,638 bytes; the existing validated C++ loader supplies 15 sectors
  and 280 wall records. Each wall has vertex A, vertex B, type and extra.
- DALLE1.DAL: 239,760 bytes = 54 records of 4,440 bytes. The current 74x60 indexed
  layout and palette conversion are described in [the DAL investigation](dalle-format.md).
  All 54 records have different pixel bytes, including darker/repeated motifs.
- Images are identified by their position in the bank: record `i` begins at
  `i * 4440`. No per-image material identifier or mapping table is stored in the
  decoded file. The executable independently confirms direct fixed-stride addressing.
- EXTREMIS.EXE: 149,303 bytes, SHA-256
  `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
  Evidence below applies to this supplied version, which includes a crack notice;
  other releases have not been compared. The original executable was only read,
  never run, patched or copied into the repository.

## Complete level 1 identifier census

The rows below enumerate **all 55 distinct type words**, with full frequencies.
The lower-12-bit column includes high-byte modifiers rather than silently masking
these into an image index. All indices/counts refer to stored DAL records.

| Type | Count | `type & 0x0fff` (decimal) | Verified index `type & 0xff` |
| --- | ---: | ---: | ---: |
| `0x0001` | 16 | 1 | 1 |
| `0x0009` | 1 | 9 | 9 |
| `0x000a` | 1 | 10 | 10 |
| `0x000b` | 16 | 11 | 11 |
| `0x000c` | 2 | 12 | 12 |
| `0x000d` | 2 | 13 | 13 |
| `0x000e` | 4 | 14 | 14 |
| `0x000f` | 6 | 15 | 15 |
| `0x0010` | 5 | 16 | 16 |
| `0x0011` | 6 | 17 | 17 |
| `0x0012` | 5 | 18 | 18 |
| `0x0013` | 2 | 19 | 19 |
| `0x0014` | 2 | 20 | 20 |
| `0x0015` | 4 | 21 | 21 |
| `0x0018` | 6 | 24 | 24 |
| `0x0019` | 9 | 25 | 25 |
| `0x001a` | 8 | 26 | 26 |
| `0x001b` | 1 | 27 | 27 |
| `0x001c` | 4 | 28 | 28 |
| `0x001d` | 4 | 29 | 29 |
| `0x001e` | 6 | 30 | 30 |
| `0x001f` | 2 | 31 | 31 |
| `0x0022` | 3 | 34 | 34 |
| `0x0023` | 6 | 35 | 35 |
| `0x0024` | 6 | 36 | 36 |
| `0x0025` | 5 | 37 | 37 |
| `0x0026` | 6 | 38 | 38 |
| `0x0027` | 8 | 39 | 39 |
| `0x0028` | 1 | 40 | 40 |
| `0x0029` | 1 | 41 | 41 |
| `0x002c` | 6 | 44 | 44 |
| `0x002d` | 4 | 45 | 45 |
| `0x002e` | 5 | 46 | 46 |
| `0x002f` | 4 | 47 | 47 |
| `0x0030` | 3 | 48 | 48 |
| `0x0031` | 1 | 49 | 49 |
| `0x0032` | 6 | 50 | 50 |
| `0x0033` | 4 | 51 | 51 |
| `0x1000` | 4 | 0 | 0 |
| `0x1017` | 6 | 23 | 23 |
| `0x1021` | 6 | 33 | 33 |
| `0x102b` | 6 | 43 | 43 |
| `0x1035` | 6 | 53 | 53 |
| `0x1809` | 2 | 2057 | 9 |
| `0x2003` | 4 | 3 | 3 |
| `0x2016` | 14 | 22 | 22 |
| `0x2020` | 14 | 32 | 32 |
| `0x202a` | 14 | 42 | 42 |
| `0x2034` | 14 | 52 | 52 |
| `0x3002` | 2 | 2 | 2 |
| `0x3104` | 1 | 260 | 4 |
| `0x3205` | 3 | 517 | 5 |
| `0x3308` | 1 | 776 | 8 |
| `0x3508` | 1 | 1288 | 8 |
| `0x3606` | 1 | 1542 | 6 |

### Family differences

| High nibble | Wall count | Extra distribution | Rendering in this prototype |
| --- | ---: | --- | --- |
| `0x0000` normal | 181 | 0 on all 181 | Texture selected by low byte |
| `0x1000` portal | 30 | Each value 0–14 occurs twice | Omitted as before, regardless of stored material |
| `0x2000` frame | 60 | 0 on all 60 | Low-byte texture; experimental uniform height retained |
| `0x3000` special | 9 | 0 twice, 1 twice, 263/4596/8292/12289/16385 once each | Low-byte texture, high-byte behavior not implemented |

Across all records the low byte refers to 53 distinct image indices in `[0,53]`;
only index 7 is absent. The 250 opaque walls use 48 distinct indices. Indices
0, 7, 23, 33, 43 and 53 are not used by opaque walls; 30 portal records remain
transparent to visibility. Every visible wall index fits the loaded bank.

The lower 12 bits are **not** a universal image index: nine records exceed 53.
For example, `0x3104 & 0x0fff == 260` but its verified texture index is 4.
Likewise `0x3205` selects 5, `0x3308` and `0x3508` both select 8, and `0x3606`
selects 6. Normal `0x0001` selects catalog image 1, not image 0.

The frame indices 3, 22, 32, 42 and 52 coincide with the five images containing
30 zero rows. This was useful corroboration, but is not the proof used to enable
mapping. In particular, it does not authorize inventing frame geometry/heights.

### Complete extra-field distribution

| Extra (hex) | Decimal | Count |
| --- | ---: | ---: |
| `0x0000` | 0 | 245 |
| `0x0001` | 1 | 4 |
| `0x0002` | 2 | 2 |
| `0x0003` | 3 | 2 |
| `0x0004` | 4 | 2 |
| `0x0005` | 5 | 2 |
| `0x0006` | 6 | 2 |
| `0x0007` | 7 | 2 |
| `0x0008` | 8 | 2 |
| `0x0009` | 9 | 2 |
| `0x000a` | 10 | 2 |
| `0x000b` | 11 | 2 |
| `0x000c` | 12 | 2 |
| `0x000d` | 13 | 2 |
| `0x000e` | 14 | 2 |
| `0x0107` | 263 | 1 |
| `0x11f4` | 4596 | 1 |
| `0x2064` | 8292 | 1 |
| `0x3001` | 12289 | 1 |
| `0x4001` | 16385 | 1 |

`extra` is not read by the proven material-address path. The portal values match
sector indices and the special values look like additional state/parameters, but
that interpretation is not needed for material selection and is not implemented.
No universal material ID is inferred from this field.

## Independent executable evidence

All instruction addresses in the left column are **absolute file offsets** in the
supplied MZ executable, not runtime virtual addresses. Bracketed addresses inside
instructions are image-relative runtime storage. The MZ header is 512 bytes.
The inspected game routines decode as x86 **32-bit** code; the real-mode entry
stub uses 16-bit code and is not the material path. Relative call targets below
are computed as `file_offset + instruction_length + displacement`.

### 1. Locate the source wall table

At file offsets `0x113dd..0x1141c`, the original sector parser reads a neighbor
count, skips `count * 2`, reads a vertex count, skips `count * 4`, reads the wall
count into `[0x51ba]`, and stores the following address in `[0x51bc]`. It later
skips `wall_count * 8`. This is the same wall layout validated by our loader.

At `0x11657..0x11666`, the count `[0x51ba]` and pointer `[0x51bc]` are passed to
routine `0x117fe`. That routine uses `[esi]` and `[esi+2]` as vertex indices
(`0x11863..0x11888`), and advances `esi` by 8 per wall at `0x11c81`.

### 2. Preserve the type in the projected wall record

```asm
; absolute file offsets
0x11c1d: mov ax, word ptr [esi + 4]    ; source wall.type
0x11c21: mov word ptr [edi + 16], ax   ; projected record's type
```

The projected records have a 30-byte stride (`add edi,30` at `0x11c50`). Their
reader at `0x12805..0x12819` multiplies the selected projected-wall index by 30
and adds 2 to skip the record-array counter. This links the record consumed by
the drawing dispatcher to the record written above.

### 3. Extract the low byte, without treating flags as material bits

```asm
0x128da: mov ax, word ptr [edi + 16]
0x128de: xor ah, ah
0x128e0: inc ax
0x128e2: mov ebx, dword ptr [ebp + 20] ; DAL bank base argument
0x128e5: mov word ptr [0x159a3], ax
0x128eb: mov dword ptr [0x1599f], ebx
0x128f1: call 0x15427
```

Clearing AH is exactly a low-byte extraction; the upper byte is not part of the
record number. The increment is only an intermediate drawing-API convention.
The surrounding dispatch tests projected flags with `0xc000` at `0x1282a`;
the level 1 wall families `0x0000..0x3000` lie in this geometry path.

### 4. Compute the direct DAL record address

```asm
0x15501: mov esi, dword ptr [0x1599f]
0x15507: movzx eax, word ptr [0x159a3]
0x1550e: dec eax
0x1550f: mov ebx, 4440
0x15514: mul ebx
0x15516: add esi, eax
```

Combining the two fragments gives exactly:

```text
record_address = DAL_base + (((wall.type & 0xff) + 1) - 1) * 4440
               = DAL_base + (wall.type & 0xff) * 4440
```

No correspondence table, wall-order lookup or modulo operation occurs here.

### 5. Confirm that the base is the loaded DAL resource

The loader at `0x7c41..0x7c6d` allocates a resource buffer in `[0x4ac4]`, supplies
it when reading the filename at image-relative `0x4988` (file offset `0x4b88`,
`data\dalle10.dal` in the bank-name template), then retains the buffer pointer.
The frame dispatcher pushes `[0x4ac4]` at `0x725a` and calls `0x12658` at
`0x7272`; with the intervening arguments this is `[ebp+20]` in the material
selection routine. Thus the multiply targets the loaded DAL bytes, not an
unidentified material table.

A separate ADALLE-copy routine at `0x152c0` also uses the same bank pointer and
multiplies the selected record by 4440 (`0x152da..0x152e1`). It uses a 74-byte
storage stride while copying (`mov ax,74`, and `movsb` plus `add edi,73`). This
independently confirms the record size and storage stride, while also showing
that original sprite compositing may transpose coordinates relative to our
current preview. **This task does not change the working pixel layout or UVs**;
full original sampler/overlay orientation remains a separate investigation.

## Implementation

- New `wall_material.h/.cpp` implements the verified low-byte selection with an
  explicit bounds check. Portals and unsupported families have no opaque material.
- The existing ray hit already carries the source type. It now selects the bank
  image at that index before the existing column sampler. There is no need to
  copy walls, reinterpret GAME.LAB, change rays or pass `extra` into the renderer.
- Valid materials use the existing loader, palette, framebuffer, U/V sampling,
  projection and shading. An invalid index or malformed texture gives a flat-color
  wall rather than an out-of-range access or a guessed replacement image.
- Original selection is the default. N/M preserve the explicit provisional
  single-image preview. The title/log distinguish these modes. Restart the app
  to return from manual override to default original material selection; no new
  keys, player movement or geometry behavior were introduced.
- No binary resources or disassembly dumps are tracked. Only the short evidence
  excerpts above are documentation. Full CSV/raw disassembly reports are ignored
  under `build/texture-debug/`.

## Reproduce the audit

No additional packages were installed. The executable inspector uses the LLVM
library already supplied with the macOS Command Line Tools; it does not execute
original x86 instructions.

```sh
cmake -S . -B build
cmake --build build --target inspect_wall_materials
mkdir -p build/texture-debug
./build/inspect_wall_materials "$HOME/Desktop/In_Extremis/GAME.LAB" > build/texture-debug/wall-materials.csv
python3 tools/inspect_original_code.py "$HOME/Desktop/In_Extremis/EXTREMIS.EXE" 0x113dd 0x1141e
python3 tools/inspect_original_code.py "$HOME/Desktop/In_Extremis/EXTREMIS.EXE" 0x11c1d 0x11c25
python3 tools/inspect_original_code.py "$HOME/Desktop/In_Extremis/EXTREMIS.EXE" 0x128da 0x128f6
python3 tools/inspect_original_code.py "$HOME/Desktop/In_Extremis/EXTREMIS.EXE" 0x15501 0x15518
cmake --build build
ctest --test-dir build --output-on-failure
./build/InExtremisPort "$HOME/Desktop/In_Extremis/GAME.LAB"
```

## Verification

- CMake configuration and compilation succeed. All three CTest suites pass.
- Selection tests cover normal/frame/special flags, index zero, invalid indices,
  unsupported families and portal exclusion. Pixel tests render different materials
  on adjacent segments, check that `extra` does not affect them, retain the manual
  override and check flat fallback for invalid/malformed materials.
- Existing texture-perspective, clipping, palette, missing-file and loader tests
  continue to pass; they use synthetic assets.
- The original level has 250 opaque wall records with valid indices, using 48
  different bank entries. A direct read-only render check tests eight orientations
  at three output sizes. The 30 portal records remain omitted.

## Still unconfirmed or deliberately unchanged

- Original UV scale/orientation/mirroring, sector heights and nonuniform frames.
- Runtime changes to wall flags/materials, palette animation and light variants.
- Complete meanings of bits 8–15 and `extra`, including special interactions.
- ADALLE frame placement/timing/compositing and exact original sampler axes.
- Other levels, DAL banks and executable revisions. No generalization from just
  index bounds or matching file size is made.
- There is no pixel-perfect in-game reference capture comparison yet; static
  address tracing establishes record selection, not complete original appearance.

# File formats: what's hiding in DATA?

Here's the format map we wish we'd had when we first started. This is deliberately a **practical guide to the structures we've verified**, not a claim that every byte of every file has been decoded.

All multibyte integer fields shown here are **little-endian**. Unless noted, indices in the port are **zero-based**.

## At a glance

| File/resource | Contents | Reconstruction entry point |
| --- | --- | --- |
| `GAME.LAB` | 30 level records, sector tables, enemies, objects, routes, codes | [`game_lab.cpp`](../../src/game_lab.cpp) |
| `PAL<n>.PAL` | VGA palette, 256 × RGB 6-bit | [`texture_bank.cpp`](../../src/texture_bank.cpp) |
| `DALLE<n>.DAL` | 60×74 indexed wall materials | [`texture_bank.cpp`](../../src/texture_bank.cpp) |
| `SOL<n>.DAS` | Floor and ceiling, two 128×128 indexed planes | [`texture_bank.cpp`](../../src/texture_bank.cpp) |
| `OBJET<n>.SPR` | Decorative sprites and animation source images | [`spr_format.cpp`](../../src/spr_format.cpp) |
| `MONS<n>.SPR` | Enemy sprite images | [`sprite_bank.cpp`](../../src/sprite_bank.cpp) |
| `MONS<n>.SPT` | Per-image 8×8 hit masks | [`enemy_system.cpp`](../../src/enemy_system.cpp) |
| `ADALLE.SPR` | Wall-animation source patches | [`wall_animation.cpp`](../../src/wall_animation.cpp) |
| `*.DSS` / sound bank | Audio sample data and loops | [`audio_data.cpp`](../../src/audio_data.cpp) |
| `*.ANM` | Full-screen/delta video animation | [`anm_format.cpp`](../../src/anm_format.cpp) |

## GAME.LAB: directory, then levels

```text
+0x00  uint16 levelCount = 30
+0x02  uint32 offsets[31]
        ... packed level records ...
```

A level has bank IDs (`PAL`, `DALLE`, `OBJET`, `MONS`) and offset fields to its route, connection, enemy and code tables. The sector directory starts at level offset `+0x12`: `sectorCount`, followed by repeated triples of `sectorOffset`, `gx`, `gy` words.

Within a sector: neighbour list, vertices, walls, EntityA records, polygons and connections. A wall or EntityA is **four 16-bit words / eight bytes**; an enemy is **fourteen 16-bit words / twenty-eight bytes**.

This format is described in detail, with a real start-position example, in [Levels](../systems/levels.md). Don't use fixed offsets from level 1 to parse the rest; the header provides the section locations for a reason.

## SPR: a sprite is a little file inside a file

The reader checks a four-byte opening area containing a word it preserves and an image count, then an offset table:

```text
file +0x02: uint16 imageCount
file +0x04: uint32 relativeOffsets[imageCount]
image record:
    uint16 width
    uint16 height
    uint16 anchorX
    uint16 anchorY
    uint8  pixels[width * height]
```

Image pixel bytes are palette indices. For regular sprite drawing, index **0 is transparent**; do not turn every RGB-black pixel transparent. The parser validates that each image's dimensions agree with its remaining bytes. See [`src/spr_format.cpp`](../../src/spr_format.cpp).

Some banks contain images that aren't independent objects. They're **patch sources** for animation; others include entire multi-frame sequences. The first image's presence doesn't tell you which images the level actually places.

## PAL and DALLE: the VGA pair

`PAL<n>.PAL` is **768 bytes**: 256 successive `(R,G,B)` triples, components in 0–63.

DALLE textures are fixed **4,440-byte** records. There is no per-image header in that bank:

```text
recordSize = 60 * 74
byteOffset = recordStart + x * 74 + y
```

That's **60 columns, 74 pixels high**, column-major. Most original DALLE banks contain 54 records; DALLE5 contains 35. These counts are supported directly in the reader. Applying an extra 90-degree rotation while decoding them is a known early mistake.

## SOL: not another DALLE texture

A `SOL<n>.DAS` file is **32,768 bytes**, interpreted as rows of 256 indices:

```text
128 floor indices + 128 ceiling indices
```

The port builds two separate **128×128** images. The bank number comes from the level's DALLE/SOL selector, and their colours from its PAL bank.

## SPT: tiny masks that matter a lot

Each MONS sprite image has an associated **8-byte / 8×8** hit mask. A **1 bit means reject**; a **0 bit means the projectile may hit**. The frame and its flip state affect projection, but the original doesn't simply mirror the mask bytes because the image flipped.

See [Enemies](../systems/enemies.md). This is a combat data structure, **not** an animation list.

## DSS1: audio with loop instructions

The sample reader expects a `DSS1` signature, a **44-byte header** and PCM sample bytes. Fields include:

| Byte offset in sample | Data |
| --- | --- |
| `+0` | ASCII `DSS1` |
| `+4` | Sample name (24 bytes) |
| `+28` | Sample rate (16-bit; verified 5000 Hz) |
| `+30` | PCM sample count (32-bit) |
| `+34` | Loop flag (16-bit) |
| `+36`, `+40` | Loop start/end (32-bit) |

A sound bank wraps repeated entries with volume, continuous and priority metadata. The code checks bounds and rejects inconsistent loop ranges. Those bits are the difference between a proper door sound and a voice that never finishes.

## ANM1: a small film projector

`ANM1` begins with a four-byte signature, frame count and a VGA palette; the decoded frame is **320×200 (64,000 indexed pixels)**. The first frame is full; later frames may apply delta updates. Image data uses a 4 KiB LZSS dictionary and the reader enforces record boundaries.

The parser is [`src/anm_format.cpp`](../../src/anm_format.cpp). You don't need a modern video codec to reconstruct these little sequences, but you do need to respect their delta table and palette.

## One very important limitation

These layouts are **verified for the original resource sets examined**, with bounds checks and some exceptional shipped data handled explicitly. They are not a generic promise that every similarly named file from another game/version will parse. This repository doesn't include the original assets; you must supply them yourself.

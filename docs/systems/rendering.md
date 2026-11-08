# Drawing a 1993 world on a modern screen

A lot of the reconstruction starts with a deceptively simple question: *which texture should this wall have?* Then comes the harder one: *why is it rotated, mirrored, stretched or bleeding onto the next wall?*

The port has a software rendering pipeline for a world built from sectors, walls, doors, animated materials, sprites, floors and ceilings. The original file formats are odd by today's standards — which is exactly why we document them.

## From level data to pixels

```text
GAME.LAB sector walls + door state
            |
            v
  wall material / geometry
            |
            v
      ray / projection
            |
            v
    software framebuffer
            +-- SOL floors and ceilings
            +-- OBJET / MONS / projectiles with depth
            +-- HUD and original modal screens
            |
            v
        SDL3 display
```

The important bit is that SDL3 is mainly the **modern presentation and input layer**. Calculating the scene remains CPU-side, and that has implications for performance. See [Performance](../reference/performance.md).

## DALLE: the wall texture that was sideways on disk

A typical original DALLE bank stores **60 × 74 palette indices** per texture, **4,440 bytes per image**. The source layout is **column-major**: bytes go from top to bottom within a source column.

```text
source byte for texture (x,y) = textureStart + x * 74 + y
```

A normal modern row-major image buffer wants `y * width + x`, so the loader transposes the storage order **without an extra flip or quarter turn**. Early experiments assumed a 74×60 row-major texture; tracing the original rasterizer corrected that mistake.

| Bank case | Original records | Size |
| --- | ---: | ---: |
| Most DALLE banks | 54 | 239,760 bytes |
| DALLE5 | 35 | 155,400 bytes |
| One texture | — | 4,440 bytes |

The visible material index is based on `wall.type & 0x00ff`. The original drawing API has a one-based interface internally, but it cancels that adjustment before final DALLE addressing. Don't “fix” the zero-based texture selection because it looks unusual.

The loader is [`src/texture_bank.cpp`](../../src/texture_bank.cpp); wall selection and projection pass through [`wall_material.cpp`](../../src/wall_material.cpp) and [`first_person_view.cpp`](../../src/first_person_view.cpp).

## PAL: six-bit VGA colours

A PAL file contains **256 RGB entries**, three bytes each: **768 bytes**. Each component is in the original **0–63 range**. The port expands it to the modern 0–255 range, keeping the same rounding used by the research extractor.

This is also why simply adding a modern grey overlay to an interface doesn't necessarily reproduce a DOS palette change. Some original effects operate at the palette level, and a few are still under investigation.

## SOL: two 128×128 planes

The floor and ceiling live in **`SOL<n>.DAS`**, selected using the level's DALLE/SOL bank ID, **not** from the currently visible wall type.

One file is **32,768 bytes**: two 128×128 indexed images. In the loader the floor uses the left half of each 256-byte row and the ceiling uses the right half.

```text
row y: 128 floor indices | 128 ceiling indices
        <------------- 256 bytes -------------->
```

The original F control toggles floor and ceiling texturing together. The port also supports that toggle; rendering the planes is considerably more expensive than drawing a flat background at high output resolution.

## Sprites: pixels plus a meaningful anchor

The original SPR images store width, height, **anchor X/Y**, and palette indices. **Index 0 alone is transparent**, regardless of its RGB colour. Other pixels that happen to look black must remain opaque.

The anchor isn't decorative metadata. It defines where the image sits relative to the world point when projected; ignore it and fingers, weapons, decorations or enemy parts jump around the screen.

The engine draws OBJET decorations, enemies and projectiles with a shared depth/occlusion strategy. Some sprites are horizontally flipped; some change complete images; others mutate image pixels with patches and column rotations. Those details are covered in [Decorations](decorations.md) and [Enemies](enemies.md).

## Animated wall materials

The original ADALLE bank has **20 images**, which are copied into particular DALLE textures as patches. Recovered animated examples in DALLE1 include materials **11, 12 and 13**. The animation does **not** mean cycling through neighbouring full wall textures; it updates regions of the material in memory.

The port's step timing for those materials is an adaptation, not a calibrated reproduction of DOS wall-animation seconds. The implementation is in [`wall_animation.cpp`](../../src/wall_animation.cpp).

## Why doors are a special case

A door's geometric opening, its texture sampling span and its sector connection-state word are related but not identical. If you change how the door retracts without updating texture coordinates accordingly, a surface can stretch or mirror. A moving-door texture test currently exposes such a problem. See [World](world.md) for connection state and [Known differences](../reference/known-differences.md) for current test status.

## What still needs work

The major visual items include elevator textures leaking onto corridor walls, exact DOS timing for several effects, palette transitions (damage, light flicker, grayscale modal background), special death effects, aspect-ratio testing, and optimizing the expensive plane renderer.

**Original evidence:** DALLE column addressing `0x15795–0x15925`; SOL loading `0x7a08–0x7c9d` and plane drawing `0x12ac4` onward; ADALLE composition `0x152ff–0x15309`; sprite projection and anchor use near `0x11f83–0x1207f`.

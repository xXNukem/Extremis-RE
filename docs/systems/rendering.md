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

## Static wall raycasting in the port

The renderer builds a 512-unit grid at level load for levels with at least 64 static walls. Padded segment bounds cover cell edges, corners and the endpoint tolerance of the prepared intersector. DDA intersects each static ID once, on first encounter, and can stop once both next grid crossings are beyond a conservative bound derived from a static impact. Accepted intersections are replayed in original sector/wall order, preserving the distance and facing tie rules. A ray without a static hit continues to the grid exit. The prepared scan remains available for small scenes and unsupported grid inputs. Doors retain their independent prepared scan after static selection, using the current animation phase. The index is rebuilt when the active geometry is replaced.

`raycast_benchmark GAME.LAB check` compares exact prepared hits in all 30 levels. `raycast_benchmark GAME.LAB measure` reports per-frame raycasting cost, candidate counts and index construction and memory for the four benchmark levels in the selected build configuration.

### Level 23 raycasting peaks

The Debug peak was reproduced on arm64 macOS with the local renderer and `GAME.LAB` SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`. At 2292×900, the same position in level 23 (3500, 300, sector index 4) took 3.62 ms at angle 13π/8 and 10.99 ms at π/4 in an isolated 25-frame raycasting run. The original connection words were used for timing. Camera directions are in radians in the port's coordinate system.

| Debug baseline at 2292×900 | Level 1 start | Level 23, 13π/8 | Level 23, π/4 |
| --- | ---: | ---: | ---: |
| Scene preparation, ms | 0.015 | 0.053 | 0.053 |
| Clear deduplication masks, ms | 0.076 | 0.120 | 0.134 |
| Traverse cells and OR masks, ms | 0.487 | 0.283 | 0.914 |
| Enumerate, intersect and select static walls, ms | 2.053 | 0.754 | 6.987 |
| Independent door pass, ms | 0.818 | 2.608 | 3.114 |
| Cells per ray | 8.47 | 3.17 | 13.95 |
| Cell mask merges per frame | 20,342 | 10,552 | 69,099 |
| Unique static candidates tested per frame | 92,552 | 34,136 | 298,744 |
| Static intersections passing the segment test | 12,712 | 4,778 | 39,216 |
| Door faces tested per frame | 34,380 | 139,812 | 139,812 |
| Door intersections passing the segment test | 2,267 | 0 | 2,850 |

The stage timings come from a separate instrumented run and include sampling overhead. Timing the static selector separately, with empty clock-pair cost subtracted, estimated 0.02 ms in the quick level 23 view and 0.11 ms in the peak. The static candidate loop accounts for most of the change. DDA visits more cells in the slow direction and continues to the grid boundary after the nearest wall. The masks already prevent repeated tests of the same wall. The door scan is substantial in both views, and test count is identical between them. These timings do not isolate cache misses. Levels 1, 22 and 23 used the grid for all benchmark rays. Level 20 used the production small-scene scan (30 walls and one prepared door face at its starting camera).

The first optimization made the indexed static loop consume the determinant, distance and segment fraction directly. It passed accepted intersections to the existing selector in source order. It preserved the arithmetic, tolerances and tie handling, and the grid, door scan and fallback algorithms. That step changed neither the number of geometric intersection tests nor the memory requirement. Observed Debug preparation medians ranged from 0.002 to 0.060 ms across both versions, with about 0.053 ms at the level 23 cameras. Grid allocations including reserved capacity were 3,272 bytes for level 1, 14,400 for level 22 and 8,824 for level 23 at that stage.

Both builds passed the 39 CTest cases and the 2,469,600-ray campaign check against the prepared scan, including a locked-door state. A separate byte comparison against the pre-change production route covered 3,819,648 rays and 812 rendered frames across all 30 campaign levels in each build. It included different cameras and directions, door phases 0, 17, 39, 59, 60 and 90, locked doors, normal and distorted rendering, and compared selected segment IDs, hit distance and material, texture fraction/span, coverage, per-column and full depth, world colors and the final SDL surface. Additional snapshots compared all seven benchmark views at all three resolutions with the original connection words. The existing edge fixture covers diagonals, coincident walls, near ties, misses, unsupported camera coordinates and the small-scene route.

#### Frame benchmarks

Identical poses, original door connection words, textured planes, level decorations and the original HUD were rendered through SDL's software backend. Two runs in opposite before/after order used 25 measured frames per sample after three warm-ups. Values are the median of the two run medians, in milliseconds. RAY uses the same scoped timers in both builds. Frame time covers draw, composition and software present. The DOS output is 320×200 with a 320×150 world and HUD. The other resolutions give the internal world size. Camera A is (3500, 300), sector index 4. Camera B is (2050, 700), sector index 5. These are fixed-camera render benchmarks without simulation or display synchronization.

| Level | Camera | Resolution | Debug RAY | Debug frame | Release RAY | Release frame |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| 1 | Start | 320×200 | 0.484 → 0.344 | 0.967 → 0.841 | 0.048 → 0.049 | 0.245 → 0.255 |
| 1 | Start | 2292×900 | 3.365 → 2.212 | 16.113 → 14.972 | 0.184 → 0.187 | 3.596 → 3.673 |
| 1 | Start | 3420×1343 | 5.089 → 3.298 | 31.840 → 29.946 | 0.281 → 0.280 | 7.900 → 7.871 |
| 20 | Start | 320×200 | 0.234 → 0.232 | 0.706 → 0.700 | 0.007 → 0.007 | 0.108 → 0.108 |
| 20 | Start | 2292×900 | 1.642 → 1.606 | 14.111 → 13.891 | 0.040 → 0.040 | 3.637 → 3.520 |
| 20 | Start | 3420×1343 | 2.476 → 2.413 | 28.510 → 28.125 | 0.058 → 0.060 | 7.708 → 7.672 |
| 22 | Start | 320×200 | 0.686 → 0.540 | 1.159 → 1.008 | 0.031 → 0.031 | 0.137 → 0.138 |
| 22 | Start | 2292×900 | 4.672 → 3.553 | 16.914 → 15.579 | 0.204 → 0.202 | 3.791 → 3.786 |
| 22 | Start | 3420×1343 | 6.948 → 5.284 | 32.399 → 30.167 | 0.305 → 0.301 | 8.402 → 8.196 |
| 23 | Start | 320×200 | 1.152 → 0.818 | 1.665 → 1.325 | 0.045 → 0.045 | 0.157 → 0.156 |
| 23 | Start | 2292×900 | 8.015 → 5.438 | 20.312 → 17.378 | 0.310 → 0.303 | 4.169 → 4.098 |
| 23 | Start | 3420×1343 | 11.771 → 8.091 | 36.978 → 32.801 | 0.465 → 0.457 | 9.142 → 8.988 |
| 23 | A · π/4 | 320×200 | 1.703 → 1.175 | 2.202 → 1.665 | 0.073 → 0.073 | 0.175 → 0.174 |
| 23 | A · π/4 | 2292×900 | 11.153 → 7.359 | 24.269 → 20.052 | 0.482 → 0.466 | 4.058 → 3.923 |
| 23 | A · π/4 | 3420×1343 | 16.606 → 11.048 | 43.427 → 37.547 | 0.753 → 0.721 | 8.725 → 8.417 |
| 23 | A · 13π/8 | 320×200 | 0.565 → 0.504 | 1.089 → 1.021 | 0.022 → 0.023 | 0.128 → 0.127 |
| 23 | A · 13π/8 | 2292×900 | 3.730 → 3.255 | 17.154 → 16.250 | 0.144 → 0.145 | 3.797 → 3.700 |
| 23 | A · 13π/8 | 3420×1343 | 5.441 → 4.898 | 32.473 → 31.896 | 0.222 → 0.221 | 8.462 → 8.219 |
| 23 | B · π/4 | 320×200 | 1.716 → 1.159 | 2.215 → 1.649 | 0.076 → 0.074 | 0.179 → 0.174 |
| 23 | B · π/4 | 2292×900 | 11.321 → 7.299 | 24.419 → 20.010 | 0.495 → 0.482 | 4.071 → 3.942 |
| 23 | B · π/4 | 3420×1343 | 16.960 → 10.930 | 43.713 → 37.379 | 0.767 → 0.741 | 8.722 → 8.434 |

### Safe early exit for static walls

The previous two-stage DDA accumulated candidate masks all the way to the grid exit. It then intersected the candidates in source order. Stopping at a nearby hit plus one tie tolerance would be unsafe in general. For example, three same-facing hits in descending distance order can have adjacent distances within tolerance while the extremes are outside it. The first can hold the middle off, allowing the third to win. Removing the first lets the middle hold off the third. The edge fixture now checks that exact case.

The cutoff uses any confirmed hit as a witness at depth `t`, keeping the smallest witness found so far. With `N` static walls there are at most `N - 1` other hits. Among `N` consecutive open intervals above `t`, one must be empty. Each interval has width `4 * rayTieTolerance * max(1, t)`. With the grid's 65,536-wall limit, nearby distances in this range have tie tolerances smaller than that width. Distances much larger than the range are separated by more than their relative tolerance as well. Hits on opposite sides of the empty interval therefore compare by distance regardless of facing or which one comes first. Before the first hit on the witness side, the farther side cannot influence that hit. After it, the farther side cannot replace the selected result. Replaying the cached hits in source order preserves every comparison that can affect the result.

The cutoff is compared with both next crossing times in DDA coordinates. The existing padded AABBs cover accepted endpoint extensions and transverse rounding. The extra parameter margin is twice the existing backtrack bound, which covers the longitudinal displacement between the computed depth and the reconstructed point on the segment, with room for crossing-time roundoff. Thus the walk visits the padded cell of every hit on the witness side before stopping. A wall spanning multiple cells can supply a witness ahead of the current cell, but the walk continues through the bound derived from that witness. A ray with no static witness keeps the full walk. Door phases, the strict door-versus-static depth comparison and the fallback are unchanged.

The scratch is allocated once with the existing grid. Each new candidate runs the same direct determinant and division path as before. A second mask marks accepted IDs and an array stores their distance and texture fraction for replay. There are no per-ray allocations. The long-diagonal fixture tests early discovery, traversal past the first witness, single testing of repeated IDs and a miss that traverses to the far side.

At 2292×900 the scratch adds 14,904 bytes in level 23, 4,080 in level 1 and 13,112 in level 22, including the vector objects. Level 20 retains its scan and allocates no grid scratch. Scene preparation in Debug remained about 0.053 ms in the level 23 views. In an isolated diagnostic that computed the cutoff and continued through the whole grid, median RAY rose by 0.19 ms and 0.26 ms in the two peak views compared with the previous implementation. The quick view incurred 0.18 ms in the same diagnostic. This estimates the net expense of the new candidate and replay path with no work removed, including a small cost to record whether a cutoff would have happened.

Both builds passed 39 CTest cases and the 2,469,600-ray check against the prepared scan. An additional campaign check swept every integer opening phase from 0 through 60 with the opening flag set and matched all 3,535,560 rays. A deterministic synthetic run compared another 500,000 rays on 100 geometries, including near-parallel directions and cameras at grid boundaries. The byte-for-byte comparison against the previous DDA covered 3,819,648 rays and 812 frames across the 30 levels per build. It checked selected IDs, distance, material, texture coordinates, misses, coverage, depth, world pixels, HUD and final output. All 21 benchmark configurations also matched byte for byte in both builds.

#### Early-exit benchmark and verification

The timings use the same cameras, door states, software renderer and sampling procedure as the previous table. The comparison starts from the version with the direct static intersector. Each static candidate in the table represents one intersection test. Tests of dynamic faces are unchanged in every row. At 2292×900 there were 139,812 tests of dynamic faces in both A views and 137,520 in view B. All times are in milliseconds.

| Level | View | Resolution | Cells | Static tests | Debug RAY | Debug frame | Release RAY | Release frame |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | Start | 320×200 | 2,689 → 1,033 | 12,654 → 5,353 | 0.415 → 0.294 | 1.014 → 0.906 | 0.050 → 0.026 | 0.247 → 0.167 |
| 1 | Start | 2292×900 | 19,402 → 6,761 | 92,552 → 35,867 | 2.198 → 1.516 | 14.956 → 14.338 | 0.184 → 0.091 | 3.649 → 3.513 |
| 1 | Start | 3420×1343 | 28,951 → 10,086 | 138,107 → 53,505 | 3.292 → 2.264 | 29.756 → 28.696 | 0.278 → 0.140 | 7.914 → 7.668 |
| 20 | Start | 320×200 | 0 → 0 | 9,600 → 9,600 | 0.232 → 0.231 | 0.701 → 0.701 | 0.008 → 0.007 | 0.108 → 0.110 |
| 20 | Start | 2292×900 | 0 → 0 | 68,760 → 68,760 | 1.606 → 1.609 | 13.918 → 13.931 | 0.042 → 0.041 | 3.624 → 3.535 |
| 20 | Start | 3420×1343 | 0 → 0 | 102,600 → 102,600 | 2.415 → 2.446 | 28.176 → 28.447 | 0.060 → 0.062 | 7.950 → 7.732 |
| 22 | Start | 320×200 | 2,545 → 892 | 10,633 → 5,399 | 0.542 → 0.483 | 1.012 → 0.958 | 0.031 → 0.025 | 0.137 → 0.130 |
| 22 | Start | 2292×900 | 19,090 → 6,278 | 79,021 → 38,533 | 3.577 → 3.160 | 15.696 → 15.254 | 0.202 → 0.152 | 3.856 → 3.809 |
| 22 | Start | 3420×1343 | 28,484 → 9,366 | 117,897 → 57,489 | 5.303 → 4.704 | 30.358 → 29.732 | 0.304 → 0.225 | 8.384 → 8.337 |
| 23 | Start | 320×200 | 3,266 → 1,082 | 25,443 → 12,009 | 0.822 → 0.662 | 1.326 → 1.167 | 0.045 → 0.028 | 0.156 → 0.139 |
| 23 | Start | 2292×900 | 24,323 → 7,609 | 185,740 → 84,349 | 5.466 → 4.382 | 17.519 → 16.294 | 0.305 → 0.187 | 4.148 → 4.025 |
| 23 | Start | 3420×1343 | 36,289 → 11,353 | 277,162 → 125,849 | 8.120 → 6.516 | 33.304 → 31.359 | 0.459 → 0.278 | 9.133 → 8.910 |
| 23 | A · π/4 | 320×200 | 4,624 → 699 | 44,788 → 7,077 | 1.183 → 0.614 | 1.674 → 1.099 | 0.073 → 0.031 | 0.175 → 0.133 |
| 23 | A · π/4 | 2292×900 | 31,984 → 4,940 | 298,744 → 49,372 | 7.448 → 3.989 | 20.359 → 16.648 | 0.476 → 0.186 | 4.040 → 3.749 |
| 23 | A · π/4 | 3420×1343 | 47,724 → 7,372 | 445,780 → 73,682 | 11.105 → 6.015 | 37.931 → 32.512 | 0.735 → 0.279 | 8.685 → 8.230 |
| 23 | A · 13π/8 | 320×200 | 993 → 659 | 4,847 → 4,558 | 0.503 → 0.523 | 1.017 → 1.038 | 0.023 → 0.021 | 0.129 → 0.127 |
| 23 | A · 13π/8 | 2292×900 | 7,263 → 4,694 | 34,136 → 32,201 | 3.314 → 3.430 | 16.520 → 16.594 | 0.148 → 0.134 | 3.795 → 3.783 |
| 23 | A · 13π/8 | 3420×1343 | 10,836 → 7,005 | 50,932 → 48,049 | 4.907 → 5.151 | 32.001 → 32.300 | 0.224 → 0.205 | 8.547 → 8.433 |
| 23 | B · π/4 | 320×200 | 4,667 → 650 | 44,522 → 6,820 | 1.163 → 0.587 | 1.652 → 1.077 | 0.074 → 0.029 | 0.177 → 0.132 |
| 23 | B · π/4 | 2292×900 | 32,513 → 4,693 | 304,344 → 49,492 | 7.334 → 3.911 | 20.107 → 16.812 | 0.483 → 0.184 | 4.153 → 3.749 |
| 23 | B · π/4 | 3420×1343 | 48,513 → 7,002 | 454,148 → 73,844 | 11.071 → 5.829 | 37.947 → 32.582 | 0.748 → 0.278 | 8.706 → 8.242 |

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

The software renderer advances SOL texture coordinates across each scanline with 32.32 fixed-point additions. It uses the existing material sampler near texel boundaries, outside the bounded coordinate range, for widths above 8192, and for other texture dimensions. Wall coverage still skips hidden samples. The `floor_ceiling` test compares scanlines against the material sampler pixel by pixel.

## Sprites: pixels plus a meaningful anchor

The original SPR images store width, height, **anchor X/Y**, and palette indices. **Index 0 alone is transparent**, regardless of its RGB colour. Other pixels that happen to look black must remain opaque.

The anchor isn't decorative metadata. It defines where the image sits relative to the world point when projected; ignore it and fingers, weapons, decorations or enemy parts jump around the screen.

The engine draws OBJET decorations, enemies and projectiles with a shared depth/occlusion strategy. Some sprites are horizontally flipped; some change complete images; others mutate image pixels with patches and column rotations. Those details are covered in [Decorations](decorations.md) and [Enemies](enemies.md).

The port groups adjacent screen columns that sample the same sprite texel. For each row it skips transparent runs and reuses the packed colour across each opaque run, with a depth test for each opaque pixel. The first-person renderer initializes the depth buffer to infinity and copies in only finite wall depths. Projected sprite depths are finite, so an opaque pixel that passes the depth test in this buffer can write depth immediately. The generic sprite API also accepts buffers containing NaN, where a passing pixel writes colour and preserves the original depth bits. Impacts that match a wall surface still use the inverse-depth slope test. With barrel distortion enabled, the Debug SPRITES timer also includes that distortion pass.

### Debug sprite peaks at MAX

The local CPU benchmark used level 23 from `GAME.LAB` (SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`). Each comparison held the camera, assets, draw order and initial world state fixed. The frame timer covered SDL clear, world rendering, HUD and composition where applicable, texture upload and software present. The exact 320×200 **world** size was exercised without HUD because the DOS HUD path uses a 320×150 world at 1×. The 2292×900 and MAX measurements used the regular HUD path. Timing excluded game simulation and the Debug overlay. The overlay's SPRITES figure can be reconstructed by adding the separate barrel distortion time.

The raster pass walks decorations, enemies, enemy effects and projectiles in input order. The animation and scene updates happen before the pass. In Debug at MAX, measured projection across all lists took 0.005–0.007 ms and bounds setup of intersecting rectangles took up to 0.002 ms. A separate diagnostic split the opaque kernel into a depth scan and a colour/depth write loop, using scratch indices between them. These are instrumented timings that include scratch and clock overhead. In view A the split measured 7.6 ms for the depth scan and 10.5 ms for writes, versus 2.1 ms for coordinate/run setup and colour sampling and 3.6 ms for transparency checks. View B measured 8.2 ms for the scan, 8.9 ms for writes, 4.6 ms for sampling and 8.8 ms for transparency. The uninstrumented rows below give the production timings.

| View | Camera `(x, y, radians)` | Sprites with writes / intersecting rectangles | Candidate pixels | Transparent | Occluded opaque | Colour writes |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| A, image 21, sector 41 entity 3 | (7836.36039, 3563.63961, 5.49778714) | 2 / 10 | 10,024,960 | 3,557,425 | 355,209 | 6,112,326 |
| B, image 17, sector 13 entity 0 | (636.36039, 2763.63961, 5.49778714) | 2 / 64 | 7,362,847 | 2,035,258 | 1,783,175 | 3,544,414 |
| C, behind walls | (7330, 3900, 0) | 0 / 11 | 7,623,558 | 2,607,425 | 5,016,133 | 0 |
| D, many visible | (3800, 2890, 4.71238898) | 7 / 30 | 6,657,991 | 2,767,432 | 1,252,641 | 2,637,918 |
| E, default spawn | (4000, 2600, 5.06145483) | 2 / 20 | 1,822,936 | 774,387 | 569,327 | 479,222 |
| F, partly occluded at distance 320 | (7673.72583, 3726.27417, 5.49778714) | 3 / 17 | 7,856,121 | 3,542,134 | 943,347 | 3,370,640 |

Counts include overlap between objects and use screen-clipped bounds. A colour write also writes depth on the ordinary world path. In A, the decoration alone projected to approximately 4422×6137 and took 9.62 ms for 4,246,962 writes. Enemy record 56 took another 4.22 ms for 1,865,364 writes. In B the decoration projected to 2610×6137 and took about 7.2 ms for 3,155,689 writes. Enemy record 13 took about 2.6 ms and rejected 1,271,444 opaque pixels. In C the nearest examined decoration (sector 41 entity 0) took about 2.4 ms without a write. Screen clipping already excludes off-screen pixels, while geometry-hidden pixels inside the clipped bounds still incur sampling and depth tests.

Effects and projectiles were idle in these six views. A separate MAX fixture with the same assets measured 3.40 ms for decorations, 1.74 ms for enemies, 8.82 ms for deliberately close enemy death effects and 0.28 ms for projectiles. It exercised the same fill and rejection costs with active effects. Object, sector and image indices are zero-based.


Each timing is the median of 21 warm paired frames per version, in milliseconds. The timed order alternated between versions. This table shows sprites separately with barrel distortion disabled.

| View | World size | Debug sprites | Debug frame | Release sprites | Release frame |
| --- | --- | ---: | ---: | ---: | ---: |
| A | 320×200 | 0.335 → 0.270 | 1.640 → 1.578 | 0.052 → 0.047 | 0.193 → 0.188 |
| A | 2292×900 | 6.999 → 4.907 | 25.731 → 23.581 | 1.289 → 1.099 | 5.655 → 5.474 |
| A | 3420×1343 | 15.326 → 10.570 | 51.031 → 46.211 | 2.909 → 2.838 | 12.443 → 12.427 |
| B | 320×200 | 0.336 → 0.296 | 2.206 → 2.168 | 0.052 → 0.049 | 0.219 → 0.216 |
| B | 2292×900 | 6.648 → 5.247 | 29.117 → 27.708 | 1.294 → 1.201 | 5.895 → 5.790 |
| B | 3420×1343 | 13.594 → 10.632 | 54.786 → 51.833 | 3.076 → 2.916 | 13.036 → 12.874 |
| C | 320×200 | 0.349 → 0.354 | 1.535 → 1.540 | 0.058 → 0.058 | 0.195 → 0.195 |
| C | 2292×900 | 3.829 → 3.585 | 21.601 → 21.382 | 0.964 → 0.964 | 5.282 → 5.268 |
| C | 3420×1343 | 7.427 → 6.930 | 41.965 → 41.487 | 2.585 → 2.588 | 12.056 → 12.038 |
| D | 320×200 | 0.428 → 0.408 | 1.990 → 1.968 | 0.057 → 0.057 | 0.211 → 0.211 |
| D | 2292×900 | 5.622 → 4.672 | 25.129 → 24.212 | 1.157 → 1.099 | 5.752 → 5.687 |
| D | 3420×1343 | 11.158 → 9.096 | 47.211 → 45.120 | 3.180 → 2.990 | 13.252 → 13.050 |
| E | 320×200 | 0.135 → 0.131 | 1.726 → 1.724 | 0.016 → 0.017 | 0.171 → 0.171 |
| E | 2292×900 | 2.103 → 1.972 | 21.867 → 21.707 | 0.425 → 0.402 | 4.941 → 4.925 |
| E | 3420×1343 | 3.881 → 3.481 | 40.307 → 39.919 | 1.143 → 1.083 | 11.068 → 11.003 |
| F | 320×200 | 0.355 → 0.330 | 1.756 → 1.734 | 0.046 → 0.047 | 0.193 → 0.194 |
| F | 2292×900 | 5.807 → 4.649 | 25.113 → 23.932 | 1.404 → 1.353 | 6.039 → 5.992 |
| F | 3420×1343 | 12.084 → 9.531 | 48.489 → 45.898 | 3.468 → 3.136 | 13.227 → 12.878 |

With barrel distortion enabled at MAX, the separate effect took roughly 5.1–5.5 ms in Debug and 2.0–2.2 ms in Release. In view F, the Debug sprite pass fell from 12.16 to 9.61 ms and the full frame fell from 53.89 to 51.30 ms with the effect on.

In each build, 36 benchmark cases and 48 fixture cases matched byte for byte between baseline and candidate. The checks covered pre-distortion colour, depth, post-distortion colour, HUD/intermediate composition buffers and the final SDL software surface. The fixture used two enemy image frames, overlapping transparent billboards, active effects and projectiles, a diagonal wall and open/closed doors across 4:3, 16:9, 21:9 and Auto with distortion both enabled and disabled. A separate raster comparison covered equal depth, nearer and farther depth, infinity and preservation of a NaN depth payload. The repository suite passed 39/39 in each build.

### Early rejection by wall depth

Before drawing any sprites, the first-person renderer snapshots the wall depth for each screen column. The existing Z buffer starts with that value on every row. Earlier sprites can only decrease those depths, including the minimum update on matched wall impacts. For an ordinary billboard, a wall depth at or in front of the billboard depth therefore proves that every pixel in that column would fail. Run construction skips those columns and keeps separate ranges across gaps. A fully hidden rectangle returns before the row loop. Matching wall impacts still inspect live per-pixel depth and the inverse-depth slope. A wall impact that falls back to the ordinary billboard rule can use the same column bound.

The wall coverage intervals used by the plane renderer describe colour coverage. The sprite Z buffer has wall depth across the whole column, including outside those intervals. A separate hierarchy of row or block maxima could summarize live sprite occlusion, but would need maintenance after sprite writes or an extra pass over the framebuffer. The chosen bound uses one reusable row of depth and leaves sprite-on-sprite rejection to the existing live Z test.

The paired benchmark used the six level-23 views above and the previous depth-comparison optimization as its baseline. Results are medians of 15 warm draws per version and use SDL software presentation. The table excludes barrel distortion from both times. Each timing cell shows before → after in milliseconds. Counts cover clipped rectangles on the ordinary raster path. Early rejections include transparent pixels skipped before texture sampling, and avoided tests count only the pixels whose texels were opaque in the baseline.

| View | World size | Candidate pixels | Early rejections | Avoided depth tests | Debug sprites | Debug frame | Release sprites | Release frame |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 320×200 | 138,096 | 10,096 | 4,370 | 0.271 → 0.210 | 1.586 → 1.522 | 0.050 → 0.039 | 0.189 → 0.178 |
| A | 2292×900 | 4,502,266 | 376,666 | 159,524 | 4.924 → 4.102 | 23.522 → 22.714 | 1.112 → 0.959 | 5.136 → 4.975 |
| A | 3420×1343 | 10,024,960 | 838,840 | 355,209 | 10.576 → 9.172 | 46.101 → 44.761 | 2.827 → 2.235 | 11.658 → 11.012 |
| B | 320×200 | 104,115 | 8,515 | 3,611 | 0.301 → 0.219 | 2.189 → 2.084 | 0.052 → 0.039 | 0.211 → 0.198 |
| B | 2292×900 | 3,306,052 | 558,352 | 229,773 | 5.319 → 3.284 | 27.831 → 25.831 | 1.199 → 0.844 | 5.484 → 5.118 |
| B | 3420×1343 | 7,362,847 | 1,244,139 | 511,731 | 10.541 → 6.961 | 51.770 → 48.233 | 2.938 → 2.010 | 12.678 → 11.792 |
| C | 320×200 | 102,984 | 102,984 | 69,197 | 0.351 → 0.006 | 1.540 → 1.193 | 0.059 → 0.001 | 0.185 → 0.127 |
| C | 2292×900 | 3,424,596 | 3,424,596 | 2,253,451 | 3.787 → 0.013 | 21.893 → 18.088 | 1.041 → 0.004 | 5.084 → 3.839 |
| C | 3420×1343 | 7,623,558 | 7,623,558 | 5,016,133 | 6.945 → 0.014 | 41.281 → 34.351 | 2.748 → 0.006 | 11.838 → 9.123 |
| D | 320×200 | 92,248 | 13,368 | 6,207 | 0.407 → 0.310 | 1.968 → 1.869 | 0.063 → 0.041 | 0.217 → 0.195 |
| D | 2292×900 | 2,989,534 | 546,729 | 239,084 | 4.773 → 3.256 | 24.311 → 22.778 | 1.128 → 0.766 | 5.561 → 5.150 |
| D | 3420×1343 | 6,657,991 | 1,216,603 | 531,909 | 9.204 → 6.610 | 45.222 → 42.616 | 3.133 → 2.340 | 13.113 → 12.290 |
| E | 320×200 | 20,248 | 12,689 | 5,429 | 0.132 → 0.039 | 1.724 → 1.630 | 0.018 → 0.006 | 0.179 → 0.167 |
| E | 2292×900 | 818,643 | 600,343 | 255,748 | 2.032 → 0.417 | 21.810 → 20.233 | 0.448 → 0.106 | 4.782 → 4.375 |
| E | 3420×1343 | 1,822,936 | 1,336,995 | 569,327 | 3.479 → 0.908 | 39.825 → 37.242 | 1.095 → 0.399 | 10.838 → 10.095 |
| F | 320×200 | 91,889 | 16,759 | 4,509 | 0.329 → 0.245 | 1.727 → 1.644 | 0.051 → 0.032 | 0.198 → 0.179 |
| F | 2292×900 | 3,529,430 | 1,197,876 | 422,166 | 4.678 → 3.077 | 23.948 → 22.350 | 1.213 → 0.845 | 5.368 → 4.992 |
| F | 3420×1343 | 7,856,121 | 2,665,620 | 939,711 | 9.486 → 6.549 | 45.899 → 42.947 | 3.131 → 2.215 | 12.606 → 11.685 |

The extra preparation at MAX was a 26.7 KiB row copy taking 0.42–0.83 μs in Debug and 0.21–0.46 μs in Release. In Debug, instrumented run preparation in view C fell from 69.79 to 8.62 μs and in view A from 75.92 to 54.84 μs, before the copy. In Release view A, run preparation rose from 9.58 to 10.62 μs before the 0.21 μs copy, and the full sprite pass still fell from 2.83 to 2.24 ms. The preparation timers were measured on separate draws from the production medians.

The verification compared pre-distortion colour, Z bytes, post-distortion colour, HUD and intermediate composition buffers and the final SDL surface. In each build the six real views matched at the three benchmark sizes with distortion off and on. An additional 144 fixtures per build crossed the four aspect settings, all three render scales and both distortion states with overlapping billboards, different enemy frames, projectiles, death effects and scenes with closed doors, open doors and a diagonal wall. A direct diagonal-impact comparison exercised the special projection, including a foreground-depth override. `decorative_sprites` also covers separated visible pieces of a single magnified texel and live depth previously written by a nearer sprite.

## Animated wall materials

The original ADALLE bank has **20 images**, which are copied into particular DALLE textures as patches. Recovered animated examples in DALLE1 include materials **11, 12 and 13**. The animation does **not** mean cycling through neighbouring full wall textures; it updates regions of the material in memory.

The port's step timing for those materials is an adaptation, not a calibrated reproduction of DOS wall-animation seconds. The implementation is in [`wall_animation.cpp`](../../src/wall_animation.cpp).

## Why doors are a special case

A door's geometric opening, its texture sampling span and its sector connection-state word are related but not identical. If you change how the door retracts without updating texture coordinates accordingly, a surface can stretch or mirror. A moving-door texture test currently exposes such a problem. See [World](world.md) for connection state and [Known differences](../reference/known-differences.md) for current test status.

## What still needs work

The major visual items include elevator textures leaking onto corridor walls, exact DOS timing for several effects, palette transitions (damage, light flicker, grayscale modal background), special death effects, and aspect-ratio testing.

**Original evidence:** DALLE column addressing `0x15795–0x15925`; SOL loading `0x7a08–0x7c9d` and plane drawing `0x12ac4` onward; ADALLE composition `0x152ff–0x15309`; sprite projection and anchor use near `0x11f83–0x1207f`.

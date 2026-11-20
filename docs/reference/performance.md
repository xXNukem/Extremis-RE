# Performance: why the floor eats frames

The short version: **textured floor and ceiling rendering is expensive at large output resolutions**. The longer version is more useful because it tells us *where* the time goes and what an optimization should actually change.

These are **historical measurements**, not a universal benchmark claim. They come from a controlled Release build on an **Apple M5 (arm64, 16 GiB RAM)** with SDL3 presenting through Metal. Other machines and later engine changes may produce different numbers.

## What was compared?

The benchmark held level 1 still and measured the first-person renderer with floor/ceiling texturing (**F on**) and without it (**F off**). It separately looked at CPU software rendering, ordinary SDL/Metal presentation and a paced window. Geometry and resources were identical across cases; there was no player movement or AI workload in this particular measurement.

| Output resolution | CPU software, F off | CPU software, F on | Metal window, F off | Metal window, F on |
| --- | ---: | ---: | ---: | ---: |
| 640×400 | 0.55 ms | 2.86 ms | 8.28 ms | 8.31 ms |
| 960×600 | 0.99 ms | 6.16 ms | 8.33 ms | 8.33 ms |
| 1280×800 | 1.58 ms | 10.69 ms | 8.33 ms | 10.94 ms |
| 1920×1080 | 3.10 ms | 21.81 ms | 8.33 ms | 22.02 ms |

Those are measured frame times for the respective test routes, **not guaranteed visible FPS**. Metal calls can block, enqueue or wait. Presentation timing is not the same as the cost of drawing each pixel.

At 1920×1080 with F on, the stage breakdown found approximately:

| Stage | Measured time |
| --- | ---: |
| Floor/ceiling sampling | **18.53 ms** |
| Wall/door ray work | 0.95 ms |
| Vertical rasterization | 1.58 ms |
| Framebuffer upload | 0.59 ms |

This makes the optimization target obvious: **moving the final texture to a GPU API doesn't erase the CPU cost of computing all those plane pixels**.

## Where to go next

The current code uses a software plane path in [`plane_renderer.cpp`](../../src/vortex/render/first_person/planes/plane_renderer.cpp) and scene composition in [`frame_composition.cpp`](../../src/extremis/ui/frame_composition.cpp). Possible future work includes reducing redundant plane sampling, better scaling and evaluating a GPU-backed plane implementation. None should be adopted without a before/after picture comparison and a reproducible benchmark.

The original raw CSV measurements are preserved in [`data/`](data/) for anyone who wants to check the numbers. Debug logging and other transient run logs have not been included in the live documentation set.

**Remember:** these measurements were taken against an earlier engine snapshot. Since then enemies, animations and input have changed. Re-run the benchmark before making a claim about the latest build.

## Level 23 and SDL presentation (2026-11-20)

Measured on Apple M5, macOS 26.6.2, SDL 3.4.18 (Cocoa/Metal), with Vortex at
`3cb4712afa24b314d882b6905c467fffc756169a`. The local original `GAME.LAB`
has SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`.
The benchmark uses the adjacent `DATA` directory.

### Presentation

The port creates one SDL renderer. The main gameplay loop has no fixed sleep,
and its FPS counter measures elapsed time between completed presents. The
16 ms menu sleep is confined to the modal menu. The renderer's initial
interval was 0 and SDL reported a 60 Hz display mode. Fullscreen settled near
16.6 ms per frame even with interval 0 explicitly reapplied. The same level and
DOS canvas in a window settled near 8.0 ms per frame with either interval
(the numbers describe the frame loop, not visible display updates).

During fullscreen Release with VSync OFF, a four-second process sample captured
`SDL_RenderPresent → FlushRenderCommands → METAL_RunCommandQueue →
METAL_ActivateRenderCommandEncoder → -[CAMetalLayer nextDrawable]`. Of 3,083
main-thread samples, 1,723 were below `nextDrawable` on that call path.
The stack showed QuartzCore sleeping while acquiring the drawable. That proves
where the fullscreen loop waited on this machine. The exact OS decision that
governed drawable availability was outside this measurement. SDL accepted
the ON/OFF changes in fullscreen and reported each applied interval. The menu
uses that interval as its source of truth.

### Reproduction

Build the optional benchmark target in an ordinary Release build, Debug, and
a separate optimized build with stage instrumentation.

```sh
cmake -S . -B build/vsync-release -DCMAKE_BUILD_TYPE=Release
cmake -S . -B build/vsync-debug -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build/vsync-profile -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-DEXTREMIS_DEBUG_MENU=1
cmake --build build/vsync-release --target level_render_benchmark
cmake --build build/vsync-debug --target level_render_benchmark
cmake --build build/vsync-profile --target level_render_benchmark

build/vsync-release/level_render_benchmark /path/to/GAME.LAB fullscreen 23 20 3 throughput
build/vsync-release/level_render_benchmark /path/to/GAME.LAB fullscreen 1 20 3 throughput
build/vsync-profile/level_render_benchmark /path/to/GAME.LAB fullscreen 23 20 3 profile
build/vsync-profile/level_render_benchmark /path/to/GAME.LAB fullscreen 1 20 3 profile
build/vsync-release/level_render_benchmark /path/to/GAME.LAB software 23 40 3 throughput
build/vsync-release/level_render_benchmark /path/to/GAME.LAB software 1 40 3 throughput
build/vsync-debug/level_render_benchmark /path/to/GAME.LAB fullscreen 23 20 3 profile
build/vsync-debug/level_render_benchmark /path/to/GAME.LAB fullscreen 23 30 3 sync
build/vsync-release/level_render_benchmark /path/to/GAME.LAB window 23 36 3 sync
```

Each row is one repetition with 12 warm-up frames before sampling. The tables
use the median across the three run medians and across the three run P95s.
`throughput` measures the production first-person renderer with the original
HUD, wall/plane banks, decoration and enemy sprites, with a fixed camera and
initial closed doors. It draws no diagnostic overlay and uses the ordinary
Release render code. `profile` adds stage clocks and measures enemy updates
separately with a fixed 1/60 s step. Audio, input, collision during movement,
wall and decoration animations, and the rest of the active session are outside
the render loop timed here.

Level 23 uses the shuttle arrival (320, 3100, +X, sector 1). The second pose is
(4080, 2000, +Y, sector 23), selected from interior walkable polygon centres
by median depth over nine rays and eight candidate headings. Level 1 uses its
normal spawn (3500, 320, +Y) and an open view (3500, 1650, −Y, sector 1).
The DOS canvas partitions 320×200 into 320×150 of 3D and 50 lines of HUD.
The larger test inputs are requested presentation sizes. The real 3D dimensions
follow the production aspect and HUD scaling rules.

### Level 23 measurements

Release, fullscreen, VSync OFF, with planes and without fisheye. Values are
P50 / P95 in milliseconds. CPU + queue includes SDL texture upload.

| Requested | 3D world | View | CPU + queue | Present | Total |
| --- | --- | --- | ---: | ---: | ---: |
| 320×200 | 320×150 | arrival | 0.20 / 0.24 | 16.40 / 16.71 | 16.59 / 16.91 |
| 320×200 | 320×150 | open | 0.23 / 0.28 | 16.43 / 16.68 | 16.65 / 16.91 |
| 1280×720 | 1146×450 | arrival | 0.89 / 0.98 | 15.74 / 16.08 | 16.64 / 16.97 |
| 1280×720 | 1146×450 | open | 1.05 / 1.08 | 15.60 / 15.77 | 16.64 / 16.82 |
| 1920×1080 | 1719×675 | arrival | 1.62 / 1.68 | 15.03 / 15.31 | 16.65 / 16.92 |
| 1920×1080 | 1719×675 | open | 2.13 / 2.17 | 14.49 / 14.66 | 16.62 / 16.98 |
| 3420×2224 | 3420×1343 | arrival | 5.35 / 5.44 | 11.26 / 11.48 | 16.62 / 16.95 |
| 3420×2224 | 3420×1343 | open | 7.95 / 8.05 | 8.62 / 8.94 | 16.59 / 16.90 |

At MAX with planes and fisheye OFF, the open view takes 5.79 / 5.91 ms in CPU + queue. Enabling planes raises that to 7.95 / 8.05 ms. Enabling fisheye as well raises it to 10.19 / 10.32 ms.
With both effects enabled, Debug takes 38.26 / 38.52 ms before present in the same open view. With planes ON and fisheye OFF,
Debug takes 32.96 / 33.08 ms on arrival and 32.81 / 32.93 ms in the open view. These Debug
cases exceed the 16.67 ms frame budget in CPU work alone. Release does not
exceed it in the tested fixed render scenes. Rare tails and full gameplay
under load need a longer live-session trace.

The fullscreen drawable changed across launches, so the automatic aspect and
internal dimensions also varied. For a matched level comparison, the Release
software surface renders both levels into the same 1919×674 world for the
1920×1080 request. With planes ON and fisheye OFF, CPU + queue is 1.31 / 1.34 ms
at the level 1 spawn and 1.38 / 1.40 ms at level 23 arrival. The open views
take 1.62 / 1.65 ms and 1.94 / 1.97 ms, respectively. Software present is
about 0.64 ms P50 in all four cases.

Instrumented Release at MAX, planes ON and fisheye OFF, in milliseconds.

| Stage | Arrival P50 / P95 | Open view P50 / P95 |
| --- | ---: | ---: |
| Prepare geometry | 0.01 / 0.01 | 0.01 / 0.01 |
| Grid and intersections | 0.26 / 0.26 | 0.32 / 0.33 |
| Wall raster and depth expansion | 2.38 / 2.42 | 1.53 / 1.54 |
| Planes | 0.09 / 0.11 | 2.13 / 2.15 |
| Sprites | 0.01 / 0.01 | 1.25 / 1.30 |
| HUD | 0.04 / 0.04 | 0.06 / 0.06 |
| Composition | 0.53 / 0.54 | 0.70 / 0.71 |
| Upload and queue | 1.60 / 1.69 | 1.60 / 1.71 |
| Present | 11.36 / 11.74 | 8.66 / 9.06 |

Enemy update including sprite list rebuild reaches a P95 of 0.007 ms at
arrival and 0.050 ms in the open view in the optimized instrumented build.
The fixed 1/60 s calls include frames between enemy ticks.

### Cost by stage

The ray stage times Vortex grid traversal, static and dynamic intersections,
and grid backtracking. Extremis prepares camera-relative walls and door faces
once per frame in the geometry stage. The static grid is built on level load.
The reported candidate counter counts tested static segments only. Vortex
scans every prepared dynamic face for each ray.

The wall stage includes column rasterization and expanding column depth into
the per-pixel depth buffer. Wall projection and material lookup sit outside
that clock. The sprite stage includes all sprite layers, their depth reads
and writes, their palette index-zero transparency checks, wall occlusion,
and fisheye when enabled. Compare the same row with fisheye OFF and ON to
estimate its incremental cost. HUD includes sampling the DOS view and drawing
the HUD. Composition includes copying the world and extending/overlaying the
HUD. Upload/queue measures `SDL_UpdateTexture` and `SDL_RenderTexture`
together. Presentation also flushes queued Metal commands and acquires a drawable.

At MAX, level 23 prepares 921 static segments and 57/61 dynamic faces
(arrival/open), with 20.91/42.58 static candidates per ray and median wall
depths of 180/1000 world units. The grid uses 23,728 bytes. Level 1 has 250
static segments, 15 dynamic faces, and roughly 15/14 tested static candidates
per ray. Level 23 submits 64 decoration and 68 enemy instances, compared with
27 and 9 in level 1. Culling means submitted instances and drawn billboards
are different counts. The arrival is largely covered by the nearby wall.
The open view makes more floor, ceiling and sprites visible.

Extremis rebuilds two prepared geometry vectors per frame and rebuilds the
enemy instance list into reserved storage. Vortex reuses the large render
buffers but expands the wall depth to every row; the tested MAX world takes
about 36.7 MB of depth writes per frame. Plane coverage blocks and projected
sprite runs use temporary vectors. Native HUD composition copies about
18.4 MB of world pixels and the combined SDL texture contains about 24.5 MB.
These allocation and copy observations come from source and buffer dimensions.

The first CPU optimization to evaluate is prepacking RGBA8888 texels for the
128×128 SOL materials in Vortex, retaining the existing coordinate and coverage
logic. Target the open, F ON, fisheye OFF MAX scene where planes take about
2.13 ms. Require byte-for-byte color, depth and coverage equality and a
reduction in Release plane P50 and P95 before adoption. Near-camera wall/depth
work and the SDL drawable wait are separate costs.

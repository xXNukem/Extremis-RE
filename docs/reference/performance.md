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

The current code uses a software plane path in [`plane_renderer.cpp`](../../src/plane_renderer.cpp) and scene composition in [`first_person_view.cpp`](../../src/first_person_view.cpp). Possible future work includes reducing redundant plane sampling, better scaling and evaluating a GPU-backed plane implementation. None should be adopted without a before/after picture comparison and a reproducible benchmark.

The original raw CSV measurements are preserved in [`data/`](data/) for anyone who wants to check the numbers. Debug logging and other transient run logs have not been included in the live documentation set.

**Remember:** these measurements were taken against an earlier engine snapshot. Since then enemies, animations and input have changed. Re-run the benchmark before making a claim about the latest build.

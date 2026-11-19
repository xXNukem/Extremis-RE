# Inside In Extremis: a field guide

Welcome to the slightly dusty engine room. This is a guide to what we've actually worked out about the original *In Extremis* DOS game, plus how **Extremis RE** puts those findings back together on modern hardware.

If you've ever wondered why an alien suddenly heads home, how a corridor becomes a different sector, or why an apparently ordinary wall is secretly an elevator door, you're in the right place.

No chronological "Step 204" reports here. Each chapter explains **one system**, with readable examples first and the byte-level details when they're useful.

## Pick a rabbit hole

| Topic | What you'll find |
| --- | --- |
| [Levels and GAME.LAB](systems/levels.md) | The 30 zones, sectors, geometry, routes, enemies and resource-bank IDs. Start here. |
| [Enemies](systems/enemies.md) | Behaviour modes, inter-sector navigation, sprites, hit masks and combat. |
| [Player and movement](systems/player.md) | Coordinates, controls, crouching, head movement, health and oxygen. |
| [Weapons and projectiles](systems/weapons.md) | Six weapon descriptors, shots, ammo, projectiles and hit testing. |
| [World, doors and elevators](systems/world.md) | What walls and connections really mean, and why opening a door wakes up aliens. |
| [Decorative objects](systems/decorations.md) | OBJET banks, drops, frame sequences, pixel patches and solid/ghost objects. |
| [Inventory and items](systems/items.md) | Fixed slots, cards, supplies, readers and the rather specific rules of picking things up. |
| [HUD, map and terminals](systems/interfaces.md) | The original 320×200 presentation, transfer terminals and modal interfaces. |
| [Audio](systems/audio.md) | DSS sound banks, channels, effects, and the odd business of timing. |
| [Rendering](systems/rendering.md) | DALLE walls, SOL floor/ceiling, sprites, palette and the software renderer. |

## The reference shelf

| Reference | Why bother? |
| --- | --- |
| [File formats](reference/file-formats.md) | A quick map of the file structures we've identified. |
| [Reverse-engineering notes](reference/reverse-engineering.md) | How we separate an EXE fact from a good guess. |
| [Performance](reference/performance.md) | What was measured, on what machine, and where the time goes. |
| [Known differences](reference/known-differences.md) | The bits we haven't solved yet, including current test limitations. |

## A few rules for reading this

- **Original DOS** means evidence traced to the executable, its data or a controlled original-game observation.
- **Port** means the implementation visible in the repository's source code. A faithful result in one subsystem does not imply the whole game is finished.
- **Adaptation** means a deliberate practical substitute, often timing-related. We'll call it what it is.
- **Unknown** means we haven't proved it. That's better than shipping a confident-sounding guess.

All EXE offsets in these chapters are **file offsets** unless a note says otherwise. Sprite and sector indices are generally **zero-based**; player-facing level numbers are **1–30**. The original game uses its own units and 720 angular units per full turn in several records.

### Where did the old reports go?

The first rounds of investigation produced many overlapping, date-stamped notes. We folded their useful findings into these chapters and removed the old reports from the live documentation. Nothing has been erased from Git history: the original audits can still be found in earlier commits if you need to trace an old experiment.

This guide is meant to grow **with verified discoveries**, not with a fresh report for every command we run.

## Application startup and session

`src/app/main.cpp` owns SDL startup and teardown. The startup module parses the launch options, loads the initial level and coordinates credits, menu and the original introduction. Optional initial resources have individual fallback paths.

`PortMenuState` owns navigation, labels and selection. The SDL modal owns input, audio cues and display mode application. The painter and hit testing share the same menu layout.

`src/app/session` owns the active game state and runs the gameplay loop. A transition to another level prepares its destination separately before the elevator presentation and publication to the active session. Same-level elevator travel keeps the existing world. The communication and shuttle presentations return control to the loop, which resets its clock and decides whether to resume simulation.

Frame delivery clears the SDL target, draws the scene or map, applies the door fade, draws the Debug overlays and presents. The session samples frame timing after presentation and retains control of simulation and early exits.

## Input and audio boundaries

The session loop still decides which part of the game gets each event. The SDL survival adapter translates focus changes and diagnostic keys into survival actions, preserving logical Tab and physical function keys.

`sdl_audio` owns the audio device, callback and stream locks. `game_audio` applies the original game's sound rules through that target. The `audio_data` target contains the resource decoders and CPU mixer and has no SDL dependency.

## Using Vortex CPU independently

Configure the CPU target and its synthetic consumer without SDL or game resources.

```sh
cmake -S . -B build/vortex-cpu -DVORTEX_CPU_ONLY=ON -DBUILD_TESTING=ON
cmake --build build/vortex-cpu
ctest --test-dir build/vortex-cpu --output-on-failure
```

A separate C++17 project can add the engine directory and link the same target used by the port.

```cmake
add_subdirectory(path/to/InExtremisPort/src/vortex vortex-build)
target_link_libraries(my_game PRIVATE Vortex::CPU)
```

The headers under `include/vortex` define the engine inputs. Supply ordered segment endpoints to the grid and prepare surfaces in that order for each ray origin and dynamic surface state. The caller resolves materials and texture columns, projects visible walls, and supplies framebuffer, depth and coverage storage. The plane and sprite rasterizers borrow these buffers. Include `vortex/render/first_person/sprites/sprite_renderer.h` for sprite rendering. Set explicit `flipX`, `flipY` and `wallImpact` properties on `vortex::sprites::Instance`. Include `vortex/render/first_person/planes/plane_renderer.h` and supply the world-space repeat period along with the plane textures. Extremis supplies 400 units for SOL. Include `vortex/render/first_person/walls/wall_raster.h` for wall projection and rasterization. The wall raster returns the half-open row range for reuse as plane coverage. The fisheye effect retains its lookup and scratch storage across frames. Include `vortex/render/first_person/postprocess/fisheye_effect.h` and construct `vortex::FisheyeEffect` with explicit settings for physical pixel width divided by height and distortion strength. Extremis supplies `{{5.0 / 6.0, 0.20}}`. Existing consumers of `vortex/fisheye_effect.h` should switch to the module include and supply these values to preserve the former sampling. The [synthetic consumer](../tests/vortex/cpu_scene.cpp) shows the sequence. The Extremis-specific camera, materials, HUD and SDL presentation are coordinated by the port's [render adapter](../src/extremis/render/first_person/presentation/first_person_view.cpp).

## Shared calculations

Vortex exposes `CameraPose` in `render/first_person/camera_pose.h`. The `core/graphics` headers own RGB colors, opaque `0xRRGGBBFF` packing and row-major texture validation. Walls, planes and sprites use the same shape check. Sprites also check transparency indices. Palette conversion in the HUD, map, interactions and original presentation uses the same packer after conversion to 8-bit channels.

The `core/geometry/point.h` header owns the two-dimensional point and determinant shared by raycasting, collision and doors. `core/math/constants.h` supplies the common double-precision value of π. Extremis resource code owns [sector vertex conversion](../include/extremis/resources/sector_coordinates.h), shared by doors, collision, camera selection, scene preparation, decorative placement and supply panels. The conversion is inline to avoid two external calls per wall in linear queries.

The determinant, column direction, texture shape check and palette packing are small caller-side primitives. The three rasterizers retain direct channel assembly in their pixel loops. In the Debug control, routing those operations through the shared packer increased median frame time by 1.32–3.76%, compared with 0.48–0.84% using direct expressions. The separate prepared-intersection shapes have their own measured exception in the [rendering notes](systems/rendering.md). Sprite clipping, DAC expansion and wall-animation shape checks use distinct numeric contracts.

Extremis owns the menu and renderer's aspect selection and DOS view width calculation. The nominal PIT input and divisor live in the world clock contract, while the door counter retains integer remainder arithmetic and the cinematic players retain their individual timing rules. Elevator destination checks live in `original_travel.cpp`. The HUD compositor selects separate native, integer-scale and widescreen paths.

The grid constructor, campaign victory handoff and SDL two-voice cue adapter use separate `.tpp` definitions. Each adapts caller-supplied types and preserves the existing call order with no per-ray or per-sample type erasure. Original intro and ending share the SDL two-voice mixer, with resource failures handled by their respective players.

## Build boundaries

The root CMake file selects the CPU-only build or assembles the port. `src/extremis/CMakeLists.txt` delegates resources, world, gameplay, audio, UI, cinematics and rendering to their directories. `src/platform/sdl/CMakeLists.txt` owns the device and input targets. `src/app/CMakeLists.txt` owns the executable and session coordination. `tools/CMakeLists.txt` owns diagnostics and benchmarks, and `tests/CMakeLists.txt` owns the port's regression suite. Vortex CPU defines its library and test in its own directories. Configure with `-DVORTEX_CPU_ONLY=ON` to build the CPU library and its test without SDL.

The resource layer reads GAME.LAB, ANM, SPR, texture and audio data. Enemy behavior uses the sprite resource and scene layer. The session interface library owns the inventory and map controllers and uses gameplay, original UI and SDL. The app combines startup, event routing, simulation, travel and frame delivery. SDL adapters operate devices, translate input and deliver frames. Vortex CPU operates on generic geometry, camera poses and pixel buffers.

The gameplay session owns the active level, collision world, doors and campaign UI state. Door control borrows the active level and the collision containers by reference. Travel prepares a destination before publishing it and assigns into the existing level storage so those references stay valid. Debug frame timing belongs to one running session. The renderer borrows that profiler, and the overlay reads its latest snapshot.


The production directories mirror these boundaries. Public contracts follow the same layout under `include/`, and private headers remain with their implementations.

```text
src/
  vortex/
    render/first_person/{raycast,walls,planes,sprites,postprocess}/
  extremis/
    world/
    gameplay/{player,enemies,items}/
    render/first_person/{camera,scene,materials,sprites,presentation}/
    audio/
    ui/
    resources/
    cinematics/
  platform/sdl/{audio,input,video}/
  app/{startup,session,diagnostics}/

include/vortex/
  core/{geometry,graphics,math}/
  render/first_person/{raycast,walls,planes,sprites,postprocess}/
```

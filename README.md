<div align="center">

<img src="docs/images/extremis-re-title.png" alt="Extremis RE" width="100%">

<br>

[![C++17](https://img.shields.io/badge/C%2B%2B-17-0b140d?style=for-the-badge&labelColor=173b26&color=48de78)](https://isocpp.org/)
[![SDL3](https://img.shields.io/badge/SDL-3-0b140d?style=for-the-badge&labelColor=173b26&color=48de78)](https://www.libsdl.org/)
![Work in progress](https://img.shields.io/badge/STATUS-WORK%20IN%20PROGRESS-151515?style=for-the-badge&labelColor=501119&color=fe4457)

</div>

---

<div align="center">

## Support the project

If you'd like to support development:

<a href="https://ko-fi.com/xxnukem"><img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Support xXNukem on Ko-fi" height="48"></a>

**[ko-fi.com/xxnukem](https://ko-fi.com/xxnukem)**

</div>

---

## About Extremis RE

**Extremis RE** is an independent, unofficial C++17/SDL3 reconstruction and port of *In Extremis* (1993) for modern systems.

The project uses the original DOS executable and game data as references for gameplay, rendering, audio and interface behaviour. It aims to preserve the original game rather than redesign it. Players must supply their own game files.

## About Vortex Engine

<div align="center">
  <img src="docs/images/vortex-engine-logo.png" alt="Vortex Engine" width="280">
</div>

Vortex Engine is a C++17 and SDL3 engine currently being developed as part of Extremis RE.

Its purpose is to restore classic and largely forgotten first-person shooters, making them playable on modern systems while preserving their original gameplay, appearance and behaviour.

The idea is to build a versatile engine that adapts to each game, rather than forcing games to adapt to the engine. Different approaches to rendering, geometry, physics and game logic should be supported according to the needs of each original title.

*In Extremis* is the first game being reconstructed using Vortex Engine, and serves as the foundation for its development.

## Development progress

**Estimated completion: ~85%** (manual estimate, not a measured percentage of code or completed levels).

**🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩⬛⬛⬛ &nbsp; 85%**

Extremis RE is approaching its first public release. The major gameplay, rendering, audio, survival and interface systems have been reconstructed. Original password continuation, persistent settings and multilingual support are operational. Full-campaign verification and Windows release testing remain outstanding.

### Feature status

| Area | Status | Notes |
|:--|:--|:--|
| Original game data and 30 levels | Implemented | All level definitions load; complete campaign playthrough remains |
| Vortex Engine and 3D renderer | Functional | Multithreaded software rendering, widescreen, scalable resolution and fisheye |
| Lighting and visual effects | Functional | Colored lights, floor/ceiling rendering, infrared optics and POWER IS SURGING |
| Player controls and survival | Functional | Modern and classic controls, health, oxygen, running and collision |
| Enemies and combat | Functional | Weapons, enemy AI, radar and generators; remaining edge cases require testing |
| Doors, terminals and level travel | Functional | Access cards, elevators, shuttle and inter-level persistence |
| Inventory and pickups | Functional | Original interfaces, batteries, weapons, supplies and localized inventory graphics |
| HUD and map | Functional | Original DOS presentation and widescreen HUD extension |
| Audio | Functional | Original sound effects and working persistent master volume |
| Original cinematics | Implemented | Introduction, credits, elevator travel, communication, death and ending sequences |
| Bombs and nests | Functional | Placement, explosions, enemy generators and destroyed-nest persistence |
| Password continuation | Implemented | All 16 DOS passwords, LOAD GAME and arrival notifications |
| Port menu and settings | Implemented | Video, audio, controls and persistent settings.ini |
| Localization | Functional | English, Spanish and French; three ambiguous narrative passages remain in English |
| Full campaign | Not fully verified | All 30 levels require end-to-end gameplay testing |
| Platform support and releases | In progress | macOS development validated; native Windows testing and packaging pending |

**Latest reported automated result (24 November 2026): 46/46 CTest tests passed in both Debug and Release.** This does not establish complete DOS fidelity or full-campaign verification.

### Implemented so far

- Original 30-level data, textures, sprites, enemy resources and audio loading.
- Vortex Engine rendering with widescreen, configurable resolutions, fisheye and multithreading.
- Original colored lighting, infrared optics, radar and electrical power fluctuations.
- Player survival, combat, enemy AI, interactive objects and destructible nests.
- Doors, access cards, terminals, inventory, elevators and shuttle transitions.
- Inter-level state persistence and original password-based continuation.
- Original introduction, death sequences, communication events and final cinematic.
- Persistent audio, video and control preferences, including classic keyboard gameplay.
- English, Spanish and French localization of menus, gameplay interfaces and most narrative sequences.
- Original-style localized inventory graphics composed without modifying DOS assets.
- Automated regression tests and extensive executable-based reverse-engineering documentation.

### Main work remaining

- Complete an end-to-end playthrough of all 30 levels and verify progression and ending.
- Investigate remaining enemy AI edge cases, including the level 3 MONS5 diagnostic.
- Validate outstanding audiovisual timing, bomb presentation and DOS fidelity details.
- Visually review remaining localization layouts and three ambiguous narrative passages.
- Perform native Windows testing, release builds and distribution packaging.
- Optional future enhancement: modern full-state saving alongside original passwords.

## Build and run

### Requirements

- CMake **3.16+**
- A C++17-capable compiler
- **SDL3 3.2+** development libraries
- Your own legally obtained copy of *In Extremis*

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j4
ctest --test-dir build/release --output-on-failure
```

Start the game with the path to the original `GAME.LAB`:

```sh
./build/release/InExtremisPort "/path/to/In_Extremis/GAME.LAB"
```

The original `DATA` directory must be alongside `GAME.LAB`. Original game assets are **not** included in this repository.

Development and testing are currently focused on macOS. Windows support has not yet been fully validated.

## Technical documentation

The [technical field guide](docs/README.md) describes the file formats, implementation and reverse-engineering findings. Confirmed original behaviour, port-specific adaptations and unresolved questions are distinguished in the notes.

| System | Documentation |
|:--|:--|
| Levels and sectors | [Levels](docs/systems/levels.md) |
| Enemies and weapons | [Enemies](docs/systems/enemies.md) · [Weapons](docs/systems/weapons.md) |
| Doors, elevators and objects | [World](docs/systems/world.md) · [Decorations](docs/systems/decorations.md) |
| Player and inventory | [Player](docs/systems/player.md) · [Items](docs/systems/items.md) |
| HUD, map and terminals | [Interfaces](docs/systems/interfaces.md) |
| Audio and graphics | [Audio](docs/systems/audio.md) · [Rendering](docs/systems/rendering.md) |
| Original introduction | [Introduction research](docs/reference/exe/introduction.md) |
| Death sequences | [Death sequence research](docs/reference/exe/player-death.md) |
| Known differences | [Technical limitations](docs/reference/known-differences.md) |

Development uses Ghidra, original DOS data, C++ implementation, automated tests and manual gameplay checks.

## Credits and rights

**Port, reverse engineering and maintenance:** Carlos de la Barrera Pérez (xXNukem).

Extremis RE is an independent, unofficial, non-commercial project. *In Extremis* and its original intellectual property belong to their respective rights holders. The original game and its assets are not distributed here.

The source-code license is in [LICENSE](LICENSE); attribution information is in [NOTICE](NOTICE).

Bug reports and corrections to the technical documentation are welcome. Please do not upload copyrighted original game assets to issues or pull requests.

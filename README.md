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

## Development progress

**Estimated completion: ~45%** (manual estimate, not a measured percentage of code or completed levels).

**🟩🟩🟩🟩🟩🟩🟩🟩🟩⬛⬛⬛⬛⬛⬛⬛⬛⬛⬛⬛ &nbsp; 45%**

The engine and much of the gameplay are functional. Completion still requires save/load, the ending, more original cinematics and testing throughout the 30-level campaign.

### Feature status

| Area | Status | Notes |
|:--|:--|:--|
| Original game data and 30 level definitions | Implemented | All level definitions can be loaded; later levels require gameplay verification |
| 3D renderer | Functional | Textured walls, floors, ceilings and sprites; visual issues, aspect ratios and performance remain |
| Player controls and survival | Functional | Movement, health, oxygen, running and collision; timing and edge cases need further verification |
| Doors, terminals and elevators | Functional | Opening/closing, security, interactions and level transfers; special cases remain to be tested |
| Weapons, projectiles and enemies | Functional | Combat and enemy behaviour implemented; all banks and later-game encounters are not fully verified |
| Decorative objects | Functional | Sprite animations and collision implemented; some timing and bank-specific behaviour remain |
| HUD, inventory and map | Functional | Original 320×200 layout and interfaces; remaining interaction details need checking |
| Sound effects | Functional | Original audio resources and gameplay cues; ambient audio and some timing need verification |
| Original startup presentation | Implemented | Blue Sphere, Présente, title, four credits and all 12 PSEQ stages; some timing and presentation differences remain |
| Player death and GAME OVER | Implemented | Original health/oxygen sequences, GAME OVER and return to the original menu; timing is partly adapted |
| Original main menu | Partial | Available after GAME OVER; startup integration, OPTIONS and password continuation are unfinished |
| Final cinematic | Not implemented | Original FSEQ ending sequence still needs reconstruction |
| Inter-level elevator cinematics | Not implemented | Original transition animations still need investigation and integration |
| Save/load | Not implemented | Full game-state persistence and restoration are pending |
| Full campaign | Not verified | All 30 levels require end-to-end testing, including events, transitions and the ending |
| Platform support and releases | In progress | Development and manual testing primarily on macOS; Windows and release packaging need validation |

**Latest reported Debug test result (15 November 2026): 29/29 CTest tests passed.** This does not mean all levels or DOS behaviour have been verified.

### Implemented so far

- Original level, texture, sprite and audio data loading.
- First-person renderer with floor and ceiling textures.
- Player movement, survival systems, weapons, projectiles and enemy AI.
- Doors, readers, terminals, elevators, inventory, HUD and map.
- Decorative sprite animation and collision handling.
- Custom Extremis RE startup presentation.
- Original Blue Sphere presentation, In Extremis title, credits and PSEQ introduction.
- Original health/oxygen death presentations and GAME OVER sequence.
- Automated tests and technical notes based on the original executable.

### Main work remaining

- Final cinematic and inter-level elevator cinematics.
- Save/load and full game-state restoration.
- Original menu integration and remaining menu functions.
- Fidelity checks for animation timing, audio and specific original effects.
- Remaining gameplay, collision and rendering edge cases.
- Full playthrough and validation of all 30 levels.
- Windows testing, performance work and release packaging.

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

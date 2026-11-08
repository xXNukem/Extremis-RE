# Extremis RE

*One very obscure DOS shooter. Thirty levels. A surprising amount going on under the hood.*

**Extremis RE** is an independent, open-source reconstruction of **In Extremis (1993)**, built in **C++17 with SDL3 and CMake**. The aim is simple: get the original PC game running on modern systems without losing the peculiar little things that made it *In Extremis*.

This isn't a remake dressed up as a port. The levels, enemies, weapons, terminals, old-school interfaces and oddball game rules are being rebuilt from the original data files and reverse-engineered DOS executable. Some parts are already playable; others still need work. That's the fun, and occasionally the headache, of the project.

## What's working?

The engine can read the **30-level GAME.LAB** structure and the game's original graphic and audio banks. It has a first-person view, textured walls, floor and ceiling rendering, doors, elevators, a lower HUD, inventory and terminals, weapons, enemies, the motion detector, and a map based on the original interface. The exact behaviour and completeness vary by system and level.

This is **work in progress**, not a promise of a finished 30-level playthrough. Some timings are adapted to the port, and several visual details and special mechanics still need verification against DOS.

## Run it

You'll need your **own copy of the original game data**. No original game assets are included in this repository.

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j4
ctest --test-dir build/release --output-on-failure
./build/release/InExtremisPort "/path/to/In_Extremis/GAME.LAB"
```

CMake needs a working SDL3 development installation. The port expects the relevant original resource files alongside the supplied game-data directory. Check [known differences and test issues](docs/reference/known-differences.md) if something isn't behaving as expected.

## Fancy a look inside?

The most interesting part of this project is what we've learned about the original game. Rather than dump an endless development diary on you, we've put together a **[technical field guide](docs/README.md)**: how levels are wired up, why certain aliens give up chasing you, what makes a door block the map, how decorative sprites actually animate, and where the reconstructed code differs from the DOS version.

Start with [levels and GAME.LAB](docs/systems/levels.md), [enemy behaviour](docs/systems/enemies.md), or [the graphics pipeline](docs/systems/rendering.md). You don't need to know x86 assembly to follow along.

## How it's made

This is original port code developed through reverse engineering, hand-written C++, Ghidra and other inspection tools, AI-assisted development, lots of small tests, and a frankly ridiculous amount of patience. Technical claims are labelled as **confirmed**, **adapted**, or **still under investigation** in the documentation. When the evidence doesn't settle something, we say so.

## Project and credits

**Created by Carlos de la Barrera Pérez (xXNukem).** Original development, reverse engineering, architecture, implementation and project maintenance.

Issues, testing, corrections and contributions are welcome. Please don't upload copyrighted game data with a bug report or pull request.

## License and disclaimer

The **Extremis RE source code** is published under **Apache License 2.0**; see [LICENSE](LICENSE) and [NOTICE](NOTICE). This is an independent, unofficial project. **In Extremis** and its original intellectual property belong to their respective rights holders. No original game assets are distributed here.

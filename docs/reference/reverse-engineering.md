# How we took the old engine apart (without guessing our way through it)

Rebuilding a DOS game isn't a matter of staring at one screenshot and coding something that looks close enough. The tricky parts are often invisible: **who owns a counter, when a door changes sector connectivity, why a hit mask rejects a pixel, or what an enemy does when you leave the room**.

This is the practical workflow behind Extremis RE. No mystique, no “the AI figured it out”. Just evidence, code, testing, and the occasional deeply annoying 16-bit detail.

## The original binary is a witness, not a specification manual

The studied executable is a DOS x86 real-mode program. We use disassembly and controlled traces to recover actual behaviour, and the original resource files to confirm what is being read.

Reference identity for the examined files:

| File | SHA-256 |
| --- | --- |
| `EXTREMIS.EXE` | `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397` |
| `GAME.LAB` | `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c` |

These hashes identify the versions used in the investigation. They're useful when two people disagree about a table and turn out to have different game files.

**Address rule:** EXE offsets in these chapters are **absolute file offsets**, unless stated otherwise. Addresses like `[0x1428]` name runtime storage; they are **not** interchangeable with file offsets. Several statically stored data references in the studied image map via a `+0x200` file adjustment, but that is **not** a universal conversion rule for arbitrary pointers.

## The evidence ladder

| Label | Means | Example |
| --- | --- | --- |
| **EXE-confirmed** | Traced to concrete original instructions/data and their call path | Door connectivity uses `(state & 0x01ff) != 0` |
| **DATA-confirmed** | Parsed from a known original resource, with bounds and interpretation checked | A level's 28-byte enemy record |
| **DOS-observed** | Reproduced in the running original with context | A particular visual effect occurs in a specific level |
| **Port-tested** | Verified against the reconstructed implementation | A 16-bit animation counter wraps without dropping steps |
| **Adapted** | Deliberate substitute, stated plainly | 18 Hz fixed simulation for logic originally tied to a variable loop |
| **Unknown** | Not settled by evidence | Which levels contain destroyable alien nests |

These labels can coexist. An exact recovered table implemented at a fixed 18 Hz is **data-faithful and time-adapted**. That's more useful than a blanket claim of “100% accurate”.

## A typical investigation, from mystery to code

Imagine a door starts opening and an enemy on the other side wakes up *before the door looks open*. A sensible modern engine might use raycasting; we checked the original instead:

1. Find the **connection-state word** used by the sector traversal routine.
2. Trace the **open bit** (`0x0100`) and the **progress** updates separately.
3. Verify that the traversal tests the **low nine bits** (`0x01ff`).
4. Establish the decisive edge case: **opening bit set, progress still zero → connected**.
5. Use one shared port query for both enemy activation and map blocking.
6. Add tests for opening, closing, unlocking and cyclic sector graphs.
7. Finally, play the game and compare the actual feel.

Without step 4, a well-meaning `progress > 0` condition would have been *almost* right. “Almost” is how subtle compatibility bugs survive for years.

## Work in small, testable slices

The native port is **C++17 + SDL3 + CMake**, with separate modules for parsing, movement, rendering, enemies, audio, inventory and interfaces. A subsystem should have one clear owner and tests that can run without the entire game window when possible.

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j4
ctest --test-dir build/release --output-on-failure
```

Not every test is guaranteed green on every snapshot. Check [Known differences](known-differences.md) for current known failures before claiming a clean run.

Other tools have helped: **Ghidra**, targeted LLVM disassembly, Python inspection scripts, manually inspected binary tables and AI-assisted code exploration. The results are checked against the actual C++ and original bytes. An automatically generated report is *not* evidence just because it has a lot of headings.

## Three mistakes we're trying not to repeat

**1. Confusing names with behaviour.** A sample called something suggestive isn't necessarily played on every oxygen decrement. Trace the caller.

**2. Confusing visual scale with collision.** OBJET collision uses flags and per-image half-extent tables, not the visible sprite dimensions.

**3. Confusing a working port with exact DOS timing.** The original mixes PIT interrupts, variable main-loop iterations, FPS estimates and per-instance counters. One fixed update rate cannot magically replace all four.

## What belongs in this repository?

- Readable implementation code and tests.
- Small, checked reference tables needed for the engine.
- Documentation that teaches how systems operate, with examples and source references.
- Reproducible measurement data when it actually supports a claim.

What doesn't belong here: copies of copyrighted game assets, enormous generated disassembly dumps, transient logs of every attempted command, or ten successive “latest status” reports contradicting each other.

If you want the earlier raw investigation reports, Git history preserves them. This handbook is the **curated explanation**, not an archive of every debugging session.

## A note on credit and openness

Extremis RE is a personal, independent reconstruction by **Carlos de la Barrera Pérez (xXNukem)**. It uses human-written code, reverse engineering and AI-assisted tooling; the repository is candid about that. Credit the original game and its rights holders, and don't distribute their original assets with the port. Source licensing details remain in [`LICENSE`](../../LICENSE) and [`NOTICE`](../../NOTICE).

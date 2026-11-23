# Extremis RE — instructions for coding and reverse-engineering agents

This repository is a fidelity-oriented C++17/SDL3 reimplementation of *In Extremis* (1993). Do not invent original DOS behavior or replace unresolved mechanics with plausible modern equivalents unless the user explicitly requests an adaptation.

## Read-first workflow (save context and avoid repeated EXE analysis)

1. Read [`docs/reference/exe-index.md`](docs/reference/exe-index.md) to locate existing knowledge.
2. Read **only the relevant** `docs/reference/exe/*.md` technical notes and linked `docs/systems/*.md` chapter before disassembling the original EXE.
3. Check the actual code and tests: documentation may predate locally modified or newly committed code. Do not conflate a documented DOS behavior with a completed port implementation.
4. Only investigate `EXTREMIS.EXE` when the specific answer is absent, disputed, or needs validation; target the smallest relevant code and caller/data ranges instead of producing broad disassembly dumps.
5. When learning something new, add its **exact evidence** (file offsets, memory addresses, instruction/data semantics, conditions, file hash, test evidence), implementation location, and remaining uncertainties to the relevant knowledge entry; update the index if needed. Preserve prior conclusions when supported; revise them explicitly if contradicted by new evidence.
6. Ask for approval before changing gameplay code if the current task is an investigation. Never commit or push unless the user requests it.

## Persistent knowledge and token-efficient task execution

- **Every task starts at the index, not at the EXE.** Use the index to choose only the relevant technical note, source files and tests. Do not reread every chapter or regenerate reports already in Git history.
- **Every investigation that establishes new facts must update the appropriate existing note in the same task**, with bounded offsets, exact conditions, source/hash and evidence level; update `exe-index.md` when a new topic or status changes. No separate documentation prompt is needed. If the task is explicitly read-only, report the proposed note text without writing it.
- **After a gameplay implementation**, update any note whose implementation-status claim became obsolete, even when no new EXE facts were discovered. Keep historical findings, but clearly mark superseded port status; never describe old test results as current.
- **Do not document speculation as DOS truth.** Separate traced instructions, deductions, port implementation, automated tests and actual DOS-vs-port observations. Record unresolved details once and reference them, rather than re-explaining them in each task.
- **Spend context on the requested change.** Start from relevant functions and existing tests, inspect only the missing call paths, make the minimum coherent implementation and run focused tests plus the applicable suite. Avoid multi-stage investigation-only detours when enough evidence exists to implement safely.
- **Report briefly at completion:** what changed, tests run and results, new original-game findings (if any), documentation updated and remaining verification. No commit/push unless explicitly requested.

## Evidence quality

Follow [`docs/reference/reverse-engineering.md`](docs/reference/reverse-engineering.md). Distinguish `EXE-confirmed`, `DATA-confirmed`, `DOS-observed`, `Port-tested`, `Adapted` and `Unknown`. A report summarizing an EXE analysis is **not** proof of independent re-verification; say when a statement was inherited from an earlier report. Never claim exact elapsed seconds solely from a PIT tick count or phase value.

Reference original files, when available:

- `EXTREMIS.EXE`: SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`
- `GAME.LAB`: SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`

**Addresses:** documented EXE offsets are absolute **file offsets**, unless stated otherwise; bracketed `[0x....]` values denote DOS memory addresses. Do not use a universal conversion between them.

## Development preferences

- Keep changes compact and scoped; avoid unnecessary helpers/refactors.
- Preserve working behavior unless an original-game discrepancy has been demonstrated.
- Prefer small tests for each recovered rule; run Debug configuration when the development level selector is needed:

  `cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build/debug -j4`

- Do not include copyrighted original game assets, original EXE data, broad disassembly dumps, or Ghidra project files in the repository.

## Architecture modernization — Vortex Engine

The current priority is to reorganize Extremis RE into a
readable, modular and maintainable codebase before resuming
rendering optimizations or expanding game functionality.

### Architectural goals

- Make Vortex Engine reusable independently of In Extremis.
- Separate engine, game-specific logic, platform integration,
  resource loading and application orchestration.
- Split oversized functions into cohesive responsibilities.
- Use meaningful names and explicit data flow.
- Establish clear module ownership and dependency direction.
- Avoid circular dependencies and unnecessary global state.
- Avoid speculative abstractions, trivial wrappers and overengineering.
- Keep performance-critical code efficient and understandable.
- Prefer focused modules over monolithic source files.

### Specialized agents

- vortex_architect: read-only architecture analysis and planning.
- vortex_refactorer: implementation of explicitly approved changes.
- vortex_reviewer: independent regression and architecture review.
- extremis_dos_researcher: investigation of original DOS behavior
  only when existing evidence is insufficient.

Use the corresponding local skills when relevant.

Do not launch every agent automatically.
Delegate only when the task benefits from specialization.
Avoid parallel modifications to overlapping files.

### Refactoring procedure

1. Audit the existing architecture without modifying code.
2. Propose module boundaries and a migration sequence.
3. Obtain user approval before implementing changes.
4. Refactor one coherent subsystem at a time.
5. Preserve existing algorithms and observable behavior.
6. Build and execute relevant tests after each change.
7. Compare rendering output byte-for-byte where applicable.
8. Benchmark affected hot paths in Debug and Release.
9. Review results before proceeding to the next subsystem.

### Non-negotiable requirements

- Preserve all existing Vortex rendering optimizations.
- Preserve original DOS behavior and gameplay semantics.
- Preserve supported resolutions, aspect ratios and fisheye.
- Preserve exact color, depth, coverage and composition.
- Do not combine refactoring with new functionality.
- Do not introduce new GPU rendering or modify existing multithreading without explicit approval. Preserve Vortex's current multicore renderer.
- Do not sacrifice performance for architectural elegance.
- Do not commit or push unless explicitly requested.
- Never perform broad architectural rewrites in one operation.

The objective is a clean, reusable engine and a readable
game implementation, not a larger or more abstract codebase.

## Vortex Engine — permanent architecture contract

**Architecture decision: VORTEX-ARCH-001**

Before any architectural work involving Vortex Engine, read:

1. `external/Vortex-Engine/AGENTS.md`
2. `external/Vortex-Engine/docs/architecture-contract.md`
3. `external/Vortex-Engine/docs/architecture.md`
4. The relevant existing source files and tests.

The architecture contract is the single authoritative reference.
Do not invent or propose a different architecture in each session.

### Permanent objective

Vortex Engine must evolve into a complete, independent, modular,
extensible and highly decoupled game engine with its own development
tools, editor and easy-to-use high-level API.

The existing low-level rendering, audio and input APIs must remain
available and compatible with existing consumers.

### Mandatory rules

- Preserve the current architecture and established module ownership.
- Do not reorganize modules without a demonstrated need and explicit approval.
- Never introduce circular dependencies or game-specific logic into Vortex.
- Keep each module's purpose, public API and implementation location clear.
- Preserve all existing rendering optimizations, including multicore support.
- Do not introduce new GPU or threading architectures without approval.
- Make Vortex easy to integrate into independent third-party projects.
- Preserve existing Extremis RE behavior and compatibility.
- Implement one justified change at a time.
- Never replace the approved architecture with a new assistant proposal.

The previous architecture-modernization instructions remain applicable
where compatible with VORTEX-ARCH-001. They do not authorize additional
refactoring merely because architectural cleanup was once a priority.

Any structural architecture change requires a documented proposal,
compatibility assessment and explicit approval from the project owner.

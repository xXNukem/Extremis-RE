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

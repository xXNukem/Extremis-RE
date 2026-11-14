# Extremis RE — instructions for coding and reverse-engineering agents

This repository is a fidelity-oriented C++17/SDL3 reimplementation of *In Extremis* (1993). Do not invent original DOS behavior or replace unresolved mechanics with plausible modern equivalents unless the user explicitly requests an adaptation.

## Read-first workflow (save context and avoid repeated EXE analysis)

1. Read [`docs/reference/exe-index.md`](docs/reference/exe-index.md) to locate existing knowledge.
2. Read **only the relevant** `docs/reference/exe/*.md` technical notes and linked `docs/systems/*.md` chapter before disassembling the original EXE.
3. Check the actual code and tests: documentation may predate locally modified or newly committed code. Do not conflate a documented DOS behavior with a completed port implementation.
4. Only investigate `EXTREMIS.EXE` when the specific answer is absent, disputed, or needs validation; target the smallest relevant code and caller/data ranges instead of producing broad disassembly dumps.
5. When learning something new, add its **exact evidence** (file offsets, memory addresses, instruction/data semantics, conditions, file hash, test evidence), implementation location, and remaining uncertainties to the relevant knowledge entry; update the index if needed. Preserve prior conclusions when supported; revise them explicitly if contradicted by new evidence.
6. Ask for approval before changing gameplay code if the current task is an investigation. Never commit or push unless the user requests it.

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

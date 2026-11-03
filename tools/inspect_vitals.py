#!/usr/bin/env python3
"""Read-only reproduction of the audited clock, player vitals and death evidence."""

import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import struct

from inspect_original_code import disassemble


EXE_SHA256 = "7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397"
OUTPUT = Path(__file__).resolve().parents[1] / "build/vitals-debug"
RANGES = ((0x183c, 0x18a1), (0x18bd, 0x1918), (0x446a, 0x4480),
          (0x4825, 0x484e), (0x6c00, 0x6c3a), (0x6c6c, 0x6c96),
          (0x76a9, 0x7705), (0x1663f, 0x1665d), (0x16ad8, 0x16b5a),
          (0x16db9, 0x16dd8), (0x1da7b, 0x1da9d), (0x1de72, 0x1defc),
          (0x1bd3b, 0x1bd6f), (0x1be19, 0x1bf38), (0x1723a, 0x17254),
          (0x17300, 0x1730d), (0x1750f, 0x1751c),
          (0x17486, 0x1748b), (0x1759e, 0x175a3))


def new_game_template(code):
    if len(code) < 0x1d8ca:
        raise ValueError("Truncated executable: new-game template unavailable")
    words = struct.unpack_from("<15H3i", code, 0x1d8a0)
    return {"file_offset": "0x1d8a0", "health": words[14], "oxygen": words[17],
            "bottle_reserve": words[16], "weapon": words[13], "ammunition": words[12]}


def inspect(exe, output, llvm_library):
    code = exe.read_bytes()
    if hashlib.sha256(code).hexdigest() != EXE_SHA256:
        raise ValueError("Executable differs from the audited reference")
    output = output.resolve()
    if output != OUTPUT and OUTPUT not in output.parents:
        raise ValueError("Diagnostics must remain under build/vitals-debug")
    template = new_game_template(code)
    report = {"exe_sha256": EXE_SHA256, "new_game": template,
              "pit_divisor": struct.unpack_from("<H", code, 0x184f)[0],
              "nominal_ticks_per_second": 160,
              "oxygen_warning_below": 19200, "oxygen_visual_alert_below": 4800}
    output.mkdir(parents=True, exist_ok=True)
    (output / "manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    with (output / "executable-evidence.txt").open("w") as evidence, contextlib.redirect_stdout(evidence):
        print(f"EXE SHA-256 {EXE_SHA256}; absolute file offsets; data pointers map to file +0x200")
        print(json.dumps(report, indent=2))
        for start, end in RANGES:
            print(f"\nFile [{start:#x},{end:#x})")
            disassemble(code, start, end, 32, llvm_library)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    parser.add_argument("--llvm-library", type=Path, default=Path(
        "/Library/Developer/CommandLineTools/usr/lib/libLTO.dylib"))
    args = parser.parse_args()
    try:
        inspect(args.exe, args.output, args.llvm_library)
        print(f"Original player vitals evidence: {args.output}")
    except (OSError, ValueError, AttributeError) as error:
        parser.exit(1, f"Vitals inspection failed: {error}\n")


if __name__ == "__main__":
    main()

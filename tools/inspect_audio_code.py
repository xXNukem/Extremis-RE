#!/usr/bin/env python3
"""Reproduce audited audio evidence from EXTREMIS.EXE without modifying it.

Offsets are absolute file offsets; data pointers in this build map to file +0x200.
The call scan reports byte candidates, not reachable instructions or a complete
call graph. Indirect calls and self-modifying code cannot be excluded by it.
"""

import argparse
import hashlib
from pathlib import Path

from inspect_original_code import disassemble


EXE_SHA256 = "7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397"
RANGES = {
    "vitals": ((0x1663f, 0x166c0), (0x16ad8, 0x16b5a), (0x16bca, 0x16be0),
               (0x76a9, 0x7705), (0x1723a, 0x17254), (0x17300, 0x1730d),
               (0x1750f, 0x1751c), (0x17486, 0x1748b), (0x1759e, 0x175a3)),
    "steps": ((0x169b7, 0x16a3d),),
    "state": ((0x6aea, 0x6b2e), (0x16ad3, 0x16b3d), (0x16b5a, 0x16cc7)),
    "oxygen": ((0x1723a, 0x17254), (0x174e2, 0x1759e)),
    "machinery": ((0x1858b, 0x18598),),
    "scheduler": ((0x12f0d, 0x12f7b), (0x13078, 0x13090), (0x13103, 0x1315a)),
}


def call_candidates(data, target):
    return [offset for offset in range(len(data) - 4)
            if data[offset] == 0xe8 and offset + 5 + int.from_bytes(
                data[offset+1:offset+5], "little", signed=True) == target]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("--section", choices=(*RANGES, "all"), default="all")
    parser.add_argument("--llvm-library", type=Path, default=Path(
        "/Library/Developer/CommandLineTools/usr/lib/libLTO.dylib"))
    args = parser.parse_args()
    try:
        data = args.exe.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if digest != EXE_SHA256:
            raise ValueError(f"Unverified executable SHA-256: {digest}; audited offsets do not apply")
        print(f"EXE SHA-256: {digest}")
        for target in (0x13103, 0x13123, 0x45ab):
            offsets = call_candidates(data, target)
            print(f"E8 byte candidates to file {target:#x}: " + ", ".join(hex(p) for p in offsets))
        for pointer in (0x217e9, 0x21890, 0x177db):
            start = pointer + 0x200
            print(f"Data pointer {pointer:#x}, file {start:#x}: "
                  f"{data[start:data.index(0, start)].decode('ascii')!r}")
        sections = RANGES if args.section == "all" else (args.section,)
        for section in sections:
            for start, end in RANGES[section]:
                print(f"\n{section}: file [{start:#x}, {end:#x})")
                disassemble(data, start, end, 32, args.llvm_library)
    except (OSError, ValueError, AttributeError) as error:
        parser.exit(1, f"Audio inspection failed: {error}\n")


if __name__ == "__main__":
    main()

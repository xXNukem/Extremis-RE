#!/usr/bin/env python3
"""Read-only original HUD catalog, contact sheet and reproducible x86 evidence.

Diagnostic artwork remains under build/hud-debug; no original files are changed.
"""

import argparse
import contextlib
import hashlib
import json
import struct
from pathlib import Path

from PIL import Image, ImageDraw
from extract_dalle import decode_spr, decode_palette, paletted_image
from inspect_original_code import disassemble


OUTPUT = Path(__file__).resolve().parents[1] / "build/hud-debug"
EXE_SHA256 = "7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397"
RANGES = ((0x68b9, 0x68fe), (0x7ad4, 0x7ae8), (0x7efe, 0x7f44),
          (0x13984, 0x13aa2), (0x13aa2, 0x13b07),
          (0x13cb8, 0x13d89), (0x13db1, 0x14092), (0x140e8, 0x1413f),
          (0x14092, 0x140e8), (0x14184, 0x14379), (0x14398, 0x1459a))


def inspect(directory, exe, output, llvm_library):
    code = exe.read_bytes()
    if hashlib.sha256(code).hexdigest() != EXE_SHA256:
        raise ValueError("Executable differs from the audited reference")
    output = output.resolve()
    if output != OUTPUT and OUTPUT not in output.parents:
        raise ValueError("Diagnostics must remain under build/hud-debug")
    palette = decode_palette((directory / "PAL1.PAL").read_bytes())
    records = decode_spr((directory / "CASQUE.SPR").read_bytes())
    if len(records) != 86:
        raise ValueError("Expected 86 CASQUE records")
    output.mkdir(parents=True, exist_ok=True)
    catalog = []
    sheet = Image.new("RGB", (960, 920), (45, 45, 45))
    draw = ImageDraw.Draw(sheet)
    for record in records:
        catalog.append({"index": record.index, "size": [record.width, record.height],
                        "anchor": struct.unpack("<HH", record.metadata),
                        "pixel_sha256": hashlib.sha256(record.pixels).hexdigest()})
        picture = paletted_image(record, palette).convert("RGB")
        if record.index == 1:
            sheet.paste(picture.resize((960, 150), Image.Resampling.NEAREST), (0, 0))
            continue
        x, y = record.index % 12 * 80, 170 + record.index // 12 * 100
        draw.text((x, y), str(record.index), fill="white")
        # Integer nearest enlargement where space permits; never filter originals.
        scale = max(1, min(2, 76 // record.width, 75 // record.height))
        picture = picture.resize((record.width * scale, record.height * scale), Image.Resampling.NEAREST)
        sheet.paste(picture, (x, y + 20))
    sheet.save(output / "casque-catalog.png")
    report = {"exe_sha256": EXE_SHA256, "casque": catalog,
              "ammo_positions": [struct.unpack_from("<HH", code, 0x56d7 + 112*i) for i in range(6)],
              "corner_table_dwords_and_skip": [struct.unpack_from("<HH", code, 0x13d89 + 4*i)
                                                for i in range(10)]}
    (output / "catalog.json").write_text(json.dumps(report, indent=2) + "\n")
    with (output / "executable-evidence.txt").open("w") as evidence, contextlib.redirect_stdout(evidence):
        print(f"EXE SHA-256: {EXE_SHA256}; absolute file offsets, data pointers map to file +0x200")
        for start, end in RANGES:
            print(f"\nFile [{start:#x},{end:#x})")
            disassemble(code, start, end, 32, llvm_library)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    parser.add_argument("--llvm-library", type=Path, default=Path(
        "/Library/Developer/CommandLineTools/usr/lib/libLTO.dylib"))
    args = parser.parse_args()
    try:
        inspect(args.data_dir, args.exe, args.output, args.llvm_library)
        print(f"HUD catalog and executable evidence: {args.output}")
    except (OSError, ValueError, AttributeError) as error:
        parser.exit(1, f"HUD inspection failed: {error}\n")


if __name__ == "__main__":
    main()

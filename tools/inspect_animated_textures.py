#!/usr/bin/env python3
"""Read-only ADALLE composition audit; writes previews only under build/texture-debug/."""

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
from extract_dalle import decode_dalle, decode_spr, decode_palette, paletted_image, safe_output


def overlays(tick, masks):
    yield 12, 4 + (tick & 7), 10, 25
    yield 12, 12 + tick % 6, 36, 25
    yield 13, (tick >> 5) & 3, 11, 17
    for row in range(7):
        bit = 6 - row if tick & 32 else row
        yield 11, 19 if masks[tick & 31] & (1 << bit) else 18, 12, 14 + row * 7


def inspect(data_dir, exe, output):
    code = exe.read_bytes()
    expected_hash = "7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397"
    if hashlib.sha256(code).hexdigest() != expected_hash:
        raise ValueError("Executable differs from the disassembled reference; re-audit addresses")
    masks = code[0x1524f:0x1526f]
    palette = decode_palette((data_dir / "PAL1.PAL").read_bytes())
    base = decode_dalle((data_dir / "DALLE1.DAL").read_bytes())
    frames = decode_spr((data_dir / "ADALLE.SPR").read_bytes())
    ticks = list(range(8)) + [21, 30, 31, 32, 33, 64, 96, 127]
    sheet = Image.new("RGB", (len(ticks) * 132, 3 * 180), (24, 24, 24))
    draw = ImageDraw.Draw(sheet)
    report = {"exe_sha256": expected_hash, "mask_file_offset": "0x1524f",
              "mask_hex": masks.hex(" "), "timing": "original loop ticks, not milliseconds",
              "frames": []}
    for column, tick in enumerate(ticks):
        composites = {i: paletted_image(base[i], palette) for i in (11, 12, 13)}
        for material, image, x, y in overlays(tick, masks):
            composites[material].paste(paletted_image(frames[image], palette), (x, y))
        entry = {"tick": tick, "overlays": list(overlays(tick, masks)), "changed_pixels": {}}
        for row, (material, composite) in enumerate(composites.items()):
            source = base[material].pixels
            entry["changed_pixels"][material] = sum(a != b for a, b in zip(source, composite.tobytes()))
            draw.text((column * 132, row * 180), f"DAL {material} t={tick}", fill="white")
            sheet.paste(composite.convert("RGB").resize((120, 148), Image.Resampling.NEAREST),
                        (column * 132, row * 180 + 22))
        report["frames"].append(entry)
    output.mkdir(parents=True, exist_ok=True)
    sheet.save(output / "composed-panels.png")
    (output / "audit.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"3 materials, 20 ADALLE images, 4 tracks; previews: {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path("build/texture-debug/animated"))
    args = parser.parse_args()
    inspect(args.data_dir, args.exe, safe_output(args.output))


if __name__ == "__main__":
    main()

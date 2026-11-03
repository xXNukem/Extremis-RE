#!/usr/bin/env python3
"""OBJET1 catalog with index-0 transparency and SPR anchors; originals are read-only."""

import argparse
import hashlib
import json
import struct
from pathlib import Path

from PIL import Image, ImageDraw
from extract_dalle import decode_spr, decode_palette, paletted_image, safe_output


def inspect(data_dir, exe, output):
    code = exe.read_bytes()
    expected = "7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397"
    if hashlib.sha256(code).hexdigest() != expected:
        raise ValueError("Executable differs from the audited reference")
    raw = (data_dir / "OBJET1.SPR").read_bytes()
    records = decode_spr(raw)
    palette = decode_palette((data_dir / "PAL1.PAL").read_bytes())
    heights = struct.unpack_from("<22H", code, 0x5d23)
    sheet = Image.new("RGB", (4 * 320, 6 * 260), (30, 30, 30))
    draw = ImageDraw.Draw(sheet)
    manifest = []
    for record in records:
        ax, ay = struct.unpack("<HH", record.metadata)
        manifest.append({"image": record.index, "width": record.width, "height": record.height,
                         "anchor": [ax, ay], "transparent_pixels": record.pixels.count(0),
                         "world_height": heights[record.index] if record.index < 22 else None})
        if record.index >= 22:
            continue  # Remaining images are object animation patches, not new world instances.
        left, top = record.index % 4 * 320, record.index // 4 * 260
        draw.text((left + 8, top + 5), f"{record.index:02}: {record.width}x{record.height} "
                  f"anchor={ax},{ay} H={heights[record.index]}", fill="white")
        image = paletted_image(record, palette).convert("RGBA")
        image.putalpha(Image.frombytes("L", (record.width, record.height),
                                      bytes(255 if p else 0 for p in record.pixels)))
        image = image.resize((record.width * 2, record.height * 2), Image.Resampling.NEAREST)
        for y in range(0, 224, 8):
            for x in range(0, 304, 8):
                shade = 70 if (x // 8 + y // 8) % 2 else 100
                draw.rectangle((left + 8 + x, top + 28 + y, left + 15 + x, top + 35 + y),
                               fill=(shade,) * 3)
        sheet.paste(image, (left + 8, top + 28), image)
        x, y = left + 8 + ax * 2, top + 28 + ay * 2
        draw.line((x - 4, y, x + 4, y), fill="cyan")
        draw.line((x, y - 4, x, y + 4), fill="cyan")
    output.mkdir(parents=True, exist_ok=True)
    sheet.save(output / "objet1-anchors.png")
    comparison = Image.new("RGB", (3 * 140, 2 * 260), (60, 60, 60))
    labels = ImageDraw.Draw(comparison)
    patches = [(15,22,3,27), (15,23,3,51), (16,30,2,21),
               (17,33,15,65), (17,38,15,88), (17,40,15,104)]
    for column, index in enumerate((15, 16, 17)):
        before = paletted_image(records[index], palette)
        after = before.copy()
        for target, source, x, y in patches:
            if target == index:
                after.paste(paletted_image(records[source], palette), (x, y))
        for row, image in enumerate((before, after)):
            labels.text((column * 140 + 4, row * 260 + 4),
                        f"{index}: {'raw' if row == 0 else 'tick 0'}", fill="white")
            comparison.paste(image.convert("RGB").resize((image.width * 2, image.height * 2),
                Image.Resampling.NEAREST), (column * 140 + 4, row * 260 + 25))
    comparison.save(output / "initial-patches.png")
    report = {"objet1_sha256": hashlib.sha256(raw).hexdigest(), "images": manifest,
              "height_table_file_offset": "0x5d23", "transparent_index": 0,
              "palette_zero_rgb": list(palette[:3])}
    (output / "manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"{len(records)} OBJET images; palette[0]={list(palette[:3])}; catalog: {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path("build/texture-debug/sprites"))
    args = parser.parse_args()
    inspect(args.data_dir, args.exe, safe_output(args.output))

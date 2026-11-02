#!/usr/bin/env python3
"""Extract the investigated DALLE1 and ADALLE samples; see docs/dalle-format.md."""

import argparse
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import struct

try:
    from PIL import Image, ImageDraw
except ImportError as error:
    raise SystemExit("Pillow is required. No dependency has been installed.") from error


REPOSITORY = Path(__file__).resolve().parents[1]
DEBUG_ROOT = REPOSITORY / "build" / "texture-debug"
DAL_WIDTH = 74
DAL_HEIGHT = 60
DAL_IMAGE_SIZE = DAL_WIDTH * DAL_HEIGHT
DAL_REFERENCE_SIZE = 239760


@dataclass(frozen=True)
class IndexedImage:
    index: int
    offset: int
    width: int
    height: int
    pixels: bytes
    metadata: bytes = b""


def u16(data, offset):
    if offset < 0 or offset + 2 > len(data):
        raise ValueError(f"Truncated uint16 at byte {offset}")
    return struct.unpack_from("<H", data, offset)[0]


def decode_palette(data):
    if len(data) != 768 or max(data) > 63:
        raise ValueError("PAL1 must contain 256 RGB triplets with 6-bit components")
    # Full-range expansion for previews, not a claim about the original DAC output.
    return bytes(round(component * 255 / 63) for component in data)


def decode_dalle(data):
    # Dimensions are established for the investigated sample, not read from a header.
    # Refuse other sizes rather than silently interpreting an unverified variant.
    if len(data) != DAL_REFERENCE_SIZE:
        raise ValueError(f"Only the investigated {DAL_REFERENCE_SIZE}-byte DALLE1 is supported")
    return [
        IndexedImage(index, offset, DAL_WIDTH, DAL_HEIGHT,
                     data[offset:offset + DAL_IMAGE_SIZE])
        for index, offset in enumerate(range(0, len(data), DAL_IMAGE_SIZE))
    ]


def decode_spr(data):
    count = u16(data, 2)
    table_end = 4 + count * 4
    if count == 0 or table_end > len(data):
        raise ValueError("Invalid SPR image count or truncated offset table")
    offsets = [4 + struct.unpack_from("<I", data, 4 + index * 4)[0]
               for index in range(count)]
    if offsets[0] != table_end:
        raise ValueError("Unexpected gap or overlap before first SPR image")
    images = []
    for index, start in enumerate(offsets):
        end = offsets[index + 1] if index + 1 < count else len(data)
        if start < table_end or start + 8 > end or end > len(data):
            raise ValueError(f"SPR image {index}: invalid or overlapping offsets")
        width, height = u16(data, start), u16(data, start + 2)
        if width == 0 or height == 0 or start + 8 + width * height != end:
            raise ValueError(f"SPR image {index}: dimensions do not match record length")
        images.append(IndexedImage(index, start, width, height,
                                   data[start + 8:end], data[start + 4:start + 8]))
    return images


def paletted_image(record, palette):
    image = Image.frombytes("P", (record.width, record.height), record.pixels)
    image.putpalette(palette)
    return image


def save_catalog(records, palette, path, title, columns, scale):
    padding, label_height = 12, 22
    cell_width = max(record.width for record in records) * scale + padding * 2
    cell_height = max(record.height for record in records) * scale + padding + label_height
    rows = (len(records) + columns - 1) // columns
    catalog = Image.new("RGB", (columns * cell_width, 36 + rows * cell_height), (28, 28, 28))
    draw = ImageDraw.Draw(catalog)
    draw.text((padding, 10), title, fill="white")
    for record in records:
        x = record.index % columns * cell_width + padding
        y = 36 + record.index // columns * cell_height
        draw.text((x, y), f"{record.index:02} | {record.width}x{record.height}", fill="white")
        image = paletted_image(record, palette).resize(
            (record.width * scale, record.height * scale), Image.Resampling.NEAREST)
        catalog.paste(image.convert("RGB"), (x, y + label_height))
    catalog.save(path)


def export_images(records, palette, output, name, columns, scale):
    folder = output / name
    folder.mkdir(parents=True, exist_ok=True)
    for record in records:
        paletted_image(record, palette).save(folder / f"{record.index:02}.png")
    catalog_path = output / f"{name}-catalog.png"
    save_catalog(records, palette, catalog_path,
                 f"{name} | zero-based indices | PAL1 preview", columns, scale)
    return catalog_path


def image_manifest(records):
    return [{
        "index": record.index,
        "offset": record.offset,
        "width": record.width,
        "height": record.height,
        "pixel_bytes": len(record.pixels),
        "metadata_hex": record.metadata.hex(),
        "unique_palette_indices": len(set(record.pixels)),
        "pixels_sha256": hashlib.sha256(record.pixels).hexdigest(),
    } for record in records]


def safe_output(path):
    output = path.resolve()
    if output != DEBUG_ROOT.resolve() and DEBUG_ROOT.resolve() not in output.parents:
        raise ValueError("Output must be inside this repository's build/texture-debug/")
    return output


def extract(data_dir, output):
    output = safe_output(output)
    names = ("DALLE1.DAL", "PAL1.PAL", "ADALLE.SPR")
    sources = {name: (data_dir / name).read_bytes() for name in names}
    palette = decode_palette(sources["PAL1.PAL"])
    dalle = decode_dalle(sources["DALLE1.DAL"])
    sprites = decode_spr(sources["ADALLE.SPR"])
    # Validate all inputs before creating any artifacts. Originals are only read.
    output.mkdir(parents=True, exist_ok=True)
    dalle_catalog = export_images(dalle, palette, output, "dalle1", 6, 3)
    sprite_catalog = export_images(sprites, palette, output, "adalle", 5, 4)
    report = {
        "sources": {name: {
            "path": str((data_dir / name).resolve()),
            "size": len(data), "sha256": hashlib.sha256(data).hexdigest(),
        } for name, data in sources.items()},
        "palette": {"entries": 256, "component_bits": 6,
                    "preview_expansion": "round(value * 255 / 63)",
                    "association": "Visually coherent with PAL1; original runtime not traced"},
        "dalle_layout": {"header_bytes": 0, "record_bytes": DAL_IMAGE_SIZE,
                         "pixel_order": "row-major, top to bottom, left to right",
                         "dimension_evidence": "Size divisibility, alignment of all images, visual inspection",
                         "dimensions_stored_in_file": False},
        "dalle_images": image_manifest(dalle),
        "adalle_header_word": u16(sources["ADALLE.SPR"], 0),
        "adalle_images": image_manifest(sprites),
        "uncertainties": [
            "No traced mapping from GAME.LAB wall fields to texture indices",
            "ADALLE overlay destinations and animation timing unknown",
            "No proof of transparency or runtime interpretation of repeated/shaded images",
        ],
    }
    (output / "manifest.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"DALLE1: {len(dalle)} images, {DAL_WIDTH}x{DAL_HEIGHT}, uncompressed indexed pixels")
    print(f"ADALLE: {len(sprites)} images, explicit dimensions validated against every record")
    print(f"Palette: 256 RGB entries, 6-bit components, expanded for previews")
    print(f"Catalogs: {dalle_catalog}\n          {sprite_catalog}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", type=Path, required=True,
                        help="Read-only original DATA directory")
    parser.add_argument("--output", type=Path, default=DEBUG_ROOT,
                        help="Destination within build/texture-debug/")
    args = parser.parse_args()
    try:
        extract(args.data_dir, args.output)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Extraction failed: {error}\n")


if __name__ == "__main__":
    main()

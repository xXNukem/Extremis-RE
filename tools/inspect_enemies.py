#!/usr/bin/env python3
"""Read-only MONS1/level-1 evidence catalog. Offsets are absolute EXE file offsets."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from PIL import Image, ImageDraw
from extract_audio import decode_bank, sample_at
from extract_dalle import decode_spr, decode_palette, paletted_image, safe_output

EXE_SHA = "7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397"
FIELDS = ("health", "behavior", "state", "phase", "sector", "x", "y", "polygon",
          "angle", "reserved", "sprite", "elevation", "home_x", "home_y")


def inspect(lab, exe, directory, output):
    code = exe.read_bytes()
    if hashlib.sha256(code).hexdigest() != EXE_SHA:
        raise ValueError("Executable differs from the audited reference")
    data = lab.read_bytes()
    begin, end = struct.unpack_from("<II", data, 2)
    level = data[begin:end]
    monster_bank = struct.unpack_from("<H", level, 6)[0]
    start, limit = struct.unpack_from("<HH", level, 12)
    count = struct.unpack_from("<H", level, start)[0]
    if monster_bank != 1 or start + 2 + 28 * count != limit:
        raise ValueError("Unexpected level 1 monster table")
    enemies = []
    for index in range(count):
        enemy = dict(zip(FIELDS, struct.unpack_from("<14H", level, start + 2 + 28 * index)))
        _, gx, gy = struct.unpack_from("<3H", level, 20 + 6 * enemy["sector"])
        enemy.update(index=index, global_x=gx + enemy["x"], global_y=gy + enemy["y"],
                     lab_offset=hex(begin + start + 2 + 28 * index))
        enemies.append(enemy)
    route_base = struct.unpack_from("<H", level, 8)[0]
    sector_count = struct.unpack_from("<H", level, 18)[0]
    routes = []
    for sector in range(sector_count):
        offset = struct.unpack_from("<H", level, route_base + 2 * sector)[0]
        link_count = struct.unpack_from("<H", level, route_base + offset)[0]
        routes.append(dict(sector=sector, links=[list(struct.unpack_from("<3H", level,
            route_base + offset + 2 + 6 * i)) for i in range(link_count)]))
    if code[0x16645:0x16649] != bytes.fromhex("66 83 e8 02") or code[0x166a6:0x166aa] != bytes.fromhex("66 83 f8 08"):
        raise ValueError("Health reaction instructions differ from audited EXE")
    raw = (directory / "MONS1.SPR").read_bytes()
    masks = (directory / "MONS1.SPT").read_bytes()
    frames = decode_spr(raw)
    if len(masks) != 8 * len(frames):
        raise ValueError("MONS1.SPT is not eight bytes per image")
    palette = decode_palette((directory / "PAL1.PAL").read_bytes())
    sheet = Image.new("RGB", (600, 600), (40, 40, 40))
    draw = ImageDraw.Draw(sheet)
    images = []
    for frame in frames:
        x, y = frame.index % 5 * 120, frame.index // 5 * 150
        anchor = struct.unpack("<HH", frame.metadata)
        draw.text((x, y), f"{frame.index}: {anchor}", fill="white")
        sheet.paste(paletted_image(frame, palette).convert("RGB"), (x, y + 20))
        images.append(dict(index=frame.index, width=frame.width, height=frame.height,
                           anchor=anchor, mask=list(masks[frame.index * 8:frame.index * 8 + 8])))
    goutte = decode_spr((directory / "GOUTTE.SPR").read_bytes())
    bank = decode_bank((directory / "BANK.DSS").read_bytes())
    standalone_explosion = sample_at((directory / "EXPLO.DSS").read_bytes())[0]
    report = dict(exe_sha256=EXE_SHA, monster_bank=monster_bank, enemies=enemies,
                  generator_count=struct.unpack_from("<H", level, struct.unpack_from("<H", level, 16)[0])[0],
                  images=images, spr_sha256=hashlib.sha256(raw).hexdigest(),
                  spt_sha256=hashlib.sha256(masks).hexdigest(), world_height=270,
                  attack_table_file="0xf1f1", attack_table=list(struct.unpack_from("<7H", code, 0xf1f1)),
                  waiting_table_file="0xf1ff", waiting_table=list(struct.unpack_from("<32H", code, 0xf1ff)),
                  sector_routes=routes, health_reaction=dict(damage_per_update=2, updates=8,
                    exe_file_range="0x165db..0x166b5", indicator_duration_is_not_damage=50),
                  note="DOS animation is iteration-based; fixed 18 Hz is a documented port adaptation.")
    report["death_effects"] = dict(descriptor_file="0x5ad5", recipe=struct.unpack_from("<33h", code, 0x5ad5),
        burst_byte_table=list(code[0x100ef:0x100f6]), world_slot="[0x5777]/[0x5779]",
        slime_resource="GOUTTE.SPR", slime_images=len(goutte), slime_counter_steps=30,
        slime_offset_stride=4, slime_drift=[struct.unpack_from("<H",code,0x13b94+4*i)[0] for i in range(8)],
        slime_positions=[struct.unpack_from("<3H",code,0x13bb6+6*i) for i in range(43)],
        slime_sha256=hashlib.sha256((directory/"GOUTTE.SPR").read_bytes()).hexdigest(),
        explosion_audio={k:bank[17][k] for k in ("name","count","loop_flag","continuous","priority")},
        standalone_EXPLO_matches=[r["id"] for r in bank if r["pcm"]==standalone_explosion["pcm"]])
    report["detector"] = dict(scan_file="0x100f6..0x101e4", hud_file="0x13e00..0x13ef1",
        x_limits=[-27,27], y_limits=[-3,32], divisor=64, contacts=10, detects_stationary=True,
        skips_health=65535, checks_walls=False, cosine_file="0x3164", atan_file="0x3ca4",
        battery_initial=struct.unpack_from("<i",code,0x1d8be)[0],
        animation_images=list(range(5,13)), marker_images=[13,14,15],
        cosine_sha256=hashlib.sha256(code[0x3164:0x3704]).hexdigest())
    output.mkdir(parents=True, exist_ok=True)
    sheet.save(output / "mons1.png")
    (output / "manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"{count} original enemies, {len(frames)} MONS1 images, {len(masks)} SPT bytes")
    for enemy in enemies:
        print(f"{enemy['index']}: sector {enemy['sector']}, global ({enemy['global_x']},{enemy['global_y']})")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lab", type=Path, required=True)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path("build/texture-debug/enemies"))
    args = parser.parse_args()
    inspect(args.lab, args.exe, args.data_dir, safe_output(args.output))

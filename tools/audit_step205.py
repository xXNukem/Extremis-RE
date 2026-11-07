#!/usr/bin/env python3
"""Read-only resource/level census and bounded EXE evidence for step 205.

Outputs metadata and disassembly only, under ignored build/audit205.
Never writes into the original game directory.
"""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import struct

from extract_dalle import decode_spr
from inspect_original_code import disassemble
from inspect_vitals import EXE_SHA256

RANGES = (
    (0x6826, 0x6895), (0x6f9d, 0x7021), (0x73e7, 0x75ad),
    (0x7ad4, 0x7b14), (0x7d00, 0x7dbc),
    (0x10db7, 0x10f3b), (0x10fc8, 0x10ff7),
    (0x11335, 0x11460), (0x14611, 0x146c3), (0x1470f, 0x1495a),
    (0x16843, 0x16925), (0x16b5a, 0x16bca), (0x16cd9, 0x16e35),
    (0x18f97, 0x19039), (0x1bc24, 0x1bd1c), (0x1c2e9, 0x1c434),
    (0x1a5ab, 0x1aa7b), (0x183ce, 0x1860f),
    (0x189a2, 0x18b10), (0x191dd, 0x193ca), (0x19429, 0x194c7),
    (0x8042, 0x8270), (0x19a4f, 0x19c80),
    (0x937c, 0x9434), (0x9aca, 0x9c2b), (0xce1a, 0xcfd8),
)


def inspect(directory):
    code = (directory / 'EXTREMIS.EXE').read_bytes()
    if hashlib.sha256(code).hexdigest() != EXE_SHA256:
        raise ValueError('Executable differs from audited reference')
    lab = (directory / 'GAME.LAB').read_bytes()
    u16 = lambda at: struct.unpack_from('<H', lab, at)[0]
    count = u16(0)
    offsets = struct.unpack_from(f'<{count+1}I', lab, 2)
    report = dict(exe_sha256=EXE_SHA256,
                  lab_sha256=hashlib.sha256(lab).hexdigest(),
                  map_scancode=struct.unpack_from('<H', code, 0x5621)[0],
                  object1_radii=struct.unpack_from('<22H', code, 0x5d4f),
                  levels=[], resources={})
    for index, base in enumerate(offsets[:-1]):
        banks = struct.unpack_from('<4H', lab, base)
        level = dict(level=index+1, banks=banks, lights=[], batteries=[], polygon_extra={})
        for sector in range(u16(base+18)):
            local, gx, gy = struct.unpack_from('<3H', lab, base+20+sector*6)
            p = base+local
            p += 2+2*u16(p)
            vertices = [struct.unpack_from('<2H', lab, p+2+i*4) for i in range(u16(p))]
            p += 2+4*u16(p)
            for i in range(u16(p)):
                a, b, kind, extra = struct.unpack_from('<4H', lab, p+2+8*i)
                if kind >> 8 == 0x32 and extra != 65535 and extra >> 12 == 0:
                    level['batteries'].append(dict(sector=sector, wall=i, seconds=extra&4095))
            p += 2+8*u16(p)
            for i in range(u16(p)):
                vertex, kind, p1, p2 = struct.unpack_from('<4H', lab, p+2+8*i)
                if banks[2] == 1 and kind & 255 in (2, 3):
                    x, y = vertices[vertex]
                    level['lights'].append(dict(sector=sector, entity=i, type=hex(kind),
                        xy=[gx+x, gy+y], local_xy=[x,y], params=[p1,p2],
                        excluded_by_flags=bool(kind & 0x3000)))
            p += 2+8*u16(p)
            for i in range(u16(p)):
                poly = p+u16(p+2+i*2)
                extra = str(u16(poly+2+2*u16(poly)))
                level['polygon_extra'][extra] = level['polygon_extra'].get(extra, 0)+1
        report['levels'].append(level)
    names = ('MAP.SPR', 'BOUSSOLE.SPR', 'B_PLAN2.SPR', 'B_PLAN3.SPR',
             'CHARME.SPR', 'CHARME2.SPR', 'CHARME3.SPR', 'CHARME4A.SPR',
             'CH_OBJ2.SPR', 'CH_OBJ3.SPR', 'CH_OBJ4A.SPR', 'CH_LEVEL.SPR',
             'B_CODE.SPR', 'B_PORTE.SPR', 'A_ARME.ANM', 'INJECT.ANM', 'OXYGEN.ANM',
             'PAL_DI.PAL')
    for name in names:
        data = (directory/'DATA'/name).read_bytes()
        entry = dict(size=len(data), sha256=hashlib.sha256(data).hexdigest())
        if name.endswith('.SPR'):
            entry['images'] = [dict(index=s.index, width=s.width, height=s.height,
                anchor=list(struct.unpack('<2H', s.metadata))) for s in decode_spr(data)]
        if name.endswith('.ANM'):
            entry['header_hex'] = data[:32].hex()
            entry['frame_count'] = struct.unpack_from('<H', data, 4)[0]
        report['resources'][name] = entry
    output = Path(__file__).resolve().parents[1]/'build/audit205'
    output.mkdir(parents=True, exist_ok=True)
    (output/'manifest.json').write_text(json.dumps(report, indent=2)+'\n')
    with (output/'evidence.txt').open('w') as stream, contextlib.redirect_stdout(stream):
        print('EXE SHA256', EXE_SHA256, '\nAbsolute file offsets; data pointer + 0x200.')
        for start, end in RANGES:
            print(f'\n[{start:#x},{end:#x})')
            disassemble(code, start, end, 32,
                        Path('/Library/Developer/CommandLineTools/usr/lib/libLTO.dylib'))
    print(output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    inspect(parser.parse_args().directory)

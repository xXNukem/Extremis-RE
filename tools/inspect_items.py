#!/usr/bin/env python3
"""Read-only evidence for original wall cabinets and fixed inventory.

Absolute EXE file offsets; data pointers map to file +0x200.
The catalogue reads level 1 only; it does not classify Table A as pickups.
"""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import struct
from inspect_original_code import disassemble
from inspect_vitals import EXE_SHA256

OUTPUT = Path(__file__).resolve().parents[1] / 'build/items-debug'
RANGES = ((0x10b9c,0x10c19),(0x1117a,0x111ef),(0x16925,0x1698d),
          (0xa567,0xa5ac),(0x937c,0x9399),(0x9434,0x948e),
          (0x95a1,0x95e4),(0x961d,0x9634),(0x1ac94,0x1acbe),
          (0x1afbc,0x1aff2),(0x14399,0x1459a),(0x109a8,0x10a03),
          (0x1c93c,0x1c97a),(0xa608,0xa615),(0x19d3d,0x19d60),
          (0xa508,0xa590),(0x183ce,0x183fc),(0x18f97,0x18fa0),
          (0x19039,0x191a7),(0x8de6,0x8e20),(0x1bd21,0x1bd37),
          (0x1bd59,0x1bd6f),(0x1be19,0x1bf38),(0x14184,0x14379),
          (0x1858b,0x18594),(0x8c0b,0x8c14),(0x193ca,0x1941a))


def catalogue(data):
    limit = len(data)
    def words(offset, count):
        if offset < 0 or offset+count*2 > limit:
            raise ValueError('Truncated GAME.LAB')
        return struct.unpack_from('<'+'H'*count,data,offset)
    if words(0,1)[0] != 30 or len(data) < 126:
        raise ValueError('Expected 30-level GAME.LAB')
    begin,end = struct.unpack_from('<II',data,2)
    if not 126 <= begin < end <= len(data):
        raise ValueError('Invalid first-level bounds')
    limit = end
    count = words(begin+0x12,1)[0]
    panels,entities = [],0
    for s in range(count):
        offset,gx,gy = words(begin+0x14+6*s,3)
        if offset < 0x14+6*count or begin+offset >= end:
            raise ValueError('Invalid sector offset')
        p = begin+offset
        neighbours = words(p,1)[0]; p += 2+2*neighbours
        nv = words(p,1)[0]; p += 2
        vertices = [words(p+4*i,2) for i in range(nv)]; p += 4*nv
        nw = words(p,1)[0]; p += 2
        for w in range(nw):
            a,b,kind,extra = words(p+8*w,4)
            if a >= nv or b >= nv:
                raise ValueError('Invalid wall vertex')
            if (kind & 0xff00) not in (0x3100,0x3200,0x3300,0x3500,0x3600):
                continue
            panels.append(dict(sector=s,wall=w,file_offset=hex(p+8*w),
                               type=hex(kind),contents=hex(extra),
                               kind=extra>>12,amount=extra&0xfff,
                               a=[gx+vertices[a][0],gy+vertices[a][1]],
                               b=[gx+vertices[b][0],gy+vertices[b][1]]))
        p += 8*nw
        entities += words(p,1)[0]
        if p >= end:
            raise ValueError('Sector exceeds level bounds')
    return dict(level=1,table_a_count=entities,wall_panels=panels)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    parser.add_argument('--llvm-library',type=Path,default=Path(
        '/Library/Developer/CommandLineTools/usr/lib/libLTO.dylib'))
    args = parser.parse_args()
    try:
        code = (args.directory/'EXTREMIS.EXE').read_bytes()
        if hashlib.sha256(code).hexdigest() != EXE_SHA256:
            raise ValueError('Executable differs from audited reference')
        report = catalogue((args.directory/'GAME.LAB').read_bytes())
        report['exe_sha256'] = EXE_SHA256
        report['security_messages'] = {
            hex(offset): code[offset:code.index(0,offset)].decode('ascii')
            for offset in (0x21aef,0x21d37,0x21d57,0x21b32,0x2193f)}
        report['card_y_words'] = list(struct.unpack_from('<12H',code,0x14379))
        report['injector_y_words'] = list(struct.unpack_from('<4H',code,0x14391))
        OUTPUT.mkdir(parents=True,exist_ok=True)
        (OUTPUT/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
        with (OUTPUT/'executable-evidence.txt').open('w') as out, contextlib.redirect_stdout(out):
            print('Absolute file offsets; data pointers map to file +0x200; SHA256',EXE_SHA256)
            for start,end in RANGES:
                print(f'\nFile [{start:#x},{end:#x})')
                disassemble(code,start,end,32,args.llvm_library)
        print(f'Original item evidence: {OUTPUT}')
    except (OSError,ValueError,struct.error,AttributeError) as error:
        parser.exit(1,f'Item inspection failed: {error}\n')

if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Read-only reproduction of original modal resources, access code and EXE evidence."""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import struct
from extract_dalle import decode_spr
from inspect_original_code import disassemble
from inspect_vitals import EXE_SHA256

OUTPUT=Path(__file__).resolve().parents[1]/'build/panel-debug'
RANGES=((0xa508,0xa648),(0x10b9c,0x10c1d),(0x19133,0x191dd),
        (0x193c4,0x19429),(0x1c837,0x1c8b7),(0x11364,0x1139e),(0x183d9,0x183fb),(0x1852b,0x1858b),
        (0x1860f,0x18655),(0x188bc,0x189a2),(0x18f2d,0x18fc4),
        (0x19784,0x19a2d),(0x8b04,0x8c18),(0x90c5,0x929d),
        (0x937c,0x9399),(0x9434,0x948e),(0x95a1,0x9634),
        (0x1abdb,0x1ac21),(0x1ac94,0x1acbe),(0x1afbc,0x1aff2),
        (0x1ce21,0x1ce72),(0x1cf04,0x1d0c6),(0x16cd9,0x16e02),
        (0x19f30,0x1a01c),(0x1a1c3,0x1a235),(0x1a348,0x1a478),
        (0xa617,0xa648),(0xa759,0xa933),(0x9e34,0x9e7b),(0x193ca,0x1941a),
        (0x9b7e,0x9b90),(0x9c2b,0x9c72),(0x9d85,0x9df3),(0xa1af,0xa3e4),
        (0x6fd3,0x700a),(0x22ee,0x234d),(0x19587,0x1970c),
        (0x19e60,0x19f14),(0x6826,0x6895),(0x16df7,0x16e16),
        (0x6eda,0x6f2e),(0x1de72,0x1defc),(0x1b9d0,0x1bc24),
        (0x1c166,0x1c2aa),(0x1c434,0x1c8b7),(0x1bc24,0x1c038),
        (0xce1a,0xcfd8),(0xd0db,0xd1c4),(0xd27b,0xd31c),
        (0xd63b,0xd694),(0xd718,0xd941),(0x14399,0x14415),(0x13090,0x13103),(0x13123,0x1315a),
        (0x45ab,0x4681),(0x1658d,0x165bb),(0x16726,0x1679a),
        (0x16ad8,0x16b05),(0x1810f,0x18158),(0x18267,0x182a5),(0x183c,0x1918),(0x738e,0x73a8),
        (0x17a20,0x17bdd),(0x17bdd,0x1810f),(0x1810f,0x18255))


def inspect(directory, library):
    code=(directory/'EXTREMIS.EXE').read_bytes()
    if hashlib.sha256(code).hexdigest()!=EXE_SHA256:
        raise ValueError('Executable differs from audited reference')
    lab=(directory/'GAME.LAB').read_bytes()
    begin,end=struct.unpack_from('<II',lab,2)
    offset=struct.unpack_from('<H',lab,begin+14)[0]
    if not 20<=offset<end-begin-1:
        raise ValueError('Invalid level access code offset')
    canonical=code[0x6167:0x616d]
    report=dict(exe_sha256=EXE_SHA256,level=1,zone=code[0x60ef:0x60f3].decode('ascii'),
                access_code_file_offset=hex(begin+offset),
                access_code_word=struct.unpack_from('<H',lab,begin+offset)[0],
                canonical_letters=''.join(chr(65+x) for x in canonical),
                weighted_byte_checksum=sum(x<<i for i,x in enumerate(canonical))&255,
                code_targets=list(struct.unpack_from('<2H',lab,begin+offset+4)),
                code_terminal_records=[dict(index=i,zone=code[0x60ef+4*i:0x60f3+4*i].decode('ascii'),
                    letters=''.join(chr(65+x) for x in code[0x6167+6*i:0x616d+6*i])) for i in range(2)],
                supply_descriptors=[struct.unpack_from('<8h',code,0x1835e+16*i) for i in range(7)],
                weapon_descriptors=[struct.unpack_from('<8h',code,0x7fe2+16*i) for i in range(6)],resources={})
    report['new_game_start'] = dict(zip(('local_x','local_y','angle_720','sector'),
                                              struct.unpack_from('<4H',code,0x1d8a0)))
    report['inventory_scancode_pc'] = hex(struct.unpack_from('<H',code,0x561f)[0])
    report['weapon_idle_descriptors'] = [dict(id=i,file_offset=hex(0x56d7+112*i),
        sprite_index=1,position=list(struct.unpack_from('<2H',code,0x56d7+112*i+8))) for i in range(6)]
    report['weapon_fire_descriptors'] = [dict(id=i,file_offset=hex(0x56d7+112*i),
        words=list(struct.unpack_from('<56h',code,0x56d7+112*i))) for i in range(6)]
    report['elevators'] = []
    for index in range(2):
        record=code[0xdacf+54*index:0xdacf+54*(index+1)]
        report['elevators'].append(dict(index=index,file_offset=hex(0xdacf+54*index),
            routes=[list(record[i:i+4]) for i in range(0,16,4)],
            position=list(struct.unpack_from('<3H',record,16)),labels=record[22:].decode('ascii')))
    for name in ('CHARME.SPR','CH_OBJ4A.SPR','CHARME4A.SPR','INVENT_A.SPR',
                 'INVENT_T.SPR','B_PORTE.SPR','B_PLAN3.SPR','B_CODE.SPR','PAL_DI.PAL','TYPO_3X8.BIN','CH_LEVEL.SPR','TYPO_3X5.BIN'):
        data=(directory/'DATA'/name).read_bytes()
        entry=dict(sha256=hashlib.sha256(data).hexdigest(),size=len(data))
        if name.endswith('.SPR'):
            entry['images']=[dict(index=s.index,width=s.width,height=s.height,pivot=list(struct.unpack('<2H',s.metadata))) for s in decode_spr(data)]
        report['resources'][name]=entry
    OUTPUT.mkdir(parents=True,exist_ok=True)
    (OUTPUT/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    with (OUTPUT/'executable-evidence.txt').open('w') as out,contextlib.redirect_stdout(out):
        print('Absolute file offsets; data pointers map to file +0x200; EXE SHA256',EXE_SHA256)
        for start,end in RANGES:
            print(f'\nFile [{start:#x},{end:#x})')
            disassemble(code,start,end,32,library)
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    parser.add_argument('--llvm-library',type=Path,default=Path('/Library/Developer/CommandLineTools/usr/lib/libLTO.dylib'))
    args=parser.parse_args()
    try:
        report=inspect(args.directory,args.llvm_library)
        print('Original interactions:',OUTPUT,'zone',report['zone'],'code',report['canonical_letters'])
    except (OSError,ValueError,struct.error,AttributeError) as error:
        parser.exit(1,f'Interaction inspection failed: {error}\n')
if __name__=='__main__':
    main()

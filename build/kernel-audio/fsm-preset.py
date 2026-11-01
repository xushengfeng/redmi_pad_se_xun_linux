#!/usr/bin/env python3
"""Dump the FSM preset file (fs1815.fsm) used by the Foursemi amplifiers.

The format is the one parsed by the vendor driver (fsm-dev.h): a preset
header, an index of dev_list offsets, one dev_list per amplifier address and,
inside it, index entries to the common register table, the per-scene tables
and the speaker info.  Offsets are relative to the index array of the
structure they belong to.

Usage: fsm-preset.py fs1815.fsm
"""
import struct
import sys

DEV_INFO, SPK_INFO, REG_COMMON, REG_SCENES = 0, 1, 2, 3
SCENE_NAMES = {1 << i: n for i, n in enumerate(
    ['music', 'voice', 'ringtone', 'voip', 'custom', 'offline', 'factory'])}


def u16(data, off):
    return struct.unpack_from('<H', data, off)[0]


def parse(path):
    data = open(path, 'rb').read()
    version = u16(data, 0)
    customer = data[2:10].rstrip(b'\0').decode(errors='replace')
    project = data[10:18].rstrip(b'\0').decode(errors='replace')
    date = struct.unpack_from('<I', data, 18)[0]
    size = u16(data, 0x16)
    crc = u16(data, 0x18)
    ndev = u16(data, 0x1a)
    print(f'header: version={version:#06x} customer={customer!r} '
          f'project={project!r} size={size} (file {len(data)}) crc={crc:#06x} '
          f'ndev={ndev}')

    index_base = 0x1c
    for i in range(ndev):
        off = u16(data, index_base + 4 * i)
        typ = u16(data, index_base + 4 * i + 2)
        print(f'index[{i}]: offset={off:#06x} type={typ}')
        if typ != DEV_INFO:
            continue
        d = off
        preset_ver = u16(data, d)
        dl_project = data[d + 2:d + 10].rstrip(b'\0').decode(errors='replace')
        dl_customer = data[d + 10:d + 18].rstrip(b'\0').decode(errors='replace')
        dl_date = struct.unpack_from('<I', data, d + 18)[0]
        data_len = u16(data, d + 22)
        dl_crc = u16(data, d + 24)
        length = u16(data, d + 26)
        bus = u16(data, d + 28)
        addr = u16(data, d + 30)
        dev_type = u16(data, d + 32)
        npreset = u16(data, d + 34)
        reg_scenes = u16(data, d + 36)
        eq_scenes = u16(data, d + 38)
        print(f'  dev_list @{d:#06x}: preset_ver={preset_ver:#06x} '
              f'project={dl_project!r} customer={dl_customer!r} '
              f'data_len={data_len} crc={dl_crc:#06x} len={length} bus={bus} '
              f'addr={addr:#04x} dev_type={dev_type:#06x} npreset={npreset} '
              f'reg_scenes={reg_scenes:#06x} eq_scenes={eq_scenes:#06x}')
        entries = {}
        for j in range(length):
            eoff = u16(data, d + 40 + 4 * j)
            etyp = u16(data, d + 40 + 4 * j + 2)
            entries[etyp] = d + 40 + eoff
        print(f'  data index: { {k: hex(v) for k, v in entries.items()} }')

        if SPK_INFO in entries:
            p = entries[SPK_INFO]
            n = u16(data, p)
            vals = [u16(data, p + 2 + 2 * k) for k in range(n)]
            names = ['tmax', 'tcoef', 'tsel', 'spk_res', 'rapp', 'tre',
                     'tm6', 'tm24', 'terr', 'tm01', 'tm02', 'tm03', 'tm04',
                     'tm05', 'tm06', 'rsrl_ratio', 'position']
            print('  speaker info: ' + ' '.join(
                f'{names[k] if k < len(names) else k}={v}'
                for k, v in enumerate(vals)))

        def regs(off, count, stride):
            out = []
            for k in range(count):
                r = off + k * stride
                raw = u16(data, r)
                ra = raw & 0xff
                rp = (raw >> 8) & 0xf
                rl = (raw >> 12) & 0xf
                rv = u16(data, r + 2)
                out.append((ra, rp, rl, rv))
            return out

        if REG_COMMON in entries:
            p = entries[REG_COMMON]
            n = u16(data, p)
            print(f'  reg common ({n}):')
            for ra, rp, rl, rv in regs(p + 2, n, 4):
                print(f'    {ra:#04x} pos={rp} len={rl} value={rv:#06x}')

        if REG_SCENES in entries:
            p = entries[REG_SCENES]
            n = u16(data, p)
            print(f'  reg scenes ({n}):')
            for k in range(n):
                r = p + 2 + k * 6
                scene = u16(data, r)
                raw = u16(data, r + 2)
                ra = raw & 0xff
                rp = (raw >> 8) & 0xf
                rl = (raw >> 12) & 0xf
                rv = u16(data, r + 4)
                names = ','.join(SCENE_NAMES.get(b, hex(b))
                                 for b in [scene] if scene) or hex(scene)
                print(f'    {names:12s} {ra:#04x} pos={rp} len={rl} '
                      f'value={rv:#06x}')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    parse(sys.argv[1])

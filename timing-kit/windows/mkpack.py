#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# mkpack.py <dlls dir> <out.pack> : the Windows timing kit's DLL pack, uncompressed (compress with zstd afterwards). Layout (little endian):
#   "FSR4PK1\n", u32 count, then per file: u16 path length, path (UTF-8, '/' separated, relative to dlls\), u64 size, u64 FNV-1a 64 of the data;
#   then the files' data in the same order. The order puts similar DLLs next to each other, so a long match window finds what they share.
import sys, os, struct
src, out = sys.argv[1], sys.argv[2]
names = sorted(d for d in os.listdir(src) if os.path.isfile(os.path.join(src, d, 'amd_fidelityfx_upscaler_dx12.dll')))
order = sorted(names, key=lambda n: (3 if n.startswith('rdna') else 0 if n.startswith('1') else 1 if 'exact' in n else 2, 'igpu' in n, n))      # AMD's, the exact ones, the lossy ones, the study sets
assert len(order) <= 128, 'fsr4time.exe reads at most 128 DLLs from a pack (PACK_MAX)'
def fnv(b):
    h = 0xcbf29ce484222325
    for i in range(0, len(b), 1 << 20):
        for x in b[i:i + (1 << 20)]: h = ((h ^ x) * 0x100000001b3) & 0xffffffffffffffff
    return h
files = [(n + '/amd_fidelityfx_upscaler_dx12.dll', open(os.path.join(src, n, 'amd_fidelityfx_upscaler_dx12.dll'), 'rb').read()) for n in order]
with open(out, 'wb') as o:
    o.write(b'FSR4PK1\n' + struct.pack('<I', len(files)))
    for p, d in files:
        pb = p.encode(); o.write(struct.pack('<H', len(pb)) + pb + struct.pack('<QQ', len(d), fnv(d)))
    for _, d in files: o.write(d)
print(f'{len(files)} files, {sum(len(d) for _, d in files) / 1e6:.1f} MB:', ' '.join(order))

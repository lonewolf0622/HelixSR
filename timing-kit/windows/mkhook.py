#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# mkhook.py > hook.inc : the address fsr4time.exe sends results to, scrambled for the C source. It is taken from
# ../submit.py, which holds the same address scrambled (see the note there: this only keeps it from programs that
# scan public code for such addresses; it is not hidden from a reader).
import hashlib, os, re
s = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'submit.py')).read()
a = s.index('_H = ('); b = s.index('def _hook'); c = s.index('.decode()\n', b) + 10
ns = {'hashlib': hashlib}
exec(s[a:c], ns)
u = ns['_hook']().encode()
key = hashlib.sha256(b'fsr4 timing kit, windows').digest()[:24]
enc = bytes(ch ^ key[i % len(key)] ^ (i * 7 & 255) for i, ch in enumerate(u))[::-1]
row = lambda v: ', '.join(f'0x{x:02x}' for x in v)
print('// written by mkhook.py; see there')
print('static const uint8_t _hook_key[] = {' + row(key) + '};')
print('static const uint8_t _hook_bytes[] = {' + row(enc) + '};')

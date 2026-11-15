#!/usr/bin/env python3
"""Compare frozen A/B rendering with the current engine on the same platform."""
import array
from pathlib import Path
import math
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix='logisth-legacy-') as temp:
    root = Path(temp)
    for name, program in [('reference', sys.argv[1]), ('current', sys.argv[2])]:
        subprocess.run([program, str(root / name)], check=True)
    maximum = 0.0
    exact = 0
    for ref in sorted((root / 'reference').glob('*.bin')):
        before, after = ref.read_bytes(), (root / 'current' / ref.name).read_bytes()
        if len(before) != len(after):
            raise ValueError(f'Render length changed: {ref.name}')
        exact += before == after
        for pos in range(0, len(before), 2052):
            if before[pos + 2048:pos + 2052] != after[pos + 2048:pos + 2052]:
                raise ValueError(f'Legacy voice lifetime changed: {ref.name}')
            a, b = array.array('f'), array.array('f')
            a.frombytes(before[pos:pos + 2048]); b.frombytes(after[pos:pos + 2048])
            if not all(math.isfinite(x) for x in (*a, *b)):
                raise ValueError(f'Non-finite legacy audio: {ref.name}')
            error = max(abs(x-y) for x,y in zip(a,b))
            maximum = max(maximum, error)
            if error > 2e-6:
                raise ValueError(f'Legacy A/B audio changed: {ref.name}, peak error {error}')
    if len(list((root / 'reference').glob('*.bin'))) != 20:
        raise ValueError('Expected all 20 reference fixtures')
    print(f'PASS legacy A/B: 20 fixtures, voice lifetimes, max error {maximum:.9g}; {exact}/20 byte-identical')

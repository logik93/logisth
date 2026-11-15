#!/usr/bin/env python3
"""Verify the supplied theme files and the unmodified bundled font payloads."""
import hashlib
import json
from pathlib import Path


def verify(root=None):
    root = Path(root) if root else Path(__file__).resolve().parents[1]
    themes = sorted((root / 'assets/themes').glob('*.json'))
    if len(themes) != 10:
        raise ValueError('Expected ten bundled themes')
    supplied = json.loads((root / 'assets/theme-provenance.json').read_text())
    if len(supplied['files']) != 4:
        raise ValueError('Expected four supplied theme files')
    for name, digest in supplied['files'].items():
        path = root / name
        if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise ValueError(f'Supplied theme changed: {name}')
        theme = json.loads(path.read_bytes())
        if theme['font'] != 'IoskeleyMonoTerm Nerd Font Mono':
            raise ValueError(f'Incorrect theme font: {name}')
    folder = root / 'assets/fonts/ioskeley'
    manifest = json.loads((folder / 'UPSTREAM.json').read_text())
    for name, digest in manifest['files'].items():
        if hashlib.sha256((folder / name).read_bytes()).hexdigest() != digest:
            raise ValueError(f'Upstream font file changed: {name}')
    print('Verified ten themes, four exact supplied JSON files and unmodified Ioskeley v2.2.0 fonts/licenses')


if __name__ == '__main__':
    verify()

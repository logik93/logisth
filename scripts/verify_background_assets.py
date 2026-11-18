#!/usr/bin/env python3
"""Validate the original factory background catalogue without image dependencies."""
import binascii
import collections
import hashlib
import json
import struct
from pathlib import Path


def verify(root: Path) -> None:
    directory = root / 'assets/backgrounds'
    catalogue = json.loads((directory / 'catalogue.json').read_text())
    if catalogue['format'] != 'logisth-background-library' or catalogue['version'] != 1:
        raise ValueError('Unsupported background catalogue')
    entries = catalogue['backgrounds']
    themes = {p.stem for p in (root / 'assets/themes').glob('*.json')}
    counts = collections.Counter(e['theme'] for e in entries)
    if len(entries) != 20 or set(counts) != themes or set(counts.values()) != {2}:
        raise ValueError('Expected two original backgrounds for each of ten themes')
    filenames, digests = set(), set()
    for entry in entries:
        name = entry['file']
        if Path(name).name != name or name in filenames:
            raise ValueError('Invalid or duplicate background filename')
        data = (directory / name).read_bytes()
        if len(data) > 16 * 1024 * 1024 or data[:8] != b'\x89PNG\r\n\x1a\n':
            raise ValueError('Background exceeds bounds or is not PNG')
        width, height = struct.unpack_from('>II', data, 16)
        if not (1240 <= width <= 4096 and 860 <= height <= 4096):
            raise ValueError('Background resolution is outside the supported range')
        at = 8
        while at + 12 <= len(data):
            size = struct.unpack_from('>I', data, at)[0]
            chunk = data[at + 4:at + 8 + size]
            crc = struct.unpack_from('>I', data, at + 8 + size)[0]
            if binascii.crc32(chunk) & 0xffffffff != crc:
                raise ValueError('Corrupt PNG chunk')
            at += size + 12
            if chunk[:4] == b'IEND':
                break
        if at != len(data) or chunk[:4] != b'IEND':
            raise ValueError('Missing PNG end or trailing data')
        filenames.add(name)
        digests.add(hashlib.sha256(data).digest())
    if len(digests) != 20 or filenames != {p.name for p in directory.glob('*.png')}:
        raise ValueError('Missing, repeated or uncatalogued artwork')
    print('Verified twenty distinct original PNGs: two per factory theme, dimensions, CRCs and catalogue')


if __name__ == '__main__':
    verify(Path(__file__).resolve().parents[1])

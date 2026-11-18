#!/usr/bin/env python3
"""Verify the checked-in release assets before GitHub Actions publishes them."""
import hashlib
import csv
import io
import json
import re
import struct
import sys
import tarfile
import zipfile
from pathlib import Path


def validate_tag(tag: str) -> str:
    if not re.fullmatch(r"v\d+\.\d+\.\d+(?:\.\d+)?", tag):
        raise ValueError("Expected a three- or four-part version tag")
    return tag[1:]


def verify(tag: str) -> None:
    validate_tag(tag)
    root = Path(__file__).resolve().parents[1]
    version = tag[1:]
    project = re.search(r"project\(Logisth VERSION ([\d.]+)",
                        (root / "CMakeLists.txt").read_text())
    if project is None or project.group(1) != version:
        raise ValueError("Tag and CMake project version do not match")
    folder = root / "releases" / f"logisth-{version}"
    manifest = folder / "SHA256SUMS"
    names = set()
    for line in manifest.read_text().splitlines():
        checksum, name = line.split("  ", 1)
        if (not re.fullmatch(r"[a-f0-9]{64}", checksum)
                or Path(name).name != name or name in names):
            raise ValueError("Invalid or duplicate checksum entry")
        asset = folder / name
        if asset.is_symlink() or not asset.is_file():
            raise ValueError(f"Missing regular asset: {name}")
        if hashlib.sha256(asset.read_bytes()).hexdigest() != checksum:
            raise ValueError(f"Checksum mismatch: {name}")
        names.add(name)
    actual = {p.name for p in folder.iterdir()
              if p.is_file() and p.name not in {"SHA256SUMS", "README.md"}}
    required = {f"logisth-{version}-source.tar.gz", "logisth-demo.wav", "logisth.png"}
    required |= {"logisth-Manual-English.pdf", "logisth-Manual-English.md",
                 "logisth-LICENSE-MIT.txt", "logisth-ThirdPartyNotices.md",
                 f"logisth-{version}-themes.zip", f"logisth-{version}-calibration.zip"}
    for name in names:
        if not name.endswith('.png'):
            continue
        data = (folder / name).read_bytes()
        if not data.startswith(b'\x89PNG\r\n\x1a\n'):
            raise ValueError(f'Invalid PNG signature: {name}')
        at = 8
        while at + 12 <= len(data):
            length = struct.unpack_from('>I', data, at)[0]
            kind = data[at + 4:at + 8]
            at += length + 12
            if kind == b'IEND':
                break
        else:
            raise ValueError(f'Missing PNG end: {name}')
        if at != len(data):
            raise ValueError(f'Trailing or concatenated PNG data: {name}')
    if names != actual or not required.issubset(names):
        raise ValueError("Release manifest does not cover all expected assets")
    if not (root / "docs" / f"RELEASE-{tag}.md").is_file():
        raise ValueError("Missing release notes")
    with tarfile.open(folder / f"logisth-{version}-source.tar.gz") as archive:
        for name in ["LICENSE", "THIRD_PARTY_NOTICES.md", "docs/logisth-Manual-English.pdf"]:
            if archive.extractfile("logisth/" + name).read() != (root / name).read_bytes():
                raise ValueError(f"Source archive is missing the current file: {name}")
    if (folder / "logisth-LICENSE-MIT.txt").read_bytes() != (root / "LICENSE").read_bytes():
        raise ValueError("Release MIT license differs from source")
    with zipfile.ZipFile(folder / f"logisth-{version}-themes.zip") as archive:
        for file in (root / "assets/themes").glob("*.json"):
            if archive.read(file.name) != file.read_bytes():
                raise ValueError(f"Theme archive mismatch: {file.name}")
    if tuple(map(int, version.split('.'))) >= (0, 9, 4):
        required = {f"logisth-{version}-factory-presets.zip", "logisth-AlgorithmAudit.json", "logisth-ProjectReview.md"}
        if not required.issubset(names):
            raise ValueError("Missing preset bank or review assets")
        with zipfile.ZipFile(folder / f"logisth-{version}-factory-presets.zip") as archive:
            patches = [n for n in archive.namelist() if n.endswith('.logisth')]
            if len(patches) != 48 or len(set(patches)) != 48:
                raise ValueError("Expected 48 distinct authored presets")
            for name in ['factory-demo.wav', 'factory-measurements.csv', 'CATALOGUE.md', 'README.md', 'LICENSE']:
                if not archive.read(name):
                    raise ValueError(f"Empty preset bank input: {name}")
    if tuple(map(int, version.split('.'))) >= (0, 9, 4, 1):
        from verify_theme_assets import verify as verify_themes
        verify_themes(root)
        with zipfile.ZipFile(folder / f"logisth-{version}-themes.zip") as archive:
            for file in (root / 'assets/fonts/ioskeley').iterdir():
                if file.is_file() and archive.read('fonts/ioskeley/' + file.name) != file.read_bytes():
                    raise ValueError(f'Bundled theme font mismatch: {file.name}')
    if tuple(map(int, version.split('.'))) >= (0, 9, 6, 1):
        name = f'logisth-{version}-stereo-reverb-calibration.zip'
        if name not in names:
            raise ValueError('Missing stereo/reverb calibration bundle')
        with zipfile.ZipFile(folder / name) as archive:
            for file in ('stereo-before.csv', 'stereo-after.csv', 'reverb-before.csv', 'reverb-after.csv', 'README.md'):
                if not archive.read(file):
                    raise ValueError('Empty stereo/reverb measurements')
    if tuple(map(int, version.split('.'))) >= (0,9,6,2):
        name=f'logisth-{version}-dynamics-cpu-calibration.zip'
        if name not in names:raise ValueError('Missing dynamics/CPU measurements')
        with zipfile.ZipFile(folder/name) as archive:
            for file in ('cpu-before.csv','cpu-after.csv','compressor-curves.csv','clipper-curves.csv','README.md'):
                if not archive.read(file):raise ValueError('Empty dynamics/CPU measurement')
    # The engineering reference was introduced after the immutable v0.9.6.2
    # release. Require it for subsequent versions without rewriting old assets.
    if tuple(map(int, version.split('.'))) > (0, 9, 6, 2):
        from verify_technical_manual import verify as verify_technical_manual
        verify_technical_manual(root)
        documents = {'docs/TECHNICAL_MANUAL.md': 'logisth-Technical-Manual-English.md',
                     'docs/logisth-Technical-Manual-English.pdf': 'logisth-Technical-Manual-English.pdf',
                     'docs/technical-manual.json': 'logisth-Technical-Manual-Manifest.json'}
        with tarfile.open(folder / f'logisth-{version}-source.tar.gz') as archive:
            for source, name in documents.items():
                if name not in names or (folder / name).read_bytes() != (root / source).read_bytes():
                    raise ValueError(f'Missing or stale technical document: {name}')
                if archive.extractfile('logisth/' + source).read() != (root / source).read_bytes():
                    raise ValueError(f'Source archive technical document mismatch: {source}')
    if tuple(map(int, version.split('.'))) >= (0, 9, 7):
        name = f'logisth-{version}-rack-skin-review.zip'
        if name not in names:
            raise ValueError('Missing rack/skin validation bundle')
        with zipfile.ZipFile(folder / name) as archive:
            rows = list(csv.DictReader(io.StringIO(archive.read('theme-contrast.csv').decode('utf-8'))))
            cases = {(r['theme'], r['style'], r['screen'], r['small']) for r in rows}
            if len(rows) != 120 or len(cases) != 120 or len({r['theme'] for r in rows}) != 10:
                raise ValueError('Incomplete ten-theme contrast matrix')
            for row in rows:
                for key, minimum in [('pointer_top', 4.49), ('pointer_bottom', 4.49),
                                     ('active', 2.99), ('track', 2.99), ('raster', 3.0)]:
                    if not float(row[key]) >= minimum:
                        raise ValueError(f'Contrast fixture below its target: {row["theme"]}/{key}')
            if archive.read('README.md').replace(b'\r\n', b'\n') != (root / 'docs/RACK_REVIEW.md').read_bytes().replace(b'\r\n', b'\n'):
                raise ValueError('Stale rack/skin review description')
    if tuple(map(int, version.split('.'))) >= (0, 9, 8):
        name = f'logisth-{version}-gui-dsp-review.zip'
        if name not in names:
            raise ValueError('Missing GUI/DSP and real-WAV stereo evidence')
        with zipfile.ZipFile(folder / name) as archive:
            rows = list(csv.DictReader(io.StringIO(archive.read('stereo-convolution.csv').decode('utf-8'))))
            cases = {(r['sample_rate'], r['ir'], r['source'], r['normalized']) for r in rows}
            if len(rows) != 48 or len(cases) != 48 or any(not 0 <= float(r['max_error']) < 2e-5 for r in rows):
                raise ValueError('Incomplete or failed stereo review cases')
            if len([n for n in archive.namelist() if n.endswith('.wav')]) != 21:
                raise ValueError('Missing stereo review sample WAVs')
            if archive.read('README.md') != (root / 'docs/GUI_DSP_REVIEW.md').read_bytes():
                raise ValueError('Stale GUI/DSP review description')
            if archive.read('stereo-convolution.csv') != (root / 'docs/gui-review/stereo-convolution.csv').read_bytes():
                raise ValueError('Stale stereo review measurements')
        with zipfile.ZipFile(folder / f'logisth-{version}-themes.zip') as archive:
            if archive.read('logisth.svg') != (root / 'assets/branding/logisth.svg').read_bytes():
                raise ValueError('Supplied SVG logo differs from the theme archive')
    if tuple(map(int, version.split('.'))) >= (0, 9, 8, 1):
        name = f'logisth-{version}-appearance-review.zip'
        if name not in names:
            raise ValueError('Missing appearance regression evidence')
        with zipfile.ZipFile(folder / name) as archive:
            probe = json.loads(archive.read('windows-probe.json'))
            if probe.get('phase') != 'passed' or 'PASS 0.9.8.1 appearance' not in probe.get('test.log', ''):
                raise ValueError('Missing native Windows glyph validation')
            rows = list(csv.DictReader(io.StringIO(archive.read('text-rendering.csv').decode())))
            cases = {(r['ui_scale'], r['dpi'], r['quality'], r['antialias']) for r in rows}
            if len(rows) != 96 or len(cases) != 96:
                raise ValueError('Incomplete text scale/DPI/quality matrix')
            for row in rows:
                if row['vector_identical'] != '1' or (int(row['gray_levels']) <= 5 if row['antialias'] == '1' else int(row['gray_levels']) != 2):
                    raise ValueError('Failed glyph/vector rendering measurement')
            for name, path in [('README.md', 'docs/APPEARANCE_REVIEW.md'), ('text-rendering.csv', 'docs/appearance/text-rendering.csv')]:
                if archive.read(name).replace(b'\r\n', b'\n') != (root / path).read_bytes().replace(b'\r\n', b'\n'):
                    raise ValueError('Stale appearance evidence')
    if tuple(map(int, version.split('.'))) >= (0, 9, 8, 2):
        with zipfile.ZipFile(folder / f'logisth-{version}-backgrounds.zip') as archive:
            catalogue = json.loads(archive.read('catalogue.json'))['backgrounds']
            if len(catalogue) != 20 or len({entry['theme'] for entry in catalogue}) != 10:
                raise ValueError('Incomplete factory background catalogue')
            for entry in catalogue:
                data = archive.read(entry['file'])
                if data != (root / 'assets/backgrounds' / entry['file']).read_bytes():
                    raise ValueError('Background original differs from source')
                if not data.startswith(b'\x89PNG\r\n\x1a\n'):
                    raise ValueError('Background is not PNG')
        with zipfile.ZipFile(folder / f'logisth-{version}-compact-review.zip') as archive:
            if len([n for n in archive.namelist() if n.endswith('.png')]) != 12:
                raise ValueError('Missing compact layout/background evidence')
    if tuple(map(int, version.split('.'))) >= (0, 9, 8, 3):
        with zipfile.ZipFile(folder / f'logisth-{version}-multiband-review.zip') as archive:
            rows = list(csv.DictReader(io.StringIO(archive.read('reconstruction.csv').decode())))
            cases = {(r['sample_rate'], r['bands'], r['frequency_hz']) for r in rows}
            if len(rows) != 120 or len(cases) != 120:
                raise ValueError('Incomplete multiband runtime reconstruction matrix')
            if any(not abs(float(r['magnitude_error_db'])) < .005 for r in rows):
                raise ValueError('Multiband reconstruction exceeds unity-magnitude tolerance')
            summary = list(csv.DictReader(io.StringIO(archive.read('summary.csv').decode())))
            if (len(summary) != 1 or summary[0]['legacy_exact'] != '1'
                    or not float(summary[0]['dc_automation_error']) < 1e-5
                    or not float(summary[0]['maximum_transition_jump']) < .08):
                raise ValueError('Multiband continuity or Legacy parity failed')
            isolation = list(csv.DictReader(io.StringIO(archive.read('isolation.csv').decode())))
            if (len(isolation) != 2
                    or not float(isolation[0]['input_frequency_hz']) < 200
                    or not float(isolation[0]['third_harmonic_amplitude']) > .02
                    or not float(isolation[1]['input_frequency_hz']) > 2000
                    or not 0 <= float(isolation[1]['third_harmonic_amplitude']) < 1e-5):
                raise ValueError('Incomplete frequency-isolation evidence')
            if len([n for n in archive.namelist() if n.endswith('.png')]) != 14:
                raise ValueError('Incomplete gradient/grid/multiband screenshots')
            for name in ('reconstruction.csv', 'isolation.csv', 'summary.csv'):
                if archive.read(name) != (root / 'docs/multiband' / name).read_bytes():
                    raise ValueError('Stale multiband measurement archive')
            if archive.read('README.md') != (root / 'docs/MULTIBAND_REVIEW.md').read_bytes():
                raise ValueError('Stale multiband review description')
    print(f"Verified {len(names)} release assets for {tag}")


if __name__ == "__main__":
    try:
        verify(sys.argv[1] if len(sys.argv) == 2 else "")
    except (OSError, ValueError) as error:
        raise SystemExit(str(error)) from error

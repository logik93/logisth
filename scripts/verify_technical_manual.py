#!/usr/bin/env python3
"""Check that the technical reference/PDF match the current implementation."""
import hashlib
import json
from pathlib import Path
import re


def text_hash(path: Path) -> str:
    # Git may check text out with CRLF on Windows. Documentation identity is
    # defined using LF; PDF identity always uses its exact binary bytes.
    return hashlib.sha256(path.read_bytes().replace(b'\r\n', b'\n')).hexdigest()


def implementation_hash(root: Path) -> str:
    digest = hashlib.sha256()
    files = [root / 'CMakeLists.txt'] + sorted(
        (p for p in (root / 'Source').rglob('*') if p.is_file()),
        key=lambda p: p.relative_to(root).as_posix())
    for path in files:
        name = path.relative_to(root).as_posix().encode()
        data = path.read_bytes().replace(b'\r\n', b'\n')
        digest.update(len(name).to_bytes(8, 'big') + name)
        digest.update(len(data).to_bytes(8, 'big') + data)
    return digest.hexdigest()


def verify(root: Path) -> None:
    version = re.search(r'project\(Logisth VERSION ([\d.]+)',
                        (root / 'CMakeLists.txt').read_text(encoding='utf-8')).group(1)
    markdown = root / 'docs/TECHNICAL_MANUAL.md'
    pdf = root / 'docs/logisth-Technical-Manual-English.pdf'
    record = json.loads((root / 'docs/technical-manual.json').read_text(encoding='utf-8'))
    if not markdown.read_text(encoding='utf-8').startswith(f'# logisth {version} — Technical Manual\n'):
        raise ValueError('Update the technical manual software baseline before releasing')
    expected = {'format': 1, 'software_version': version,
                'implementation_sha256': implementation_hash(root),
                'markdown_sha256': text_hash(markdown),
                'pdf_sha256': hashlib.sha256(pdf.read_bytes()).hexdigest()}
    if record != expected or not pdf.read_bytes().startswith(b'%PDF-'):
        raise ValueError('Review the technical manual and regenerate it with scripts/build_manual.py')
    print(f'Verified current technical manual and PDF for logisth {version}')


if __name__ == '__main__':
    try:
        verify(Path(__file__).resolve().parents[1])
    except (OSError, ValueError, AttributeError) as error:
        raise SystemExit(str(error)) from error

#!/usr/bin/env python3
"""Prepare a new release's source, manual, demo, themes and calibration assets."""
import argparse
import gzip
import hashlib
import io
import re
import shutil
import subprocess
import tarfile
import zipfile
from pathlib import Path


def package(preview: Path, calibration: Path, kick_benchmark: Path | None = None, factory_bank: Path | None = None) -> Path:
    root = Path(__file__).resolve().parents[1]
    version = re.search(r'project\(Logisth VERSION ([\d.]+)',
                        (root / 'CMakeLists.txt').read_text()).group(1)
    folder = root / 'releases' / f'logisth-{version}'
    if folder.exists():
        raise ValueError('Refusing to replace an existing release asset folder')
    paths = ['.github', '.gitignore', '.gitattributes', 'CMakeLists.txt', 'README.md', 'BUILD_STATUS.md', 'LICENSE',
             'THIRD_PARTY_NOTICES.md', 'Source', 'tests', 'scripts', 'assets', 'docs', 'licenses']
    subprocess.run(['git', 'diff', '--exit-code', 'HEAD', '--', *paths], cwd=root,
                   check=True, stdout=subprocess.DEVNULL)
    if subprocess.check_output(['git', 'ls-files', '--others', '--exclude-standard', '--', *paths], cwd=root):
        raise ValueError('Commit all new source/documentation files before packaging')
    source = subprocess.check_output(['git', 'archive', '--format=tar',
                                     '--prefix=logisth/', 'HEAD', *paths], cwd=root)
    # Prove the committed archive contains the exact source/docs being packaged.
    with tarfile.open(fileobj=io.BytesIO(source)) as archive:
        for member in archive.getmembers():
            if member.isfile():
                local = root / member.name.removeprefix('logisth/')
                if archive.extractfile(member).read() != local.read_bytes():
                    raise ValueError(f'Source archive differs from working copy: {local}')
    copies = [(root / 'docs/MANUAL.md', 'logisth-Manual-English.md'),
              (root / 'docs/logisth-Manual-English.pdf', 'logisth-Manual-English.pdf'),
              (root / 'docs/CALIBRATION.md', 'logisth-GainCalibration.md'),
              (root / 'LICENSE', 'logisth-LICENSE-MIT.txt'),
              (root / 'THIRD_PARTY_NOTICES.md', 'logisth-ThirdPartyNotices.md'),
              (root / 'docs/ALGORITHM_AUDIT.json', 'logisth-AlgorithmAudit.json'),
              (root / 'docs/PROJECT_REVIEW.md', 'logisth-ProjectReview.md'),
              (preview / 'logisth-demo.wav', 'logisth-demo.wav'),
              (root / 'docs/images/logisth-dark.png', 'logisth.png')]
    if (root / 'docs' / f'REVIEW-v{version}.md').is_file():
        copies.append((root / 'docs' / f'REVIEW-v{version}.md', f'logisth-Review-v{version}.md'))
    copies += [(p, p.name) for p in sorted((root / 'docs/images').glob('*.png'))]
    calibration_files = [(p, p.name) for p in sorted(calibration.iterdir()) if p.is_file()]
    calibration_files += [(root / p, Path(p).name) for p in (
        'docs/CALIBRATION.md', 'docs/ANTICLICK.md',
        'docs/calibration/before-0.5.1.csv', f'docs/calibration/after-{version}.csv')]
    theme_files = [(p, p.name) for p in sorted((root / 'assets/themes').glob('*.json'))]
    theme_files += [(root / 'docs/THEMES.md', 'README.md'), (root / 'LICENSE', 'LICENSE'),
                    (root / 'assets/skins/ap-music-group-white.png','ap-music-group-white.png'),
                    (root / 'licenses/AP-Music-Group-Logo.txt','AP-Music-Group-Logo.txt')]
    theme_files += [(p, 'fonts/ioskeley/' + p.name) for p in sorted((root / 'assets/fonts/ioskeley').iterdir()) if p.is_file()]
    for file, _ in copies + calibration_files + theme_files:
        if not file.is_file():
            raise ValueError(f'Missing release input: {file}')
    for name in ['measurements.csv', 'logisth-sine.wav', 'reference-sine-0dBFS.wav',
                 'Calibration - sine 0 dBFS.logisth']:
        if not (calibration / name).is_file():
            raise ValueError(f'Missing calibration input: {name}')
    if len(list((root / 'assets/themes').glob('*.json'))) != 10:
        raise ValueError('All ten standard theme templates are required')
    if not (root / 'docs' / f'RELEASE-v{version}.md').is_file():
        raise ValueError('Missing versioned release notes')
    bundles=[('calibration', calibration_files), ('themes', theme_files)]
    if kick_benchmark is not None:
        required=['kick-reference.wav','kick--12st.wav','kick--7st.wav','kick-0st.wav','kick-7st.wav','kick-12st.wav','measurements.csv']
        if any(not (kick_benchmark/name).is_file() for name in required):
            raise ValueError('Kick benchmark is incomplete')
        files=[(p,p.name) for p in sorted(kick_benchmark.iterdir()) if p.is_file()]
        files += [(root/'LICENSE','LICENSE')]
        bundles.append(('kick-benchmark',files))
    if factory_bank is not None:
        presets = sorted((factory_bank / 'Presets').rglob('*.logisth'))
        if len(presets) != 48:
            raise ValueError('Factory bank must contain 48 native presets')
        required = ['factory-demo.wav', 'factory-measurements.csv', 'README.md']
        if any(not (factory_bank / name).is_file() for name in required):
            raise ValueError('Factory bank demo, measurements or description missing')
        files = [(p, p.relative_to(factory_bank).as_posix()) for p in presets]
        files += [(factory_bank / name, name) for name in required]
        files += [(root / 'LICENSE', 'LICENSE'), (root / 'docs/FACTORY_PRESETS.md', 'CATALOGUE.md')]
        bundles.append(('factory-presets', files))
    folder.mkdir(parents=True)
    for file, name in copies:
        shutil.copyfile(file, folder / name)
    for label, files in bundles:
        with zipfile.ZipFile(folder / f'logisth-{version}-{label}.zip', 'w',
                             compression=zipfile.ZIP_DEFLATED) as archive:
            for file, name in files:
                archive.write(file, name)
    with (folder / f'logisth-{version}-source.tar.gz').open('wb') as stream:
        with gzip.GzipFile(filename='', mode='wb', fileobj=stream, mtime=0) as archive:
            archive.write(source)
    (folder / 'SHA256SUMS').write_text(''.join(
        hashlib.sha256(file.read_bytes()).hexdigest() + '  ' + file.name + '\n'
        for file in sorted(folder.iterdir()) if file.is_file()))
    print(f'Packaged {len(list(folder.iterdir()))} original release files: {folder}')
    return folder


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preview', type=Path, required=True)
    parser.add_argument('--calibration', type=Path, required=True)
    parser.add_argument('--kick-benchmark', type=Path)
    parser.add_argument('--factory-bank', type=Path)
    args = parser.parse_args()
    try:
        package(args.preview.resolve(), args.calibration.resolve(), args.kick_benchmark.resolve() if args.kick_benchmark else None,
                args.factory_bank.resolve() if args.factory_bank else None)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error)) from error

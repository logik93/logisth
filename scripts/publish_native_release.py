#!/usr/bin/env python3
"""Publish tested platforms in stages while preserving already published binaries."""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
from build_native import source_fingerprint
from verify_release import verify

PLATFORMS = ('linux-x86_64', 'windows-x86_64', 'macos-universal')


def command(*args):
    return subprocess.check_output(list(map(str, args)), text=True).strip()


def checksum(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_record(record, archive, platform, version, fingerprint, commit):
    if (platform not in PLATFORMS or record['version'] != version
            or record['platform'] != platform or record['source_commit'] != commit
            or record['source_fingerprint'] != fingerprint
            or record['formats'] != ['VST3', 'CLAP', 'Standalone']
            or record['tests']['passed'] < (11 if platform.startswith('linux') else 9)
            or record['tests']['failed'] != 0 or record['tests']['skipped'] != 0):
        raise ValueError(f'Native build does not match validated release source: {platform}')
    name = record['archive']
    if Path(name).name != name or not name.startswith(f'logisth-{version}-{platform}.'):
        raise ValueError('Invalid archive name')
    if archive.name != name or checksum(archive) != record['archive_sha256']:
        raise ValueError(f'Corrupt artifact: {name}')
    wanted = {'x86_64', 'arm64'} if platform == 'macos-universal' else {'x86_64'}
    if len(record['binaries']) != 3 or any(set(b['architectures']) != wanted
                                          for b in record['binaries']):
        raise ValueError(f'Unexpected binary architectures: {platform}')


def verify_existing_manifest(folder):
    files = {p.name for p in folder.iterdir() if p.is_file()}
    if not files:
        return
    if 'SHA256SUMS' not in files:
        raise ValueError('Existing release has no checksum manifest')
    entries = {}
    for line in (folder / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        if (name in entries or Path(name).name != name
                or not re.fullmatch(r'[a-f0-9]{64}', digest)):
            raise ValueError('Invalid existing release manifest')
        entries[name] = digest
    if set(entries) != files - {'SHA256SUMS'}:
        raise ValueError('Existing release manifest does not cover its assets')
    for name, digest in entries.items():
        if checksum(folder / name) != digest:
            raise ValueError(f'Existing published asset checksum mismatch: {name}')


def stage_asset(source, existing, uploads):
    """Only the cumulative manifest may replace an existing published asset."""
    target = existing / source.name
    if target.exists():
        if checksum(target) != checksum(source):
            raise ValueError(f'Refusing to replace a published asset: {source.name}')
        return
    shutil.copy2(source, target)
    uploads.append(target)


def build_notes(authored, records, tag, commit):
    rows = []
    for platform in PLATFORMS:
        if platform not in records:
            continue
        record = records[platform]
        requirement = ('glibc >= ' + record['minimum_glibc'] if platform.startswith('linux')
                       else 'macOS 11+, ad-hoc signed, not notarized' if platform.startswith('macos')
                       else 'Windows x64, static MSVC runtime')
        rows.append(f'| {platform} | {record["tests"]["passed"]} / 0 skipped | {requirement} |')
    pending = [p for p in PLATFORMS if p not in records]
    text = (authored + '\n## Native build validation\n\n'
            + '| Platform | Tests passed | Requirements |\n| --- | --- | --- |\n'
            + '\n'.join(rows)
            + f'\n\nSource: `{tag}` (`{commit}`). '
            + f'[Build and validation]({os.environ.get("LOGISTH_RUN_URL", "")}).\n')
    if pending:
        text += ('\nWindows and Linux are available now. macOS Universal will be built '
                 'after this publication and added to this same release.\n')
    return text, pending


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tag', required=True)
    parser.add_argument('--artifacts', type=Path, required=True)
    parser.add_argument('--result', type=Path, required=True)
    parser.add_argument('--platforms', nargs='+', choices=PLATFORMS, default=list(PLATFORMS[:2]))
    args = parser.parse_args()
    if not re.fullmatch(r'v\d+\.\d+\.\d+', args.tag):
        raise ValueError('Invalid release tag')
    if len(args.platforms) != len(set(args.platforms)):
        raise ValueError('Duplicate selected platform')
    verify(args.tag)
    version = args.tag[1:]
    repository = os.environ['GITHUB_REPOSITORY']
    commit = command('git', 'rev-parse', f'{args.tag}^{{commit}}')
    fingerprint = source_fingerprint(Path('.'), commit)
    selected = {}
    for platform in args.platforms:
        matches = list(args.artifacts.rglob(f'build-info-{platform}.json'))
        if len(matches) != 1:
            raise ValueError(f'Missing or ambiguous native artifact: {platform}')
        info = matches[0]
        record = json.loads(info.read_text())
        archive = info.parent / record['archive']
        validate_record(record, archive, platform, version, fingerprint, commit)
        selected[platform] = (record, archive, info)
    release = subprocess.run(['gh', 'api', f'repos/{repository}/releases/tags/{args.tag}'],
                             capture_output=True, text=True)
    if release.returncode:
        if 'HTTP 404' not in release.stderr:
            raise RuntimeError(release.stderr)
        if set(args.platforms) != set(PLATFORMS[:2]):
            raise ValueError('Publish Windows/Linux together before appending macOS')
        subprocess.run(['gh', 'release', 'create', args.tag, '--draft', '--verify-tag', '--title',
                        f'wavelogik logisth {version}', '--notes-file', f'docs/RELEASE-{args.tag}.md',
                        '--repo', repository], check=True)
        release_info = json.loads(command('gh', 'api', f'repos/{repository}/releases/tags/{args.tag}'))
    else:
        release_info = json.loads(release.stdout)
    with tempfile.TemporaryDirectory(prefix='logisth-release-') as directory:
        existing = Path(directory)
        published = {a['name']: a for a in release_info.get('assets', [])}
        if published:
            if any(not re.fullmatch(r'[A-Za-z0-9_.-]+', name) for name in published):
                raise ValueError('Unsafe existing release asset name')
            subprocess.run(['gh', 'release', 'download', args.tag, '--repo', repository,
                            '--dir', str(existing)], check=True)
        verify_existing_manifest(existing)
        records = {}
        for platform in PLATFORMS:
            path = existing / f'build-info-{platform}.json'
            if path.exists():
                record = json.loads(path.read_text())
                validate_record(record, existing / record['archive'], platform, version,
                                fingerprint, commit)
                records[platform] = record
        if 'macos-universal' in selected and not set(PLATFORMS[:2]).issubset(records):
            raise ValueError('Both Windows/Linux downloads must exist before appending macOS')
        uploads = []
        original = Path('releases') / f'logisth-{version}'
        for path in sorted(original.iterdir()):
            if path.is_file() and path.name not in {'README.md', 'SHA256SUMS'}:
                stage_asset(path, existing, uploads)
        for platform, (record, archive, info) in selected.items():
            stage_asset(archive, existing, uploads)
            stage_asset(info, existing, uploads)
            records[platform] = record
        manifest = existing / 'SHA256SUMS'
        manifest.write_text(''.join(checksum(path) + '  ' + path.name + '\n'
                                    for path in sorted(existing.iterdir())
                                    if path.is_file() and path.name != 'SHA256SUMS'))
        uploads.append(manifest)
        subprocess.run(['gh', 'release', 'upload', args.tag, *map(str, uploads), '--clobber',
                        '--repo', repository], check=True)
        notes, pending = build_notes((Path('docs') / f'RELEASE-{args.tag}.md').read_text(),
                                     records, args.tag, commit)
        notes_file = existing / 'release-notes.txt'
        notes_file.write_text(notes)
        subprocess.run(['gh', 'release', 'edit', args.tag, '--notes-file', str(notes_file),
                        '--repo', repository], check=True)
        verified = json.loads(command('gh', 'api', f'repos/{repository}/releases/tags/{args.tag}'))
        assets = {a['name']: a for a in verified['assets']}
        for path in uploads:
            if path.name not in assets or assets[path.name]['size'] != path.stat().st_size:
                raise ValueError(f'Release upload verification failed: {path.name}')
        # Appending macOS must retain both existing native downloads and metadata.
        for name, asset in published.items():
            if name != 'SHA256SUMS' and assets.get(name, {}).get('id') != asset['id']:
                raise ValueError(f'An existing published asset was replaced: {name}')
        if verified.get('draft'):
            subprocess.run(['gh', 'release', 'edit', args.tag, '--draft=false', '--latest',
                            '--repo', repository], check=True)
            verified = json.loads(command('gh', 'api', f'repos/{repository}/releases/tags/{args.tag}'))
            assets = {a['name']: a for a in verified['assets']}
        args.result.write_text(json.dumps({
            'phase': 'published', 'tag': args.tag, 'release_url': verified['html_url'],
            'release_commit': commit, 'published_at': verified.get('published_at'),
            'publication_stage': 'complete' if not pending else 'windows-linux',
            'pending_platforms': pending,
            'platforms': [records[p] for p in PLATFORMS if p in records],
            'assets': [{'name': name, 'id': a['id'], 'size': a['size'],
                        'url': a['browser_download_url']} for name, a in sorted(assets.items())]
        }, indent=2) + '\n')
    print('Published and verified:', verified['html_url'], 'pending:', pending)


if __name__ == '__main__':
    main()

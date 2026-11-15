"""Regression checks for staged publication and immutable native downloads."""
import contextlib
import copy
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import publish_native_release as publisher


class FakeGitHub:
    def __init__(self):
        self.release = None
        self.files = {}
        self.next_id = 1
        self.publications = []

    def api(self):
        result = copy.deepcopy(self.release)
        result['assets'] = [dict(name=name, id=identity, size=len(data),
                                 browser_download_url='https://example.test/' + name)
                            for name, (identity, data) in self.files.items()]
        return result

    def execute(self, args, **kwargs):
        args = list(map(str, args))
        if args[:2] == ['git', 'rev-parse']:
            return subprocess.CompletedProcess(args, 0, 'abc123\n', '')
        if args[:2] == ['gh', 'api']:
            if self.release is None or ('/tags/' in args[2] and self.release['draft']):
                return subprocess.CompletedProcess(args, 1, '', 'HTTP 404')
            return subprocess.CompletedProcess(args, 0, json.dumps(self.api()), '')
        action = args[2]
        if action == 'view':
            if self.release is None:
                return subprocess.CompletedProcess(args, 1, '', 'release not found')
            return subprocess.CompletedProcess(args, 0, json.dumps({'databaseId': 101}), '')
        if action == 'create':
            self.release = {'id': 101, 'draft': True, 'html_url': 'https://example.test/v0.9.3',
                            'published_at': None}
        elif action == 'download':
            folder = Path(args[args.index('--dir') + 1])
            for name, (_, data) in self.files.items():
                (folder / name).write_bytes(data)
        elif action == 'upload':
            for value in args[4:args.index('--clobber')]:
                path = Path(value)
                self.files[path.name] = (self.next_id, path.read_bytes())
                self.next_id += 1
        elif action == 'edit':
            if '--notes-file' in args:
                self.release['notes'] = Path(args[args.index('--notes-file') + 1]).read_text()
            if '--draft=false' in args:
                self.release['draft'] = False
                self.release['published_at'] = '2026-11-14T12:00:00Z'
                self.publications.append(set(self.files))
        else:
            raise AssertionError(args)
        return subprocess.CompletedProcess(args, 0, '', '')

    def output(self, args, **kwargs):
        result = self.execute(args, **kwargs)
        if result.returncode:
            raise subprocess.CalledProcessError(result.returncode, args, result.stdout)
        return result.stdout


class PublisherTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def artifact(self, platform):
        folder = self.root / 'artifacts' / platform
        folder.mkdir(parents=True, exist_ok=True)
        archive = folder / f'logisth-0.9.3-{platform}.zip'
        archive.write_bytes(('native ' + platform).encode())
        architectures = ['arm64', 'x86_64'] if platform.startswith('macos') else ['x86_64']
        record = dict(version='0.9.3', platform=platform, source_commit='abc123',
                      source_fingerprint='fingerprint', formats=['VST3', 'CLAP', 'Standalone'],
                      tests=dict(passed=12 if platform.startswith('linux') else 10,
                                 failed=0, skipped=0),
                      archive=archive.name, archive_sha256=publisher.checksum(archive),
                      binaries=[dict(architectures=architectures) for _ in range(3)],
                      minimum_glibc='2.35')
        info = folder / f'build-info-{platform}.json'
        info.write_text(json.dumps(record))
        return record, archive

    def validate(self, record, archive):
        publisher.validate_record(record, archive, record['platform'], '0.9.3',
                                  'fingerprint', 'abc123')

    def test_rejects_unverified_or_wrong_source_native_build(self):
        record, archive = self.artifact('linux-x86_64')
        self.validate(record, archive)
        for field, value in [('source_commit', 'different'), ('source_fingerprint', 'stale'),
                             ('formats', ['VST3']), ('version', '0.9.2')]:
            with self.subTest(field=field):
                changed = dict(record, **{field: value})
                with self.assertRaises(ValueError):
                    self.validate(changed, archive)
        for field, value in [('failed', 1), ('skipped', 1), ('passed', 11)]:
            with self.subTest(test=field):
                changed = dict(record, tests=dict(record['tests'], **{field: value}))
                with self.assertRaises(ValueError):
                    self.validate(changed, archive)
        archive.write_bytes(b'corrupted download')
        with self.assertRaises(ValueError):
            self.validate(record, archive)

    def test_macos_must_contain_both_architectures(self):
        record, archive = self.artifact('macos-universal')
        self.validate(record, archive)
        record['binaries'][1]['architectures'] = ['x86_64']
        with self.assertRaises(ValueError):
            self.validate(record, archive)

    def test_published_files_cannot_be_replaced(self):
        existing = self.root / 'published'
        existing.mkdir()
        source = self.root / 'native.zip'
        source.write_bytes(b'original')
        uploads = []
        publisher.stage_asset(source, existing, uploads)
        self.assertEqual(len(uploads), 1)
        publisher.stage_asset(source, existing, uploads)
        self.assertEqual(len(uploads), 1)
        source.write_bytes(b'replacement')
        with self.assertRaisesRegex(ValueError, 'Refusing to replace'):
            publisher.stage_asset(source, existing, uploads)
        self.assertEqual((existing / source.name).read_bytes(), b'original')

    def test_manifest_requires_complete_valid_assets(self):
        publisher.verify_existing_manifest(self.root)
        asset = self.root / 'native.zip'
        asset.write_bytes(b'binary')
        with self.assertRaisesRegex(ValueError, 'no checksum'):
            publisher.verify_existing_manifest(self.root)
        manifest = self.root / 'SHA256SUMS'
        line = publisher.checksum(asset) + '  native.zip\n'
        manifest.write_text(line)
        publisher.verify_existing_manifest(self.root)
        manifest.write_text(line + line)
        with self.assertRaisesRegex(ValueError, 'Invalid'):
            publisher.verify_existing_manifest(self.root)
        manifest.write_text(line)
        (self.root / 'unlisted.txt').write_text('extra')
        with self.assertRaisesRegex(ValueError, 'cover'):
            publisher.verify_existing_manifest(self.root)
        (self.root / 'unlisted.txt').unlink()
        asset.write_bytes(b'corrupted')
        with self.assertRaisesRegex(ValueError, 'checksum mismatch'):
            publisher.verify_existing_manifest(self.root)

    def test_initial_publication_then_macos_append_preserves_native_ids_and_bytes(self):
        originals = self.root / 'releases/logisth-0.9.3'
        originals.mkdir(parents=True)
        (originals / 'manual.pdf').write_bytes(b'English manual')
        (originals / 'SHA256SUMS').write_text('recomputed at publication')
        docs = self.root / 'docs'
        docs.mkdir()
        (docs / 'RELEASE-v0.9.3.md').write_text('Release notes\n')
        for platform in publisher.PLATFORMS:
            self.artifact(platform)
        github = FakeGitHub()
        result = self.root / 'result.json'
        previous_cwd = Path.cwd()
        os.chdir(self.root)
        self.addCleanup(os.chdir, previous_cwd)
        with patch.object(publisher, 'verify'), \
                patch.object(publisher, 'source_fingerprint', return_value='fingerprint'), \
                patch.object(publisher.subprocess, 'run', side_effect=github.execute), \
                patch.object(publisher.subprocess, 'check_output', side_effect=github.output), \
                patch.dict(os.environ, {'GITHUB_REPOSITORY': 'owner/logisth'}), \
                contextlib.redirect_stdout(io.StringIO()):
            def publish(platforms):
                with patch.object(sys, 'argv', ['publisher', '--tag', 'v0.9.3', '--artifacts',
                                              str(self.root / 'artifacts'), '--result', str(result),
                                              '--platforms', *platforms]):
                    publisher.main()
                return json.loads(result.read_text())
            with self.assertRaisesRegex(ValueError, 'Windows/Linux together'):
                publish(['macos-universal'])
            self.assertIsNone(github.release)
            first = publish(['linux-x86_64', 'windows-x86_64'])
            self.assertEqual(first['pending_platforms'], ['macos-universal'])
            self.assertEqual(first['publication_stage'], 'windows-linux')
            self.assertFalse(github.release['draft'])
            self.assertEqual(len(github.publications), 1)
            self.assertFalse(any('macos' in name for name in github.publications[0]))
            first_files = copy.deepcopy(github.files)
            complete = publish(['macos-universal'])
            self.assertEqual(complete['pending_platforms'], [])
            self.assertEqual(complete['publication_stage'], 'complete')
            self.assertEqual(len(complete['platforms']), 3)
            self.assertEqual(complete['release_url'], first['release_url'])
            self.assertEqual(complete['release_id'], first['release_id'])
            for name, asset in first_files.items():
                if name != 'SHA256SUMS':
                    self.assertEqual(github.files[name], asset, name)
            self.assertNotIn('will be built', github.release['notes'])
            download = self.root / 'download'
            download.mkdir()
            for name, (_, data) in github.files.items():
                (download / name).write_bytes(data)
            publisher.verify_existing_manifest(download)
            # Retrying an already successful append keeps all native asset IDs too.
            before_retry = copy.deepcopy(github.files)
            publish(['macos-universal'])
            for name in before_retry:
                if name != 'SHA256SUMS':
                    self.assertEqual(github.files[name], before_retry[name], name)


if __name__ == '__main__':
    unittest.main()

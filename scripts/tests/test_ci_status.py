"""Failed native tests must leave actionable, redacted CI receipts."""
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from write_ci_status import native_test_failures

class NativeFailureTests(unittest.TestCase):
    def test_exception_and_output_are_retained_and_credentials_redacted(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            self.assertEqual(native_test_failures(folder), [])
            (folder / 'logs').mkdir()
            report = folder / 'logs/test-results.xml'
            report.write_text('<testsuite><testcase name="passed"/>'
                              '<testcase name="processor"><failure message="SEGFAULT"/>'
                              '<system-out>stack-overflow github_pat_private123 '
                              'https://private@github.com/repo</system-out>'
                              '</testcase></testsuite>')
            failures = native_test_failures(folder)
            self.assertEqual(len(failures), 1)
            self.assertEqual(failures[0]['name'], 'processor')
            self.assertEqual(failures[0]['message'], 'SEGFAULT')
            self.assertIn('stack-overflow', failures[0]['output'])
            self.assertNotIn('private', failures[0]['output'])
            report.write_text('<incomplete>')
            self.assertIn('Incomplete', native_test_failures(folder)[0]['message'])

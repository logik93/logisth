import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from verify_release import validate_tag

class VersionTests(unittest.TestCase):
    def test_three_and_four_part_tags(self):
        for tag in ['v0.9.4', 'v0.9.4.1', 'v1.0.0', 'v12.34.56.78']:
            self.assertEqual(validate_tag(tag), tag[1:])
    def test_invalid_tags_are_rejected(self):
        for tag in ['', '0.9.4.1', 'v0.9', 'v0.9.4.1.2', 'v0.9.4.', 'v0.9.4.1-rc1', 'v0.9.4/1', 'v0.9.4\n']:
            with self.subTest(tag=tag), self.assertRaises(ValueError):
                validate_tag(tag)

if __name__ == '__main__':
    unittest.main()

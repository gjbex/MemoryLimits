import os
import sys
import subprocess
import tempfile
import unittest

SCRIPT = os.path.join(os.path.dirname(__file__), '..', 'mem_limit', 'check_pinning.py')

class CheckPinningTests(unittest.TestCase):

    def run_script(self, content, *args):
        with tempfile.NamedTemporaryFile('w+', delete=False) as f:
            f.write(content)
            fname = f.name
        try:
            result = subprocess.run([
                sys.executable, SCRIPT, *args, fname
            ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        finally:
            os.unlink(fname)
        return result

    def test_normal_input(self):
        content = """====\nfoo 0#0 bar 0\nfoo 0#0 bar 2\nfoo 1#0 bar 0\n====\n"""
        res = self.run_script(content)
        self.assertEqual(res.returncode, 0)
        self.assertIn('Summary: 1/2 threads moved, 1 total moves', res.stdout)
        self.assertEqual(res.stderr, '')

    def test_malformed_input(self):
        content = """====\nfoo 0#0 bar 0\nbad line\nfoo 0#0 bar 1\n====\n"""
        res = self.run_script(content)
        self.assertEqual(res.returncode, 0)
        self.assertIn('warning', res.stderr.lower())
        self.assertIn('Summary: 1/1 threads moved, 1 total moves', res.stdout)


if __name__ == '__main__':
    unittest.main()

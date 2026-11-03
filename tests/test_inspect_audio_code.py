import contextlib
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
import inspect_audio_code as inspector


class AudioInspectionTests(unittest.TestCase):
    def test_relative_calls_and_final_instruction(self):
        target = 0x100
        data = bytearray(0x200)
        for offset in (0, 0x180, len(data)-5):
            data[offset:offset+5] = b"\xe8" + (target-offset-5).to_bytes(4, "little", signed=True)
        # A jump is not a direct call; a truncated E8 is not a candidate.
        data[16:21] = b"\xe9" + (target-16-5).to_bytes(4, "little", signed=True)
        self.assertEqual(inspector.call_candidates(data, target), [0, 0x180, len(data)-5])
        self.assertEqual(inspector.call_candidates(b"\xe8\x00", target), [])

    def test_unknown_executable_rejected_before_disassembly(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "unknown.exe"
            path.write_bytes(b"unverified build")
            with patch.object(sys, "argv", ["inspect_audio_code.py", str(path)]), \
                    patch.object(inspector, "disassemble") as disassemble, \
                    contextlib.redirect_stderr(io.StringIO()) as error:
                with self.assertRaises(SystemExit) as result:
                    inspector.main()
                self.assertEqual(result.exception.code, 1)
                self.assertIn("Unverified executable SHA-256", error.getvalue())
                disassemble.assert_not_called()


if __name__ == "__main__":
    unittest.main()

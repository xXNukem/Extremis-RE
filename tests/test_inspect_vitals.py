from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import inspect_vitals


class VitalsEvidenceTests(unittest.TestCase):
    def test_template_word_and_dword_offsets(self):
        data = bytearray(0x1d8ca)
        fields = [0]*18
        fields[12:18] = [500, 0, 1000, -1, -1, 288000]
        struct.pack_into("<15H3i", data, 0x1d8a0, *fields)
        result = inspect_vitals.new_game_template(data)
        self.assertEqual((result["health"],result["oxygen"],result["bottle_reserve"]),
                         (1000,288000,-1))
        with self.assertRaises(ValueError):
            inspect_vitals.new_game_template(data[:-1])

    def test_unknown_executable_writes_no_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            exe = root / "unknown.exe"
            exe.write_bytes(b"unverified executable")
            with self.assertRaisesRegex(ValueError,"audited reference"):
                inspect_vitals.inspect(exe, root / "report", root / "missing-library")
            self.assertFalse((root / "report").exists())


if __name__ == "__main__":
    unittest.main()

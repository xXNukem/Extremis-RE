import importlib.util
from pathlib import Path
import struct
import unittest

SPEC = importlib.util.spec_from_file_location(
    "extract_audio", Path(__file__).resolve().parents[1] / "tools/extract_audio.py")
audio = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audio)


def sample():
    return b"DSS1" + b"test".ljust(24, b" ") + struct.pack("<HIHII", 5000, 3, 0, 0, 0) + bytes([0,128,255])


class AudioExtractionTests(unittest.TestCase):
    def test_unsigned_pcm_and_header(self):
        record, end = audio.sample_at(sample())
        self.assertEqual(end, len(sample()))
        self.assertEqual(record["pcm"], bytes([0,128,255]))
        self.assertEqual(record["rate"], 5000)

    def test_bank_and_truncation(self):
        bank = bytes([1,0,64,255,1]) + sample()
        record, = audio.decode_bank(bank)
        self.assertEqual(record["id"], 0)
        self.assertEqual(record["continuous"], 255)
        for invalid in [bank[:-1], bank + b"0", bytes([2,0]) + bank[2:], b""]:
            with self.assertRaises(ValueError):
                audio.decode_bank(invalid)

    def test_output_restricted_to_build(self):
        with self.assertRaises(ValueError):
            audio.extract(Path("."), Path("outside-audio-debug"))


if __name__ == "__main__":
    unittest.main()

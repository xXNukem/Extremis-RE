"""Synthetic format checks; no original game files are required."""

from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import extract_dalle


class TextureFormatTests(unittest.TestCase):
    def test_palette_scaling_and_validation(self):
        palette = bytes(range(64)) * 12
        expanded = extract_dalle.decode_palette(palette)
        self.assertEqual(expanded[0], 0)
        self.assertEqual(expanded[63], 255)
        for invalid in (b"", bytes(767), bytes([64]) * 768):
            with self.subTest(length=len(invalid)):
                with self.assertRaises(ValueError):
                    extract_dalle.decode_palette(invalid)

    def test_dalle_boundaries_and_indices(self):
        data = bytes(index % 256 for index in range(extract_dalle.DAL_REFERENCE_SIZE))
        images = extract_dalle.decode_dalle(data)
        self.assertEqual(len(images), 54)
        self.assertEqual(images[0].pixels[74], data[74])
        self.assertEqual(images[-1].offset, 235320)
        self.assertEqual(b"".join(image.pixels for image in images), data)
        for invalid in (b"", data[:-1], data + b"\x00"):
            with self.assertRaises(ValueError):
                extract_dalle.decode_dalle(invalid)

    @staticmethod
    def sprite_fixture():
        header = struct.pack("<HHII", 0, 2, 8, 20)
        first = struct.pack("<HH4s", 2, 2, b"abcd") + bytes([1, 2, 3, 4])
        second = struct.pack("<HH4s", 1, 2, b"efgh") + bytes([5, 6])
        return header + first + second

    def test_sprite_offsets_dimensions_and_metadata(self):
        images = extract_dalle.decode_spr(self.sprite_fixture())
        self.assertEqual([image.offset for image in images], [12, 24])
        self.assertEqual([(image.width, image.height) for image in images], [(2, 2), (1, 2)])
        self.assertEqual(images[0].metadata, b"abcd")
        self.assertEqual(images[1].pixels, bytes([5, 6]))

    def test_sprite_truncation(self):
        data = self.sprite_fixture()
        for length in range(len(data)):
            with self.subTest(length=length):
                with self.assertRaises(ValueError):
                    extract_dalle.decode_spr(data[:length])

    def test_sprite_bad_offsets_and_counts(self):
        for position, value, fmt in (
                (2, 0, "<H"), (2, 65535, "<H"),
                (4, 0, "<I"), (4, 0xffffffff, "<I"),
                (8, 8, "<I"), (8, 19, "<I"), (8, 0xffffffff, "<I"),
                (12, 0, "<H"), (14, 65535, "<H")):
            data = bytearray(self.sprite_fixture())
            struct.pack_into(fmt, data, position, value)
            with self.subTest(position=position, value=value):
                with self.assertRaises(ValueError):
                    extract_dalle.decode_spr(data)

    def test_png_preserves_native_indices(self):
        record = extract_dalle.decode_spr(self.sprite_fixture())[0]
        palette = extract_dalle.decode_palette(bytes(range(64)) * 12)
        extract_dalle.DEBUG_ROOT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=extract_dalle.DEBUG_ROOT) as directory:
            path = Path(directory) / "test.png"
            extract_dalle.paletted_image(record, palette).save(path)
            with extract_dalle.Image.open(path) as image:
                self.assertEqual(image.mode, "P")
                self.assertEqual(image.size, (record.width, record.height))
                self.assertEqual(image.tobytes(), record.pixels)
                self.assertEqual(bytes(image.getpalette()), palette)
                self.assertNotIn("transparency", image.info)

    def test_output_is_confined_to_debug_directory(self):
        self.assertEqual(extract_dalle.safe_output(extract_dalle.DEBUG_ROOT),
                         extract_dalle.DEBUG_ROOT.resolve())
        for path in (Path("/tmp"), extract_dalle.REPOSITORY / "docs",
                     extract_dalle.DEBUG_ROOT / ".."):
            with self.assertRaises(ValueError):
                extract_dalle.safe_output(path)


if __name__ == "__main__":
    unittest.main()

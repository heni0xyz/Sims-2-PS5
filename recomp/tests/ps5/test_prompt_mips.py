"""Independent DDS/BC2 decode oracle against the licensed, upright source PNGs."""
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import unittest

try:
    from PIL import Image
except ImportError:
    Image = None

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/ps5'))


@unittest.skipIf(Image is None, 'Offline mip compiler check requires Pillow; patch application does not')
class PromptMipTests(unittest.TestCase):
    def test_bc2_orientation_alpha_color_and_every_mip_against_png_oracle(self):
        from compile_prompt_mips import compile_mips
        root = Path(__file__).resolve().parents[2] / 'mods/dualsense'
        manifest = json.loads((root / 'manifest.json').read_text())
        self.assertEqual(manifest['upstream']['license'], 'CC0-1.0')
        self.assertEqual(len(manifest['replacements']), 17)
        self.assertIn('Creative Commons Zero', (root / 'kenney/License.txt').read_text(errors='replace'))
        for item in manifest['replacements']:
            with self.subTest(resource=item['resource']):
                source = root / 'kenney/PNG' / (item['glyph'] + '.png')
                vector = root / 'kenney/SVG' / (item['glyph'] + '.svg')
                self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), item['png_sha256'])
                self.assertEqual(hashlib.sha256(vector.read_bytes()).hexdigest(), item['svg_sha256'])
                data = (root / item['mips_file']).read_bytes()
                self.assertEqual(data, compile_mips(source))
                self.assertEqual(len(data), 1344)
                image = Image.open(source).convert('RGBA')
                offset = 0
                for size in (32, 16, 8):
                    count = size * size  # one BC2 byte per texel
                    header = bytearray(128)
                    header[:4] = b'DDS '
                    struct.pack_into('<7I', header, 4, 124, 0x81007, size, size, count, 0, 0)
                    struct.pack_into('<II4s', header, 76, 32, 4, b'DXT3')
                    struct.pack_into('<I', header, 108, 0x1000)
                    decoded = Image.open(io.BytesIO(header + data[offset:offset + count])).convert('RGBA')
                    decoded = decoded.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                    expected = image.resize((size, size), Image.Resampling.LANCZOS)
                    # Compare premultiplied pixels: hidden RGB in transparent texels is immaterial.
                    alpha_error = 0
                    color_error = 0
                    for a, b in zip(decoded.getdata(), expected.getdata()):
                        alpha_error = max(alpha_error, abs(a[3] - b[3]))
                        color_error += sum(abs(a[k] * a[3] - b[k] * b[3]) / 255 for k in range(3))
                    self.assertLessEqual(alpha_error, 8)
                    self.assertLess(color_error / (size * size * 3), 8)
                    offset += count
                self.assertEqual(offset, len(data))


if __name__ == '__main__':
    unittest.main()

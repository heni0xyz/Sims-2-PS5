"""Compile the licensed PNG sources into the game's three BC2 mip levels.

Requires Pillow only for offline image decoding/resampling. The archive patcher
itself needs no third party Python libraries. BC2 encoding is deterministic;
alpha is explicit 4-bit coverage, color uses RGB565 endpoints per 4x4 block.
"""
import argparse
import hashlib
from pathlib import Path
import struct

from PIL import Image


def rgb565(c):
    return ((c[0] * 31 + 127) // 255 << 11) | ((c[1] * 63 + 127) // 255 << 5) | ((c[2] * 31 + 127) // 255)


def color565(v):
    return ((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31)


def encode_bc2(image):
    width, height = image.size
    assert width % 4 == height % 4 == 0
    result = bytearray()
    for y in range(0, height, 4):
        for x in range(0, width, 4):
            pixels = [image.getpixel((x + col, y + row)) for row in range(4) for col in range(4)]
            alpha = sum(((p[3] * 15 + 127) // 255) << (4 * i) for i, p in enumerate(pixels))
            opaque = [p[:3] for p in pixels if p[3]] or [(0, 0, 0)]
            # Choose the pair furthest apart, retaining colored D-pad indicators.
            a, b = max(((a, b) for a in set(opaque) for b in set(opaque)),
                       key=lambda pair: (sum((a - b) ** 2 for a, b in zip(*pair)), pair))
            c0, c1 = sorted((rgb565(a), rgb565(b)), reverse=True)
            ca, cb = color565(c0), color565(c1)
            palette = (ca, cb, tuple((2 * a + b) // 3 for a, b in zip(ca, cb)),
                       tuple((a + 2 * b) // 3 for a, b in zip(ca, cb)))
            indices = 0
            for i, p in enumerate(pixels):
                best = min(range(4), key=lambda j: sum((p[k] - palette[j][k]) ** 2 for k in range(3)))
                indices |= best << (2 * i)
            result += struct.pack('<QHHI', alpha, c0, c1, indices)
    return bytes(result)


def compile_mips(source):
    image = Image.open(source).convert('RGBA')
    if image.size != (64, 64):
        raise ValueError('Expected the original Kenney 64x64 PNG')
    # The verified Xbox assets store bottom-up UI glyphs. Preserve that convention.
    # Resize premultiplied RGBA to avoid dark fringes, with all levels from the source.
    return b''.join(encode_bc2(image.resize((size, size), Image.Resampling.LANCZOS)
                              .transpose(Image.Transpose.FLIP_TOP_BOTTOM)) for size in (32, 16, 8))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    data = compile_mips(args.source)
    with args.output.open('xb') as output:
        output.write(data)
    print(hashlib.sha256(data).hexdigest())


if __name__ == '__main__':
    main()

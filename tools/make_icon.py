from PIL import Image
import os
import struct
import colorsys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, 'assets')
SOURCE = os.path.join(ASSETS, 'logo_source.png')
TARGET_RGB = (0x35, 0x31, 0x4D)
SIZES = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]
CONTENT_RATIO = 0.88


def shift_hue_to_target(image):
    counts = Counter()
    for r, g, b, a in image.getdata():
        if a > 128:
            counts[(r, g, b)] += 1
    dominant = counts.most_common(1)[0][0]
    source_hue = colorsys.rgb_to_hsv(*(v / 255 for v in dominant))[0]
    target_hue = colorsys.rgb_to_hsv(*(v / 255 for v in TARGET_RGB))[0]
    delta = target_hue - source_hue
    out = Image.new('RGBA', image.size)
    src = image.load()
    dst = out.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = src[x, y]
            if a == 0:
                dst[x, y] = (0, 0, 0, 0)
                continue
            h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            rr, gg, bb = colorsys.hsv_to_rgb((h + delta) % 1.0, s, v)
            dst[x, y] = (round(rr * 255), round(gg * 255), round(bb * 255), a)
    return out


def resize_rgba(image, size):
    # Resize premultiplied alpha to avoid dark fringes around the logo.
    return image.convert('RGBa').resize(size, Image.Resampling.LANCZOS).convert('RGBA')


def render_icon(source, size):
    canvas = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    target = max(1, int(round(size * CONTENT_RATIO)))
    scale = min(target / source.width, target / source.height)
    width = max(1, int(round(source.width * scale)))
    height = max(1, int(round(source.height * scale)))
    art = resize_rgba(source, (width, height))
    canvas.alpha_composite(art, ((size - width) // 2, (size - height) // 2))
    return canvas


def dib_bytes(image):
    width, height = image.size
    pixels = image.load()
    xor = bytearray()
    for y in range(height - 1, -1, -1):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            xor.extend((b, g, r, a))

    mask_stride = ((width + 31) // 32) * 4
    mask = bytearray(mask_stride * height)
    for y in range(height - 1, -1, -1):
        row = (height - 1 - y) * mask_stride
        for x in range(width):
            if pixels[x, y][3] == 0:
                mask[row + x // 8] |= 0x80 >> (x % 8)

    header = struct.pack('<IiiHHIIiiII', 40, width, height * 2, 1, 32, 0,
                         len(xor) + len(mask), 0, 0, 0, 0)
    return header + bytes(xor) + bytes(mask)


def write_ico(path, images):
    entries = []
    for size in SIZES:
        image = images[size]
        if size >= 256:
            tmp = os.path.join(ASSETS, '.ico_%d.png' % size)
            image.save(tmp, format='PNG', optimize=True)
            with open(tmp, 'rb') as f:
                payload = f.read()
            os.remove(tmp)
        else:
            payload = dib_bytes(image)
        entries.append((size, payload))

    offset = 6 + 16 * len(entries)
    out = bytearray(struct.pack('<HHH', 0, 1, len(entries)))
    for size, payload in entries:
        out.extend(struct.pack('<BBBBHHII', size & 0xFF, size & 0xFF, 0, 0, 1, 32,
                               len(payload), offset))
        offset += len(payload)
    for _, payload in entries:
        out.extend(payload)
    with open(path, 'wb') as f:
        f.write(out)


def main():
    source = Image.open(SOURCE).convert('RGBA')
    bbox = source.getchannel('A').getbbox()
    if bbox:
        source = source.crop(bbox)
    source = shift_hue_to_target(source)

    images = {size: render_icon(source, size) for size in SIZES}
    render_icon(source, 1024).save(os.path.join(ASSETS, 'novoline_1024.png'))
    for size in SIZES:
        images[size].save(os.path.join(ASSETS, 'icon_%d.png' % size))
    write_ico(os.path.join(ASSETS, 'novoline.ico'), images)

    # Contact sheet for checking small-size legibility.
    cell = 160
    sheet = Image.new('RGBA', (cell * 5, cell * 2), (33, 36, 41, 255))
    for i, size in enumerate(SIZES):
        preview = images[size].resize((128, 128), Image.Resampling.NEAREST)
        x = (i % 5) * cell + (cell - preview.width) // 2
        y = (i // 5) * cell + (cell - preview.height) // 2
        sheet.alpha_composite(preview, (x, y))
    sheet.convert('RGB').save(os.path.join(ASSETS, 'icon_sizes_preview.png'))

    print('icon sizes:', SIZES)


if __name__ == '__main__':
    main()

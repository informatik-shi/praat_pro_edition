"""Inspect only our isolated Xvfb test application's mapped windows."""
import json
import pathlib
import re
import subprocess
import sys
import time
from PIL import Image, ImageStat

pid, output, size = int(sys.argv[1]), pathlib.Path(sys.argv[2]), sys.argv[3]
screen_width, screen_height = map(int, size.split('x'))

def call(*arguments):
    return subprocess.check_output(arguments, text=True).strip()

time.sleep(1)
ids = call('xdotool', 'search', '--onlyvisible', '--pid', str(pid)).splitlines()
records = []
expected = {'tablet_sound', 'tablet_spectrum', 'tablet_pitch', 'tablet_intensity',
            'tablet_formants', 'tablet_samples', 'tablet_smart', 'tablet_tracks',
            'tablet_lua_plot', 'tablet_lua_editor', 'tablet_praat_editor', 'tablet_scroll_dialog'}
seen = set()
for window in ids:
    title = call('xdotool', 'getwindowname', window)
    if not title:
        continue
    geometry = dict(line.split('=', 1) for line in call('xdotool', 'getwindowgeometry', '--shell', window).splitlines())
    x, y, width, height = [int(geometry[k]) for k in ('X', 'Y', 'WIDTH', 'HEIGHT')]
    if width < 50 or height < 50:
        continue
    assert x >= 0 and y >= 0 and x + width <= screen_width and y + height <= screen_height, (title, geometry, size)
    call('xdotool', 'windowactivate', '--sync', window)
    time.sleep(0.2)
    name = re.sub(r'[^a-zA-Z0-9_-]+', '_', title).strip('_')[:90] or window
    capture = output / (name + '-' + window + '.xwd')
    subprocess.run(['xwd', '-silent', '-id', window, '-out', str(capture)], check=True)
    # xwd has a small, stable TrueColor header; convert through ImageMagick is unnecessary.
    import struct
    raw = capture.read_bytes()
    header = struct.unpack('>25I', raw[:100])
    header_size, pixmap_format, depth, iw, ih = [header[i] for i in (0, 2, 3, 4, 5)]
    byte_order, bits_per_pixel, stride, ncolors = header[7], header[11], header[12], header[19]
    assert pixmap_format == 2 and depth == 24 and bits_per_pixel == 32, header
    pixels = raw[header_size + ncolors * 12:]
    picture = Image.frombytes('RGB', (iw, ih), pixels, 'raw', 'BGRX' if byte_order == 0 else 'XRGB', stride)
    assert max(ImageStat.Stat(picture).stddev) > 2, 'Blank window: ' + title
    picture.save(capture.with_suffix('.png'))
    capture.unlink()
    seen.update(name for name in expected if name in title)
    records.append({'title': title, 'x': x, 'y': y, 'width': width, 'height': height,
                    'screenshot': capture.with_suffix('.png').name})
assert expected <= seen, 'Missing windows: ' + repr(sorted(expected - seen))
assert any('TextGrid' in r['title'] for r in records), 'Missing annotation editor'
assert len(records) >= 15, f'Too few mapped windows: {len(records)}'
(output / 'windows.json').write_text(json.dumps({'screen': size, 'windows': records}, ensure_ascii=False, indent=2), encoding='utf-8')
print(f'TABLET WINDOWS: PASS ({size}, {len(records)} mapped windows, bounds and nonblank captures)')

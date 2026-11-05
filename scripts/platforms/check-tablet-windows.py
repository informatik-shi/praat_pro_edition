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
    return subprocess.check_output(arguments, text=True, timeout=10).strip()

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
    info = call('xwininfo', '-id', window)
    x, y, width, height = [int(re.search(label + r':\s*(-?\d+)', info).group(1)) for label in
                           ('Absolute upper-left X', 'Absolute upper-left Y', 'Width', 'Height')]
    geometry = {'x': x, 'y': y, 'width': width, 'height': height}
    if width < 50 or height < 50:
        continue
    assert x >= 0 and y >= 0 and x + width <= screen_width and y + height <= screen_height, (title, geometry, size)
    call('xdotool', 'windowactivate', '--sync', window)
    call('xdotool', 'windowraise', window)
    time.sleep(0.5)
    name = re.sub(r'[^a-zA-Z0-9_-]+', '_', title).strip('_')[:90] or window
    capture = output / (name + '-' + window + '.png')
    # Capture the visible root with a standard X11 image decoder; client backing
    # pixels and raw XWD pixel layouts differ between X servers.
    subprocess.run(['scrot', '-o', str(capture)], check=True)
    with Image.open(capture) as root:
        picture = root.convert('RGB').crop((x, y, x + width, y + height))
    picture.save(capture)
    assert max(ImageStat.Stat(picture).stddev) > 2, 'Blank window: ' + title
    seen.update(name for name in expected if name in title)
    records.append({'title': title, 'x': x, 'y': y, 'width': width, 'height': height,
                    'screenshot': capture.name})
assert expected <= seen, 'Missing windows: ' + repr(sorted(expected - seen))
assert any('TextGrid' in r['title'] for r in records), 'Missing annotation editor'
assert len(records) >= 15, f'Too few mapped windows: {len(records)}'
(output / 'windows.json').write_text(json.dumps({'screen': size, 'windows': records}, ensure_ascii=False, indent=2), encoding='utf-8')
print(f'TABLET WINDOWS: PASS ({size}, {len(records)} mapped windows, bounds and nonblank captures)')

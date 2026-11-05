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
titles = {window: call('xdotool', 'getwindowname', window) for window in ids}
# A modeless dialog remains above other windows of its application in Openbox.
# Verify it first, then close it through the real WM_DELETE_WINDOW path.
ids.sort(key=lambda window: titles[window] != 'tablet_scroll_dialog')
records = []
expected = {'tablet_sound', 'tablet_spectrum', 'tablet_pitch', 'tablet_intensity',
            'tablet_formants', 'tablet_samples', 'tablet_smart', 'tablet_tracks',
            'tablet_lua_plot', 'tablet_lua_editor', 'tablet_praat_editor', 'tablet_scroll_dialog'}
seen = set()
def inspect(window):
    title = call('xdotool', 'getwindowname', window)
    if not title:
        return
    info = call('xwininfo', '-id', window)
    x, y, width, height = [int(re.search(label + r':\s*(-?\d+)', info).group(1)) for label in
                           ('Absolute upper-left X', 'Absolute upper-left Y', 'Width', 'Height')]
    geometry = {'x': x, 'y': y, 'width': width, 'height': height}
    if width < 50 or height < 50:
        return
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
    interior = picture.crop((5, 5, width - 5, height - 5))
    assert max(ImageStat.Stat(interior).stddev) > 2, 'Blank window: ' + title
    seen.update(name for name in expected if name in title)
    records.append({'title': title, 'x': x, 'y': y, 'width': width, 'height': height,
                    'screenshot': capture.name})
    (output / 'windows.json').write_text(json.dumps({'screen': size, 'windows': records}, ensure_ascii=False, indent=2), encoding='utf-8')

for window in ids:
    title = titles[window]
    inspect(window)
    if title == 'tablet_scroll_dialog':
        call('wmctrl', '-ic', hex(int(window)))
        time.sleep(0.3)
assert expected <= seen, 'Missing windows: ' + repr(sorted(expected - seen))
assert any('TextGrid' in r['title'] for r in records), 'Missing annotation editor'
assert len(records) >= 15, f'Too few mapped windows: {len(records)}'

# Exercise the real editor shortcuts and the production GTK file chooser.
lua = next(window for window in ids if titles[window] == 'tablet_lua_editor')
for key, label in [('ctrl+o', 'Open'), ('ctrl+s', 'Save')]:
    call('xdotool', 'windowactivate', '--sync', lua)
    call('xdotool', 'key', '--clearmodifiers', key)
    chooser = None
    for attempt in range(40):
        visible = call('xdotool', 'search', '--onlyvisible', '--pid', str(pid)).splitlines()
        candidates = [window for window in visible if window not in ids]
        for window in candidates:
            title = call('xdotool', 'getwindowname', window)
            if label.lower() in title.lower():
                chooser = window
                break
        if chooser:
            break
        time.sleep(0.1)
    assert chooser, label + ' file chooser did not open'
    inspect(chooser)
    call('xdotool', 'key', '--clearmodifiers', 'Escape')
    time.sleep(0.3)
    assert chooser not in call('xdotool', 'search', '--onlyvisible', '--pid', str(pid)).splitlines(), label + ' did not cancel'

(output / 'windows.json').write_text(json.dumps({'screen': size, 'windows': records}, ensure_ascii=False, indent=2), encoding='utf-8')
print(f'TABLET WINDOWS: PASS ({size}, {len(records)} mapped windows, bounds and nonblank captures)')

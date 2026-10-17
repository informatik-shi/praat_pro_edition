"""Execute textbook examples in isolated folders with the actual Praat build."""
from pathlib import Path
import csv
import json
import shutil
import struct
import subprocess
import sys
from html.parser import HTMLParser

ROOT = Path(__file__).resolve().parents[1]
BOOK = ROOT / 'docs/tutorials/lua-praat'
WORK = ROOT / '.local-build/lua-textbook'
EXE = ROOT / 'dist/praat-custom.exe'

class BookParser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.ids, self.links, self.codes = [], [], {}
        self.current = None
        self.external_assets = []

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if 'id' in a:
            self.ids.append(a['id'])
        if tag == 'a' and a.get('href', '').startswith('#'):
            self.links.append(a['href'][1:])
        if tag == 'code' and a.get('id', '').startswith('code-'):
            self.current = a['id']
            self.codes[self.current] = ''
        if tag in ('script', 'img', 'link') and ('src' in a or 'href' in a) and not a.get('src', '').startswith('data:image/png;base64,'):
            self.external_assets.append(a)

    def handle_endtag(self, tag):
        if tag == 'code':
            self.current = None

    def handle_data(self, data):
        if self.current:
            self.codes[self.current] += data

def main():
    WORK.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((BOOK / 'examples-manifest.json').read_text(encoding='utf-8'))
    parsers = []
    for lang in ('ru', 'en'):
        p = BookParser()
        p.feed((BOOK / f'lua-praat-{lang}.html').read_text(encoding='utf-8'))
        assert len(p.ids) == len(set(p.ids)), 'Duplicate HTML ids'
        assert set(p.links) <= set(p.ids), 'Broken internal links'
        assert not p.external_assets, 'Offline book has external assets'
        assert len(p.codes) == 29
        for lesson in manifest:
            source = (BOOK / 'examples' / lesson['filename']).read_text(encoding='utf-8')
            assert p.codes['code-' + lesson['slug']] == source
        parsers.append(p)
    assert parsers[0].codes == parsers[1].codes
    print('HTML: both editions, 29 identical runnable listings, anchors and offline assets PASS', flush=True)
    report, failed = [], []
    for lesson in manifest:
        folder = WORK / lesson['slug']
        folder.mkdir(exist_ok=True)
        script = folder / lesson['filename']
        shutil.copy2(BOOK / 'examples' / lesson['filename'], script)
        if lesson['slug'] == '23-batch':
            corpus = folder / 'corpus'
            corpus.mkdir(exist_ok=True)
            for name in ('lesson-tone.wav', 'lesson-part.wav'):
                shutil.copy2(WORK / '06-files' / name, corpus / name)
            (corpus / 'broken.wav').write_text('deliberately invalid audio', encoding='utf-8')
            (corpus / 'directory.wav').mkdir(exist_ok=True)
        if lesson['mode'] == 'gui':
            syntax = folder / 'syntax.lua'
            syntax.write_text('assert(loadfile("' + lesson['filename'] + '"))\nprint("GUI EXAMPLE SYNTAX PASS")\n', encoding='utf-8')
            script = syntax
        result = subprocess.run([str(EXE), '--utf8', '--no-pref-files', '--no-plugins', '--run', str(script)],
                                capture_output=True, timeout=90)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        (folder / 'execution.log').write_text(output, encoding='utf-8')
        passed = result.returncode == 0
        report.append(dict(lesson=lesson['slug'], mode=lesson['mode'], passed=passed, output=output))
        print(('PASS ' if passed else 'FAIL ') + lesson['slug'] + (' (syntax only; GUI required)' if lesson['mode']=='gui' else ''), flush=True)
        if not passed:
            failed.append(lesson['slug'])
            print(output, flush=True)
    if not failed:
        batch = list(csv.DictReader((WORK/'23-batch/lesson-batch.tsv').open(encoding='utf-8'), delimiter='\t'))
        assert len(batch) == 3
        assert next(x for x in batch if x['file']=='broken.wav')['status'] == 'ERROR'
        assert sum(x['status']=='OK' for x in batch) == 2
        project = list(csv.DictReader((WORK/'29-project/project-measurements.tsv').open(encoding='utf-8'), delimiter='\t'))
        assert len(project) == 3
        assert abs(sum(float(x['duration_s']) for x in project)-1) < 1e-8
        assert all(abs(float(x['median_F0_Hz'])-125)<3 for x in project)
        for filename in ('24-plots/lesson-waveform.png', '25-heatmap/lesson-spectrogram.png', '29-project/project-formants.png'):
            data = (WORK/filename).read_bytes()
            assert data[:8] == b'\x89PNG\r\n\x1a\n'
            assert struct.unpack('>II', data[16:24]) == (1200,600)
        print('OUTPUTS: TSV row counts, corrupt-file recovery, project F0/durations, PNG dimensions PASS', flush=True)
    (WORK / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if failed:
        raise SystemExit('Failed: ' + ', '.join(failed))
    print('28 batch examples PASS; 1 GUI example syntax PASS.', flush=True)

if __name__ == '__main__':
    main()

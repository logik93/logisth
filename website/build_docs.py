"""Build offline HTML editions from the two canonical English manuals using Pandoc."""
import html
import re
import shutil
import subprocess
from pathlib import Path
from urllib.parse import quote, urlsplit
from PIL import Image

root = Path(__file__).resolve().parents[1]
public = root / 'website/public'
version = re.search(r'project\(Logisth VERSION ([\d.]+)', (root / 'CMakeLists.txt').read_text())[1]
repository = 'https://github.com/logik93/logisth/blob/v' + version + '/'
manuals = {'MANUAL.md': ('docs', 'User manual', 'logisth-Manual-English.pdf'),
           'TECHNICAL_MANUAL.md': ('technical', 'Technical manual', 'logisth-Technical-Manual-English.pdf')}
asset_dir = public / 'assets/manual'
asset_dir.mkdir(parents=True, exist_ok=True)
for filename, (route, title, pdf) in manuals.items():
    source = root / 'docs' / filename
    text = source.read_text(encoding='utf-8')
    text = re.sub(r'^# [^\n]+\n', '', text, count=1)
    text = re.sub(r'^## Contents\n.*?(?=^## )', '', text, flags=re.S | re.M)
    body = subprocess.run(['pandoc', '-f', 'gfm', '-t', 'html5', '--wrap=none'], input=text,
                          capture_output=True, text=True, check=True).stdout

    def link(match):
        attribute, url = match.groups()
        url = html.unescape(url)
        parsed = urlsplit(url)
        if parsed.scheme or url.startswith('#'):
            return match.group()
        target = (source.parent / parsed.path).resolve()
        if not target.exists():
            raise ValueError('Missing manual link: ' + url)
        relative = target.relative_to(root)
        if attribute == 'src':
            name = target.name if target.suffix == '.svg' else target.stem + '.webp'
            if target.suffix == '.svg':
                shutil.copyfile(target, asset_dir / name)
            else:
                Image.open(target).save(asset_dir / name, quality=94, method=6)
            url = '../assets/manual/' + quote(name)
        elif target.parent == source.parent and target.name in manuals:
            url = '../' + manuals[target.name][0] + '/'
        else:
            url = repository + quote(relative.as_posix())
        if parsed.fragment:
            url += '#' + parsed.fragment
        return f'{attribute}="{html.escape(url, quote=True)}"'

    body = re.sub(r'(href|src)="([^"]+)"', link, body)
    toc = ''.join(f'<a href="#{key}">{heading}</a>' for key, heading in
                  re.findall(r'<h2 id="([^"]+)">(.+?)</h2>', body))
    body = body.replace('<table>', '<div class="table-wrap"><table>').replace('</table>', '</table></div>')
    body = body.replace('<img ', '<img loading="lazy" ')
    page = f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>{title} — logisth {version} | wavelogik</title><link rel="icon" href="../assets/icon.svg"><link rel="stylesheet" href="../site.css"></head>
<body class="reading"><a class="skip" href="#content">Skip to content</a><header class="top"><div class="wrap nav">
<a class="brand" href="../"><img src="../assets/logisth.svg" alt="">wavelogik</a><nav aria-label="Documentation"><a href="../docs/">User manual</a><a href="../technical/">Technical manual</a></nav><a class="get-small" href="{pdf}" download>PDF <span aria-hidden="true" class="arrow down"></span></a></div></header>
<main id="content"><div class="book-mast"><div class="wrap"><p class="eyebrow">LOGISTH / DOCUMENTATION</p><h1>{title}</h1><p class="formats">VERSION {version} · ENGLISH</p></div></div>
<div class="book wrap"><details class="contents" open><summary>Contents</summary><nav aria-label="Chapters">{toc}</nav></details><article>{body}</article></div></main>
<footer><div class="wrap foot"><a class="brand" href="../"><img src="../assets/logisth.svg" alt="">wavelogik</a><span>wavelogik.pl</span><a href="#content">Back to top</a></div></footer><script src="../site.js" defer></script></body></html>'''
    directory = public / route
    directory.mkdir(exist_ok=True)
    (directory / 'index.html').write_text(page, encoding='utf-8')
    shutil.copyfile(source.parent / pdf, directory / pdf)
    print(f'Built {title} {version} with {toc.count("<a ")} chapter links')

"""Bundle the Next.js export and PHP release service for Apache."""
from pathlib import Path
import re
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

website = Path(__file__).resolve().parent
export = website / "out"
archive = website / "wavelogik.pl.zip"
next_css = export / "_next/static/css"
webpack_media_url = re.compile(rb"""url\((['"]?)(?:\./|/)?_next/static/media/""")

manual_libraries = (
    ("marked/lib/marked.esm.js", "marked.js"),
    ("marked/LICENSE", "marked-LICENSE.md"),
    ("dompurify/dist/purify.es.mjs", "purify.js"),
    ("dompurify/LICENSE", "dompurify-LICENSE"),
)

required = ("index.html", "docs/index.html", "technical/index.html")
if any(not (export / path).is_file() for path in required):
    raise SystemExit("Run npm run build before packaging the website.")

temporary = archive.with_suffix(".tmp")
with ZipFile(temporary, "w", compression=ZIP_DEFLATED) as output:
    for folder in (export, website / "php"):
        for source in sorted(folder.rglob("*")):
            if not source.is_file():
                continue
            entry = ZipInfo(source.relative_to(folder).as_posix(), (2026, 1, 1, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = 0o100644 << 16
            content = source.read_bytes()
            if source.parent == next_css and source.suffix == ".css":
                # Webpack uses its page-relative asset prefix for font URLs as
                # well. CSS resolves those URLs relative to the stylesheet.
                content = webpack_media_url.sub(rb"url(\1../media/", content)
            output.writestr(entry, content, compress_type=ZIP_DEFLATED)
    for source, filename in manual_libraries:
        entry = ZipInfo("manual-vendor/" + filename, (2026, 1, 1, 0, 0, 0))
        entry.external_attr = 0o100644 << 16
        output.writestr(entry, (website / "node_modules" / source).read_bytes(), compress_type=ZIP_DEFLATED)
temporary.replace(archive)
print(archive)

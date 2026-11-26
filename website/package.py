"""Bundle the Next.js export and PHP release service for Apache."""
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

website = Path(__file__).resolve().parent
export = website / "out"
archive = website / "wavelogik.pl.zip"

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
            output.writestr(entry, source.read_bytes(), compress_type=ZIP_DEFLATED)
temporary.replace(archive)
print(archive)

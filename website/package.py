"""Package the web root for deployment on a static or PHP host."""
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

website = Path(__file__).resolve().parent
public = website / "public"
archive = website / "wavelogik.pl.zip"

with ZipFile(archive, "w", compression=ZIP_DEFLATED) as output:
    for source in sorted(public.rglob("*")):
        if source.is_file():
            entry = ZipInfo(source.relative_to(public).as_posix(), (2026, 1, 1, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = 0o100644 << 16
            output.writestr(entry, source.read_bytes(), compress_type=ZIP_DEFLATED)
print(archive)

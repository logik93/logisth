# wavelogik.pl

The logisth homepage uses Next.js, React, TypeScript, Tailwind CSS, Motion,
Three.js and Lucide. The static build ships with PHP for live GitHub downloads
and manuals. The two bundled HTML manuals remain available under `/docs/` and
`/technical/`.

## Deploy on Apache

Extract the entire [wavelogik.pl.zip](wavelogik.pl.zip) into the document
root or a subdirectory on the site. Keep `_next/`, `assets/` and `media/`
alongside `index.php` and `index.html`. Open the directory URL over HTTP or
HTTPS, with a trailing slash for a subdirectory such as `/logisth/`.
PHP 8.2 or newer must handle `.php` files and Apache must serve
`index.php` or `index.html` as a directory index. The ZIP includes both
entry points, `_next/`, `media/`, the release service and both manuals.
The production site runs on Apache and PHP without a Node.js process or a
database.

PHP uses cURL or `allow_url_fopen`, HTTPS support and outbound access to
`api.github.com`. You can set `WAVELOGIK_CACHE_DIR` to an absolute directory
outside the web root that is writable by PHP. By default PHP uses a
site-specific directory under its temporary directory.

The PHP service caches GitHub release metadata for five minutes. If PHP is
unavailable, the browser can request the public API directly. Bundled download
links point to verified platform packages for the last packaged public release.
Packages that GitHub has yet to publish appear as pending.

The manual opens in the site's reader with a locally bundled HTML edition.
When GitHub has a newer release, the reader can load its Markdown, render it
in the same layout and use the PDF from that release. The offline edition
includes a local PDF.

## Build and preview

Run these commands from `website/` with Node.js 22 or newer, PHP 8.2 or newer
and Python 3 available.

```sh
npm ci
npm run package
```

Extract the resulting ZIP to a separate directory and run
`php -S 127.0.0.1:8000 -t <directory>` to preview the exact Apache package.
`npm run dev` runs the frontend development server. Use the PHP preview to
verify live release links.

The Markdown sources for the manuals live in `docs/` at the repository root.
Run `python3 website/build_docs.py` from the root with Pillow and Pandoc
installed to update the bundled HTML editions after refreshing their PDFs.
Run `npm run package` again to include those files in the deployment ZIP.

# wavelogik.pl

The site introduces the logisth synthesizer and includes English user and
technical manuals in HTML and PDF. The home page gets the latest published
stable release from GitHub and shows download links as each platform archive
becomes available. The Documentation section uses the same release tag for
both manuals on GitHub and links to the English PDF assets once uploaded.
The bundled HTML manuals stay available as a fallback.

## Deploy on your own server

Download the [website ZIP](wavelogik.pl.zip) and extract its contents to the
document root for `wavelogik.pl`. The root must contain `index.php`,
`release.php`, `assets/`, `docs/`, `technical/`, `site.css` and `site.js`.
Configure Apache or Nginx to execute PHP 8.2 or later and to serve
`index.php` ahead of any old `index.html`. Keep `index.html` enabled for
the two manual directories.

PHP needs `allow_url_fopen`, HTTPS support and outbound access to
`api.github.com`. The home page caches the release for five minutes in a
site-specific directory under PHP's temporary directory. You can set
`WAVELOGIK_CACHE_DIR` to an absolute path writable by PHP outside the
document root. If GitHub is unavailable, the home page uses the last successful
response or links directly to the latest GitHub release. During staged
publication, any missing packages are marked as pending.

To preview locally, run `php -S 127.0.0.1:8000 -t website/public` from the
repository root. Check the home page, `docs/` and `technical/` after deploying.

## Update the documentation

The English Markdown sources and PDFs live in the repository's `docs/`
directory. After updating the PDFs, run the following commands from the
repository root. The HTML generator requires Python 3 with Pillow and Pandoc.

```sh
python3 website/build_docs.py
python3 website/package.py
```

The packaging script puts the contents of `website/public/` into
`website/wavelogik.pl.zip`. The download and GitHub documentation links refresh on the same five-minute
cache. Rebuild the website ZIP when deploying changes to the site itself or
refreshing its bundled HTML fallback.

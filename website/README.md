# wavelogik.pl

The site introduces the logisth synthesizer and includes its user and technical manuals in English, in both HTML and PDF. The package includes all fonts, images and manuals. The instrument download links point to the GitHub release.

## Deploy on your own server

Download the [website ZIP](wavelogik.pl.zip) and extract its contents to the document root for `wavelogik.pl`. Place `index.html` at the top level alongside `assets/`, `docs/`, `technical/`, `site.css` and `site.js`.

Serve the files directly from Apache or Nginx, including on a host that supports PHP. If the target directory already has an `index.php`, use a separate document root or configure your host to serve `index.html` first. Configure the domain and HTTPS certificate through your hosting setup.

Open the home page, then check `docs/` and `technical/`. Each manual links to its bundled PDF. The site uses relative paths, so you can also preview it in a subdirectory.

## Update the documentation

The English Markdown sources and PDFs live in the repository's `docs/` directory. After updating the PDFs, run these commands from the repository root. The HTML generator requires Python 3 with Pillow and Pandoc.

```sh
python website/build_docs.py
python website/package.py
```

The packaging script puts the contents of `website/public/` into `website/wavelogik.pl.zip`. When updating the instrument release, also update the version number and download links in `website/public/index.html`.

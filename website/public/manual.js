import { latestRelease, manualFiles } from "./release-client.js";

const chapters = document.querySelector(".chapters");
const desktop = matchMedia("(min-width: 54rem)");

function configureChapters() {
  if (!chapters) return () => {};
  const links = [...chapters.querySelectorAll("a")];
  const positions = links
    .map((link) => ({ link, heading: document.getElementById(decodeURIComponent(link.hash.slice(1))) }))
    .filter(({ heading }) => heading);
  const sizeContents = () => { chapters.open = desktop.matches; };
  sizeContents();
  desktop.addEventListener("change", sizeContents);

  let current;
  let frame = 0;
  function trackCurrentChapter() {
    frame = 0;
    const offset = document.querySelector(".manual-header").getBoundingClientRect().bottom + 28;
    let next;
    for (const { link, heading } of positions) {
      if (heading.getBoundingClientRect().top > offset) break;
      next = link;
    }
    if (next === current) return;
    current?.removeAttribute("aria-current");
    next?.setAttribute("aria-current", "location");
    current = next;
  }
  const update = () => {
    if (!frame) frame = requestAnimationFrame(trackCurrentChapter);
  };
  window.addEventListener("scroll", update, { passive: true });
  window.addEventListener("resize", update);
  window.addEventListener("hashchange", update);
  update();
  for (const { link, heading } of positions) {
    link.addEventListener("click", () => {
      if (!desktop.matches) chapters.open = false;
      heading.tabIndex = -1;
      heading.focus({ preventScroll: true });
    });
  }
  return () => {
    if (frame) cancelAnimationFrame(frame);
    desktop.removeEventListener("change", sizeContents);
    window.removeEventListener("scroll", update);
    window.removeEventListener("resize", update);
    window.removeEventListener("hashchange", update);
  };
}

let stopChapters = configureChapters();

async function refreshManual() {
  const { manual: key, version } = document.body.dataset;
  const files = manualFiles[key];
  if (!files) return;
  const release = await latestRelease("../releases.php");
  if (!release) return;
  const [filename] = files;
  const tag = "v" + release.version;
  const base = "https://raw.githubusercontent.com/logik93/logisth/" + tag + "/docs/";
  const source = "https://github.com/logik93/logisth/blob/" + tag + "/docs/";
  const manual = release.manuals[key];

  const current = release.version.split(".").map(Number);
  const local = version.split(".").map(Number);
  let order = 0;
  for (let part = 0; part < Math.max(current.length, local.length) && !order; ++part)
    order = (current[part] ?? 0) - (local[part] ?? 0);
  if (order <= 0) {
    if (order === 0 && manual.pdf) document.querySelector(".manual-action").href = manual.pdf;
    return;
  }
  const response = await fetch(base + filename, { signal: AbortSignal.timeout(4500) });
  if (!response.ok) return;
  const markdown = await response.text();
  if (markdown.length > 500_000 || !markdown.startsWith("# logisth ")) return;

  const [{ marked }, { default: DOMPurify }] = await Promise.all([
    import("./manual-vendor/marked.js"),
    import("./manual-vendor/purify.js"),
  ]);
  const fragment = DOMPurify.sanitize(marked.parse(markdown), {
    USE_PROFILES: { html: true },
    FORBID_TAGS: ["style", "iframe", "form"],
    FORBID_ATTR: ["style"],
    RETURN_DOM_FRAGMENT: true,
  });
  fragment.querySelector("h1")?.remove();
  for (const heading of fragment.querySelectorAll("h2")) {
    if (heading.textContent.trim() !== "Contents") continue;
    while (heading.nextElementSibling && heading.nextElementSibling.tagName !== "H2")
      heading.nextElementSibling.remove();
    heading.remove();
    break;
  }
  if (!fragment.querySelector("h2")) return;

  const toc = document.createDocumentFragment();
  const ids = new Set();
  for (const heading of fragment.querySelectorAll("h2, h3, h4, h5, h6")) {
    const label = heading.textContent.trim();
    const stem = label.toLowerCase().replace(/[^\p{Letter}\p{Number}_ -]/gu, "")
      .replace(/ +/g, "-") || "section";
    let id = stem;
    for (let index = 1; ids.has(id); ++index) id = stem + "-" + index;
    ids.add(id);
    heading.id = id;
    if (heading.tagName !== "H2") continue;
    const link = document.createElement("a");
    link.href = "#" + id;
    link.textContent = label;
    toc.append(link);
  }

  for (const link of fragment.querySelectorAll("a[href]")) {
    const href = link.getAttribute("href");
    if (href.startsWith("#")) continue;
    const url = new URL(href, source);
    if (url.protocol !== "https:" && url.protocol !== "http:" && url.protocol !== "mailto:") {
      link.removeAttribute("href");
      continue;
    }
    const companion = Object.values(manualFiles).find(([name]) => url.href.startsWith(source + name) &&
      url.pathname.endsWith("/docs/" + name));
    link.href = companion ? "../" + companion[2] + url.hash : url.href;
  }
  for (const image of fragment.querySelectorAll("img[src]")) {
    const url = new URL(image.getAttribute("src"), base);
    if (url.protocol !== "https:") {
      image.remove();
      continue;
    }
    image.src = url.href;
    image.loading = "lazy";
  }
  for (const table of fragment.querySelectorAll("table")) {
    const wrap = document.createElement("div");
    wrap.className = "table-wrap";
    table.before(wrap);
    wrap.append(table);
  }
  const currentHash = decodeURIComponent(location.hash.slice(1));
  stopChapters();
  document.querySelector(".manual-body").replaceChildren(fragment);
  chapters.querySelector("nav").replaceChildren(toc);
  document.querySelector(".manual-version").textContent = "logisth " + tag;
  document.querySelector(".manual-source").href = manual.online;
  if (manual.pdf) document.querySelector(".manual-action").href = manual.pdf;
  document.title = document.querySelector("h1").textContent + " · logisth " + release.version;
  stopChapters = configureChapters();
  if (currentHash) document.getElementById(currentHash)?.scrollIntoView();
}

void refreshManual().catch(() => { /* Keep the offline edition if GitHub is unavailable. */ });

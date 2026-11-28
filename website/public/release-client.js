const repository = "https://github.com/logik93/logisth";
const releaseRoot = repository + "/releases";
const api = "https://api.github.com/repos/logik93/logisth/releases/latest";
const tagPattern = /^v\d+(?:\.\d+){2,3}$/;

const packages = {
  Windows: ["windows-x86_64.zip", "x64 · ZIP"],
  macOS: ["macos-universal.zip", "Universal · ZIP"],
  Linux: ["linux-x86_64.tar.gz", "x86-64 · TAR.GZ"],
};
export const manualFiles = {
  user: ["MANUAL.md", "logisth-Manual-English.pdf", "docs/"],
  technical: ["TECHNICAL_MANUAL.md", "logisth-Technical-Manual-English.pdf", "technical/"],
};

function releaseWithAssets(tag, uploaded) {
  const version = tag.slice(1);
  const base = releaseRoot + "/download/" + tag + "/";
  const asset = (name) => uploaded.get(name) === base + name ? base + name : null;
  return {
    version,
    url: releaseRoot + "/tag/" + tag,
    downloads: Object.fromEntries(Object.entries(packages).map(([platform, [suffix, format]]) =>
      [platform, { format, url: asset("logisth-" + version + "-" + suffix) }]
    )),
    manuals: Object.fromEntries(Object.entries(manualFiles).map(([key, [markdown, pdf]]) =>
      [key, { online: repository + "/blob/" + tag + "/docs/" + markdown, pdf: asset(pdf) }]
    )),
  };
}

// Shipped links for a release whose three archives and PDFs are already public.
const fallbackTag = "v1.0.10";
const fallbackNames = [
  ...Object.values(packages).map(([suffix]) => "logisth-" + fallbackTag.slice(1) + "-" + suffix),
  ...Object.values(manualFiles).map(([, pdf]) => pdf),
];
export const bundledRelease = releaseWithAssets(fallbackTag, new Map(fallbackNames.map((name) =>
  [name, releaseRoot + "/download/" + fallbackTag + "/" + name]
)));

function parseGitHub(data) {
  if (data?.draft !== false || data.prerelease !== false ||
      !tagPattern.test(data.tag_name) || !Array.isArray(data.assets)) return null;
  const uploaded = new Map(data.assets
    .filter((item) => item?.state === "uploaded")
    .map(({ name, browser_download_url }) => [name, browser_download_url]));
  return releaseWithAssets(data.tag_name, uploaded);
}

export function compareVersions(left, right) {
  const first = left.split(".").map(Number);
  const second = right.split(".").map(Number);
  for (let part = 0; part < Math.max(first.length, second.length); ++part) {
    const difference = (first[part] ?? 0) - (second[part] ?? 0);
    if (difference) return difference;
  }
  return 0;
}

async function json(url, signal) {
  const timeout = AbortSignal.timeout(4500);
  const response = await fetch(url, {
    signal: signal ? AbortSignal.any([signal, timeout]) : timeout,
    headers: { Accept: "application/json" },
  });
  if (!response.ok || !response.headers.get("content-type")?.includes("application/json"))
    return null;
  return response.json();
}

export async function latestRelease(localUrl, signal) {
  let cached = null;
  try {
    const local = await json(localUrl, signal);
    if (tagPattern.test("v" + local?.version) && local.downloads && local.manuals) {
      if (!local.stale) return local;
      cached = local;
    }
  } catch {
    if (signal?.aborted) return null;
  }

  try {
    const live = parseGitHub(await json(api, signal));
    return live && (!cached || compareVersions(live.version, cached.version) >= 0) ? live : cached;
  } catch {
    return signal?.aborted ? null : cached;
  }
}

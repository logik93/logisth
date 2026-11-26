import type { MetadataRoute } from "next";

export const dynamic = "force-static";

export default function sitemap(): MetadataRoute.Sitemap {
  return ["/", "/docs/", "/technical/"].map((path) => ({
    url: new URL(path, "https://wavelogik.pl").href,
  }));
}

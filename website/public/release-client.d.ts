type Package = { format: string; url: string | null };
type Manual = { online: string; pdf: string | null };
type Release = {
  version: string;
  url: string;
  downloads: Record<"Windows" | "macOS" | "Linux", Package>;
  manuals: Record<"user" | "technical", Manual>;
};
export const bundledRelease: Release;
export function compareVersions(left: string, right: string): number;
export function latestRelease(localUrl: string, signal?: AbortSignal): Promise<Release | null>;

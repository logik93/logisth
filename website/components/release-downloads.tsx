"use client";

import { BookOpen, Code2, Download, FileText, GitFork, Laptop, Monitor, Terminal } from "lucide-react";
import { useEffect, useState } from "react";
import { latest } from "../lib/project";

type Package = { format: string; url: string | null };
type Manual = { online: string; pdf: string | null };
type Release = {
  version: string;
  url: string;
  downloads: Record<"Windows" | "macOS" | "Linux", Package>;
  manuals: Record<"user" | "technical", Manual>;
};

const platforms = [
  { name: "Windows", format: "x64 · ZIP", Icon: Monitor },
  { name: "macOS", format: "Universal · ZIP", Icon: Laptop },
  { name: "Linux", format: "x86-64 · TAR.GZ", Icon: Terminal },
] as const;

const manuals = [
  { id: "user", label: "User manual", description: "Controls, workflows and presets", local: "/docs/", Icon: BookOpen },
  { id: "technical", label: "Technical manual", description: "Architecture, DSP and measurements", local: "/technical/", Icon: Code2 },
] as const;

export function ReleaseDownloads() {
  const [release, setRelease] = useState<Release | null>(null);

  useEffect(() => {
    const controller = new AbortController();
    void fetch("/releases.php", { signal: controller.signal, headers: { Accept: "application/json" } })
      .then((response) => response.ok && response.headers.get("content-type")?.includes("application/json") ? response.json() : null)
      .then((data: Release | null) => {
        if (data?.version && data.downloads && data.manuals) setRelease(data);
      })
      .catch(() => { /* The initial GitHub and bundled manual links stay available. */ });
    return () => controller.abort();
  }, []);

  return (
    <>
      <section className="shell documentation" id="docs" data-arrival>
        <div>
          <p className="eyebrow">Documentation</p>
          <h2>Inside logisth.</h2>
          <p className="section-text">Find your way around the instrument or go deeper into its signal processing.</p>
        </div>
        <div className="manual-stack">
          {manuals.map(({ id, label, description, local, Icon }) => {
            const manual = release?.manuals[id];
            return (
              <article className="manual surface" key={id}>
                <Icon size={24} />
                <div>
                  <a className="manual-title" href={manual?.online ?? local}>{label}</a>
                  <p>{description}</p>
                </div>
                <div className="manual-links">
                  {manual?.pdf && <a title="Download PDF" aria-label={label + " PDF"} href={manual.pdf}><FileText size={17} /> PDF</a>}
                  {manual && <a title="Bundled HTML edition" aria-label={label + " bundled HTML"} href={local}>HTML</a>}
                </div>
              </article>
            );
          })}
        </div>
      </section>
      <section className="shell download-section" id="download" data-arrival>
        <div className="download-box surface">
          <div className="download-heading">
            <p className="eyebrow"><span className="signal-dot" /> {release ? "Release v" + release.version : "Latest stable release"}</p>
            <h2>Download logisth.</h2>
            <p>VST3, CLAP and Standalone are included in each platform package.</p>
          </div>
          <div className="platforms">
            {platforms.map(({ name, format, Icon }) => {
              const item = release?.downloads[name];
              if (release && !item?.url) {
                return (
                  <div className="platform is-pending" key={name}>
                    <Icon size={22} />
                    <span><strong>{name}</strong><small>Package pending</small></span>
                  </div>
                );
              }
              return (
                <a className="platform" key={name} href={item?.url ?? latest} aria-label={item?.url ? "Download logisth for " + name : "Find the " + name + " download on GitHub"}>
                  <Icon size={22} />
                  <span><strong>{name}</strong><small>{item?.format ?? format}</small></span>
                  <Download size={18} className="platform-download" />
                </a>
              );
            })}
          </div>
          <a className="text-link release-notes" href={release?.url ?? latest}><GitFork size={17} /> Release notes on GitHub</a>
        </div>
      </section>
    </>
  );
}

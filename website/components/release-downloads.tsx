"use client";

import { BookOpen, Code2, Download, FileText, GitFork, Laptop, Monitor, Terminal } from "lucide-react";
import { useEffect, useState } from "react";
import { bundledRelease, latestRelease } from "../public/release-client.js";

const platforms = [
  { name: "Windows", format: "x64 · ZIP", Icon: Monitor },
  { name: "macOS", format: "Universal · ZIP", Icon: Laptop },
  { name: "Linux", format: "x86-64 · TAR.GZ", Icon: Terminal },
] as const;

const manuals = [
  { id: "user", label: "User manual", description: "Controls, workflows and presets", local: "./docs/", pdf: "./docs/logisth-Manual-English.pdf", Icon: BookOpen },
  { id: "technical", label: "Technical manual", description: "Architecture, DSP and measurements", local: "./technical/", pdf: "./technical/logisth-Technical-Manual-English.pdf", Icon: Code2 },
] as const;

export function ReleaseDownloads() {
  const [release, setRelease] = useState(bundledRelease);

  useEffect(() => {
    const controller = new AbortController();
    void latestRelease("./releases.php", controller.signal).then((current) => {
      if (current && !controller.signal.aborted) setRelease(current);
    });
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
          {manuals.map(({ id, label, description, local, pdf, Icon }) => {
            const manual = release.manuals[id];
            return (
              <article className="manual surface" key={id}>
                <Icon size={24} />
                <div>
                  <a className="manual-title" href={local}>{label}</a>
                  <p>{description}</p>
                </div>
                <div className="manual-links">
                  <a aria-label={label + " PDF"} href={manual?.pdf ?? pdf}><FileText size={17} /> PDF</a>
                  {manual && <a aria-label={label + " on GitHub"} href={manual.online}>GitHub</a>}
                </div>
              </article>
            );
          })}
        </div>
      </section>
      <section className="shell download-section" id="download" data-arrival>
        <div className="download-box surface">
          <div className="download-heading">
            <p className="eyebrow"><span className="signal-dot" /> {"Release v" + release.version}</p>
            <h2>Download logisth.</h2>
            <p>VST3, CLAP and Standalone are included in each platform package.</p>
          </div>
          <div className="platforms">
            {platforms.map(({ name, format, Icon }) => {
              const item = release.downloads[name];
              if (!item.url) {
                return (
                  <div className="platform is-pending" key={name}>
                    <Icon size={22} />
                    <span><strong>{name}</strong><small>Package pending</small></span>
                  </div>
                );
              }
              return (
                <a className="platform" key={name} href={item.url} aria-label={"Download logisth for " + name}>
                  <Icon size={22} />
                  <span><strong>{name}</strong><small>{item.format ?? format}</small></span>
                  <Download size={18} className="platform-download" />
                </a>
              );
            })}
          </div>
          <a className="text-link release-notes" href={release.url}><GitFork size={17} /> Release notes on GitHub</a>
        </div>
      </section>
    </>
  );
}

import type { Metadata, Viewport } from "next";
import "@fontsource-variable/geist";
import "@fontsource/ibm-plex-mono/latin-400.css";
import "./globals.css";

const description = "logisth is an open-source polyphonic synthesizer with classic, wavetable, sampler and granular engines. Available as VST3, CLAP and Standalone.";

export const metadata: Metadata = {
  metadataBase: new URL("https://wavelogik.pl"),
  title: "logisth — Polyphonic synthesizer by wavelogik",
  description,
  alternates: { canonical: "/" },
  icons: { icon: "./assets/icon.svg" },
  openGraph: {
    type: "website",
    title: "logisth — Polyphonic synthesizer",
    description,
    url: "/",
    siteName: "wavelogik",
    images: ["/media/phase-loop.webp"],
  },
  twitter: { card: "summary_large_image" },
};

export const viewport: Viewport = { themeColor: "#0a0910" };

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}

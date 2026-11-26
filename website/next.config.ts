import type { NextConfig } from "next";

const config: NextConfig = {
  output: "export",
  // Keep scripts and lazy chunks next to the page on shared Apache hosting.
  assetPrefix: ".",
  trailingSlash: true,
  images: { unoptimized: true },
};

export default config;

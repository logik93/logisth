"use client";

import { Download, Pause, Play } from "lucide-react";
import { useEffect, useRef, useState } from "react";
import { MagneticLink } from "./ui";

export function Hero() {
  const art = useRef<HTMLDivElement>(null);
  const canvas = useRef<HTMLCanvasElement>(null);
  const coordinates = useRef({ x: 0, y: 0 });
  const pausedRef = useRef(false);
  const refreshMotion = useRef<() => void>(null);
  const [paused, setPaused] = useState(false);

  function toggleMotion() {
    pausedRef.current = !pausedRef.current;
    setPaused(pausedRef.current);
    refreshMotion.current?.();
  }

  useEffect(() => {
    const surface = canvas.current;
    const artwork = art.current;
    if (!surface || !artwork) return;
    const preference = matchMedia("(prefers-reduced-motion: reduce)");
    let scene: ReturnType<typeof import("./wave-scene").createWaveScene> = null;
    let loading = false;
    let attempted = false;
    let disposed = false;
    let inView = false;
    let frame = 0;
    let lastFrame = 0;

    const canAnimate = () => !pausedRef.current && !preference.matches && !document.hidden && inView;
    function tick(now: number) {
      frame = 0;
      if (!canAnimate() || !scene) return;
      if (now - lastFrame >= 30) {
        const { x, y } = coordinates.current;
        scene.render(now * 0.001, x, y);
        lastFrame = now;
      }
      frame = requestAnimationFrame(tick);
    }
    function update() {
      if (!canAnimate()) {
        cancelAnimationFrame(frame);
        frame = 0;
        artwork?.style.removeProperty("--parallax-x");
        artwork?.style.removeProperty("--parallax-y");
        return;
      }
      if (scene) {
        if (!frame) frame = requestAnimationFrame(tick);
        return;
      }
      if (loading || attempted) return;
      loading = true;
      void import("./wave-scene").then(({ createWaveScene }) => {
        loading = false;
        if (disposed || !canAnimate()) return;
        attempted = true;
        scene = createWaveScene(surface!);
        if (scene) frame = requestAnimationFrame(tick);
      }).catch(() => { loading = false; attempted = true; });
    }
    refreshMotion.current = update;
    const observer = new IntersectionObserver(([entry]) => {
      inView = entry.isIntersecting;
      update();
    });
    observer.observe(surface);
    preference.addEventListener("change", update);
    document.addEventListener("visibilitychange", update);
    return () => {
      disposed = true;
      refreshMotion.current = null;
      observer.disconnect();
      document.removeEventListener("visibilitychange", update);
      preference.removeEventListener("change", update);
      cancelAnimationFrame(frame);
      scene?.dispose();
    };
  }, []);

  return (
    <section
      className="hero"
      data-motion={paused ? "paused" : "playing"}
      onPointerMove={(event) => {
        if (paused || event.pointerType !== "mouse" || matchMedia("(prefers-reduced-motion: reduce)").matches) return;
        const rect = event.currentTarget.getBoundingClientRect();
        const x = (event.clientX - rect.left) / rect.width - 0.5;
        const y = (event.clientY - rect.top) / rect.height - 0.5;
        coordinates.current = { x, y };
        art.current?.style.setProperty("--parallax-x", x * 20 + "px");
        art.current?.style.setProperty("--parallax-y", y * 16 + "px");
      }}
      onPointerLeave={() => {
        coordinates.current = { x: 0, y: 0 };
        art.current?.style.removeProperty("--parallax-x");
        art.current?.style.removeProperty("--parallax-y");
      }}
    >
      <div className="hero-stage shell">
        <div className="hero-art" ref={art} aria-hidden="true">
          <picture>
            <source media="(max-width: 640px)" srcSet="./media/phase-loop-small.webp" />
            <img src="./media/phase-loop.webp" alt="" width="1320" height="1056" fetchPriority="high" />
          </picture>
          <canvas ref={canvas} className="wave-canvas" />
        </div>
        <div className="hero-copy">
          <p className="eyebrow"><span className="signal-dot" /> Open-source synthesizer</p>
          <h1 aria-label="logisth — Sound in motion">Sound in<br /><em>motion.</em></h1>
          <p className="hero-lede"><strong>logisth</strong> brings four synthesis engines into one polyphonic instrument. Build patches from waves, samples and grains.</p>
          <div className="hero-actions">
            <MagneticLink href="#download"><Download size={18} /> Download logisth</MagneticLink>
            <a className="text-link" href="#instrument">Explore the instrument</a>
          </div>
        </div>
        <div className="hero-index">
          <span>VST3 <b>·</b> CLAP <b>·</b> Standalone</span>
          <button
            type="button"
            className="motion-switch"
            onClick={toggleMotion}
            aria-label={paused ? "Resume hero animation" : "Pause hero animation"}
            title={paused ? "Resume animation" : "Pause animation"}
          >
            {paused ? <Play size={15} /> : <Pause size={15} />}
          </button>
        </div>
      </div>
    </section>
  );
}

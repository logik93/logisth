"use client";

import { motion, useMotionValue, useReducedMotion, useSpring } from "motion/react";
import { type MouseEvent, type ReactNode, useEffect, useRef, useState } from "react";

export function MagneticLink({ href, children }: { href: string; children: ReactNode }) {
  const reduce = useReducedMotion();
  const x = useMotionValue(0);
  const y = useMotionValue(0);
  const springX = useSpring(x, { stiffness: 300, damping: 26 });
  const springY = useSpring(y, { stiffness: 300, damping: 26 });

  function move(event: MouseEvent<HTMLSpanElement>) {
    if (reduce || !matchMedia("(pointer: fine)").matches) return;
    const rect = event.currentTarget.getBoundingClientRect();
    x.set((event.clientX - rect.left - rect.width / 2) * 0.13);
    y.set((event.clientY - rect.top - rect.height / 2) * 0.22);
  }

  return (
    <span className="magnet" onMouseMove={move} onMouseLeave={() => { x.set(0); y.set(0); }}>
      <motion.a className="pill primary" href={href} style={{ x: springX, y: springY }}>
        {children}
      </motion.a>
    </span>
  );
}

export function SectionMotion() {
  useEffect(() => {
    const preference = matchMedia("(prefers-reduced-motion: reduce)");
    if (preference.matches) return;
    const observer = new IntersectionObserver((entries) => {
      for (const entry of entries) {
        if (entry.isIntersecting) {
          entry.target.classList.add("has-arrived");
          observer.unobserve(entry.target);
        }
      }
    }, { threshold: 0.2 });
    document.querySelectorAll("[data-arrival]").forEach((element) => observer.observe(element));
    const stop = () => { if (preference.matches) observer.disconnect(); };
    preference.addEventListener("change", stop);
    return () => {
      observer.disconnect();
      preference.removeEventListener("change", stop);
    };
  }, []);
  return null;
}

export function Counter({ value, label }: { value: number; label: string }) {
  const ref = useRef<HTMLSpanElement>(null);
  const [display, setDisplay] = useState(value);

  useEffect(() => {
    const element = ref.current;
    const preference = matchMedia("(prefers-reduced-motion: reduce)");
    if (!element || preference.matches) return;
    let frame = 0;
    const observer = new IntersectionObserver(([entry]) => {
      if (!entry.isIntersecting) return;
      observer.disconnect();
      const started = performance.now();
      function tick(now: number) {
        const elapsed = Math.min((now - started) / 720, 1);
        setDisplay(Math.round(value * (1 - (1 - elapsed) ** 3)));
        if (elapsed < 1) frame = requestAnimationFrame(tick);
      }
      frame = requestAnimationFrame(tick);
    }, { threshold: 0.5 });
    observer.observe(element);
    const stop = () => {
      if (preference.matches) {
        observer.disconnect();
        cancelAnimationFrame(frame);
        setDisplay(value);
      }
    };
    preference.addEventListener("change", stop);
    return () => {
      observer.disconnect();
      cancelAnimationFrame(frame);
      preference.removeEventListener("change", stop);
    };
  }, [value]);

  return (
    <div className="stat-cell">
      <span ref={ref} className="stat-value" aria-hidden="true">{display}</span>
      <span className="sr-only">{value} </span>
      <span className="stat-label">{label}</span>
    </div>
  );
}

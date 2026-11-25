const selector = document.querySelector(".engine-tabs");
if (selector) {
  const tabs = [...selector.querySelectorAll("[data-engine]")];
  selector.setAttribute("role", "tablist");
  function select(id) {
    for (const tab of tabs) {
      const panel = document.getElementById(tab.dataset.engine);
      const active = panel.id === id;
      tab.setAttribute("aria-selected", String(active));
      tab.tabIndex = active ? 0 : -1;
      panel.hidden = !active;
    }
  }
  tabs.forEach((tab, i) => {
    const id = tab.dataset.engine;
    tab.id = "tab-" + id;
    tab.setAttribute("role", "tab");
    tab.setAttribute("aria-controls", id);
    const panel = document.getElementById(id);
    panel.setAttribute("role", "tabpanel");
    panel.setAttribute("aria-labelledby", tab.id);
    tab.addEventListener("click", (event) => {
      event.preventDefault();
      select(id);
    });
    tab.addEventListener("keydown", (event) => {
      const next = {
        ArrowRight: (i + 1) % tabs.length,
        ArrowLeft: (i + tabs.length - 1) % tabs.length,
        Home: 0,
        End: tabs.length - 1,
      }[event.key];
      if (next === undefined) return;
      event.preventDefault();
      select(tabs[next].dataset.engine);
      tabs[next].focus();
    });
  });
  select(
    tabs.some((tab) => "#" + tab.dataset.engine === location.hash)
      ? location.hash.slice(1)
      : "classic",
  );
}
const contents = document.querySelector(".contents");
if (contents) {
  const wide = matchMedia("(min-width: 851px)");
  function layout() {
    contents.open = wide.matches;
  }
  layout();
  wide.addEventListener("change", layout);
  contents.addEventListener("click", (event) => {
    if (!wide.matches && event.target.closest("a")) contents.open = false;
  });
}

// These three decorative traces illustrate modulation, band splitting and impulse decay.
document.querySelectorAll(".plot").forEach((surface, index) => {
  const canvas = document.createElement("canvas");
  surface.append(canvas);
  function paint() {
    const w = surface.clientWidth,
      h = surface.clientHeight;
    const scale = Math.min(2, window.devicePixelRatio || 1);
    canvas.width = Math.round(w * scale);
    canvas.height = Math.round(h * scale);
    const g = canvas.getContext("2d");
    if (!g) return;
    g.scale(scale, scale);
    for (let x = 0; x < w; x += 24) {
      g.strokeStyle = "#262330";
      g.beginPath();
      g.moveTo(x, 4);
      g.lineTo(x, h - 4);
      g.stroke();
    }
    ["#8062cf", "#b69bfc", "#81dae6"].forEach((color, n) => {
      g.beginPath();
      g.strokeStyle = color;
      g.lineWidth = 1.3;
      for (let x = 0; x < w; x++) {
        const t = x / w;
        const amp =
          index === 0
            ? Math.sin(t * 10 + n * 0.46) * 0.65
            : index === 1
              ? 0.84 - 1.6 * Math.exp(-Math.pow((t - (n + 0.5) / 3) * 4, 4))
              : Math.sin(108 * t + n * 0.47) *
                Math.cos(t * 22) *
                Math.exp(-4 * t);
        const y = h * (0.5 + 0.46 * amp);
        if (x === 0) g.moveTo(x, y);
        else g.lineTo(x, y);
      }
      g.stroke();
    });
  }
  paint();
  if ("ResizeObserver" in window) new ResizeObserver(paint).observe(surface);
});

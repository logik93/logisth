const chapters = document.querySelector(".chapters");
if (chapters) {
  const desktop = matchMedia("(min-width: 54rem)");
  const links = [...chapters.querySelectorAll("a")];
  const positions = links
    .map((link) => ({ link, heading: document.getElementById(decodeURIComponent(link.hash.slice(1))) }))
    .filter(({ heading }) => heading);

  const sizeContents = () => { chapters.open = desktop.matches; };
  sizeContents();
  desktop.addEventListener("change", sizeContents);

  let current;
  let frame = 0;
  function trackCurrentChapter() {
    frame = 0;
    const offset = document.querySelector(".manual-header").getBoundingClientRect().bottom + 28;
    let next;
    for (const { link, heading } of positions) {
      if (heading.getBoundingClientRect().top > offset) break;
      next = link;
    }
    if (next === current) return;
    current?.removeAttribute("aria-current");
    next?.setAttribute("aria-current", "location");
    current = next;
  }
  const update = () => {
    if (!frame) frame = requestAnimationFrame(trackCurrentChapter);
  };
  window.addEventListener("scroll", update, { passive: true });
  window.addEventListener("resize", update);
  window.addEventListener("hashchange", update);
  update();

  for (const { link, heading } of positions) {
    link.addEventListener("click", () => {
      if (!desktop.matches) chapters.open = false;
      heading.tabIndex = -1;
      heading.focus({ preventScroll: true });
    });
  }
}

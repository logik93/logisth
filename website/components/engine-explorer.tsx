"use client";

import { Expand } from "lucide-react";
import { useRef, useState } from "react";
import { engines } from "../lib/project";

export function EngineExplorer() {
  const [active, select] = useState(0);
  const buttons = useRef<(HTMLButtonElement | null)[]>([]);
  const engine = engines[active];

  return (
    <div className="explorer">
      <div className="engine-tabs" role="tablist" aria-label="Synthesis engines">
        {engines.map(({ id, name }, index) => (
          <button
            key={id}
            ref={(node) => { buttons.current[index] = node; }}
            type="button"
            role="tab"
            id={"engine-tab-" + id}
            aria-controls={"engine-panel-" + id}
            aria-selected={index === active}
            tabIndex={index === active ? 0 : -1}
            onClick={() => select(index)}
            onKeyDown={(event) => {
              const destinations: Record<string, number> = {
                ArrowRight: (index + 1) % engines.length,
                ArrowLeft: (index + engines.length - 1) % engines.length,
                Home: 0,
                End: engines.length - 1,
              };
              const target = destinations[event.key];
              if (target === undefined) return;
              event.preventDefault();
              select(target);
              buttons.current[target]?.focus();
            }}
          >{name}</button>
        ))}
      </div>
      {engines.map(({ id }, index) => (
        <div
          key={id}
          role="tabpanel"
          id={"engine-panel-" + id}
          aria-labelledby={"engine-tab-" + id}
          hidden={index !== active}
          tabIndex={0}
        >
          {index === active && (
            <div className="engine-panel">
              <div className="engine-visual">
                <img src={engine.image} alt={engine.alt} width="1240" height="860" loading="lazy" decoding="async" />
                <a className="expand-image" href={engine.image} target="_blank" rel="noreferrer" aria-label={"Open " + engine.name + " screenshot at full size"} title="Full-size screenshot">
                  <Expand size={17} />
                </a>
              </div>
              <div className="engine-note">
                <span className="eyebrow">{engine.name}</span>
                <h3>{engine.heading}</h3>
                <p>{engine.description}</p>
                <p className="engine-details">{engine.detail}</p>
              </div>
            </div>
          )}
        </div>
      ))}
      <noscript><p className="engine-nojs">See all engines in the <a className="text-link" href="./docs/">user manual</a>.</p></noscript>
    </div>
  );
}

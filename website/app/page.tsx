import {
  AudioWaveform, Boxes, CircleHelp, Download, GitFork, Plus, Radio, SlidersHorizontal, Waves,
} from "lucide-react";
import { EngineExplorer } from "../components/engine-explorer";
import { Hero } from "../components/hero";
import { ReleaseDownloads } from "../components/release-downloads";
import { Counter, SectionMotion } from "../components/ui";
import { latest, repository } from "../lib/project";

const questions = [
  {
    question: "Which formats and platforms are supported?",
    answer: "Each native package includes VST3, CLAP and Standalone. Builds are available for Windows x86-64, Linux x86-64 and macOS Universal for Intel and Apple Silicon.",
  },
  {
    question: "Can I use different engines in one patch?",
    answer: "Yes. Each of the six oscillators can use its own engine. They are paired across three parts with independent filters and envelopes.",
  },
  {
    question: "Does the standalone app support ASIO?",
    answer: "Yes, on Windows. In Standalone, open Audio/MIDI Settings and select ASIO as the audio device type. Then choose your installed interface driver.",
  },
  {
    question: "How do I set up Master Pitch in FL Studio?",
    answer: "Enable main pitch on the channel, set RANGE to 12 and enable “Send pitch bend range” at 12 semitones in the wrapper’s MIDI settings. The plug-in’s BEND / st control sets the maximum applied shift and defaults to 12.",
  },
  {
    question: "Where are the source and factory presets?",
    answer: "The source is on GitHub. The release includes a separate factory preset archive with 48 patches.",
    link: "Browse the release assets",
  },
];

function Brand() {
  return (
    <a className="brand" href="./" aria-label="logisth home">
      <img src="./assets/icon.svg" alt="" width="40" height="40" />
      <span>logisth</span>
    </a>
  );
}

export default function Home() {
  return (
    <>
      <a className="skip-link" href="#main">Skip to content</a>
      <header className="site-header">
        <div className="header-inner shell">
          <Brand />
          <nav aria-label="Main navigation">
            <a href="#instrument">Instrument</a>
            <a href="#sound-design">Sound design</a>
            <a href="#docs">Manuals</a>
          </nav>
          <a className="header-download pill" href="#download"><Download size={15} /> Download</a>
        </div>
      </header>
      <main id="main">
        <Hero />
        <section className="stats shell" aria-label="Instrument specifications">
          <Counter value={6} label="Oscillators" />
          <Counter value={4} label="Synthesis engines" />
          <Counter value={3} label="Parts per patch" />
          <Counter value={32} label="Max polyphony (notes)" />
        </section>
        <section className="shell section" id="instrument" data-arrival>
          <div className="section-intro">
            <div>
              <p className="eyebrow">The instrument</p>
              <h2>Four ways to<br />start a sound.</h2>
            </div>
            <p>Layer a wavetable with a field recording, or send a pair of classic oscillators through their own filter. Each oscillator can run a different engine.</p>
          </div>
          <EngineExplorer />
        </section>
        <section className="shell section sound-design" id="sound-design" data-arrival>
          <div className="section-intro">
            <div>
              <p className="eyebrow">Sound design</p>
              <h2>From source<br />to signal.</h2>
            </div>
            <p>Route modulation, rearrange the effects and shape each frequency band. Save those choices with the patch.</p>
          </div>
          <div className="bento">
            <article className="surface rack-card">
              <div className="rack-copy">
                <SlidersHorizontal className="card-icon" size={23} />
                <h3>Build your effects chain</h3>
                <p>Reorder nine effects, split distortion across up to four bands and compare the signal before and after processing.</p>
              </div>
              <img src="./assets/manual/logisth-distortion-multiband.webp" alt="logisth multiband distortion rack with crossover controls and a spectrum display" width="1240" height="860" loading="lazy" decoding="async" />
            </article>
            <article className="surface modulation-card">
              <Radio className="card-icon" size={23} />
              <h3>Modulate each layer</h3>
              <p>Use envelopes, LFOs and controllers to shape one oscillator or sweep an entire part.</p>
              <div className="routing" aria-hidden="true"><span>LFO</span><i /><span>A1</span><i /><span>A2</span></div>
            </article>
            <article className="surface small-card">
              <Waves className="card-icon" size={23} />
              <h3>Load your own space</h3>
              <p>Use a stereo impulse response for convolution reverb and save it in the preset.</p>
            </article>
            <article className="surface preset-card">
              <Boxes className="card-icon" size={23} />
              <h3>Start from 48 patches</h3>
              <p>Explore the downloadable factory bank before building your own sounds.</p>
              <a className="text-link" href={latest}>Find factory presets</a>
            </article>
          </div>
        </section>
        <section className="shell section workflow" aria-labelledby="workflow-title" data-arrival>
          <div className="workflow-main">
            <AudioWaveform className="card-icon" size={24} />
            <h2 id="workflow-title">Play in your DAW.<br />Or on its own.</h2>
            <p>Use the same synthesis engines in VST3, CLAP or Standalone. Automate Bend and Mod from a host, or play a MIDI controller directly through Standalone.</p>
          </div>
          <div className="studio surface">
            <GitFork className="card-icon" size={24} />
            <h3>Studio notes</h3>
            <p>Made a patch with logisth? Share your setup, a sound or feedback with the project.</p>
            <div className="studio-links">
              <a className="text-link" href={repository + "/issues/new?title=Studio%20feedback"}>Share feedback</a>
              <a className="text-link" href={repository + "/issues?q=is%3Aissue%20feedback"}>Read feedback</a>
            </div>
          </div>
        </section>
        <section className="shell section faq" id="faq" data-arrival>
          <div>
            <CircleHelp className="card-icon" size={25} />
            <h2>Before you play.</h2>
            <p className="section-text">Compatibility, setup and the factory bank.</p>
          </div>
          <div className="faq-items">
            {questions.map(({ question, answer, link }) => (
              <details key={question}>
                <summary>{question}<Plus size={20} /></summary>
                <div className="faq-answer"><p>{answer}{link && <> <a className="text-link" href={latest}>{link}</a>.</>}</p></div>
              </details>
            ))}
          </div>
        </section>
        <ReleaseDownloads />
      </main>
      <footer>
        <div className="shell footer-inner">
          <Brand />
          <span>Synthesizer by wavelogik</span>
          <nav aria-label="Footer">
            <a href="#docs">Manuals</a>
            <a href="#faq">FAQ</a>
            <a href={repository}><GitFork size={16} /> GitHub</a>
          </nav>
        </div>
      </footer>
      <SectionMotion />
    </>
  );
}

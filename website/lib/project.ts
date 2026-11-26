export const repository = "https://github.com/logik93/logisth";
export const latest = repository + "/releases/latest";

export const engines = [
  {
    id: "classic",
    name: "Classic",
    image: "/assets/classic.webp",
    alt: "logisth with Classic oscillators and unison controls",
    heading: "Build from a waveform.",
    description: "Stack up to eight unison voices per oscillator. Spread the stereo field, or shape the spectrum with FM, AM and PWM.",
    detail: "Unison · Detune · FM / AM / PWM",
  },
  {
    id: "wavetable",
    name: "Wavetable",
    image: "/assets/wavetable.webp",
    alt: "logisth showing the Wavetable oscillator and its waveform display",
    heading: "Move through the table.",
    description: "Select a frame with WT POS. Sweep the position with an LFO to shift the harmonic character while a note plays.",
    detail: "Frame position · LFO routing",
  },
  {
    id: "sampler",
    name: "Sampler",
    image: "/assets/sampler.webp",
    alt: "logisth showing a sampler waveform and its editable playback region",
    heading: "Make a recording playable.",
    description: "Drop in a sample, select a region and automate its start and end. Keep the attack intact while Loop Tail repeats the sustain.",
    detail: "WAV / AIFF / FLAC · Loop Tail",
  },
  {
    id: "granular",
    name: "Granular",
    image: "/assets/granular.webp",
    alt: "logisth displaying a source waveform in Granular mode",
    heading: "Pull a sound apart.",
    description: "Turn a sample into grains with Cloud or Granulizer. Change their length, spread and position as they play.",
    detail: "Cloud · Granulizer",
  },
] as const;

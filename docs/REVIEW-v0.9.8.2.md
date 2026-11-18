# logisth v0.9.8.2 — engineering review

The fixed-height modulation row wasted space when its controls were not needed.
A disclosure button now hides the complete row, repositions performance controls,
updates the host editor's aspect ratio and resize bounds, and stores the layout
preference. Existing parameter attachments remain alive and DSP is untouched.
Two original generated backgrounds per factory palette are embedded as compressed
resources. Selecting a background decodes only that resource on the message
thread and reuses the portable theme-image codec. See COMPACT_LAYOUT.md for the
interaction, persistence, scaling and artwork checks.

Existing glyph rendering, real-WAV convolution, gain/factory, rack/contrast and
native memory regressions remain required. Published older releases are unchanged.

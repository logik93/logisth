# logisth 1.0.2

A held Piano Roll note could stop when the host temporarily omitted its transport report while switching views. A sample-counter change could also release playing notes even with a continuous musical clock. The instrument now carries sample and beat predictions across missing reports and checks both clocks on resumption. Incoming MIDI keeps its original offsets. Transport stops, genuine seeks and note-offs still release the affected voices.

The VST3 host regression reproduces the interruption with 1.0.1 and compares Classic, Sampler, Cloud and Granulizer with uninterrupted reference instances. Both English manuals cover the change. Download VST3, CLAP and Standalone together in the package for your system.

# logisth 1.0.3

Adding a VST3 instrument can briefly suspend the whole audio graph. On the same-rate restart, logisth now keeps held notes sounding even if they began with the transport stopped or if the host supplied no timing reports. The instance keeps its own envelopes and sample cursors. MIDI note-offs retain their sample offsets. Reported transport stops and seeks during playback still release notes, and offline bounces start with cleared voices.

The binary test compares Classic, Sampler, Cloud and Granulizer with uninterrupted stereo references while creating, playing and closing a second instrument. The user and technical manuals cover the change. The package for each system includes VST3, CLAP and Standalone. [Engineering review](REVIEW-v1.0.3.md)

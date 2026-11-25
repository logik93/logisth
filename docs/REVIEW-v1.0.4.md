# logisth 1.0.4 — VST3 pitch bend verification

The VST3 smoke test queries the edit controller for the channel 1 pitch bend mapping and passes positions through the mapped parameter queue. It holds MIDI note 60 at 48 kHz, then estimates frequency from interpolated rising zero crossings in the left output. Each result must be within one cent of its expected frequency.

Measurements from the Linux VST3 built at `fd09ee8` matched the expected
frequencies to the precision shown.

| Input range | BEND / st | Applied bend | Measured down | Measured up |
| --- | --- | --- | ---: | ---: |
| Default ±2 st | 12 | ±200 cents | 233.082 Hz | 293.665 Hz |
| ±12 st | 12 | ±1200 cents | 130.813 Hz | 523.251 Hz |
| ±24 st | 12 | ±1200 cents | 130.813 Hz | 523.251 Hz |

The center position and disabled-bend case measured 261.626 Hz. With an
octave input range, partial wheel travel targeting ±200 cents measured
233.088 Hz down and 293.661 Hz up after 14-bit wheel quantization.

The published Linux VST3 from v1.0.3 reproduced the regression. With the
default two-semitone input and a twelve-semitone limit, the positive endpoint
measured 523.207 Hz against the expected 293.665 Hz.

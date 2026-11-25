# logisth 1.0.4 — VST3 pitch bend verification

The VST3 smoke test queries the edit controller for the channel 1 pitch bend mapping and passes positions through the mapped parameter queue. It holds MIDI note 60 at 48 kHz, then estimates frequency from interpolated rising zero crossings in the left output. Each result must be within one cent of its expected frequency.

| Input range | BEND / st | Applied bend | Expected frequencies for down and up |
| --- | --- | --- | --- |
| Default ±2 st | 12 | ±200 cents | 233.082 Hz and 293.665 Hz |
| ±12 st | 12 | ±1200 cents | 130.813 Hz and 523.251 Hz |
| ±24 st | 12 | ±1200 cents | 130.813 Hz and 523.251 Hz |
| ±24 st | 0 | 0 cents | 261.626 Hz |

The center position is also checked at 261.626 Hz.

The published Linux VST3 from v1.0.3 reproduced the regression. With the
default two-semitone input and a twelve-semitone limit, the positive endpoint
measured 523.207 Hz against the expected 293.665 Hz.

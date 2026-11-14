# Third-party notices

The MIT license in the repository root covers the original logisth code and
documentation by wavelogik. It does not relicense third-party dependencies or
remove their distribution requirements.

| Component | License / notice |
| --- | --- |
| JUCE 8.0.6 | AGPLv3 or commercial JUCE license; see [upstream notice](licenses/JUCE-LICENSE.md) and its module notices. |
| clap-juce-extensions | MIT; [copyright and license](licenses/CLAP-JUCE-EXTENSIONS-LICENSE.md). |
| CLAP | MIT; [copyright and license](licenses/CLAP-LICENSE.txt). |
| Open Sans | Apache-2.0; [license](assets/fonts/LICENSE.txt) and [notice](assets/fonts/NOTICE.txt). |
| VST3 SDK bundled with the pinned JUCE dependency | Retains its upstream SDK licensing; see the SDK LICENSE.txt under JUCE's VST3_SDK directory. |
| Optional VST2 SDK | Separately obtained licensed SDK; not included in this repository or binary packages. |

JUCE's notice lists further bundled third-party components and their licenses.
Binary distribution must satisfy the applicable JUCE/SDK terms as well as the
notices above; the original-code MIT grant remains available independently.

## Signalsmith Stretch and DSP

Signalsmith Stretch 1.1.0, commit 44c8f865af9da8c29cc4a70a2d5a3ec83639c711,
https://github.com/Signalsmith-Audio/signalsmith-stretch, MIT. Includes Signalsmith
DSP under its own MIT notice. Vendored headers are unchanged in
Source/vendor/signalsmith-stretch. Full notices are in licenses/Signalsmith-Stretch-MIT.txt
and licenses/Signalsmith-DSP-MIT.txt. Used by Spectral and Kick sampler modes.

## AP logo artwork

The supplied AP mark is used in AP always flexin'!, with its handwritten signature
removed at the project owner's request. See licenses/AP-Music-Group-Logo.txt.
No XtreemFatDemo font software is included.

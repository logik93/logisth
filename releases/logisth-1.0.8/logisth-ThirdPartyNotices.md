# Third-party notices

The MIT license in the repository root covers the original logisth code and
documentation by wavelogik. It does not relicense third-party dependencies or
remove their distribution requirements.

| Component | License / notice |
| --- | --- |
| JUCE 8.0.6 | AGPLv3 or commercial JUCE license; see [upstream notice](licenses/JUCE-LICENSE.md) and its module notices. |
| clap-juce-extensions | MIT; [copyright and license](licenses/CLAP-JUCE-EXTENSIONS-LICENSE.md). |
| CLAP | MIT; [copyright and license](licenses/CLAP-LICENSE.txt). |
| Ioskeley Mono Term Nerd Font Mono v2.2.0 | SIL OFL 1.1; unmodified Regular/Bold, [provenance and notices](assets/fonts/ioskeley/NOTICE.md), with Iosevka/Nerd Fonts notices. |
| Open Sans | Apache-2.0; [license](assets/fonts/LICENSE.txt) and [notice](assets/fonts/NOTICE.txt). |
| VST3 SDK bundled with the pinned JUCE dependency | Retains its upstream SDK licensing; see the SDK LICENSE.txt under JUCE's VST3_SDK directory. |
| Optional VST2 SDK | Separately obtained licensed SDK; not included in this repository or binary packages. |
| Steinberg ASIO SDK headers | GPLv3 for the Windows Standalone backend. See [SDK license](licenses/Steinberg-ASIO-SDK.txt), [upstream files](https://github.com/juce-framework/JUCE/tree/654bdbc7a9db0dae90849b44abe91f339e9f0768/modules/juce_audio_devices/native/asio) and [GPLv3 text](licenses/GPL-3.0.txt). |

Convolution reverb uses JUCE's dsp::Convolution (non-uniform partitioned FFT,
zero added latency), under the same JUCE terms above. Its IR adapter and the
synthetic factory chamber response are original wavelogik code/material.

JUCE's notice lists further bundled third-party components and their licenses.
Binary distribution must satisfy the applicable JUCE/SDK terms as well as the
notices above; the original-code MIT grant remains available independently.

The ASIO-enabled Windows Standalone uses JUCE under [AGPLv3](licenses/AGPL-3.0.txt)
and the Steinberg interfaces under GPLv3. ASIO is a trademark of Steinberg
Media Technologies GmbH. Corresponding source is available through the release
source archive and the exact dependency commits in CMakeLists.txt.

## Signalsmith Stretch and Linear

Signalsmith Stretch 1.3.2, commit 88061fd4eda56a2353c5d65c74aa3f4ccf76f9da,
https://github.com/Signalsmith-Audio/signalsmith-stretch, MIT. It uses Signalsmith
Linear 0.6.6, commit 7bb1c8f36794d6980080ce817c91f8da8fe56a66, MIT.
Vendored upstream headers are unchanged in Source/vendor/signalsmith-stretch;
UPSTREAM.json records their hashes. Full notices are in
licenses/Signalsmith-Stretch-MIT.txt and licenses/Signalsmith-Linear-MIT.txt.
Both Signalsmith Stretch and the independent Kick hybrid use the same upstream
processor.

## AP logo artwork

The supplied AP mark is used in AP always flexin'!, with its handwritten signature
removed at the project owner's request. See licenses/AP-Music-Group-Logo.txt.
No XtreemFatDemo font software is included.

# Dependency provenance and licensing

| Dependency | Pin/source | Usage |
|---|---|---|
| JUCE | 8.0.15, `91ad83ae34a81e0833b1a2b0866f54846370ae53`, https://github.com/juce-framework/JUCE/tree/8.0.15 | Native UI, audio backends, processor adapter |
| Fedora development libraries | System packages; versions recorded in testing.md | ALSA/JACK, X11, fonts |

JUCE is obtained using pinned CMake FetchContent, or the same exact local Git
checkout in `build/vendor/JUCE`. The project does not vendor the full dependency
in tracked source. A deterministic project-owned callback patch is applied to
that checkout; see [architecture.md](architecture.md). The pin is checked against
the official tag. JUCE requires CMake 3.22 and a C/C++ project; this project uses
C++20. `JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0`, no OpenGL module, and no third-party
plugin hosting are used. No WebKit/GTK/OpenGL development packages are required.

The JUCE sources declare AGPLv3/commercial licensing. Read the pinned checkout's
`LICENSE.md`, module notices and [JUCE terms](https://juce.com/legal/juce-8-licence/)
before distributing a product. Upstream font/rendering library notices remain
in the pinned JUCE checkout. No project distribution license has been selected
by this implementation. Factory presets are project-owned configuration examples. M5 distributes an
original synthetic CC0 filter, documented in [asset provenance](../assets/ir/README.md).
No downloaded/measured third-party cabinet or recording is included.

Required Fedora packages for this configuration:

```bash
sudo dnf install gcc-c++ cmake ninja-build git python3 pkgconf-pkg-config \
  alsa-lib-devel pipewire-jack-audio-connection-kit-devel \
  freetype-devel fontconfig-devel libX11-devel libXcomposite-devel \
  libXcursor-devel libXext-devel libXinerama-devel libXrandr-devel libXrender-devel
```

Diagnostic ASan/UBSan builds also need:

```bash
sudo dnf install libasan libubsan
```

Dependencies are installed by the owner. GuitarRack changes no system audio
server, scheduling policy, desktop routing or driver installation.


M3 adds no library or creative assets. The EQ equations are attributed to the
[RBJ/W3C Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook/);
`src/dsp/EQ.cpp` is an original implementation of the published mathematical
formulae. No source listing, recording, cabinet IR or factory preset is copied.
Overdrive/Delay are project-owned mathematical DSP implementations. Existing
JUCE licensing/provenance remains unchanged.


M5 adds no dependency or system package. Distortion's fixed FIR oversampling,
stereo Reverb and radix-2 partitioned Cabinet convolution are original C++ code.
The pinned JUCE stereo reverb was inspected and found to sum both inputs into
one wet excitation; independent channel networks meet the required stereo
contract without modifying JUCE. The original synthetic filter's CC0 dedication
applies to that WAV only. User IRs retain their own asset rights. No overall
project license, distribution, remote, global audio policy or VST3 integration
was introduced. The official JUCE 8.0.15 repository requirements were rechecked:
CMake ≥3.22 and C++ ≥17; this project retains C++20 and the exact pin.

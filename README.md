# GuitarRack

Fedora Linux standalone guitar monitoring foundation using C++20, CMake and
JUCE 8.0.15. M0/M1 software is implemented; real-guitar verification is pending.
Effects, presets and VST3 are later milestones in [PLAN.md](PLAN.md).

The application provides a native resizable window, device/backend/channel
selection, supported sample rate/buffer choices, explicit Start/Stop/Mute,
mono-to-stereo monitoring, trim/level, peak meters, latched clipping warnings and
output protection. It does not start monitoring or select an input device on
launch. Start opens the selected stream; Stop fades and closes it. Device or
configuration failure stops monitoring and requires a new explicit Start.

## Build and run

Required Fedora package commands and license provenance are in
[docs/dependencies.md](docs/dependencies.md). CMake may probe missing optional
OpenGL/WebKit/GTK packages; those features are disabled for this application.

From the project directory:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
./build/linux-debug/GuitarRack_artefacts/Debug/GuitarRack
```

Release:

```bash
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure
./build/linux-release/GuitarRack_artefacts/Release/GuitarRack
```

Normally CMake fetches the exact pinned JUCE commit. To fetch it manually or use
the local checkout already provided by the owner:

```bash
mkdir -p build/vendor
git clone --branch 8.0.15 --depth 1 https://github.com/juce-framework/JUCE.git build/vendor/JUCE
git -C build/vendor/JUCE rev-parse HEAD
```

The expected commit is `91ad83ae34a81e0833b1a2b0866f54846370ae53`.
CMake automatically detects this path, verifies the commit, and applies the
project's source-checked real-time callback patch. This produces deliberate
local changes inside the ignored JUCE checkout. A different local path can be
supplied using `-DFETCHCONTENT_SOURCE_DIR_JUCE=/absolute/path/to/JUCE` at configure
time. Do not reset the patched checkout while a build is running.

## Test your M-Track Solo guitar

1. Plug the guitar into **physical Input 2**. Set Line/Instrument to
   **Instrument** and Direct/USB to **USB**. Connect wired headphones or monitors
   to the interface and start with low headphone volume.
2. Launch GuitarRack. Verify the initial state is stopped and silent. Choose
   JACK on the existing PipeWire system if it exposes the desired interface;
   choose ALSA as a fallback if the device is available. GuitarRack never starts
   a server or changes global PipeWire settings.
3. Select the interface for input/output, **mono channel 2**, and two distinct
   headphone output channels (normally 1 and 2). Device names vary; no device
   name or card index is built in. JACK uses the existing server's format.
4. Select **48 kHz and 256 frames**, if supported, with input trim **0 dB** and
   output **-12 dB**. Press **Start**. Confirm the requested and actual formats
   shown by the application.
5. Play soft and strong notes. Confirm the guitar is audible in **both ears**
   and Input 1 is not mixed in. Adjust the interface gain first. The input meter
   is after trim; input clipping also detects full-scale raw input. If output
   protection lights, reduce trim/output. It hard-clamps peaks at ±0.98 and can
   change tone when hit.
6. Check Mute/Unmute, gain changes, and Stop/Start for clicks. Mute keeps the input
   meter active; Stop closes the stream. Escape requests Stop. Tab reaches the
   controls; sliders support numeric entry and double-click reset.
7. Play for **at least 10 minutes**. Try 128 frames only if exposed and stable.
   Record backend, requested/actual sample rate and buffer, xruns if available,
   clipping, noise and any audible dropouts. Device-reported I/O latency is an
   estimate; measured round trip remains separate and requires a loopback test.
8. Lower headphone volume, disconnect the interface, and confirm the app stops.
   Reconnect, press **Refresh / Reconnect**, reselect devices/channels, and press
   Start. It must stay silent until Start. Also verify a new launch stays stopped.

Device/channel/rate/buffer edits require Stop. A server format/route change during
playback safely stops monitoring and requires Refresh/reselection/Start. Device
preferences are not persisted in M1. No hardware listening, driver stability,
round-trip or Windows results are inferred from the software tests.

## Independent and diagnostic checks

These core checks work without JUCE, a display, or audio hardware:

```bash
cmake --preset core-debug
cmake --build --preset core-debug
ctest --preset core-debug --output-on-failure
```

ASan/UBSan run separately from audio performance measurements:

```bash
cmake --preset core-sanitize
cmake --build --preset core-sanitize
ctest --preset core-sanitize --output-on-failure
```

In environments where LeakSanitizer cannot inspect processes (including the
current restricted agent session), run address/undefined checks with
`ASAN_OPTIONS=detect_leaks=0 ctest --preset core-sanitize --output-on-failure`.
That result does not verify leak detection.

`GuitarRack --smoke-test` opens the same native window and exits after 1.2 seconds,
without starting monitoring. It requires access to the desktop display.
Architecture/real-time ownership is documented in
[docs/architecture.md](docs/architecture.md). Actual verification results are in
[docs/testing.md](docs/testing.md).

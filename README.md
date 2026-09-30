# GuitarRack

Fedora Linux standalone guitar monitoring and a modular serial rack using C++20,
CMake and JUCE 8.0.15. M3 software adds Overdrive, EQ and Delay to the working
Gain rack. The owner reports testing the M2 rack with a real guitar: Gain
controls work well and the application appeared correct during that test.
Earlier M-Track Solo Input 2 monitoring at PipeWire 256/48000 and 512/48000 was
acceptable; 256 is the preference. This is user-reported listening evidence.
M3 effect listening, the monitoring soak and physical failure/latency checks
remain pending. Presets, further effects, VST3 and Windows stay in their planned
milestones in [PLAN.md](PLAN.md).

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

Preferred PipeWire launch (from the project directory):

```bash
PIPEWIRE_LATENCY=256/48000 pw-jack ./build/linux-release/GuitarRack_artefacts/Release/GuitarRack
```

Choose JACK, the interface, mono physical Input 2 and outputs 1/2, then Start.
This is a client latency request; confirm the application's **actual** rate/buffer.
GuitarRack does not change global audio settings. The GUI prefers 256 when that
buffer is supported. The reported 512/48000 launch also sounded acceptable;
256 is the current preference.

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
7. Play for **at least 10 minutes** at the preferred 256 frames.
   Record backend, requested/actual sample rate and buffer, xruns if available,
   clipping, noise and any audible dropouts. Device-reported I/O latency is an
   estimate; measured round trip remains separate and requires a loopback test.
8. Lower headphone volume, disconnect the interface, and confirm the app stops.
   Reconnect, press **Refresh / Reconnect**, reselect devices/channels, and press
   Start. It must stay silent until Start. Also verify a new launch stays stopped.

Device/channel/rate/buffer edits require Stop. A server format/route change during
playback safely stops monitoring and requires Refresh/reselection/Start. Device
preferences are not yet persisted. The owner reported acceptable basic monitoring
at both tested launch settings; driver stability, round trip and Windows results
are not inferred from software checks or that listening report.

## Live Gain rack checklist

1. Start with interface volume low, input trim 0 dB and output -12 dB. Add two
   **Gain** instances using the picker and **Add Effect**; each starts at neutral **0 dB**. Adjust one to -6 dB and
   the other to +3 dB. Both have numeric entry and double-click reset.
2. While holding a note, change levels and toggle each **Bypass**. Toggle
   **Bypass rack**, then **Mute** separately. Listen for clicks or interruptions;
   bypass must keep guitar monitoring active, while Mute silences output.
3. Move either card Up/Down. Its slot label, own level and bypass must stay with
   it. Gain chains commute: their steady tone should stay the same after reorder.
   Automated noncommuting test processors verify actual processing order.
4. Add/remove/reorder repeatedly while playing. Watch “Preparing / transitioning”
   become “Applied”. Confirm removing one Gain leaves the other unchanged.
   A removed slot may briefly remain reserved until its graph is reclaimed.
5. Check Stop/Start with the rack present, both output channels and meter/clipping
   warnings. Record the actual backend/rate/buffer and any clicks/dropouts.

The picker contains **Gain, Overdrive, EQ and Delay**. Every card has labeled
controls, units, numeric entry, double-click default reset and a Reset button.
The rack scrolls vertically to reach every card. It supports **16 instances**
and 20 ms gain/bypass/structural transitions.
Rack bypass retains input/output gain and protection. Gain adds no DSP latency.
There is no preset save or cross-launch rack restore until M4. The 10-minute
soak, real disconnect/reconnect, measured round trip and detailed edit/bypass/
Stop-Start acceptance remain pending; report only checks you actually performed.

## M3 guitar checklist

Use physical Input 2 / Instrument / USB monitoring, low interface volume, trim
0 dB and output -12 dB. Confirm the actual device format after **Start**.

1. **Overdrive alone:** select Overdrive, Add Effect. Try Drive 6→18 dB, Tone
   1500→5000 Hz, Level -6 dB and Mix 25→100%. Compare Mix 0% and Bypass with dry.
   Match loudness using Level; listen for harshness and clicks. Reset restores
   conservative controls. This is a native-rate soft clipper with aliasing,
   without amp/cabinet simulation.
2. **EQ alone:** remove/bypass Overdrive. Add EQ; 0/0/0 dB should sound neutral.
   Try Low +3 dB, Mid -3/+3 dB, High -3 dB separately, then Reset. Listen for
   predictable bass/mid/treble changes and smooth adjustment.
3. **Delay alone:** add Delay with other effects bypassed. Try Time 350 ms,
   Feedback 25%, Mix 25%; pluck once and hear decaying repeats. Change Time to
   150/600 ms while sustaining. Bypass hides echoes while state keeps running;
   unbypass can reveal echoes. Stop/Start clears history. Avoid high feedback
   until levels are comfortable.
4. **Combined:** order **Overdrive → EQ → Delay** using Up/Down. Start at Drive
   12 dB, Tone 3000 Hz, Level -6 dB, Mix 50%; EQ Low 0, Mid +2, High -3 dB;
   Delay 350 ms, Feedback 25%, Mix 20%. Reduce output if protection lights.
   Compare EQ before/after Overdrive: tone should change while each card keeps
   its own UUID/slot and settings.
5. Add another instance, scroll to it, set a different value, then remove it;
   the first instance must retain its controls. Try card/rack bypass and Mute,
   rapid edits, numeric entry and double-click/Reset. Structural edits fade out
   old delay history and can shorten tails or cause wet-only dips. Record any
   clicks, interruptions, unexpected level jumps or harshness.

[DSP details and fixed mappings](docs/effects.md) explain the algorithms,
reset/bypass/tail policy and prototype limitations. Automated signal tests do
not establish subjective tone quality. Dry-guitar-recording and live M3
listening, the 10-minute soak, real reconnect and measured latency are pending.

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

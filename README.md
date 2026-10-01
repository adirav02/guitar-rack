# GuitarRack

Fedora Linux standalone guitar monitoring and a native pedalboard using C++20,
CMake and pinned JUCE 8.0.15. The tone upgrade adds eight original inspired amp
families, six drives, gate/EQ, modulation, echo/reverb, spectral expression pitch,
swell/hold, Dual Amp and five original synthetic cabinet responses. There are 93
complete sounds and 196 reusable voicings. All 81 prior sounds, 174 prior
voicings and executable effect versions remain unchanged. **Art Rock /
Subterranean Pulse** is the owner's confirmed approved sound and is protected
exactly. New sounds are technically validated starting points with listening
pending; no album/circuit accuracy is claimed.

The [song templates](docs/song-templates.md) add clean/space versions of **Let Down**,
**No Surprises** and **Subterranean Homesick Alien**, plus Weird Fishes and six Muse
guitar sounds. Search **Song templates** in Choose Preset...; all fit Legacy 128.
New Stereo / Tape Echo v2 adds wet low cut, adjustable wow rate and stereo phase,
while existing echo versions retain their saved sound. Read the
[research and access limitations](docs/song-tone-research.md) before treating
these inferred settings as recording matches.

Additional Muse templates cover **Citizen Erased** (distortion/clean ending),
**New Born** (rhythm/lead), **Hysteria**, **Psycho**, **Reapers** (tapping/lead),
**Defector** and **The Globalist** (clean intro/heavy middle/clean outro). Search
**Muse expansion** to find this additional collection.

These are complete loadable chains in the application. Rebuild and restart,
open **Choose Preset...**, set filters to **All**, search a song and press
**Load**. Individual importable bundles can also be exported with
`./build/linux-release/guitarrack_export_sounds --song-pack build/song-presets`;
see [bundle import instructions](docs/song-templates.md#choose-and-adjust).

Choose Preset... opens a searchable native window; selecting/searching does not
change audio. Load/Enter/double-click uses the dirty guard and transactional
recall. Audio Setup and per-pedal All controls expand locally without dirtying
sound. Default geometry is 1280×720; the old 1030-pixel minimum is removed.
Start/Stop/Mute, bypass, meters, clipping and gains remain accessible.

Read [the upgrade contract](docs/tone-upgrade.md), [exact new controls](docs/tone-controls.md),
[current verification](docs/testing.md#tone-upgrade-current-verification-2026-10-01)
and [launch/listening worksheet](docs/listening.md). Linux software is implemented;
native visual/HiDPI checks, new guitar listening, hardware latency/soak, measured
IR realism, VST3 and Windows remain pending. The prior M3/M4 owner reports are
historical acceptance of those workflows, not approval of this new catalog.

The release benchmark has useful margin for ordinary 48-kHz rhythm chains, but
dense oversampled/long-IR racks and combined Dual Amp model/graph fades can exceed
block periods. See current verification before using heavy chains live; the
16-slot capacity does not guarantee hardware deadlines.

The application provides a native resizable window, device/backend/channel
selection, supported sample rate/buffer choices, explicit Start/Stop/Mute,
mono-to-stereo monitoring, trim/level, peak meters, latched clipping warnings and
output protection. It restores valid local device choices but requires explicit Start on
every launch. A missing remembered input stays unselected; another microphone is
never substituted. Start opens the selected stream; Stop fades and closes it. Device or
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
preferences and window size are now persisted separately from tone presets. The owner reported acceptable basic monitoring
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

The picker contains all **42 implemented effect types**. Each pedal has rotary
controls, units, numeric entry, double-click default reset and a Reset button.
The board wraps in signal order and scrolls vertically. Earlier/Later move a
pedal; its settings and stable identity follow it. The footswitch LED shows engagement. It supports **16 instances**
and 20 ms gain/bypass/structural transitions.
Rack bypass retains input/output gain and protection.
Legacy reserves 128 samples of DSP latency for aligned oversampling/bypass/transitions: 2.67 ms
at 48 kHz, including empty and bypassed racks. Gain itself adds no stage latency.
M4 saves custom racks and recalls the last selected saved/loaded tone after a
normal close. Every launch remains stopped. The 10-minute soak, real disconnect/reconnect, measured round trip and detailed edit/bypass/
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
not establish subjective tone quality. The owner reports successful individual/combined live M3 effects. Dry-guitar
recording tests, detailed tone/aliasing appraisal, the 10-minute soak, real
reconnect and measured latency remain pending.

## M5 guitar checklist: Distortion, Cabinet and Reverb

Use physical Input 2, **Instrument** and **USB** monitoring, low headphone volume,
trim 0 dB and output -12 dB. Confirm actual rate/buffer after Start. All new cards
support independent instances, numeric entry, double-click and Reset. Software
checks do not establish subjective tone or physical hardware stability.

1. **Distortion alone:** start at Drive 12 dB, Tone 2800 Hz, Level -12 dB, Mix 50%.
   Compare with Overdrive at matched loudness; increase Drive to 24 dB and Mix to
   100%, then try Tone 1500–5000 Hz. Expect stronger clipping/tighter bass. Compare
   Mix 0% with Bypass; both retain latency. Listen for clicks, harshness and
   unexpected level jumps. Residual aliasing remains despite fixed 4× oversampling.
2. **Cabinet alone:** add Cabinet/IR and compare the labeled synthetic starter
   with Bypass. **Load IR...** selects your own mono WAV, 8–192 kHz, ≤500 ms and
   ≤1 MiB. Match volume with Level; IRs are not peak-normalized. While sustaining,
   replace the IR and try a missing/stereo/invalid WAV: expect a clear error and
   the previous sound/preset/dirty state retained. Save As and Export a `.grbundle`;
   import it after moving the original WAV away. The managed copy/bundle preserves
   it. Cabinet convolution does not supply nonlinear amp behavior.
3. **Reverb alone:** Size 35%, Damping 55%, Mix 15%; pluck once and hear decay in
   both ears. Try Size 20–65%, Damping 30–75%, Mix 10–25%. Compare two independent
   instances. Bypass hides the tail, Mute silences while processing continues,
   Stop/Start clears it. Structural edits/IR changes can shorten tails; seamless
   spillover is not promised.
4. **Chain:** use **Distortion → Cabinet → EQ → Delay → Reverb**, or select
   **Driven Synthetic Cabinet**. Start with Distortion 12 dB/2800 Hz/-12 dB/50%;
   Cabinet synthetic or your IR/0 dB; EQ 0/0/-2 dB; Delay 350 ms/25%/15%; Reverb
   35%/55%/10%. Reduce output if protection lights. Compare Cabinet before/after
   Distortion; order should change the sound while UUID/slot/controls stay fixed.
5. While holding notes, test individual/rack bypass, Mute, add/remove/reorder,
   rapid edits, IR cancellation, and preset Save/Discard/Cancel. Check both ears,
   meters and slot labels. Export/import and saved-preset deletion must keep IRs.
   Record clicks/dropouts, level jumps, tail dips and harshness. Run a 10-minute
   soak at preferred 256 frames; report actual backend/rate/buffer. Measured round
   trip, reconnect, detailed tone appraisal and native Delete/dialog checks remain
   pending until performed on your desktop.

DSP algorithms, fixed mappings, latency and tail limits are in
[effects.md](docs/effects.md); actual checks and synthetic processing-cost results
are in [testing.md](docs/testing.md). This checklist describes the first M5
subset; the later tone upgrade extends its catalog. Physical/listening acceptance,
M5 as a whole, VST3 and Windows remain pending.

## Presets and restart checklist (M4)

1. Build/launch the release below. Select your device/channels and press **Start**.
   Add effects, set controls, reorder cards, and choose bypass/gain settings.
2. Press **Save As**, name the rack (Hebrew/UTF-8 is supported), and Save. Confirm
   its name is selected and the unsaved `*` disappears. Save As changes the preset
   UUID while retaining the rack's instance UUIDs, order and slots.
3. Change a control and choose another preset. Test **Cancel** (keep edits),
   **Save** (persist then switch), and **Discard** (switch without saving).
   Factory presets use Save As to protect the originals. Close with unsaved edits
   to test the same three choices.
4. Save your custom rack, close normally, and relaunch. Confirm the saved rack,
   order, slots, gains and selected name return, device preferences are restored
   if available, and monitoring stays stopped until **Start**. Disconnecting a
   remembered device must leave selection incomplete and Start disabled.
5. **Export** to `.json` for an asset-free rack or `.grbundle` (the new default);
   IR-bearing racks export bundle bytes including their required WAVs. Switch
   to Clean, then **Import** that file. Compare
   names, order/slots, individual controls, bypass and global gains. Import loads
   the complete tone; Save stores an imported custom preset in the user library.
   Export includes current edits but does not mark them saved in the library.
6. Recall presets while playing at low headphone volume. Check transitions,
   meters, routing, and unchanged **Mute**/**Start/Stop** state. Try an invalid
   JSON import and an unwritable export location: expect a clear error and the
   previous sound/selection/edits retained. **Cancel load** applies before graph
   acceptance; an already accepted fade finishes. Delay history is reset on
   replacement, so tails can shorten as with M3 structural edits.

### Delete a saved user preset (M4 extension)

1. Select a saved user preset and press **Delete**. Factory presets, unsaved
   working racks and imports that have not been saved in the library cannot be
   deleted. Wait for pending loads/edits to finish, or cancel the pending load.
2. If edited, choose **Save**, **Discard**, or **Cancel** in the existing dirty
   workflow. Then confirm **Delete** in the dialog showing the preset name.
   **Cancel** has initial focus and both Enter/Escape cancel. Cancelling either
   dialog preserves the selection, sound, edits and saved bytes; the Save/Discard
   choice is staged until confirmation. Save then deletion is one transaction.
3. After deletion, the same sound remains as **Untitled rack**, with an unsaved
   `*`. Start/Stop, Mute, routing, all parameters/gains/bypass and ongoing delay
   history stay unchanged. Use **Save As** to save it under a new preset identity.
4. Check the deleted entry is absent after restart (restart still requires
   **Start**). Deletion immediately clears matching local recall to Clean;
   unsaved working racks are not automatically restored after closing with
   Discard. Save As before closing to keep that working rack for future recall.
5. Check Hebrew/emoji names, duplicate display names, both cancellation points
   and dirty choices. Keep an exported copy and an unrelated asset: both must
   remain. A permission/changed-file failure must show an error and retain the
   selected entry, rack and dirty indication. Retry after fixing the reported
   directory/file issue. Preset operations in progress disable Delete; a stale
   confirmation cannot delete a different selection.

Delete targets only the validated `<UUID>.json` entry in the managed user preset
store. It never uses the name or Import path, and never deletes factory files,
exports or IR assets. No filesystem work runs in the audio callback. Explicitly
importing an exported copy later remains possible; old pending loads and cached
library selections cannot revive a deleted entry. These checks pass in automated
state/adapter fixtures; the new native dialogs and real-guitar deletion checklist
still need desktop verification.

The library has **93 full rack presets** and **196 pedal voicings**.
Original factory examples: **Clean**, **Crunch**, **Lead Delay**, **Driven Synthetic
Cabinet** (Distortion → Cabinet → EQ → Delay → Reverb) and **Small Room** (EQ →
Reverb). Output defaults to -12 dB. The Cabinet example uses an explicitly
synthetic CC0 filter, not a measured cabinet or nonlinear amp simulation.

User presets: `$XDG_DATA_HOME/GuitarRack/presets/<UUID>.json`, falling back to
`~/.local/share/GuitarRack/presets`. Local device/window settings:
`$XDG_CONFIG_HOME/GuitarRack/settings.json`, falling back to
`~/.config/GuitarRack/settings.json`. `last-tone.json` in the data directory stores
normal-close recall of the selected saved/loaded tone; discarded edits are not
autosaved. These directories are outside tracked factory files. Device choices,
Start/Stop, Mute, and delay-buffer contents never appear in exported tone files.

The [version 1 schema](assets/presets/schema-v1.json) retains existing M4 presets;
[version 2](assets/presets/schema-v2.json) adds Cabinet checksum references.
The registry supports 42 stable types / 44 exact descriptors, including Delay
v1/v2. Legacy versions remain executable. Unknown fields/effects, unsupported
versions, unsafe/missing assets and invalid values fail transactionally. JSON
is limited to 128 KiB; portable IR bundles to 17 MiB. See
[asset/bundle format](docs/assets.md) and [synthetic asset rights](assets/ir/README.md).
Loading restores exact slot mappings, which can change the meaning of future
slot automation; host integration remains M6. See [state design](docs/presets.md).

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

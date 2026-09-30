# GuitarRack — Product and implementation plan

Created: 2026-09-30. Working name: GuitarRack. Status: M0 foundation and M1 software implemented and built; native-window launch verification and physical guitar acceptance remain pending. No effects or later milestones are implemented.

This is an implementation contract for Codex. Complete one milestone at a time. Treat unchecked items as unfinished. Update this document with evidence as implementation progresses.

## 1. Product goal

Build a desktop guitar effects rack that processes a real electric guitar in real time. The player can choose effects from a growing catalog, add multiple instances, adjust their controls, change their order, bypass them, and save complete sounds as presets.

Develop and validate on Fedora Linux first. Keep the processing code portable so the same project can later produce native Windows builds. Deliver an independent desktop application for everyday playing and a VST3 plugin for use in a compatible host. A DAW is not required for the desktop application.

Product priorities, in order:

1. Reliable input and output, predictable channel routing, and comfortable playing latency.
2. Useful guitar sounds and a clear rack interface.
3. Safe rack editing and dependable preset recall.
4. A maintainable effect architecture that supports adding more effects.
5. VST3 integration and Windows support.

Use English for application labels and technical documentation. Support user-selected preset names in UTF-8, including Hebrew. The owner communicates in Hebrew; explain progress to them in Hebrew unless asked otherwise.

## 2. Confirmed setup and initial assumptions

| Item | Confirmed information |
|---|---|
| Development computer | Fedora Linux with KDE Plasma |
| Audio interface | M-Audio M-Track Solo |
| USB enumeration | `08bb:29c0`, Texas Instruments PCM2900C Audio CODEC |
| ALSA enumeration | `USB AUDIO CODEC`, identifier `CODEC`; card index was 3 in the supplied capture |
| Intended guitar input | Physical Input 2, Line/Instrument switch set to Instrument |
| Listening | Wired headphones or powered monitors connected to the interface |
| Software monitoring | Interface Direct/USB switch set to USB |
| Initial audio format | 48 kHz; evaluate 128 and 256 frame buffers |
| Later platform | Windows with the official M-Track Solo/Duo ASIO driver |

Do not hardcode the ALSA card index, USB bus number, or a single display name. These can change. Device enumeration confirms detection, not tested recording, playback, latency, or sound quality. Implementation checks found Fedora 44 KDE, GCC 16.2.1, CMake 4.3.0 and Ninja 1.13.2; physical audio access is unavailable in the restricted agent session.

The interface supports 16-bit audio at up to 48 kHz. Process audio internally as floating point. Processing effects does not require replacing this interface. Its practical latency and noise performance still require a physical test.

## 3. Release scope

### Linux MVP

- Standalone desktop application with explicit input/output selection and Start, Stop, and Mute controls.
- Mono guitar input routed to a stereo processing chain and stereo output.
- Input/output meters, clipping indication, input trim, and output level.
- An ordered rack of built-in effects with add, remove, move, and bypass operations.
- Multiple instances of the same effect.
- Gain, Overdrive, EQ, and Delay as the first catalog.
- Factory presets, editable user presets, import/export, and persistent user settings.
- A usable resizable native GUI and keyboard access to core controls.

### Complete v1

Extend the MVP with Noise Gate, Compressor, Reverb, Chorus, a basic amp stage, and cabinet IR loading. Validate the Linux VST3 version in a real host. Produce and physically test Windows standalone and VST3 builds. Ship installation documentation and clear dependency/asset license notices.

### Later backlog

Tremolo, Phaser, Flanger, Wah, Fuzz, tuner, metronome, MIDI mappings, recording dry/wet audio, parallel paths, an A/B preset comparison, and more advanced amp models. Third-party VST hosting, neural amp models, cloud accounts, mobile versions, online preset sharing, and a marketplace are future projects, not v1 requirements.

The v1 rack is a serial chain. Start with 16 available effect instances per rack and a growing catalog. This is a deliberate capacity limit for bounded processing and stable plugin automation. Display the limit in the UI. Revisit it before freezing the plugin parameter layout; increasing it after release requires compatibility planning.

## 4. Player workflow and interface

### First launch

1. Show audio setup without automatically enabling a live input.
2. Let the player choose the device, physical input channel, output channels, sample rate, and supported buffer size.
3. Show a short hardware setup hint for guitar input and software monitoring.
4. Require an explicit Start action; begin with a conservative output level.
5. Show live input and output meters and the selected configuration.

Remember valid device preferences locally, but require explicit Start on subsequent standalone launches too. If a device is missing, show setup and keep output silent rather than substituting a microphone or another output automatically.

### Main window

- Top bar: audio status, Start/Stop/Mute in standalone, input/output meters, trim/level, rack bypass, and settings.
- Preset area: preset selection, Save, Save As, import/export, and an unsaved-changes indicator.
- Rack: ordered effect cards with a name, bypass button, controls with units, remove button, and reorder handle.
- Catalog: searchable effect picker with category and short descriptions.
- Status area: actionable device errors, clipping, and CPU/load estimates where available. Never label unavailable dropout or latency measurements as zero.

Provide move up/down buttons before implementing drag and drop. Both operations must use the same rack commands. Provide numeric entry for parameters, sensible ranges, double-click reset, tooltips, and visual disabled states. Do not create a web UI or embed a browser for this application.

Preset changes must offer Save, Discard, or Cancel when the current preset has edits. This is a product data-preservation interaction, not a development approval gate.

## 5. Audio routing and lifecycle

Standalone routing:

`selected physical input -> input trim -> mono-to-stereo mapping -> ordered effects -> output gain -> protection -> selected output channels`

For this owner's setup, select physical Input 2 as mono and output it to both headphone channels. Do not treat Inputs 1 and 2 as a stereo guitar recording. Channel selection belongs to the standalone adapter, not the DSP effects.

Plugin routing:

`host audio bus -> input trim -> channel mapping -> same ordered effects -> output gain -> protection -> host output bus`

Support and explicitly validate mono input/stereo output and stereo input/stereo output plugin layouts. Reject unsupported layouts clearly. The plugin does not open an audio device, choose a physical guitar input, or own the host's transport.

- Clear unused output channels.
- Accept changing sample rates, variable block lengths, zero-sample blocks, and device restarts.
- Process oversized blocks in preallocated bounded chunks if they exceed the prepared scratch capacity; do not resize in the callback.
- On disconnect or unrecoverable device failure, stop standalone processing, silence output, and expose a reconnect action.
- Treat mute, rack bypass, and individual effect bypass as separate controls.
- Rack bypass bypasses the effects while retaining input/output gain and output protection. Individual bypass crossfades to dry; define audible tail behavior consistently. Stop clears temporal state. Do not promise seamless tails after a structural replacement in v1.
- Ramp gain, mute, bypass, and sensitive parameters to avoid discontinuities.
- Reject non-finite control values. Sanitize non-finite processed samples, silence affected output, and surface a diagnostic outside the callback.
- Include an output peak protection stage with bounded behavior and document its effect on tone. It is not a substitute for proper gain staging or safe headphone volume.

Use JUCE audio devices with ALSA and JACK support on Linux. Prefer testing JACK through the existing PipeWire system where available. Do not replace PipeWire, start a competing audio server, or change global scheduling policies automatically. Direct ALSA is a fallback to test when appropriate; it can conflict with other applications holding the device.

Record requested versus actual sample rate and buffer configuration. Device-reported latency, DSP latency, and measured round-trip latency are different values. A first physical target is approximately 15 ms or less total round trip if the hardware/system permits, but this is a test target, not a guarantee. Never infer real round-trip latency from buffer size alone.

## 6. Technology and repository layout

- C++20, with shared portable DSP and no platform-specific includes in the core.
- JUCE, pinned to an exact stable release/tag and recorded commit selected at implementation time.
- CMake 3.22 or newer, raising the minimum if the selected JUCE release requires it; Ninja for Linux builds.
- Native JUCE widgets for the GUI; no embedded WebKit requirement.
- JSON for portable presets and explicit schema versioning.
- CTest for focused DSP/state tests; choose a small test framework and pin it if needed.
- Git, with build directories, personal settings, captures, and generated packages ignored.

Use a JUCE `AudioProcessor` adapter and shared editor for standalone/plugin products. A custom standalone shell may wrap this processor to guarantee explicit Start, input channel selection, and safe device recovery. Verify JUCE's stock standalone behavior before relying on it; never duplicate the DSP chain in the shell.

Proposed organization; do not create empty placeholders for all future features:

| Path | Responsibility |
|---|---|
| `CMakeLists.txt`, `CMakePresets.json` | Reproducible build targets and presets |
| `src/core/` | Rack model, registry, graph builder, processing interfaces |
| `src/dsp/` | Individual effect implementations |
| `src/state/` | Presets, migrations, persistence, validation |
| `src/plugin/` | JUCE processor, host parameters, state integration |
| `src/standalone/` | Device lifecycle, channel mapping, app settings |
| `src/ui/` | Rack editor, effect cards, catalog, meters, settings |
| `tests/` | DSP, state, routing, and graph lifecycle tests |
| `assets/presets/` | Versioned factory presets |
| `assets/ir/` | Only redistributable IR assets and their provenance |
| `docs/` | Architecture decisions, Linux setup, testing, Windows setup |
| `third_party/` | Pinned dependency integration if using submodules |

Choose either an exact Git submodule or pinned CMake FetchContent for JUCE; document the choice. Avoid tracking a moving branch or committing the entire dependency source manually.

Initial Fedora dependency command, to verify against the owner's release:

```bash
sudo dnf install \
  gcc-c++ cmake ninja-build git pkgconf-pkg-config \
  alsa-lib-devel pipewire-jack-audio-connection-kit-devel \
  freetype-devel fontconfig-devel libcurl-devel \
  libX11-devel libXcomposite-devel libXcursor-devel \
  libXext-devel libXinerama-devel libXrandr-devel \
  libXrender-devel libXi-devel
```

Build with `JUCE_WEB_BROWSER=0`; no OpenGL module is needed initially. Install additional libraries only when an implemented feature needs them.

Implemented M0 commands (see README.md for executable paths):

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
cmake --preset linux-release
cmake --build --preset linux-release
```

## 7. Effect registry and rack model

Separate persistent state, control operations, and running DSP.

An effect descriptor contains a stable type ID, display name, category, effect schema version, parameter descriptors, defaults, and a factory. Each parameter descriptor defines a stable key, units, physical range, normalization, smoothing behavior, and automation-bank index.

Every effect processor implements preparation, bounded block processing, reset, latency reporting, and tail reporting. Effects do not access files, UI widgets, device APIs, JSON, or mutable rack containers during processing.

Each rack instance has:

- A persistent instance UUID.
- An immutable slot ID for host parameter identity.
- An effect type and effect version.
- Bypass state and parameter values.
- A position in a separate ordered instance list.

Reordering changes only processing order. It does not change instance identity, slot identity, or parameter values. Removing one instance must not alter another instance of the same effect. The graph builder resolves the registry and produces prepared DSP instances in the requested order.

Keep serial processing simple. Do not build a general-purpose graph editor or an extensibility ABI in v1. Adding a built-in effect should require a processor, descriptor, parameter mapping, and relevant tests rather than changes throughout the application.

## 8. Real-time and graph editing contract

The audio callback must perform bounded work and meet its deadline. No allocation/deallocation, container growth, file access, JSON, network access, logging, mutex acquisition, blocking wait, GUI calls, or plugin/effect construction is allowed there.

Use a two-layer design:

1. A control-thread rack definition and off-thread builder that owns editable state and prepares replacements.
2. A prepared processing graph consumed exclusively by the audio thread, with preallocated buffers and atomic/bounded parameter transfer.

Parameter movement normally updates atomics/smoothers without rebuilding the graph. Structural changes (add/remove/reorder/type change/preset/IR load) build a replacement away from the callback and hand it over at a block boundary.

Before live structural editing ships, document ownership and reclamation explicitly. Use bounded single-producer/single-consumer handoff and retirement queues, or an equivalently proven mechanism. Designate one control producer. A rejected/full queue must backpressure or coalesce outside the callback; audio must never spin or wait. Destroy retired graphs on the control/worker thread after acknowledgment. An atomic `shared_ptr` is not sufficient if its last release can destroy the graph in the callback.

During a structural swap, crossfade old/new output over a short bounded interval using preallocated scratch space. Keep a strict limit on simultaneously running graphs. If this is not ready in an early milestone, require standalone Stop before structural editing and do not expose live editing as complete. Stop-only editing is a temporary stage, not the final MVP acceptance condition.

Rapid repeated edits, preset loads, sample-rate changes, and IR loads must carry generation IDs so stale worker results cannot overwrite newer requested state. Retain the previous rack if a replacement fails. Publish committed state to the UI after the graph handoff, not before pretending it succeeded.

Meters use atomics or bounded snapshots polled by the UI. Expensive diagnostics run outside processing. Verify actual callback paths, including wrapper code, not only custom effect functions.

## 9. Host parameters and automation compatibility

Dynamic racks and fixed host parameter lists require an explicit contract before VST3 implementation:

- Reserve 16 slots, each with one bypass parameter and 16 generic normalized control parameters, plus global input/output gain and rack bypass.
- Use stable IDs such as `slot_03_param_00` and fixed host-visible names such as `Slot 03 Control 01`. The GUI shows descriptive effect controls and units.
- Persist the parameter-bank mapping and slot assignments in state. The effect registry permanently defines each parameter's bank index and normalization for its version.
- Expose parameters once; do not create/delete parameters while a host is running or depend on hosts accepting dynamic renaming.
- Structural rack operations and effect type selection are not host-automatable in v1.
- Reorder retains slot assignment. Changing effect type reuses that slot's generic bank for the new effect, which changes the meaning of existing automation. Explain this in the UI and docs; never silently claim automation followed the removed effect.
- Loading a full preset restores its slot mapping and may likewise change automation meaning. Preserve exact slot assignments in host session state.
- Use parameter gestures and host notifications appropriately. Host-initiated changes must not create UI notification loops.
- Choose one authority for current parameter values. Do not keep unrelated APVTS, rack model, and DSP copies that can diverge.
- Do not overwrite incoming host automation with stale values embedded in a rebuilt graph. Resolve current parameter values at activation through the stable parameter bank.

Unused bank controls are no-ops. Validate the fixed layout early in a host. If this model proves inadequate, revise it before publishing a plugin release, with an architecture note and updated plan. Never change published IDs or normalization silently.

Report total DSP latency to the host when applicable, summing serial stages. Latency changes and bus updates follow JUCE/host lifecycle rules outside the callback. Compensate dry paths inside effects and transition paths when latency differs. Declare tail behavior truthfully.

## 10. Effect catalog and sound development

| Priority | Effect | Initial controls | Notes |
|---|---|---|---|
| MVP | Gain | Level | Reference effect for routing, smoothing, meters |
| MVP | Overdrive | Drive, Tone, Level, Mix | Start with documented soft clipping; tune by listening |
| MVP | EQ | Low, Mid, High | Stable filters; document frequency choices |
| MVP | Delay | Time, Feedback, Mix | Bound feedback below unstable values; interpolate reads |
| v1 | Noise Gate | Threshold, Attack, Release, Hysteresis | Avoid chatter and abrupt note truncation |
| v1 | Compressor | Threshold, Ratio, Attack, Release, Makeup | Predictable envelope and gain reduction |
| v1 | Reverb | Size, Damping, Mix | Start with a bounded JUCE-based implementation |
| v1 | Chorus | Rate, Depth, Mix | Smooth modulation; support stereo output |
| v1 | Basic Amp | Gain, Tone controls, Master | Clearly describe as a basic voiced nonlinear model |
| v1 | Cabinet IR | IR selection, Output level | Load/validate off-thread; bounded convolution |
| Later | Fuzz/Tremolo/Phaser/Flanger/Wah | Effect-specific | Add independently through registry |

Effects do not need the same controls. Use meaningful ranges and units. Provide neutral defaults when appropriate and conservative output levels. Maintain input/output loudness awareness when comparing tones.

For drive/amp stages, document aliasing in the first prototype. Add oversampling for the polished v1 if quality tests justify it, with latency reporting and wet/dry alignment; do not call a naive clipper a faithful commercial amp emulation.

Cabinet simulation matters for a direct, distorted guitar tone. Support user-provided mono WAV cabinet IRs initially, validate finite samples and supported channels/rates, and bound initial duration to 500 ms. Resample and prepare convolution away from audio processing. Show rejected-file reasons and retain the previous cabinet on failure.

Factory presets should demonstrate clean, light crunch, lead with delay, and ambient sounds. Tune them on actual guitar recordings and the owner's guitar. Use only IRs and other assets with documented redistribution rights; a synthetic cabinet filter may be an explicitly labeled fallback. No unlicensed downloads or brand-imitation claims.

## 11. Presets, state, and persistence

Define a versioned JSON schema before implementing import/export. A portable preset contains schema version, preset UUID, display name, global gain/bypass values, ordered instance IDs, instance records, slot mappings, effect versions, parameter values in documented units, and optional IR asset references.

Illustrative shape only; M4 must create and validate the actual schema and fixtures:

```json
{
  "schema_version": 1,
  "preset_id": "a-persistent-uuid",
  "name": "Light Crunch",
  "globals": {"input_db": 0.0, "output_db": -6.0, "rack_bypass": false},
  "order": ["effect-instance-uuid"],
  "instances": [
    {
      "instance_id": "effect-instance-uuid",
      "slot_id": 0,
      "effect_type": "overdrive",
      "effect_version": 1,
      "bypassed": false,
      "parameters": {"drive": 0.25, "tone": 0.5, "level_db": -3.0, "mix": 1.0}
    }
  ],
  "assets": []
}
```

Reject duplicate IDs/slots, missing references, out-of-range values, unknown required effects, unsupported future schema versions, oversized inputs, and non-finite values. Optional metadata can be tolerated. Do not silently drop a required effect and play a different sound.

Preset loading is transactional: parse, validate, resolve assets, prepare the graph, then commit. A failure leaves the prior rack and selected preset intact. Cancellation and stale background loads cannot partially apply a preset. Preserve unsaved edit indication correctly.

Write user presets atomically through temporary files and replacement, with platform-appropriate settings directories. Do not store personal presets in tracked factory asset paths. Device selections, input channel, window geometry, and audio Start state are application settings, not portable tone presets. Never export absolute personal filesystem paths as portable IR references.

For IR portability, distinguish local presets from export bundles. A bundle includes permitted asset bytes, a relative filename, and a checksum. Resolve only within the selected bundle directory; reject path traversal. Missing IRs must produce an actionable error, not quietly remove cabinet processing.

Host state includes the complete rack, current parameter bank, order, mappings, and versioned asset references; it excludes physical device settings and secrets. Restore host state reliably on reopen. Capture a coherent snapshot without racing audio or constructing DSP during state serialization. Use lock-free/bounded handoff for host-thread requests and worker preparation where necessary. Test the lifecycle behavior of the selected JUCE version rather than assuming host calls run on the GUI thread.

## 12. Implementation milestones and exit criteria

### M0 — Repository and build foundation

- [x] Check existing repository, AGENTS files, Fedora release, installed toolchain, and git state.
- [x] Pin JUCE; define license/dependency provenance and C++20/CMake build targets.
- [x] Implement Linux debug/release presets and a runnable standalone skeleton (both binaries built; native desktop launch verification pending).
- [x] Define rack IDs, slot capacity, parameter-bank strategy, and state responsibilities (docs/architecture.md; no published host parameters yet).
- [x] Add focused test targets and README with actual build/run commands.

Exit: a clean checkout builds, opens a window, and does not auto-start input monitoring. Record tested tools and commands.

Software foundation checks pass using the owner's local pinned JUCE checkout. Full M0 exit acceptance remains pending native window launch on the owner's desktop. The default online FetchContent download could not be exercised in this session; the exact upstream source patch and local dependency route were checked.

### M1 — Physical audio path

- [x] Device/channel selection, explicit Start/Stop/Mute, requested/actual backend sample rate/buffer display.
- [x] Selected mono input to stereo output, trim/level, meters, protection, 20 ms parameter ramps (synthetic Input 2 tests pass).
- [x] Device failure/disconnect handling and explicit reconnect implemented; simulated lifecycle tests pass, physical disconnect test pending.
- [x] Routing tests with synthetic channel signals, actual adapter callback checks, processor bus checks and callback allocation audit.
- [ ] Owner test with M-Track Solo, wired headphones, and real guitar.

Exit: at least 10 minutes of stable clean monitoring on the actual device; correct channel in both ears; no unexplained clipping; record configuration and reported/measured latency separately. Mark hardware verification pending until performed.

### M2 — Rack infrastructure

- [ ] Registry and immutable instance/slot identity with separate processing order.
- [ ] Gain effect; multiple instances; add/remove/reorder/bypass UI.
- [ ] Off-thread graph preparation, bounded handoff/reclamation, and safe crossfades.
- [ ] Lifecycle tests for rapid edits, stale requests, queue saturation, and failure rollback.

Exit: rack editing works during playback without callback locks/allocations or incorrect state; repeated edits do not leak graphs or alter unrelated instances.

### M3 — First useful effects

- [ ] Overdrive, EQ, and Delay with smooth controls and descriptors.
- [ ] Focused DSP tests and documented prototype limitations.
- [ ] Listening tests and gain staging on dry guitar recordings and live guitar.

Exit: distinct usable sounds, predictable controls, bounded delay feedback, and repeatable processing at common supported sample rates/block lengths.

### M4 — Presets and state

- [ ] Final JSON schema, fixtures, validation, migrations, and atomic persistence.
- [ ] Factory/user presets, import/export, dirty-state workflow, and settings separation.
- [ ] Transactional load, failure rollback, and slot/parameter-state round trips.

Exit: save/reopen and import/export reproduce the same rack, order, parameters, and tone within numerical tolerance; corrupted presets never partially replace a running rack.

### M5 — Complete guitar sound catalog

- [ ] Gate, Compressor, Reverb, Chorus, Basic Amp, and Cabinet IR.
- [ ] IR validation, bounded async load, asset portability, and provenance.
- [ ] Evaluate drive oversampling and implement alignment/latency handling if adopted.
- [ ] Tune factory presets and test representative dense racks.

Exit: clean, crunch, lead, and ambient presets are usable; IR errors are recoverable; measured performance is documented. Do not imply hardware listening tests were automated.

### M6 — Linux VST3

- [ ] Build the VST3 adapter against the same DSP/editor.
- [ ] Verify supported buses, fixed parameters, host gestures, automation, bypass, latency, and tails.
- [ ] Validate the plugin with an appropriate validator; pin/document tool and version.
- [ ] Test load/unload, multiple instances, offline rendering, editor reopen, and session restore in a real Linux host such as Ardour or Carla.

Exit: host sessions reopen with correct rack state and automation mappings, and plugin validation passes. Record host name/version and exact limitations.

### M7 — Windows

- [ ] Native Windows build using MSVC or a verified compatible toolchain.
- [ ] Document official M-Audio driver installation and ASIO selection.
- [ ] Test standalone and VST3 on Windows with the actual interface.
- [ ] Verify paths, Unicode preset names, packaging, and cross-platform preset exchange.

Exit: both products run on Windows; the same portable preset loads on Linux and Windows; physical ASIO monitoring is tested. Linux-only success does not complete this milestone.

### M8 — Release readiness

- [ ] Resizable/accessibility pass, keyboard controls, stable error messages.
- [ ] Release builds, setup documentation, reproducible packages, and license notices.
- [ ] 30-minute real-guitar soak test, rapid rack/preset edits, and device disconnect testing.
- [ ] Host-compatibility notes and performance/latency report.
- [ ] Final audit of dependency, factory preset, and IR redistribution rights.

Exit: a new user can install, select the correct input, start, create a rack, save it, and restore it using the documentation.

## 13. Verification strategy

Use tests that establish actual audio/state properties, not screenshots of implementation details:

- Gain: expected amplitudes and monotonic ramps; neutral gain is transparent.
- EQ: neutral response and measured frequency response at selected points.
- Overdrive: finite bounded output, symmetry where intended, and level/mix behavior.
- Delay: impulse positions, decay behavior, fractional-delay stability, and sample-rate changes.
- Gate/compressor: envelope behavior for known signals and silence.
- Rack: deterministic order, independent instances, identity preservation after reorder, bypass continuity.
- Routing: Input 2 selection, mono-to-stereo mapping, cleared unused channels, plugin bus validation.
- State: round trips, version migrations, duplicate IDs, unknown effects, missing IRs, rollback, stale loads.
- Lifecycle: variable/zero/oversized blocks, graph reclamation, disconnect, repeated prepare/reset, host restore.
- Real time: allocation auditing after preparation, callback path inspection, and sustained measured load with headroom. Run thread/memory sanitizers separately from low-latency benchmarking.

Do not invent a universal CPU percentage or latency result. Report CPU model, OS, build type, device/backend, sample rate, buffer, rack, and measurement method. Synthetic tests can verify processing but cannot establish driver latency or subjective guitar tone.

## 14. Decisions and risks

| Topic | Decision / mitigation |
|---|---|
| Large scope | Ship milestones; reliable monitoring before many effects |
| Rack edits | Prepare off-thread; bounded handoff and deferred destruction |
| Dynamic automation | Fixed slot bank; stable IDs; documented semantic changes |
| Guitar input routing | Explicit mono channel selection; no hardcoded ALSA index |
| Direct guitar tone | Include cabinet processing in v1; honest prototype descriptions |
| Latency | Measure actual system; distinguish buffers, DSP delay, and round trip |
| IR portability | Versioned asset references and optional permitted-asset bundles |
| Windows | Shared core, native builds, actual ASIO tests later |
| Licensing | Inspect selected JUCE license and distribution terms before choosing project license/releasing binaries; open source does not automatically mean unrestricted proprietary distribution |

Routine implementation choices are delegated to Codex within this plan. Document deviations that affect product behavior or compatibility. Ask the owner only for missing facts or consequential product choices that cannot be reasonably inferred; do not repeatedly ask to proceed with authorized milestone work.

## 15. First Codex task and progress record

Suggested first instruction after placing these files at the repository root:

> Read AGENTS.md and PLAN.md. Implement M0 and then the software portion of M1 on Fedora. Inspect the current repository and toolchain first. Build and run the standalone app, verify synthetic channel routing, and provide exact steps for my M-Track Solo guitar test. Update PLAN.md with completed checks and evidence. Keep hardware checks pending until I perform them. Do not implement the later effects yet.

Progress on 2026-09-30:

- Inspected root/applicable instructions, planning-only files, Fedora and build dependencies. The directory initially had no recognized Git repository; the owner ran `git init`. Existing AGENTS.md was preserved; no commits or remote repository were created. Files remain uncommitted.
- Pinned JUCE 8.0.15 to official commit `91ad83ae34a81e0833b1a2b0866f54846370ae53`. Shell GitHub access was unavailable; the owner downloaded the exact checkout into `build/vendor/JUCE`, which CMake verifies and detects automatically. Dependency/license provenance is in docs/dependencies.md.
- Implemented C++20 core, shared JUCE AudioProcessor adapter, custom native standalone shell, explicit device/channel/rate/buffer setup, Start/Stop/Mute, mono-to-stereo routing, trim/level, meters, latched clipping diagnostics and a documented ±0.98 output ceiling. No monitoring stream opens on launch; Start is never restored automatically.
- Inspected pinned backend dispatch: upstream ALSA/JACK use callback mutexes. Added a source-hash-checked deterministic patch with bounded atomic callback leases and off-callback draining, preallocated ALSA conversion scratch, atomic runtime status and deferred JACK shutdown handling. Direct AudioIODevice ownership avoids additional stock standalone/mixer/player callback locks. Ownership and limitations are in docs/architecture.md.
- `cmake --preset linux-debug`, `cmake --build --preset linux-debug`, `ctest --preset linux-debug --output-on-failure`: application builds; 2/2 tests pass. Linux release equivalents also build and pass 2/2 tests. Tests exercise routing, impulse timing, smoothing, finite protection, meters, zero/oversized blocks, callback reclamation, explicit Start, rejection of substituted channels, failure silence, reconnect authorization, sample-rate/buffer changes and shared processor buses. Allocation/deallocation auditing covers core processing, actual standalone callback/failure path, and shared processBlock.
- ASan/UBSan: owner installed missing libasan/libubsan. Core diagnostic checks pass with leak detection disabled; both test suites also pass in `build/adapter-sanitize` with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer cannot inspect processes in this restricted session; leak detection is not verified. No sanitizer timing is used as a performance result.
- Fresh upstream-source patching, idempotent reapplication and modified-source rejection were checked in a temporary fixture. Release `ldd` reports no missing linked libraries.
- Native desktop window launch verification is pending. This session has no exposed `/dev/snd` and cannot connect to the owner's PipeWire service. M-Track Solo/guitar listening, real disconnect/reconnect, 10-minute monitoring, actual backend/format and measured round trip remain pending. See README.md and docs/testing.md for commands/checklist.
- Implementation decisions: device/channel/format edits require Stop; unexpected backend configuration changes stop safely and require a fresh explicit Start. Device preferences are deferred to M4's standalone settings work. Rack edits, effects, preset/state serialization, VST3/host automation and Windows remain untouched.

Maintain a concise log here as work finishes: date, milestone, files/behavior changed, checks run, results, and remaining hardware/host checks.

## 16. Official references

Checked while preparing this plan on 2026-09-30; recheck version-specific requirements when pinning dependencies.

- JUCE repository and integration: https://github.com/juce-framework/JUCE
- JUCE CMake API: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md
- JUCE Linux dependencies (Ubuntu names; translate to Fedora packages): https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md
- JUCE AudioProcessor lifecycle: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html
- JUCE 8 license terms: https://juce.com/legal/juce-8-licence/
- Fedora JACK/PipeWire development package: https://packages.fedoraproject.org/pkgs/pipewire/pipewire-jack-audio-connection-kit-devel/
- M-Track Solo user guide: https://cdn.inmusicbrands.com/m-audio/maudio_documentation/M-Track%20Solo%20-%20User%20Guide%20-%20v1.6.pdf
- M-Audio driver downloads: https://www.m-audio.com/drivers
- Carla host: https://kx.studio/Applications:Carla
- Ardour: https://ardour.org/

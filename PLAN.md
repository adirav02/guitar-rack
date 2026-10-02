# GuitarRack — Product and implementation plan

Created: 2026-09-30. Working name: GuitarRack. Status: M0/M1 through M4 software implemented. Owner reports the M3 GUI opens on Fedora and Overdrive, EQ and Delay work with a real guitar, individually and together; this is user-reported desktop/listening evidence. The owner also reports that the M4 desktop preset workflow works well (user-reported acceptance). The new M4 Delete desktop checklist, unreported keyboard/live-transition details, detailed live-edit acceptance, dry-recording/gain-staging appraisal, the 10-minute soak and unreported physical checks remain pending. The focused first M5 software subset is implemented before VST3: Distortion, Reverb and Cabinet/IR, including assets/bundles, latency alignment and verification. New subjective/live-hardware acceptance remains pending. The later tone upgrade software adds the processors and UI described below; physical/listening acceptance, M5 as a whole, VST3 and Windows remain pending.

This is an implementation contract for Codex. Complete one milestone at a time. Treat unchecked items as unfinished. Update this document with evidence as implementation progresses.


## Presentation refresh — 2026-10-02

Owner-authorized UI-only work on `feat/radiohead-muse-song-tones`, based on clean
`0d4cab02b49ead5d4c09a74ee11d24ca816d06d0`. Commit to the same branch is explicitly
authorized for this task. Existing audio and preset action contracts remain.

- [x] Add an application-owned native studio theme, tactile rotary controls,
  pedal-family enclosure colours, readable control spacing, segmented meters,
  consistent menus/dialogs and visible keyboard focus styling.
- [x] Refresh preset browsing with labelled filters, two-line sound rows,
  result counts and an empty-result message; retain existing filtering/loading.
- [x] Keep transport visible while audio settings can scroll on short windows;
  preserve the existing separate preset window and board scrolling.
- [x] Compile all five affected translation units to objects with pinned JUCE;
  rerun the four existing headless state/asset/voicing/identity checks.
- [ ] Native standalone link, rendered desktop/HiDPI and keyboard acceptance:
  configuration is blocked by missing PkgConfig and native development headers
  in this environment. No display is available. See [verification](docs/testing.md)
  and [visual acceptance checklist](docs/ui-refresh.md).

### Owner-reported preset visibility and audio interruption follow-up — 2026-10-02

The artist browser choices were hard coded before the modern rock catalog and
omitted Foo Fighters, Shinedown and Alter Bridge. They now include catalog
artists, retaining existing style choices and saved string filters. The stale
Release executable was rebuilt locally. PipeWire JACK buffer choice is now a
per-client Start request from Audio Setup, with requested/actual verification;
the Legacy 128 / Creative 4096 DSP latency contract remains unchanged. See the
new [testing record](docs/testing.md). Synthetic 48-kHz/256-frame evidence
already shows some dense racks and Dual graph fades can miss the period. Physical
xruns/cuts, exact Muse trouble cases, font/HiDPI appearance, and round-trip
latency still require owner desktop/audio inspection; no acceptance is inferred.

The owner narrowed the missing glyphs to the Audio Setup button; its Unicode
triangles and the other non-ASCII UI adornments were replaced with ASCII labels.
Both Citizen Erased factory chains render nonzero at 48 kHz/Legacy 128 from the
synthetic DI, but this does not resolve the owner's reported silent hardware
output after Load. Compare another preset without changing the stream and check
the actual xrun count/output routing before altering calibrated factory recipes.


## Authorized tone upgrade A–D — current software contract (2026-10-01)

The owner's active brief supersedes the old later-backlog placement of amps,
gate, modulation, pitch, native browsing and bounded dual blending. Extend the
existing serial rack; do not restart M0. See [implementation/limitations](docs/tone-upgrade.md),
[verification](docs/testing.md#tone-upgrade-current-verification-2026-10-01),
[control mappings](docs/tone-controls.md) and [owner checklist](docs/listening.md).
Baseline checkout was clean and exactly 3511ab1ea2a49be8a3de6c107b0e07831bb63bdf.
No remote writes, package installation, purchase, deployment or replacement
architecture was authorized or performed.

- [x] A: Freeze all legacy identities/recipes/control contracts, original IR bytes
  and deterministic render metrics at 44.1/48/96 kHz before new DSP; registry,
  byte/identity and numerical regression checks pass.
- [x] A: Explicit factory/pedal/instance IDs and UUID references replace positional
  identity generation. Native browser, dirty transactional Load, search/filters/
  favorites, compact toolbar/audio collapse, UUID expansion/scroll, strict v1→v2
  local migration and reduced logical window constraints are implemented/built.
- [ ] A: Actual native keyboard/Escape/focus/open-window shutdown, 1280×720,
  1366×768, wide-screen and 150% visual acceptance. No display connection here.
- [x] B: Eight distinct original amp networks, six separate drives, five-band and
  parametric EQ, gate and five checksum-managed CC0 synthetic cab voices.
  New control maps/versions, matched-RMS difference/early EQ/Master/Output tests,
  small-signal response/gain harmonics/transient cleanup and alias factor/reference
  matrix are implemented and pass.
- [ ] B: Matched perceived-level guitar appraisal and measured-cab realism/rights
  acceptance. Synthetic cabs are not measured speaker reproductions.
- [x] C: Explicit stopped-state 128/4096 profile with aligned bounded graphs and
  transactional incompatibility; wah/envelope, phaser/flanger, BPM/tap/multihead
  echo, distinct room/plate/hall, swell/120-s hold, ±24 spectral expression pitch,
  bounded parallel Dual Amp with one shared downstream cab. Signal/lifetime
  tests pass; legacy algorithms remain untouched.
- [x] C: Separate strict metadata, 24 complete new standalone sounds and 69 new
  voicings alongside legacy originals; no scenes/song-part requirements.
- [ ] C: Pitch attack/chord listening acceptance; close chords and low shifted
  notes have reported artifacts. No chord-quality mode or low-latency claim.
- [x] D: Linux integration, compatibility/behavior suites, offline DI render/
  headroom/alias/benchmark tools and acceptance worksheet are implemented.
  The 60-row release matrix is complete; cached model math reduces representative
  mean costs, but dense racks and combined Dual model/graph fades can exceed
  device periods. See current verification for results and practical limits.
- [ ] D: New physical routing/round trip/soak and artist-character/listening
  acceptance. Only unchanged Subterranean Pulse is explicitly owner approved.

Current inventory: 42 types, 44 exact descriptors, 168 complete sounds, 305 pedal
voices. Optional chainsaw, reverse/granular delay and stutter remain deferred.
VST3/Windows/cloud/mobile/plugin hosting/general routing remain outside this
request. Checkboxes refer to software behavior with recorded evidence, not the
unfinished M5 physical or subjective acceptance.

### Research-led song templates — 2026-10-01

The owner's new request is implemented on `feat/radiohead-muse-song-tones`.
[Research](docs/song-tone-research.md) was completed before project inspection
and branch creation; network policy blocked YouTube and specialist gear sites.
Primary DSP sources were read successfully. Song-specific settings are inferred
starting points, with source/listening limitations visible in catalog details.

- [x] Add 12 complete Radiohead/Muse templates and 28 reusable voicings, including
  separate Let Down clean/space and Subterranean clean/floating-lead sounds,
  No Surprises clean guitar, and Muse fuzz/heavy/chime/lead sounds.
- [x] Version Echo v2 with wet/feedback low cut, adjustable tape wow rate and
  stereo phase; retain Echo v1 and all earlier recipes/mappings/identities.
- [x] Regenerate embedded recipes/catalog, portable fixtures, strict schemas
  and control documentation. All song templates fit Legacy 128.
- [ ] Complete owner reference listening, guitar/pickup calibration, native
  Fedora display/audio build and physical acceptance for this branch.

Software verification and environment limits are recorded in
[testing](docs/testing.md#song-template-verification-2026-10-01);
[usage and chains](docs/song-templates.md) describe the final behavior. Existing
graph/lifetime/16-control/latency/asset contracts remain. No commits, pushes,
system packages, new external audio assets or global audio settings were added.

### Additional Muse templates — 2026-10-01

The owner requested more templates for Citizen Erased (distortion and clean
ending), New Born, Hysteria, Psycho, Reapers, Defector and The Globalist.
Added 12 separate sounds and 22 reusable voicings on the existing song-tone
branch, using existing processors. New Born/Reapers include rhythm/tapping and
lead alternatives; The Globalist includes clean intro, heavy middle and clean
outro guitar. All fit Legacy 128. No new DSP, routing, scene automation or latency
profile is introduced. All prior 81 sounds / 174 voices remain unchanged.
The owner subsequently confirmed all seven album versions as the target and
requested fresh tone research. MuseWiki/artist interview access is blocked by
HTTP policy; the [research/matching guide](docs/muse-tone-research.md) distinguishes
that pending verification from inferred settings. Outputs are calibrated using
the existing synthetic phrase; physical/reference listening remains pending.
See [template roles](docs/song-templates.md#more-muse-song-templates)
and [verification](docs/testing.md#muse-template-expansion-verification-2026-10-01).

The owner's follow-up requests usable application chains. These are already
embedded in the factory/session/browser loading path. The exporter now supports
`--song-pack <directory>` to deliver individual cabinet-inclusive portable
bundles, verified through the application's importer. Native release build was
attempted here but is blocked by missing X11/audio/font development headers;
the owner must rebuild the executable on the configured Fedora desktop.
See [handoff verification](docs/testing.md#application-chain-handoff-2026-10-01).

### Green Day / Soundgarden / Nirvana / SOAD expansion — 2026-10-01

Work continues on `feat/radiohead-muse-song-tones`. The remote branch matched
`8800341a6b78b0aa7063cb0a8811b59bf7b77016`; checkpoint
`15870b779998c5d056fec53212b2669e332dacd8` preserves its exact tree before edits.
The available workspace contained no Git checkout or additional uncommitted state;
this task reconstructed the remote snapshot without claiming access to other
computer worktrees. The owner authorizes additive changes and branch commits.

- [x] Research all 14 requested recordings, artist/engineer accounts, physical
  tunings and era/role distinctions; label transcription-only and unresolved facts.
- [x] Add 42 independent racks and 60 reusable voices with stable explicit UUIDs;
  no new DSP or pitch processing. Preserve all earlier 93 sounds / 196 voices,
  catalog records, portable files and legacy fixture identities.
- [x] Regenerate embedded library/catalog and 42 portable presets; verify loading,
  round trips, callback behavior, additive edit retention and synthetic headroom.
- [ ] Actual reference listening, pickup calibration, native display/audio and
  target-hardware CPU/round-trip acceptance; Toxicity Dual Amp especially.

See [research](docs/alternative-tone-research.md),
[recipes and use](docs/alternative-templates.md) and
[verification](docs/testing.md#alternative-template-expansion-2026-10-01).

### Foo Fighters / Shinedown / Alter Bridge expansion — 2026-10-01

Continue on `feat/radiohead-muse-song-tones`. Local and remote HEAD were clean
at `c159e512932d6d95372c71779e91724f93f22104`. Before edits, checkpoint
`b8e7dc6a42ff086470c7187a3a405c1af2d01a6e` preserved the exact tree. The
owner requests the same research/template/verification and branch-commit workflow.

- [x] Research all nine album targets, distinguish three Foo Fighters eras and
  Shinedown periods, separate both Blackbird players/solos, and label tuning
  evidence strength, unreviewed embedded video and unresolved studio settings.
- [x] Add 33 separate racks and 49 voices with stable UUIDs using existing DSP;
  no pitch shift. Preserve all earlier 135 sounds / 256 voices and assets.
- [x] Regenerate presets; verify loading, callbacks, 99 synthetic headroom
  renders, full checkpoint preservation and repeatable edit retention.
  Commit and push the verified changes to the existing branch.
- [ ] Reference listening, pickup calibration, native display/audio and
  target-hardware acceptance. Only unchanged Subterranean Pulse is approved.

See [research](docs/modern-rock-tone-research.md),
[recipes/use](docs/modern-rock-templates.md) and
[verification](docs/testing.md#modern-rock-template-expansion-2026-10-01).

### GitHub review handoff — 2026-10-01

The owner now explicitly authorizes committing and uploading this song-tone
worktree to the existing GitHub repository and opening a pull request against
`main`. This supersedes the earlier local-only scope for these changes. Publish
the source, factory presets, research and verification records on
`feat/radiohead-muse-song-tones`; native build/audio/listening limitations remain
visible for review. No merge or release publication is requested.

## 1. Product goal

Build a desktop guitar effects rack that processes a real electric guitar in real time. The player can choose effects from a growing catalog, add multiple instances, adjust their controls, change their order, bypass them, and save complete sounds as presets.

Develop and validate on Fedora Linux first. Keep the processing code portable so the same project can later produce native Windows builds. Deliver an independent desktop application for everyday playing and a VST3 plugin for use in a compatible host. A DAW is not required for the desktop application.

Product priorities, in order:

1. Reliable input and output, predictable channel routing, and comfortable playing latency.
2. Useful guitar sounds and a clear rack interface.
3. Safe rack editing and dependable preset recall.
4. A maintainable effect architecture that supports adding more effects.
5. VST3 integration and Windows support.

Use English for application labels and technical documentation. Support user-selected preset names in UTF-8, including Hebrew. The owner communicates in Hebrew; communicate progress in concise English for the active tone upgrade, as instructed by the owner.

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
| Current audio preference | PipeWire launch 256/48000; owner also tried 512/48000 and found both acceptable. Actual device format/latency not yet reported |
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

Extend the MVP first with Distortion, Reverb and cabinet IR loading; then Noise Gate, Compressor, Chorus and a basic amp stage. Validate the Linux VST3 version in a real host. Produce and physically test Windows standalone and VST3 builds. Ship installation documentation and clear dependency/asset license notices.

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
- Preset area: preset selection, Save, Save As, Delete for saved user presets, import/export, and an unsaved-changes indicator.
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
| M5 first part | Distortion | Drive, Tone, Level, Mix | Distinct stronger clipping; fixed oversampling/alignment |
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

Version 1 shape; implemented schema and validated fixtures are in `assets/presets/`:

```json
{
  "schema_version": 1,
  "preset_id": "00000000-0000-4000-8000-000000000002",
  "name": "Light Crunch",
  "globals": {"input_db": 0.0, "output_db": -6.0, "rack_bypass": false},
  "order": ["10000000-0000-4000-8000-000000000001"],
  "instances": [
    {
      "instance_id": "10000000-0000-4000-8000-000000000001",
      "slot_id": 0,
      "effect_type": "overdrive",
      "effect_version": 1,
      "bypassed": false,
      "parameters": {"drive_db": 12.0, "tone_hz": 3000.0, "level_db": -6.0, "mix_percent": 50.0}
    }
  ],
  "assets": []
}
```

Reject duplicate IDs/slots, missing references, out-of-range values, unknown required effects, unsupported future schema versions, oversized inputs, and non-finite values. Version 1 uses closed objects with exact required fields; unknown fields are rejected to expose mistakes. Optional metadata can be added only through an explicit schema change. Do not silently drop a required effect and play a different sound.

Preset loading is transactional: parse, validate, resolve assets, prepare the graph, then commit. A failure leaves the prior rack and selected preset intact. Cancellation and stale background loads cannot partially apply a preset. Preserve unsaved edit indication correctly.

Write user presets atomically through temporary files and replacement, with platform-appropriate settings directories. Do not store personal presets in tracked factory asset paths. Device selections, input channel, window geometry, and audio Start state are application settings, not portable tone presets. Never export absolute personal filesystem paths as portable IR references.

For IR portability, distinguish local presets from export bundles. A bundle includes permitted asset bytes, a relative filename, and a checksum. Resolve only within the selected bundle directory; reject path traversal. Missing IRs must produce an actionable error, not quietly remove cabinet processing.

Host state includes the complete rack, current parameter bank, order, mappings, and versioned asset references; it excludes physical device settings and secrets. Restore host state reliably on reopen. Capture a coherent snapshot without racing audio or constructing DSP during state serialization. Use lock-free/bounded handoff for host-thread requests and worker preparation where necessary. Test the lifecycle behavior of the selected JUCE version rather than assuming host calls run on the GUI thread.

## 12. Implementation milestones and exit criteria

### M0 — Repository and build foundation

- [x] Check existing repository, AGENTS files, Fedora release, installed toolchain, and git state.
- [x] Pin JUCE; define license/dependency provenance and C++20/CMake build targets.
- [x] Implement Linux debug/release presets and a runnable standalone skeleton (both binaries built; owner confirms native GUI opens on Fedora).
- [x] Define rack IDs, slot capacity, parameter-bank strategy, and state responsibilities (docs/architecture.md; no published host parameters yet).
- [x] Add focused test targets and README with actual build/run commands.

Exit: a clean checkout builds, opens a window, and does not auto-start input monitoring. Record tested tools and commands.

Software foundation checks pass using the owner's local pinned JUCE checkout. The owner confirms native window launch. No-auto-start is verified synthetically; physical startup-silence and full UI/keyboard acceptance were not separately reported. The default online FetchContent download could not be exercised in this session; the exact upstream source patch and local dependency route were checked.

### M1 — Physical audio path

- [x] Device/channel selection, explicit Start/Stop/Mute, requested/actual backend sample rate/buffer display.
- [x] Selected mono input to stereo output, trim/level, meters, protection, 20 ms parameter ramps (synthetic Input 2 tests pass).
- [x] Device failure/disconnect handling and explicit reconnect implemented; simulated lifecycle tests pass, physical disconnect test pending.
- [x] Routing tests with synthetic channel signals, actual adapter callback checks, processor bus checks and callback allocation audit.
- [x] Owner reports real-guitar monitoring works with M-Track Solo physical Input 2; PipeWire launches at 256/48000 and 512/48000 sound acceptable. Current preference: 256.
- [ ] Full owner acceptance: 10-minute stable monitoring, detailed routing/gain/mute/startup checks, clipping/dropout report, and physical disconnect/reconnect.

Exit: at least 10 minutes of stable clean monitoring on the actual device; correct channel in both ears; no unexplained clipping; record configuration and reported/measured latency separately. Basic monitoring is owner-verified; the complete exit checks remain pending until performed.

### M2 — Rack infrastructure

- [x] Registry and immutable instance/slot identity with separate processing order.
- [x] Gain effect; multiple instances; add/remove/reorder/bypass UI.
- [x] Off-thread graph preparation, bounded handoff/reclamation, and safe crossfades.
- [x] Lifecycle tests for rapid edits, stale requests, queue saturation, and failure rollback.
- [x] User-reported M2 real-guitar listening: Gain controls work well; application appeared correct during the owner's test.
- [ ] Detailed owner acceptance of live edits, bypass continuity and rack Stop/Start (not separately reported).

Exit: rack editing works during playback without callback locks/allocations or incorrect state; repeated edits do not leak graphs or alter unrelated instances. Automated processing, lifetime accounting and callback heap audits pass. The owner reports a real-guitar rack test with working Gain controls; detailed edit/bypass/Stop-Start acceptance remains pending. LeakSanitizer remains unverified. M4 later records 5/5 scoped core/state/adapter TSan checks; the original M2 attempt was blocked.

### M3 — First useful effects

- [x] Overdrive, EQ, and Delay with smooth controls and versioned descriptors; picker, numeric entry, resets and scrolling cards.
- [x] Focused DSP, stateful graph/callback tests and documented prototype limitations.
- [x] User-reported Fedora M3 desktop and real-guitar listening: Overdrive, EQ and Delay work, including together in a rack.
- [ ] Dry guitar recording tests, detailed tone/aliasing appraisal and listening-based gain staging (not reported).

Exit: distinct usable sounds, predictable controls, bounded delay feedback, and repeatable processing at common supported sample rates/block lengths. Software checks pass; the owner reports working individual/combined live effects and a working M3 desktop GUI. Detailed subjective tone/aliasing appraisal, dry-recording tests and listening-based gain staging remain pending. Algorithms, permanent mappings and tail policy are in `docs/effects.md`.

### M4 — Presets and state

- [x] Version 1 JSON schema, validated factory fixtures, strict runtime validation, explicit migration entry point (v1 only; no legacy format), and atomic Linux persistence.
- [x] Factory/user presets, import/export, Save/Save As, Save/Discard/Cancel workflow, Unicode names, dirty indication, and device/window settings separation (implemented; state logic tested).
- [x] Owner reports testing the M4 desktop preset workflow: it works well (user-reported acceptance; no inferred duration or detailed subchecks).
- [ ] Unreported detailed keyboard/live preset-transition checks and new Delete desktop/guitar checklist.
- [x] Focused M4 extension: confirmed managed user-preset deletion, staged dirty choices, unchanged working sound, new Save As identity, recall cleanup, failure retention and stale-operation protection (automated state/adapter checks).
- [x] Worker-prepared transactional load, failure/cancellation/stale rollback, competing loads, exact identity/order/slot round trips and reset-condition DSP equivalence.

Exit: automated save/reopen and import/export reproduce the same rack, order, parameters and reset-condition output (maximum tested sample difference <2e-6). Corrupted/failed/stale presets retain the previous accepted configuration. The owner reports successful M4 desktop preset use. New Delete dialogs/keyboard and physical listening, plus unreported detailed transitions, still need the owner checklist; software tests do not establish live listening quality.

### M5 — Complete guitar sound catalog

- [x] First authorized M5 software part: Distortion, Reverb and Cabinet/IR; integrated DSP/UI/presets/assets, meaningful signal/lifecycle tests and build/diagnostic evidence. Native GUI/listening acceptance remains pending.
- [x] Experimental software extension: Fuzz, Basic Amp, Compressor, Chorus, Tremolo, Resonant Filter/Auto-Wah, granular Pitch Shifter and Ring Modulator; Delay v2 Digital/Analog/Tape; native pedalboard controls and reusable voicings. Core/state signal/lifetime/persistence checks pass; full native application link and physical acceptance for this branch are pending.
- [x] 77 per-pedal voicings with validated personal save/reload and 40 additional full rack recipes; preserve existing five factories and Delay v1 state.
- [ ] Remaining catalog: standalone Gate, Phaser, Flanger, dedicated Hall/Plate/Spring algorithms and higher-quality pitch/amp/fuzz DSP. No VST3/Windows work in this subset.
- [x] IR validation, bounded async load, managed/checksummed assets, portable bundle import/export, and synthetic CC0 provenance.
- [x] Evaluate Distortion oversampling; fixed 4× FIR, 8-sample internal dry alignment, compensated stage/rack bypass and fixed 128-sample graph transitions. Existing Overdrive remains native-rate.
- [x] Add conservative software factory examples and measure representative five/16-effect racks and two-graph transitions (unpaced Release DSP benchmark).
- [ ] Owner guitar/tone/aliasing/gain-staging appraisal, new GUI/IR dialog acceptance and listening-based factory tuning. **M5 as a whole is not complete.**

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

M2 progress on 2026-09-30:

- Recorded the owner's native-window and physical Input 2 monitoring reports. Both PipeWire launch settings (256/48000, 512/48000) sounded acceptable; 256 is now the GUI preference and documented launch. Actual configuration, soak duration, measured latency, disconnect/reconnect and other unreported checks remain pending.
- Preserved/integrated the existing uncommitted `Effects`, `LinearRamp` and Gain sources. Implemented a registry, validated UUID/slot identities, independent order list, prepared serial graph and one fixed atomic parameter authority (16 × 16 controls, per-slot bypass, rack bypass). Gain v1 Control 00 maps linearly from normalized values to -60…+24 dB, neutral 0 dB, with 20 ms amplitude smoothing.
- Implemented Add Gain, Remove, Up/Down, per-instance bypass and rack bypass in a native scrolling editor. Existing device selection, explicit Start/Stop/Mute, mono-to-stereo routing, trim/output controls, meters and ±0.98 protection remain in the shared processing path. Added nonfinite DSP-output silence/diagnostics and fixed 512-sample scratch chunking.
- One worker prepares and destroys graphs; a single-pointer mailbox, four-entry retirement ring and strict four-graph ownership bound support at most two processing graphs with 20 ms structural crossfades. Requests coalesce off audio. Activation rejects stale generations and reads current controls; failure rollback retains the accepted destination, including an in-progress fade. Slot reuse waits for graph reclamation. Committed UI state follows audio acknowledgment. Ownership, saturation proof, restart and automation semantics are documented in `docs/architecture.md`.
- Debug/release application and all affected tests build. `ctest --preset linux-debug --output-on-failure` and release equivalent pass 3/3. The new rack suite checks independent amplitudes, monotonic level ramps, stable identities/controls on reorder/removal, noncommuting multiply/offset order, bypass/removal continuity, 1,000 rapid concurrent edits, coalesced handoff saturation, stale blocked builds, preparation exceptions, failure during a fade, slot reservation, bounded ownership and complete off-audio processor destruction. Core and actual standalone callback/processBlock allocation/deallocation audits pass with live racks. See `docs/testing.md` for diagnostic results and commands.
- ASan/UBSan diagnostics pass 2/2 core and 3/3 full core/adapter checks with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer remains unverified. ThreadSanitizer configuration failed because `/usr/lib64/libtsan.so.2.0.0` is missing; no TSan test ran. Fedora command: `sudo dnf install libtsan` (not executed). Release `ldd` has no missing libraries; whitespace checks pass.
- Scope decision: M2 supports zero-latency, zero-tail processors only (Gain); the builder rejects others until later milestones provide dry/transition latency compensation and tail policy. No new effects, preset serialization, host parameter publication, VST3, Windows, packages, commits or global audio changes were added.
- Pending: owner M2 listening checklist, full M1 soak/physical failure tests, actual format/device latency report and measured round trip. The new rack GUI has not been visually verified on the owner's desktop. Sanitizer checks are diagnostics, not deadline/dropout or performance measurements.

M3 progress on 2026-09-30:

- Recorded the owner's exact scope of evidence: they tested the M2 rack with a real guitar, Gain controls work well, and the application appeared correct during their test. This is user-reported listening evidence, without a supplied duration or detailed edit checklist. No unreported soak, reconnect, measured latency, sanitizer, M3 tone or physical acceptance check was completed by that report.
- Added registry-backed Overdrive v1 (native-rate tanh soft clip, one-pole Tone, wet Level, Mix), EQ v1 (150 Hz low shelf / 1 kHz peak / 4 kHz high shelf), and Delay v1 (1…2000 ms interpolated stereo rings, Feedback <=85%, Mix). Controls use conservative defaults, immutable bank mappings and 20 ms smoothing; Delay Time crossfades two bounded read heads for 100 ms, coalescing rapid changes. Gain's published-internal mapping remains unchanged.
- Replaced Add Gain with the Gain/Overdrive/EQ/Delay picker and Add Effect. Cards render all descriptor controls with labels/units/numeric entry, double-click reset and control Reset; variable-height cards remain accessible in the existing vertical viewport. No DSP was added to the UI. Device routing, meters, output protection and Start/Stop/Mute use the unchanged shared audio path.
- Preserved UUID/slot/order separation, sole parameter authority, four-graph ownership, mailbox, retirement acknowledgment and off-audio destruction. Builder now allows finite tails <=120 seconds per zero-latency effect (required for M3 Delay); positive processing latency remains rejected until compensation exists. The shared adapter reports summed conservative tails through a lock-free atomic, including the fading graph. Delay ring allocation/clearing/destruction stays off audio. Structural edits start replacement Delays with empty rings, ramp input written into fresh history over 20 ms to soften leading echo edges, and fade old history for 20 ms; long tails can shorten or wet-only output can dip. Stop/restart clears histories; bypass/Mute keep processing while hiding wet output. This bounded M3 tail policy extends M2's earlier zero-tail restriction without changing ownership.
- Debug/release standalone builds and CTest pass 4/4 each. Full core/adapter ASan/UBSan diagnostics pass 4/4 with `ASAN_OPTIONS=detect_leaks=0`; LeakSanitizer remains unverified. TSan was retried: configure/link failed because `/usr/lib64/libtsan.so.2.0.0` is missing. Exact Fedora command: `sudo dnf install libtsan` (not executed). Commands/evidence are recorded in `docs/testing.md`.
- Signal tests cover Overdrive dry/mix/level/symmetry/finite bounds, EQ measured boost/cut responses and rapid stable coefficient changes, Delay fractional impulse timing/decay/max feedback/time transitions/ring wrap/reset/sample-rate changes, zero/variable blocks and bounded activation. Rack/adapter checks cover all M3 independent identities/controls, real noncommuting Overdrive/EQ order, stateful bypass/removal, failed/stale builds retaining accepted audio/latest controls, 1,000 rapid concurrent mixed-catalog edits, reclamation and zero audited callback C++ heap operations. These checks are diagnostics and signal evidence, not subjective tone, physical driver timing or performance benchmarks.
- No physical audio access: `/dev/snd` is unavailable. Native smoke launch could not produce a window in this restricted session: release exited 139, and XOpenDisplay returned null. No usable desktop or Xvfb is available; M3 UI visual/keyboard acceptance remains pending. Release linked-library and final whitespace results are in `docs/testing.md`.
- Pending: individual/combined dry-recording and live M3 listening, aliasing/tone appraisal, listening-based gain staging, detailed M2 live-edit checks, 10-minute soak, real disconnect/reconnect, actual backend/device format and device latency, measured round trip, TSan and LeakSanitizer. Presets, additional effects, amp/cabinet, VST3 and Windows remain later milestones. No packages, global audio changes, commits, remotes or publication were made.

### 2026-09-30 — M4 presets and persistent state

- Owner update recorded as user-reported evidence: M3 GUI opened on Fedora;
  Overdrive, EQ and Delay work with the real guitar, including together in a rack.
  No duration, reconnect, actual format, measured latency, sanitizer or other
  unreported physical result is inferred from that report.
- Added shared configuration-only `ToneState`, version 1 JSON schema and Clean,
  Crunch and Lead Delay fixtures; the embedded factories are checked against the
  tracked files. Exact version dispatch rejects older/future versions; no legacy
  format or IR/asset support is invented. Strict parsing rejects duplicate keys,
  invalid UTF-8/surrogates, malformed/oversized JSON, non-finite/overflowed numbers,
  wrong types/fields, invalid IDs/slots/order/effects/versions/physical values.
- Native preset bar provides selection, Save, Save As, Import, Export, pending-load
  cancellation and dirty indication. Switching/closing prompts Save/Discard/Cancel;
  factory Save routes to Save As. New preset UUIDs preserve effect UUIDs and slots.
  Imported custom presets can be saved under their existing identity.
- User presets are UUID files under `$XDG_DATA_HOME/GuitarRack/presets` (fallback
  `~/.local/share/GuitarRack/presets`); device/window preferences are versioned
  separately under `$XDG_CONFIG_HOME/GuitarRack/settings.json`. Normal close stores
  the selected saved/loaded tone in `last-tone.json`; Discard does not persist
  unsaved edits. Files use same-directory temporary files, fsync and atomic rename,
  followed by directory fsync; failures are surfaced and do not clear dirty state.
- Preset parsing uses a separate coalescing worker. Complete replacements use the
  existing graph worker/mailbox/four-graph ownership and 20 ms fades. Each graph
  has a bounded private processing view of the sole editable bank, allowing old
  slot meanings to remain intact during preset fades. Globals are converted off
  audio and ramped through existing monitor smoothers. Controls install only at
  activation; selected identity/clean baseline publish after acknowledgment.
  Generation and control-revision checks reject stale results. Transport, Mute,
  routing, meters and protection remain outside portable state.
- Debug/release each pass 5/5. ASan/UBSan (leaks disabled) and newly available
  core/state/adapter TSan each pass 5/5. LeakSanitizer still fails with a fatal
  restricted-process/ptrace inspection error and remains unverified. Exact
  commands, scope and limitations are recorded in `docs/testing.md`. The state suite checks files/Unicode/identities/slots/order,
  dirty actions, failures, cancellation, competing loads, sample-rate rebuilds,
  bounded reclamation, reset-condition DSP equivalence and callback heap audits.
  Existing M2/M3 signal, stress, lifetime, routing and adapter tests are retained.
- No new effects, cabinet/IRs, VST3 publication/host-state integration, Windows,
  packages, audio policies, commits, remotes or publication were added. Physical
  M4 acceptance, 10-minute soak, reconnect, actual latency and other unreported
  checks remain pending. Follow the README M4 checklist on the Fedora desktop.

### 2026-09-30 — M4 saved user-preset deletion

- Recorded the owner's M4 desktop preset workflow report: it works well. This is
  user-reported acceptance only. No unreported soak, reconnect, measured latency,
  detailed keyboard/transition or LeakSanitizer check is inferred.
- Extended the existing uncommitted deletion/session/store changes; preserved
  owner changes and added no effects or later milestones. Native Delete is
  enabled only for a selected managed user preset after transactions settle.
  Dirty Save/Discard/Cancel choices are staged before named confirmation; Cancel
  has initial focus and Enter/Escape shortcuts. Either cancellation preserves
  the state and saved bytes. Failures retain selection, catalog, rack and dirty
  baseline, with a path/permission or validation error.
- The store validates the canonical UUID, rejects factories and symlink/nonregular
  targets, reads/validates the opened managed file, checks entry identity before
  unlink, and updates matching recall first. Staged Save and recall/file changes
  roll back on deletion failure where the filesystem allows it; rollback errors
  are surfaced. No display name/import path selects a deletion target. Other
  presets, exports, settings, factory files and assets are kept.
- Success removes only the selected entry/file, supersedes old parser serials,
  tombstones cached selections, and detaches the identity into an unsaved
  Untitled rack. No graph request, DSP reset or control/device write occurs;
  Save As creates a new saved identity. Matching recall becomes Clean immediately,
  so restart/normal close cannot recall the deleted UUID. The working rack is
  not autosaved when closing with Discard.
- Debug/release standalone builds and regressions pass 5/5 each. Full
  ASan/UBSan (leaks disabled) and TSan diagnostics pass 5/5 each; the separate
  LeakSanitizer retry fails with the restricted-process/ptrace inspection error
  and remains unverified. Deletion tests cover cancellation/dirty choices,
  restart, Unicode/duplicate names, factory/asset/export protection, real
  permission failures/rollback, stale parser/graph operations, stopped/running
  device preservation and sample-exact ongoing Delay output (difference 0).
  Release linked libraries resolve; whitespace checks pass. Commands, scope and
  remaining checks are in docs/testing.md.
  Delete desktop/dialog/keyboard/listening tests remain pending: this session
  has no usable X display or /dev/snd. M5+, unreported soak, reconnect, measured
  latency and LeakSanitizer acceptance remain incomplete.

### 2026-09-30–2026-10-01 — First M5 catalog subset before VST3

- Read root/parent instructions, current plan/readme/architecture/effects/preset/
  testing docs and inspected actual M4 code. Initial git status was clean.
  Preserved the implemented preset-deletion workflow and its acceptance status.
  Owner confirms live Gain/Overdrive/EQ/Delay and M4 presets; no new physical
  acceptance is inferred. Distortion/Reverb DSP/tests were isolated in separate
  detached worktrees, then integrated and reviewed centrally. No commits,
  packages, remotes, publishing or global audio changes were made.
- Added original Distortion v1 (stronger hard clipping/bass voicing, fixed 4× FIR
  oversampling, 8-sample dry alignment), independent stereo algorithmic Reverb
  v1, and direct-head/partitioned-FFT Cabinet v1. Registry/cards/picker use the
  existing fixed bank and smooth controls; Cabinet adds async Load IR and status.
  UUID/immutable slot/type/version/order, routing, gain/meter/protection,
  Start/Stop/Mute, bypass and preset deletion remain integrated in shared DSP.
- Every prepared graph and rack dry path now reserves 128 samples (2.67 ms at
  48 kHz), with stage bypass compensation. Replacements process 128 warm-up
  samples before the 20 ms crossfade. This deliberately adds a small fixed delay
  to earlier empty/M4 racks, avoiding live-edit/preset latency changes. The UI
  and shared processor report it truthfully; device estimates and round trip
  remain distinct. Temporal tails may shorten on structural edits/IR replacement.
- Standard mono PCM16/24/32/float32 WAVs are validated, bounded to 1 MiB,
  8–192 kHz/500 ms/96000 source samples, resampled/prepared off audio, without
  peak/energy normalization. Per-IR destination limit 384000, graph aggregate
  768000 samples, single graph worker and existing four-owned/two-processing
  limits bound resources. Silent/nonfinite/malformed/unsupported/over-budget
  assets fail transactionally. Cabinet does not simulate nonlinear amplification.
- Retained valid M4 v1 presets; introduced v2 checksum/relative asset references
  only where needed. Local managed assets are separate from presets/recall.
  Portable .grbundle includes exact required WAV bytes, strict paths/checksums
  and bounded uncompressed records; import after the original WAV/source store
  disappears is tested. IR edits retain selection/baseline and become dirty;
  failure/cancellation/stale preparation retains prior sound/controls/dirty state.
  Deletion preserves shared IRs; asset cleanup is outside scope.
- Added embedded/tracked original synthetic starter (CC0, explicitly not measured)
  and Driven Synthetic Cabinet / Small Room conservative factory examples.
  No new library dependency. Algorithms, permanent ranges/mappings, state/asset
  format, licensing, callback ownership and limitations are documented.
- Final Debug and Release standalone builds pass **9/9** CTest suites each.
  ASan/UBSan with leaks disabled and TSan each pass **9/9**, with no reported
  diagnostics in exercised paths. Callback new/delete audits pass including
  first use, graph activation/transitions/retirement and oversized adapter calls.
  Source audit finds no C allocation, locks, I/O, lazy DSP resources or destruction
  in the new callback paths. LeakSanitizer remains unverified. Release ldd resolves
  all libraries and git diff --check passes. Exact evidence is in docs/testing.md.
- Release unpaced 48 kHz/256-frame benchmark on i7-9700F: five-effect synthetic
  starter mean 225 µs; same chain with 500 ms IR mean 392 µs; 16 effects/four 500 ms
  IRs mean 1554 µs, p99~2018 µs, two-graph transition max~3498 µs against 5333 µs
  nominal block period. These are synthetic burst wall times, separate from
  sanitizers, not a driver/physical deadline or round-trip measurement.
- Pending: new native GUI/file/dialog/keyboard and live guitar listening,
  detailed tone/aliasing appraisal, factory tuning, physical latency/reconnect,
  soak, LeakSanitizer, real-host/VST3 and Windows. No /dev/snd or usable X display
  is exposed. Gate, Compressor, Chorus, Basic Amp, other modulation/pitch effects,
  VST3 and Windows remain unimplemented; **all of M5 is not marked complete**.

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


## 17. Experimental pedalboard extension — 2026-10-01

Authorized on the separate `feat/experimental-pedalboard` branch: combine a real
pedalboard-style interface with new general-purpose effects, individual pedal
voicings and many complete experimental sounds. Radiohead/art rock is an
inspiration and listening target; no artist or branded pedal emulation is claimed.

- Added eight original processors and Delay v2 without altering the original
  Delay v1 algorithm/keys/normalization. New instances use the latest descriptor;
  restored instances use their persisted version. Host parameter IDs stay fixed.
- Grouped pedal controls validate before mutation. Bounded per-slot coherent
  snapshots retain the previous DSP view if a writer is in flight. No graph
  replacement, temporal reset, I/O or callback allocation is needed for a voicing.
- Native rotary pedals, LED footswitches, signal cables, responsive wrapping,
  complete rack recall, 77 factory pedal voices, personal pedal save/reload and
  40 added full recipes are integrated. Pedal voices are controls only; selected
  Cabinet IR, bypass, order and instance identity stay with the pedal.
- Restored the documented synthetic WAV fixture missing from the GitHub upload;
  the generated bytes exactly match the existing embedded CC0 asset/checksum.
- Verification: ten manual core/state test executables pass with GCC 14.2.0,
  including all 40 new rendered racks and callback allocation audits; strict JSON
  schemas validate all 45 tracked racks and 77 expanded pedal voices. New core
  checks pass ASan/UBSan with leak detection disabled. Both changed UI translation
  units pass compiler syntax checks against the exact pinned JUCE 8.0.15 headers.
  See docs/testing.md for environment, commands and limits.
- The cloud environment lacks CMake/Ninja and native audio/GUI development
  dependencies. Manual compilation verifies affected core/state code, not a
  complete standalone link, a displayed pedalboard, device adapters or hardware.
  Native build/UI/listening, real-time deadline measurements, physical controls
  and pedal voicing transition/tone acceptance remain pending on Fedora.

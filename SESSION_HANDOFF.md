# GuitarRack tone upgrade — current checkpoint

Updated **2026-10-01**, after continuing the 09:14 UTC checkpoint below.
**All remaining independent Linux software work from that checkpoint is now
complete. Native display, physical audio and new listening acceptance remain
pending, with precise limitations in docs/testing.md. Do not restart A–D.**
The earlier handoff below is retained for provenance; its “What remains” list
and running sanitizer/benchmark statements are superseded by this update.

## Final continuation results

- Preserved all prior implementation/owner changes. HEAD is still research
  baseline `3511ab1ea2a49be8a3de6c107b0e07831bb63bdf`; no commits, pushes, package
  installs, remote writes, purchases or global audio policy changes.
- Built final release, debug and diagnostic native standalone applications.
  Latest UI-only logs: `build/{linux-release,linux-debug,adapter-sanitize}/final-layout-build.log`.
  No compiler warnings/errors; release runtime libraries resolve.
- Release **15/15 CTest PASS, 88.77 s**;
  Debug **15/15 PASS, 591.44 s**;
  optimized ASan/UBSan **15/15 PASS, 662.69 s**, including full numerical legacy
  render. Logs: those build directories' `completion-tests.log`.
- The old `-O0` ASan render timed out at 1200 s (14/15 other tests passed).
  Reconfigured diagnostic Debug with `-DCMAKE_CXX_FLAGS_DEBUG='-O1 -g'`, keeping
  assertions and instrumentation, then built and completed all tests. No legacy
  recapture. `ASAN_OPTIONS=detect_leaks=0`; leak detection remains unverified.
- Final selected TSan rebuild **4/4 PASS, 20.93 s** (routing/rack/state/adapter).
  This does not claim full new DSP/UI TSan coverage.
- Added meaningful amp small-signal 82/750/4500-Hz response, Gain harmonics,
  impulse/DC settling and US Combo volume-cleanup checks. Existing Output,
  Master, early Mark Bass, matched-RMS differences and callback checks still pass.
  Pedal report now prints the actual count (146); targeted release/debug checks pass.
- Frozen identities/recipes/control mappings/checksums still pass. Final legacy
  render zero failures, max error **4.99981e-11**, same portable tolerance.
  Generated data/source/docs/schema/WAV regeneration checks **81 files unchanged**.
- Completed both 60-row release benchmark matrices. Cached unchanged pole/rate
  constants and shared per-base-sample amp gain/power math, without changing
  topology, control maps, FIR taps, latency or any legacy DSP source. At 48k/256,
  Modern mean **1614.01→1398.34 µs**, Dual **2132.39→1649.15 µs**.
  Dense/long-IR racks and simultaneous Dual model changes plus graph fade still
  exceed periods; scheduling outliers are retained. No hardware deadline proof.
- Final 168-case alias/reference matrix completed; worst reference convergence
  **−53.2611 dB**, important residual alias ratios unchanged. Noise-floor dB
  differences near −177 dB are documented. New 207-case catalog headroom render
  passed: max output peak **0.21192639**; peak/RMS match prior stored precision.
- Collapsed header now identifies backend/device/channels; below 600 logical
  pixels high, gain/meter rows remain compact even with setup open, preserving
  safety/rack-bypass toolbar space. UI-only edits compiled in all three builds.
  No native screenshot/display, keyboard or HiDPI result is claimed.
- Added the missing current verification anchor/entry and tracked final CSVs,
  CTest summaries/details and evidence notes in `docs/evidence/tone-upgrade`.
  Reviewed README/PLAN/AGENTS/effects/pedalboard/architecture/presets/assets,
  corrected stale current inventory/latency/rights claims, documented fixed
  legacy granular wet-window delay distinctly from intentional echoes.
  `git diff --check` and local documentation link targets pass.

## Current contracts and remaining acceptance

Inventory remains **42 types / 43 descriptors, 69 full sounds, 146 voices**.
Only unchanged Art Rock / Subterranean Pulse is owner approved. All upgraded
listening statuses remain pending. Legacy latency **128**, explicit stopped-state
Creative **4096**; no automatic load-time profile increase. Pitch is a monophonic
starting point with weak close-chord separation/transient smearing and reduced
low-note level. New cabinets are original CC0 synthetic responses, not measured
speaker reproductions. Structural replacements can shorten tails/lose captures.

A fresh display probe still fails (`DISPLAY=:0`, XOpenDisplay false), and
`/dev/snd` is absent. Remaining: native geometry/HiDPI/Escape/focus/dialog/shutdown
acceptance, actual routing/format/reconnect/10-minute soak/round trip, recorded
DI perceived-level listening and measured-cab realism/rights acceptance. See
`docs/listening.md`. No M5-as-a-whole, artist matching, VST3 or Windows completion.
Optional chainsaw/reverse/stutter are deferred, as authorized by the brief.

Current authoritative overview/results: `docs/tone-upgrade.md`,
`docs/testing.md#tone-upgrade-current-verification-2026-10-01`, `PLAN.md`.
Do not blindly repeat passed software checks without new edits or concerns.
If the owner supplies desktop/listening results, record exact conditions and
accepted UUID/recipe, then address concrete issues without migrating originals.

```bash
PIPEWIRE_LATENCY=256/48000 pw-jack ./build/linux-release/GuitarRack_artefacts/Release/GuitarRack
```

Expand Audio Setup, explicitly select existing interface/backend, mono physical
Input 2 and outputs 1/2. M-Track: Instrument and Direct/USB=USB; low headphone
volume, trim 0/output −12, then Start. Choose Preset → explicit Load. For spectral
Pitch/Bloom: Stop → Creative 4096 → Load → Start. Its 48k processor latency is
85.333 ms, separate from device/total I/O. Avoid dense chains; Stop before heavy
Dual model plus structural reconfiguration and validate actual hardware load.
Use identical recorded DI and matched perceived playback level with the listening
worksheet; evaluate CUBE AUX/small speaker separately.

---

# Earlier session handoff — historical checkpoint

Written **2026-10-01**, last status inspection **09:14 UTC / 12:14 Israel**.
The owner requested this file to continue in a new session. This is a handoff,
not a declaration that the whole task is finished. **Do not restart the project
or replace its architecture. Continue the authorized implementation below.**

## Request and authorization

Repository/workspace: `/home/adir/Projects/01_Active/guitar-effects-rack`
(`adirav02/guitar-rack`). Read the original full “GuitarRack: tone and pedalboard
upgrade” brief if supplied to the new session, root `AGENTS.md`, `PLAN.md`, README
and docs effects/pedalboard/architecture/presets/assets/testing before resuming.
Inspect parent/nested instructions and current git status, preserving all owner
changes. Initially the checkout was clean and exactly research baseline
`3511ab1ea2a49be8a3de6c107b0e07831bb63bdf`; current modifications are this local
implementation. Do not discard them or regenerate frozen fixtures to pass tests.

Authorized: local Linux implementation, reversible choices, native builds and
verification, all phases A–D. Excluded: commits/pushes/remote writes, deployment,
package installation/removal, purchases, cloud/mobile, plugin hosting/general
routing, Windows/VST3 implementation. Preserve future latency reporting
compatibility. No additional agents were used or authorized; do not spawn them.
No packages, purchases, commits, pushes or global audio policy changes occurred.

The owner **explicitly confirmed Art Rock / Subterranean Pulse is the previously
approved sound**. Approval applies only to that exact original sound, not the
new catalog. Its exact recipe, UUIDs, controls/output and old synthetic IR are
protected. These are project settings, not historical Radiohead settings.

## Environment and constraints

- Fedora 44, GCC 16.2.1, CMake 4.3.0, Ninja 1.13.2, Python 3.14.7.
- Intel Core i7-9700F, 8 cores, 3 GHz nominal; use actual command output for final
  benchmark provenance, not a hardware deadline claim.
- Local pinned JUCE 8.0.15 in `build/vendor/JUCE`, exact commit
  `91ad83ae34a81e0833b1a2b0866f54846370ae53`. Existing project callback patch stays.
- Native release/debug and diagnostic build directories already exist. No
  dependencies need installing to build here.
- `/dev/snd` absent. `DISPLAY=:0`, but ctypes/libX11 `XOpenDisplay(None)` returned
  null. No usable display connection or Xvfb. Native visual, HiDPI, keyboard and
  real-guitar tests cannot be claimed; leave explicit owner acceptance pending.
- GDB inferior tracing failed: `ptrace: Operation not permitted`. A temporary
  test-only abort backtrace isolated a bug; diagnostics were removed afterward.
- numpy/scipy/jsonschema/clang-format unavailable; don't install packages.
- Writable repo and `/tmp` only. Approval policy never. Do not set
  `sandbox_permissions`. Put logs/evidence in ignored build dirs, not ephemeral
  `/tmp` (temporary files disappeared across an earlier steering turn).
- Communicate concise progress, roughly every minute. User wants action, not a
  new plan or repeated approval questions. Physical/listening limitations are
  acknowledged; they don't block independent software work.

## Implemented compatibility and stable identity

- `tests/fixtures/legacy/library.json`: frozen 77 original pedal recipes and 40
  experimental racks with original UUID/instance/slot/order recipes.
- `registry.json`: all 16 original exact descriptors (15 types, Delay v1/v2),
  control indices/ranges/defaults/enums.
- `checksums.json`: original 45 preset files and original IR byte checksums.
- `render.txt.gz` (~9.6 MiB): deterministic numerical render metric baseline for
  all 45 legacy racks, 44.1/48/96 kHz, impulse/decaying pluck/chord multitones/
  silence/parameter+effect+rack bypass transitions, 1.5 seconds each.
- Capture occurred against original pre-upgrade libraries. Initial RackEngine
  harness showed worker timing variation; final harness prepares deterministic
  serial DSP chains directly. It compares every 256-sample stereo block's mean,
  RMS, fixed projection and peak, rather than storing every WAV sample. Engine
  lifecycle/transaction tests separately verify the actual rack. No Monitor clamp.
- Portable tolerance: absolute `2e-6 + 2e-5 * abs(reference)` per metric. Latest
  release comparison: **zero failures, maximum absolute error 4.99981e-11**.
  This is numerical software evidence, not listening or full WAV identity proof.
- `tools/legacy_render.cpp`, `check_legacy_render.py`, `check_compatibility.py`.
  Compatibility check also verifies full metadata coverage and that only
  Subterranean has a listening approval entry.
- Protected preset UUID: `50000000-0000-4000-8000-000000000008`; old IR SHA-256:
  `4a9ac650b2983ca0c49c138d11beccd2d0c8607ac9fccf47c537fbbf7bb7adae`.
- `assets/sounds/library.json` uses explicit canonical UUIDs, `instance_ids` and
  `pedal_ids`. Recipes reference voicing UUIDs, no longer names/array positions.
  Existing mappings unchanged. New additive UUID5 seeds use immutable recipe keys.
- `tools/generate_sound_library.py` emits `SoundLibrary.cpp`. Exporter compares
  and refuses to rewrite changed legacy experimental files, preserves old bytes,
  emits new fixtures with LF. Registry export/schema updater cover all descriptors.

## Implemented UI/state

- `PresetBrowser.h/.cpp`: one safely owned native resizable DocumentWindow per
  PresetBar; close hides, reopen focuses. No blocking modal loop or audition.
  Token search over names/tags/inspiration; Factory/User/Favorites, artist/style
  and gain filters. UUID favorites, metadata, chain/version/bypass and cabinet
  availability details. Explicit Load/double-click/Enter uses existing session
  dirty guard and asynchronous transaction. Progress/cancel/errors visible.
  Browser/text editor Escape closes it, without invoking main Stop.
- PresetBar: compact current name/dirty/status, Choose Preset, Save/Save As;
  browser forwards existing import/export/bundle/delete workflows and guards.
- Main: Audio Setup defaults collapsed, layout-only toggle; Start/Stop/Mute,
  trim/output/meters/clipping/rack bypass/status remain available. Collapsed
  gain and meter rows are now inline to free board space. Config details tooltip.
- Minimum main **700×450 logical**, browser **700×400**, defaults1280×720;
  initial windows constrained to available display. Old1030 height trap removed.
- Local preferences strict v2 has **24 exact fields**: original device/config/
  geometry plus collapse, browser rect, board scroll, search/source/artist/gain,
  favorites/expanded UUID arrays (unique canonical, bounded512) and explicit
  latency profile128/4096. V1 preserves device choices and caps old height768,
  defaults Legacy128. Never enters portable tones/dirty comparisons.
- Explicit chosen profile is remembered locally and restored while stopped
  before startup tone graph submission; normal startup still does not open input.
- Cards Basic/All controls retain first controls plus Gain/Drive and Output/Level,
  expression and independent Dual Gain B. Expansion and reorder scroll anchors
  are instance UUIDs, not card indices. Latest layout uses 64-pixel control rows,
  base146 height; engagement sits beside expansion, Save Voice footer. Matching
  current values show a voicing name without applying/dirtying parameters.
- Tap BPM on new Echo card, coherent BPM/clock update,250–1500ms interval.
- Cabinet menu selects unchanged original or five new synthetic assets through
  `PresetSession::selectBuiltinIR`; transaction preserves selection association,
  slot/order/other pedals and marks tone dirty. User IR file chooser retained.
- Native display acceptance is still **pending**. Latest UI-only changes compile
  in release/debug; no screenshots were possible. ASan app rebuild for these last
  UI-only edits remains to do if desired (core diagnostic tests use same DSP/state).

## Implemented DSP (legacy sources untouched)

42 total types /43 exact descriptors,69 complete racks,146 pedal voicings.
All new IDs below version1; old topologies/versions remain executable unchanged.
Exact index/range/default/enums in `docs/tone-controls.md`, generated from registry.

- `TonePrimitives.h`: double RBJ biquads, smooth coefficients, poles, fixed
  controls; new selected **8×129-tap Blackman sinc pair**,16 base samples latency.
  Model filters at actual internal rate; antialias filters separate. Earlier
  4×65 candidate measured and saved; no ADAA/circuit solver/neural dependency.
- `Amplifiers.cpp`: eight separately ordered/filtered/dynamic original networks:
  `amp_solid_clean`, `amp_us_combo`, `amp_brit_chime`, `amp_brit_crunch`,
  `amp_brit_hotrod`, `amp_mark`, `amp_modern_tight`, `amp_recto`.
  SS headroom/bright; US early tone/soft asymmetric pair; Chime interacting tone
  and meaningful late Cut; Crunch two mid-focused stages; HotRod extra filtered
  stage; Mark early Bass and separate Input/Lead Gain; Modern repeated interstage
  bass rejection/low sag; Recto broader envelope bass dynamics/bias/sag.
  Master changes power compression, independent Output after. Not measured circuits.
- `Drives.cpp`: `asymmetric_drive`, `oxford_distortion`, `hard_distortion`,
  `sustain_fuzz`, `bias_fuzz`, `tight_boost`. Independent gain/output, distinct
  filtering/clipping/compression. Bias cutoff/gate/bounded feedback≤0.65,
  Stability100 normally feedback off; no free oscillator/FuzzFactory equivalence.
- `ToneEQGate.cpp`: `graphic_eq`80/240/750/2200/6600 broad peaks+Output;
  `parametric_eq`3freq/gain/Q bands+HP/LP+Output; `noise_gate`threshold/range/
  attack/hold/release/hysteresis, independent linked stereo detector. Gate normally
  pre-amp/ambience; no hidden input-keyed downstream path.
- `CreativeEffects.cpp`: band-pass manual `wah`, distinct `envelope_wah`,4/6
  all-pass `phaser`, short fractional `flanger`, `swell`, `audio_hold`.
  Hold captures500ms,25ms seam overlap,40ms release, expires within120sec and
  requires Off→Hold to rearm. No audio history copies on structural activation.
- `Echo.cpp` ID`echo`: ms/BPM/subdivision/tap,digital/analog/tape, color/wow,
  stereo/pingpong, optional heads1/.75/.5 weights.6/.25/.15; coalesced100ms
  read-head fades. Maxfeedback70% (new type only) keeps4sec repeat tail within120s.
- `SpaceReverb.cpp` ID`space_reverb`: prepared4-line room, diffused6-line plate,
  8-line Hadamard hall, length-dependent RT60, predelay/damping/wet tone. All three
  networks run for smooth mode changes. No spring claim; tail40sec.
- `ExpressionPitch.cpp` ID`expression_pitch`: fixed2048 FFT/H256, bounded64
  sinusoidal peaks,±24st/detune/software expression/Mix/output. A monophonic
  starting point, no chord-quality/Whammy/under3ms claim. Initial stretched-spectrum
  +24 cancellation fixed by narrow Hann-lobe sinusoidal reconstruction.
  Held-note48k24-case grid error0cents at1-cent search resolution; useful amplitude.
  96k110Hz→27.5Hz amplitude0.05656 from0.1 input (reduced level); close chord
  octave projections only~0.00077–0.00206 from0.035 components (poor separation).
  Bends finite/peak0.100067. Neutral/dry exact2048 delay. Octave impulse onset634,
  peak2159 at48k before rack padding: report wet transient smearing separately.
- `dual_amp`: two prepared branches,8 family choices, independent gain/master/
  levels, blend, shared tone/output,14controls. Shared oversampler,16latency,
  at most two active models per branch during20ms model changes. One downstream
  cabinet approximation, no hidden asset paths or double-tracking claim.

### Latency/lifetime

- `StereoDelay` reserves fixed4096/channel storage. Legacy budget stays128,
  Creative4096 explicit/stopped state; serial sums/dual branch maximum; reject
  incompatible loads/profile changes transactionally, preserve accepted identity/
  dirty state. Processor adapter declaration updates outside callback.
- Warm fresh long rings for profile budget, then20ms two-graph fade. Only rings
  ≤128 samples inherit history; no huge callback copies. Existing maxfour owned/
  two processing graphs, bounded handoff/coalescing/reclamation/off-audio destroy.
- At48k128=2.667ms,4096=85.333ms,256deviceframes=5.333ms per period, none total I/O.
- New tail declarations: amp/drive/dual0.5s, EQ1s, gate2s, wah/envelope/phaser/
  swell1s,flanger2s,Echo/Hold120s,Space40s,Pitch0.6s. Rack sums bounds.
- Voicing updates retain history. Structural changes can shorten tails and lose
  held captures. No GUI DSP/sample access; all resources prepared off audio.

### Diagnostic bug found/fixed

Debug/ASan exposed new Space predelay wrap:18ms×48k rounding yielded a tiny
negative position that rounded to exactly ring size after adding size, indexing
one past storage. Fixed upper wrap boundary in new Space/Echo/Flanger readers;
added18ms predelay regression. Temporary abort logging/backtrace removed.
Legacy DSP files not edited. Initial debug/ASan failures are superseded by
corrected runs, but final evidence must honestly mention fix and timeout changes.

## Cabinets and curated library

- Five original analytic CC0 WAVs in `assets/ir`: clean-open2x12,chime2x12,
  british4x12,tight-modern4x12,dark-full4x12. Mono float32,48k,2048taps;
  normalized1k magnitude1 before float rounding. Origin/license/rate/SHA/recipe
  in `catalog.json`, embedded bytes in`CabinetAssets.cpp`; no third-party recordings.
  Original synthetic bytes/checksum unchanged. No measured speaker realism claim.
- AssetStore recognizes exact builtins; user checksum loading/resampling/bundles
  remain. Unknown/missing/corrupt asset errors, no fallback.
- New response tests:1k normalization,low20Hz/high15k rejection,distinct response
  shapes. Allfive embedded cabinet bundle roundtrips exact bytes/identity pass.
- New24 sounds cover Radiohead/Muse/Metallica/Rock/Metal palette; new69 pedal
  voices including amp clean/edge/rhythm/lead. No scenes/sections/song-part state.
  Original starting recipes, not copied historical knob values. Distinct new
  D-standard Recto low-string voice and US-combo slow-chorus clean avoid two
  initial exact duplicate recipes. No chainsaw/reverse/stutter implemented (optional).
- Strict separate `assets/sounds/catalog.json` version1 covers all69 factory IDs:
  tags/artist/style/gain/character/tuning/pickups/input/amp/cab/notes/evidence/listening.
  Only protectedSubterranean approved; all new listening pending.
- New output levels calibrated on common synthetic phrase towardRMS0.015,
  AmbientBloom0.003. Most match closely; US chorus/Dstandard~0.0116 at scale1.
  This is RMS/headroom calibration, not perceived-level listening approval.
  All legacy gains unchanged. New all69×three scales audit207 cases48k,
  input measured peaks0.028879/0.096263/0.288790, bounds0.0426/0.142/0.426.
  Prior final audit max outputpeak~0.21193; recompute from finalCSV to quote exactly.

## Builds/tests/evidence at handoff

Latest result files under ignored build dirs; tracked CSVs under
`docs/evidence/tone-upgrade/`. Do not confuse historical docs with fresh evidence.

- Release native build PASS; **15/15 CTest PASS**,79.47sec:
  `build/linux-release/final-tests.log`, `Testing/Temporary/LastTest.log`.
  Full run after wrap/state/profile fixes; subsequent new gate/cab response/bundle
  checks separately **3/3 PASS**,5.56sec (`asset-upgrade-tests.log`).
- Debug native build PASS; **15/15 PASS**,559.68sec:
  `build/linux-debug/final-tests.log`. Additional latest asset/tone/identity checks
  **3/3 PASS**,14.75sec (`asset-upgrade-tests.log`).
- Latest compact UI/native app changes built release/debug, no warnings in
  `layout-build.log`. No visual execution possible. `ldd` showed no missing libraries.
- ASan/UBSan full native diagnostic build PASS (`adapter-sanitize`). Corrected
  CTest run **STILL RUNNING at last inspection**,passed tests1–10 including
  Experimental/ToneUpgrade/Creative, currently test11 legacy full render.
  `build/adapter-sanitize/final-tests.log`, exec session**73132** if available.
  ASAN_OPTIONS=`detect_leaks=0`; do NOT claim leak detection or full15 yet.
  Additional latest asset/tone test targets compiled (`asset-upgrade-build.log`)
  but their sanitizer checks still need running after this CTest finishes.
- Legacy full render timeout increased300→1200sec: debug/sanitizers slower and
  previous run contended with other builds/renders. Old timeout+assert failures
  in `adapter-sanitize/initial-upgrade-tests.log` and debug`upgrade-tests.log`.
  Latest debug full run passed, so no baseline recapture was needed.
- TSan **4/4 selected suites PASS**,20.10sec: routing,rack,preset/device adapter.
  `build/core-thread-sanitize/upgrade-tests.log`; latest core/state sources built.
  No reported race. Do not imply every new DSP test ran under TSan.
- Legacy identity/recipe/IR/control compatibility PASS throughout. Render zero
  failures/max4.99981e-11. Heap audit covers callback allocations/deallocations,
  new first callbacks/activation/extremes/all three rates, variable/zero blocks;
  existing tests cover oversized blocks, queue/lifecycle/fades/transactions.
- Tone tests: matched-RMS amp minimum waveform pair difference**0.0641125**,
  Output scaling/Master harmonics/earlyMarkBass/graphic center/gate timing+hysteresis,
  finite output without Monitor clamp and builtin responses/checksums.
- Creative tests: subdivision/pingpong/flanger timing,wah,response networks/RT60/
  predelay/holdexpiry,pitch notes/endpoints/rates/bends/chord artifacts/latency,
  Creative profile failure retention, identicalDualbranch polarity/50/50 scaling.
- Alias matrices completed,168cases each,48k coherent frequencies~304.6875/3000/
  9000,peaks0.03/0.3,default/maxDrive for8amps+6drives. Same model references
  at8×/16×base (internal oversampling still enabled). Selected8× reference
  worst magnitude convergence−53.26dB. Excludes intended below-Nyquist harmonics;
  nonstationary/model transient contribution means this isn't pure alias proof.
  `alias-4x65.csv`, `alias-8x129.csv` tracked in docs evidence. Worst examples:
  HardDist4×−17.83→8×−25.14dB; SustainFuzz−17.56→−29.38; Chime−20.40→−26.45;
  ModernTight−23.61→−32.52. Residual aliases documented, no alias-free claim.
- Final headroomCSV207cases completed AFTER wrap fix:
  `build/linux-release/catalog-headroom-final.csv`, copied to tracked
  `docs/evidence/tone-upgrade/catalog-headroom.csv`. No nonfinite/callback heap
  errors. Catalog audit uses offline autoCreative only for pitch, never app policy.
- Offline tool command smoke PASSED: generated syntheticDI,renderedOxford
  peak0.055097/RMS0.0150247 andBloomCreative peak0.0178438/RMS0.00300268.
  WAVs ignored in build dir. Eight-second render tail, float32/no Monitor clamp.
- `git diff --check` PASS at last inspection.

## Tools and generated source commands

```bash
python3 tools/generate_sound_library.py
python3 tools/generate_sound_catalog.py
cmake --build --preset linux-release
./build/linux-release/guitarrack_export_sounds . > build/linux-release/registry.json
python3 tools/update_sound_schemas.py build/linux-release/registry.json
python3 tools/generate_effect_docs.py build/linux-release/registry.json
python3 tools/check_compatibility.py build/linux-release/guitarrack_export_sounds
```

Library JSON is authoritative after initial creation. `add_upgrade_library.py`
is idempotent additive seeding, not a replacement for edits. Cabinet generator
only rewrites five new original assets, never legacy WAV. **Do not recapture
legacy fixtures**. Registry changes require source/build/schema/fixture review.

Offline tool `guitarrack_render`:
`--synthetic new.wav`, `--catalog-audit new.csv`, or
`<factory UUID or full name> mono-DI.wav new-output.wav [--creative]`.
Input monoPCM16/24/32/float32,8–192k,≤120sec/256MiB; outputs won't overwrite.
Synthetic8sec phrase is original mathematical signal, not a recorded guitar.

## What remains, in order

1. Inspect running processes/logs. Let corrected ASan/UBSan full CTest finish;
   record actual outcome (currently legacy render test11). Exec session73132
   may still be resumable; new session can inspectlog/`ps ... comm` instead.
   Run latest additional sanitizer asset/tone checks after it finishes:
   `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/adapter-sanitize -R
   'toneupgrade_signals|ir_assets_and_bundles|legacy_identity_compatibility'
   --output-on-failure`. Optionally rebuild latest UI-only app target under ASan.
   If a diagnostic fails, fix code and rerun relevant checks; don't hide/recapture.
2. **BENCHMARK MATRIX NOT YET RUN.** Wait until all diagnostic/build/render CPU
   work finishes. Run alone in Release:
   `./build/linux-release/guitarrack_catalog_benchmark --matrix
   build/linux-release/benchmark-matrix.csv`.
   Target already built after latest benchmark code change.60rows:44.1/48/96k ×
   64/128/256/512frames ×emptyLegacy/ModernRhythm/Dualsharedcab/SpectralAmbient/
   dense16(with four500msIRs).200warm+1000samples,mean/p99/max and two-graphfade
   CPU. Dual fade additionally changes both branch models, bounding four models
   per graph during overlap. Synthetic costs aren't hardware deadline proof.
   Copy CSV to docs evidence; summarize representative/worst rows and latency.
   If representative chains exceed practical budgets, investigate/optimize
   without touching legacy DSP contracts; document dense pathological limits.
3. Finish **docs/testing.md current verification entry**, which has NOT been
   added yet. README/PLAN link to `#tone-upgrade-current-verification-2026-10-01`
   currently has no target. Add fresh commands, toolchain/log results, timeout+
   fractional-read fix, alias/convergence/pitch artifacts/headroom/bench limits,
   callback inspection and exact blockers. Copy concise test/render logs or
   evidence summary under tracked docs/evidence. Do not fabricate owner listening,
   native screenshots/hardware, LeakSanitizer/fullTSan/Windows/VST3 results.
4. Review documentation against actual final code/results. New
   `docs/tone-upgrade.md`, `tone-controls.md`, `listening.md` exist. Updated PLAN,
   AGENTS,README and docs effects/pedalboard/architecture/presets/assets plusIR
   README. Historical descriptions remain as legacy references with superseding
   links, but review stale current-tense claims. PLAN A–D software boxes reflect
   implementation; hardware/display/listening boxes remain unchecked. Update
   final status accurately, never declare M5 or artist matches complete.
5. Consider remaining targeted coverage only if justified (do not endlessly
   repeat tests). Last new UI layout/voicing-name changes only compiled, visual
   checks unavailable. No need to claim screenshots. Source review browser
   ownership/Escape/async shutdown and current geometry is useful; actual desktop
   checklist remains for owner. Recheck schemas/generated data deterministic,
   legacy checksums/UUIDs and `git diff --check` after any further edits.
6. Final local handoff: review gitstatus/diff for accidental/debug files, retain
   all changes; no commit/push. Include concise implementation/test summary,
   exact Linux launch and DI/listening steps, pending physical/native/listening
   and pitch/synthetic-cab limitations. User wanted all authorized phases, so
   don't stop at a plan. Physical acceptance can remain explicitly pending.

## Documentation/research and owner launch

Primary sources consulted read-only: officialMesaMarkIIC+ manual; firsthand
Bellamy interview; EdO'Brien BOSS account; Fidelman72Seasons and Hetfield/Hammett
BlackAlbum interviews; officialMarshall2203 controls; officialJUCE8.0.15.
Links/limited topology interpretations in `docs/tone-upgrade.md`. No internet
knob recipes or paid-video claims. No album/circuit authenticity claim.

```bash
PIPEWIRE_LATENCY=256/48000 pw-jack ./build/linux-release/GuitarRack_artefacts/Release/GuitarRack
```

ExpandAudioSetup; choose existing backend/interface,Input2/outputs1+2,Instrument,
Direct/USB=USB,low headphonevolume,trim0/output−12,thenStart. ChoosePreset and
explicitLoad. Pitch/Bloom:Stop→Creative4096→Load→Start. Perceived-level A/B on
identical recordedDI,soft/hard picks/chords/palmmutes/bends/harmonics/cleanup/
decays/silence; recordinstrument/pickups/input/outputdevice. CUBE10GX AUX/small
speaker evaluated separately from headphones/monitors, no automatic cab bypass.
See `docs/listening.md` for worksheet and native desktop checklist.

# Verification record

## Tone upgrade current verification 2026-10-01

This is fresh local Linux verification of the additive A–D implementation on
baseline `3511ab1ea2a49be8a3de6c107b0e07831bb63bdf`. Earlier dated records below
are historical. All owner changes are retained; no commit/push, package install,
asset purchase, deployment or global audio-policy change was performed.
Only the exact legacy Art Rock / Subterranean Pulse has explicit owner listening
approval. New catalog, native display and physical acceptance remain pending.

### Environment and reproducible checks

Fedora 44 x86-64; GCC 16.2.1 (Red Hat 16.2.1-2), C++20, CMake 4.3.0,
Ninja 1.13.2, Python 3.14.7. Intel Core i7-9700F, eight cores, 3 GHz nominal
(dynamic frequency/scheduling). Local JUCE 8.0.15 commit
`91ad83ae34a81e0833b1a2b0866f54846370ae53`, with the existing checked callback
patch; browser/curl/OpenGL remain disabled. No missing linked release libraries.
Optional GL/WebKit/GTK configure probes do not require installing those packages.

```bash
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure

# Diagnostic optimization retains assertions (no NDEBUG), and avoids the
# prohibitively slow unoptimized, instrumented full legacy-render matrix.
cmake -S . -B build/adapter-sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug '-DCMAKE_CXX_FLAGS_DEBUG=-O1 -g' \
  -DGUITARRACK_SANITIZERS=ON
cmake --build build/adapter-sanitize -j 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/adapter-sanitize --output-on-failure
```

Completion results: release native build and **15/15 PASS, 88.77 s**; debug
native build and **15/15 PASS, 591.44 s**; optimized ASan/UBSan native build
and **15/15 PASS, 662.69 s**. Address/undefined checks report no findings.
The final responsive device-header/UI-only edits subsequently compiled in all
three configurations; native visual execution remains unavailable.
Final full-suite logs and signal output are retained in
[evidence](evidence/tone-upgrade). The pedal persistence test also passed after
its console report was corrected to show the actual 146 voices tested.

Selected TSan checks were rebuilt with the final DSP sources and passed **4/4,
20.93 s**: monitoring/routing, serial rack lifecycle, preset persistence and actual
adapter callback lifecycle. Configuration: Debug, `-fsanitize=thread
-fno-omit-frame-pointer` compile and `-fsanitize=thread` link. This is selected
race/lifetime coverage, not every DSP/UI suite under TSan.

Initial debug/ASan runs exposed a new fractional predelay read wrap: adding ring
size to a tiny negative floating position rounded to exactly size. New Space,
Echo and Flanger readers now check that upper boundary; an 18-ms predelay
regression passes. Temporary backtrace/abort logging was removed. No legacy DSP
or frozen fixture was changed to fix it. Earlier ASan `-O0` full legacy render
then timed out at 1200 seconds (14/15 other tests passed); it was a timeout,
not a completed numerical comparison. The diagnostic `-O1` run supersedes that
result only where its completed results are stated. Leak detection remains
unverified (`detect_leaks=0`); sanitizer wall times are not performance evidence.

### Compatibility, identity, assets and state

Before expansion, fixtures froze all 45 legacy rack IDs/recipes/slots/instances/
orders, 77 pedal recipes, 16 exact registry descriptors (15 types, Delay v1/v2),
all original control ranges/defaults/enums and original cabinet bytes. The
portable numerical fixture covers 44.1/48/96 kHz × all legacy racks × impulse,
decaying pluck, chord multitone, silence and parameter/effect/rack bypass
transitions, 1.5 seconds each. It compares each 256-sample stereo block's mean,
RMS, fixed projection and peak in deterministic direct DSP chains. Actual
RackEngine acceptance, fades, state and lifetime have separate tests.

Tolerance per metric is `2e-6 + 2e-5 * abs(reference)`, allowing portable
float/libm/compiler rounding below ordinary audible changes. Release comparison:
**zero failures; maximum absolute error 4.99981e-11**. Metrics are not a stored
sample-for-sample WAV or listening certificate. Original cabinet SHA-256 remains
`4a9ac650b2983ca0c49c138d11beccd2d0c8607ac9fccf47c537fbbf7bb7adae`.
No fixture recapture occurred. Subterranean identity/output/chain/all controls
remain exact; approval was not transferred to upgraded entries.

Deterministic regeneration checks **81 generated source/schema/doc/preset/WAV
files byte-for-byte unchanged**; the separate identity/control/byte check passes.
Use the commands in [tone-upgrade.md](tone-upgrade.md). All 69 complete racks and
146 voices round-trip through strict runtime validation. Preferences v1→v2,
unknown fields/schema, UTF-8, factory protection/delete, dirty Save/Discard/Cancel,
failed/stale/cancelled loads, stopped startup and incompatible latency profiles
are covered. Required assets fail without fallback; all five new builtin bundles
round-trip exact bytes. IR invalid/missing/checksum/resampling/resource bounds
remain covered. The new 1-kHz-normalized synthetic responses have distinct
frequency shapes and sub-bass/high-frequency rejection; CC0 provenance is in
assets/ir/catalog.json. This establishes no measured speaker realism.

### Signal and callback findings

Eight amp families are separately ordered/filtered nonlinear networks. Tests
measure small-signal responses at 82/750/4500 Hz, gain-dependent third harmonics,
impulse settling/DC, independent Output scaling, Master harmonics, early Mark
Bass versus late EQ, and US Combo input-volume cleanup. See
[amp measurements](evidence/tone-upgrade/amp-response.csv). Minimum pairwise
waveform difference after matching RMS is **0.0641125** at the tested 211-Hz
signal; that is demonstrable signal difference, not perceived character approval.
Gate tests cover opening, hold/release decay, attenuation and hysteresis; its
linked detector precedes normal amp/ambience paths. Graphic EQ center gain and
cabinet responses are measured. New DSP is checked without Monitor protection
at default silence, extremes and 44.1/48/96 kHz.

Wah changes band-pass resonance; Echo dotted-eighth timing at 120 BPM and stereo
ping-pong impulses pass; Flanger's 2-ms impulse timing passes. Room/plate/hall
excite distinct networks, decay changes late energy, and predelay onset is tested.
Hold repeats captured audio, releases and expires by its declared 120 s without
retriggering while held On. Identical Dual Amp branch endpoints and 50/50 blend
match exactly in timing/polarity/level. These are behavior checks; musical
appraisal of feedback, damping, phasing and changing controls remains listening
work.

HeapAudit reports zero C++ heap operations in first callbacks, zero/variable/
oversized blocks, activation, extreme controls and actual adapter callback tests.
Source inspection covers all new `process`/model/FFT paths and bounded graph
activation/reclamation: no direct C allocator, lock, I/O, GUI, construction,
lazy static preparation or final shared-pointer release there. Coefficients,
tables, rings and graph resources prepare off audio; only small fixed histories
copy at activation. Four graphs maximum owned, two processing, one coalesced
request/mailbox, bounded retirement and worker destruction remain. No huge ring
copy preserves old tails: structural edits may shorten tails/lose held captures.
This does not instrument driver/server internals or prove physical deadlines.

### Aliasing, pitch and render limits

Three 168-case 48-kHz coherent matrices are retained: earlier 4×/65-tap,
selected 8×/129-tap, and final cached-math 8×/129-tap. Eight amps and six drives
use ~304.6875/3000/9000 Hz × input peaks 0.03/0.3 × default/max Gain/Drive.
References run the same model at 8× and 16× base rate with internal oversampling
still active. Final worst reference magnitude convergence is **−53.2611 dB**.
Intended below-Nyquist harmonics are excluded with ±3-bin neighborhoods;
nonstationary/filter settling energy remains, so nonharmonic energy is not a
pure alias measurement or complete converged circuit reference.

Worst candidate→selected ratios to fundamental: Hard Distortion −17.83→−25.14 dB,
Sustaining Fuzz −17.56→−29.38, Chime −20.40→−26.45, Modern Tight −23.61→−32.52.
Residual aliases remain. Final caching preserves these results; the largest
reported dB difference (0.289 dB) is reference energy near −177 dB, a numerical
noise-floor effect. No alias-free claim, ADAA solver or measured-circuit claim.

Pitch's 24 held-note cases at 48 kHz (82.4069–1760 Hz × ±12/24 st) measure
**0 cents at 1-cent search resolution**, with useful fundamentals. Other-rate
endpoints pass, but 96-kHz 110→27.5 Hz loses level (0.0565563 from 0.1 input).
A bend stays finite/peak 0.100067; no moving-note cents guarantee is derived
from that bound. Close-chord octave component amplitudes are only
0.00201635/0.000765009/0.00206082 from 0.035 inputs: poor separation. Octave
impulse onset/peak are **634/2159 samples at 48 kHz**, versus exact neutral/dry
2048-sample alignment. Attacks smear. There is no chord-quality mode, Whammy
identity or transparent low-latency claim. Legacy granular window artifacts and
8–118-ms algorithmic wet delay remain unchanged, separate from intentional echoes.

Processor latency remains Legacy **128 samples** or explicit stopped-state
Creative **4096**, never increased automatically by a load. At 48 kHz these
are 2.667/85.333 ms; a 256-frame device period is 5.333 ms. Neither is measured
total I/O. Serial sums, parallel maximum, per-effect/rack bypass alignment,
profile rejection/retention and bounded warm/fade transitions are tested.

Final catalog render covers all 69 racks × three input strengths at 48 kHz,
207 cases, no nonfinite output or callback heap error. Measured input peaks are
0.028879/0.096263/0.288790 (bounds 0.0426/0.142/0.426), not calibrated pickups.
Maximum output peak is **0.21192639**; final peak/RMS metrics are unchanged
from the pre-optimization audit at stored precision. Details are in [headroom CSV](evidence/tone-upgrade/catalog-headroom.csv).
New output calibration toward phrase RMS 0.015 (Bloom 0.003) is technical level
matching, not perceived loudness approval; all legacy gains remain unchanged.
The offline DI/synthetic/render commands passed, with an eight-second tail,
float32 stereo, retained processor delay and no Monitor clamp. A synthetic DI
is original mathematical audio, not a recorded guitar.

### Synthetic performance matrix and practical limits

Release benchmark ran alone, before diagnostic/build/render CPU work, 60 rows:
44.1/48/96 kHz × 64/128/256/512 frames × five scenarios. Each uses 200 warm
blocks, 1000 timed blocks and a bounded warm-up/20-ms two-graph transition probe.
These are default processor chains, not calibrated artist patches. Dense 16
uses four 500-ms IRs. Dual fade also changes both branch models: up to four
models per graph while two graphs overlap. Input generation is included; no
physical device, Monitor clamp or GUI. CSV retains mean/p99/max and fade
mean/p99/max; fade p99 often equals max because the probe has few blocks.

| 48 kHz / 256 frames | Mean µs | p99 µs | Max µs | Fade mean / max µs |
|---|---:|---:|---:|---:|
| Empty Legacy | 3.90 | 4.18 | 36.73 | — |
| Modern Rhythm | 1398.34 | 1576.12 | 1801.61 | 2467.01 / 3012.22 |
| Dual Amp, shared cab | 1649.15 | 1868.02 | 1927.87 | 4167.16 / 5736.21 |
| Spectral Ambient, Creative | 1499.88 | 1641.08 | 1800.80 | 2821.99 / 3050.67 |
| Dense 16 / four long IRs | 5658.20 | 6122.43 | 6394.90 | 9800.97 / 11718.96 |

Caching unchanged pole/rate coefficients and sharing amp derived controls per
base sample reduced 48-kHz/256 means from 1614.01→1398.34 µs for Modern and
2132.39→1649.15 for Dual. Topologies/control maps/taps/latency stay unchanged.
[Before](evidence/tone-upgrade/benchmark-before-optimization.csv) and
[final matrix](evidence/tone-upgrade/benchmark-matrix.csv) are retained.

**Some racks and transitions exceed block periods.** Dense's worst mean is
260.14% of a period at 96 kHz/256; p99 reaches 447.52% at 96 kHz/64. Even at
48 kHz/256 Dense exceeds the 5.333-ms period, and simultaneous Dual model changes
plus structural fade peak at 5.736 ms. Scheduling outliers include a 45.846-ms
Dual steady block and 65.282-ms Dense fade at 96 kHz. These are measured wall
times, including scheduling effects, not CPU-cycle certificates. The 16-slot
ceiling is not a playable worst-case guarantee. Avoid dense oversampled/long-IR
chains; for heavy Dual reconfiguration, Stop first and verify actual hardware
with the chosen buffer. Increasing a buffer cannot fix mean workload that
already exceeds its period proportionally. Physical deadline/xrun acceptance
remains pending; no synthetic result guarantees it.

### Native/physical/listening acceptance

A fresh `DISPLAY=:0` XOpenDisplay probe fails; no usable display/Xvfb, and
`/dev/snd` is absent. Native release/debug/diagnostic builds do not establish
rendered layout, keyboard/Escape/focus, HiDPI/150%, browser reopen/shutdown or
real-guitar behavior. Source review confirms owned hide/reopen browser,
asynchronous safe callbacks, local-only collapse and UUID expansion/scroll.
Below 600 logical pixels high, gains/meters stay in compact rows even when Audio
Setup is open, retaining space for the rack bypass toolbar; details stay in
tooltips. This source/layout review does not substitute for rendered acceptance.
Actual 1280×720, 1366×768, wide/HiDPI visual checks and dialogs remain pending.
Use [listening/desktop worksheet](listening.md) and record the actual guitar,
pickups/input calibration, backend/rate/buffer, output device, perceived-level
A/B and round trip/soak/reconnect. Evaluate CUBE AUX/speaker separately.

Only exact Subterranean listening approval is recorded. All upgrades, measured
cab realism, new hardware/UI acceptance, leak detection, full DSP TSan, real
host/VST3 and Windows remain unverified. M5 as a whole and artist matches are
not complete. The software phases and explicit limits are reflected in PLAN.md.


## Initial M0/M1 record — 2026-09-30

The following environment and acceptance entries describe their original stage.
The current tone-upgrade record above supersedes its software inventory/results.

### Environment

- Fedora Linux 44, KDE Plasma edition, x86-64.
- GCC/G++ 16.2.1 (Red Hat 16.2.1-2), CMake 4.3.0, Ninja 1.13.2.
- Python 3.14.7; pkg-config 2.5.1.
- pkg-config results: ALSA 1.2.16.1, JACK 3.1609.0, FreeType 26.6.20,
  Fontconfig 2.17.0, X11 1.8.13, Xext 1.3.6, Xinerama 1.1.5,
  Xrandr 1.5.4, Xcursor 1.2.3, Xcomposite 0.4.6, Xrender 0.9.12.
- JUCE 8.0.15, official commit
  `91ad83ae34a81e0833b1a2b0866f54846370ae53`, owner-provided local checkout.
- The root initially contained AGENTS.md and PLAN.md only. The owner initialized
  Git during the task. No commits/pushes or global audio changes were performed.

## M0/M1 automated results (before M2)

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure

cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure
```

Both applications build and both test suites pass (2/2) in each configuration.
The core test covers startup silence, selected physical Input 2 mapping to both
outputs, neutral stereo/mono impulse response without added delay, composed dB
gain, 20 ms Start/gain/Mute/Stop ramps, meters, invalid controls/samples, bounded
protection, zero and 8193-frame blocks, null input/output handling and concurrent
callback detach/acknowledgment. The adapter test runs the actual standalone
callback with a deterministic fake AudioIODevice. It checks no-auto-start,
open failure, substituted-channel rejection, exact physical masks, packed
routing, fade/close, all-output failure silence, explicit reconnect, rate/buffer
changes, unknown-measurement labels, and supported/rejected processor buses.

Heap auditing intercepts C++ allocation/deallocation (including aligned forms)
around core processing, callback lease/routing, the actual standalone callback,
its failure clearing path, and JUCE processBlock. All measured paths report zero
heap operations. Source inspection finds no direct C allocator, locking,
logging, serialization, construction or I/O calls in these processing paths.
It does not instrument the underlying audio server/driver or prove scheduling
performance on the physical interface.

The pinned JUCE patch was applied to fresh official files in a temporary fixture,
applied again unchanged, then tested against an unexpected edit: fresh patch and
idempotency passed, modified source was rejected. Patched native sources compile
in the debug/release/adapter diagnostic builds. Release `ldd` reports no missing
linked libraries. Online FetchContent download was not tested because shell
network access is restricted; the local pinned-source path was tested.

ASan/UBSan diagnostics:

```bash
cmake --preset core-sanitize
cmake --build --preset core-sanitize
ASAN_OPTIONS=detect_leaks=0 ctest --preset core-sanitize --output-on-failure

cmake -S . -B build/adapter-sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DGUITARRACK_SANITIZERS=ON
cmake --build build/adapter-sanitize \
  --target guitarrack_tests guitarrack_adapter_tests -j 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/adapter-sanitize --output-on-failure
```

Core diagnostics pass (1/1); the full core/adapter diagnostic tests pass (2/2).
The first sanitizer link attempt failed because libasan was missing; the owner
installed libasan/libubsan and linking succeeded. LeakSanitizer then failed to
inspect processes in the restricted environment. Address/undefined checks were
run with leak detection disabled; **leak detection remains unverified**.
These are diagnostic results, not callback-load/latency benchmarks.

## Current owner report and pending physical acceptance

The owner confirms the native GUI opens on Fedora and live guitar monitoring
works with the M-Audio M-Track Solo, physical Input 2. PipeWire launch settings
256/48000 and 512/48000 both sounded acceptable; 256 is the current preference.
The owner subsequently reports testing the M2 rack with a real guitar: Gain
controls work well, and the application appeared correct during that test. This
is user-reported listening evidence. These statements do not verify duration,
actual format, device latency, startup silence, detailed edit/bypass/Stop-Start
acceptance or disconnect/reconnect. The owner now reports opening the M3 GUI on
Fedora and testing Overdrive, EQ and Delay with a real guitar, individually and
together in a rack. This is user-reported desktop/listening evidence; it does
not complete the unreported soak/reconnect/latency checks. The owner subsequently
reports testing the M4 desktop preset workflow and says it works well. This is
user-reported M4 acceptance, without inferred detailed keyboard/transition,
new Delete, soak, reconnect, measured-latency or LeakSanitizer results.

The restricted agent session still has no `/dev/snd` or access to the owner's
PipeWire service. Native GUI smoke testing is unavailable here (no usable desktop
surface or Xvfb); the earlier native launch report is owner evidence for M0/M1,
not agent visual verification of M4. The owner has separately reported the M3
GUI opens and its effects work. Use the README's launch and live-rack
checklist and record only checks actually performed.

Record the physical result here when tested:

| Field | Result |
|---|---|
| Device / backend | Owner reports M-Track Solo physical Input 2, PipeWire launch; actual backend/device display not supplied |
| Requested / actual sample rate | Launch settings: 48000; actual rate not supplied |
| Requested / actual callback buffer | Launch settings: 256 and 512 acceptable; prefer 256. Actual callback buffer not supplied |
| Driver/device-reported input + output latency | Pending; estimate only |
| Added DSP latency | M5 now reserves 128 samples for every rack (2.67 ms at 48 kHz); earlier M3 was zero. Delay/IR response time is separate. Physical round trip remains pending |
| Measured round trip / method | Pending; do not infer from buffer size |
| 10-minute monitoring, noise, clipping, xruns/dropouts | Pending |
| Real disconnect/reconnect and startup silence | Pending |

No host, Windows, ASIO or measured real-time performance results are claimed.
The monitoring, Gain and working M3 effects reports are the owner's listening
evidence. Detailed tone/aliasing appraisal and dry-recording tests are not
established. Historical M2/M3 software evidence follows; current M4 evidence is
recorded in the final section.


## M2 implementation checks

Toolchain and pinned JUCE are unchanged from the environment above. Built the
complete standalone and affected test targets with:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure
```

After the final source change, debug and release each pass **3/3** tests:
monitoring/routing, serial rack lifecycle, and device adapter lifecycle. Full
core/adapter ASan/UBSan diagnostics pass **3/3** with leak detection disabled.
Release `ldd` reports no missing linked libraries; `git diff --check` passes.

`serial_rack_lifecycle` adds physical/state checks for independent Gain amplitudes
and neutral behavior, monotonic 20 ms level ramps, per-instance/rack bypass
continuity, structural-remove continuity, UUID/slot/control preservation,
noncommuting multiply/offset processors (0.45 vs 0.70 for opposite orders), stale
blocked preparation, slot reservation during construction, preparation exceptions,
rollback during an accepted fade, saturated single-pointer handoff coalescing,
1,000 concurrent control/structural edits, bounded graph accounting/reclamation,
fixed 16-slot capacity, sample-rate re-prepare, 1/127/512-sample chunks and zero/
8193-sample device blocks. Test-only processors are never registered in the app.

Processor constructor/destructor counters match after shutdown, no destructor
runs inside the audited audio calls, and quiescent ownership converges to one
graph (`built == reclaimed + graphs`, peak <=4). This is lifetime accounting,
not a LeakSanitizer result. C++ heap auditing covers all rack audio operations,
activation, rejection, crossfade and retirement, plus actual standalone callback
and JUCE processBlock integration. All are heap-free in the tests. Callback
source review finds bounded sample/slot loops, atomic/raw-pointer transfers and
preallocated buffers; no locks, waits, condition-variable notifications, files,
logging, serialization, construction or DSP destruction. Underlying driver/server
scheduling remains outside these software checks.

Finite protection and exact bypass endpoints are tested even with a deliberately
invalid-output test processor. The production catalog contains Gain only, with
zero latency/tail. Preparation rejects nonzero latency/tails until compensation
is implemented in the relevant later milestone. Presets/host automation are not
published or claimed tested.

Diagnostic commands (separate from performance tests):

```bash
cmake --preset core-sanitize
cmake --build --preset core-sanitize
ASAN_OPTIONS=detect_leaks=0 ctest --preset core-sanitize --output-on-failure
cmake --build build/adapter-sanitize \
  --target guitarrack_tests guitarrack_rack_tests guitarrack_adapter_tests -j 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/adapter-sanitize --output-on-failure
```

The adapter diagnostic build was configured with the same Debug/ASan/UBSan
command documented in the M0/M1 record above. LeakSanitizer remains disabled due
to the restricted environment; no leak scan or performance measurement is claimed.
A separate ThreadSanitizer configure attempt failed at compiler linking because
`/usr/lib64/libtsan.so.2.0.0` is missing. No ThreadSanitizer test ran. The exact
Fedora install command is `sudo dnf install libtsan`; installation was not
performed or requested from the owner during this task. After installation:

```bash
cmake -S . -B build/core-thread-sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DGUITARRACK_BUILD_STANDALONE=OFF \
  -DCMAKE_CXX_FLAGS='-fsanitize=thread -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread
cmake --build build/core-thread-sanitize --target guitarrack_rack_tests -j 2
ctest --test-dir build/core-thread-sanitize -R serial_rack_lifecycle --output-on-failure
```

No package, global audio policy, repository commit, remote or publication changes
were made. Physical M2 acceptance, the 10-minute monitoring soak, actual device
format/latency, disconnect/reconnect and measured round trip remain pending.


## M3 verification at implementation time (superseded owner evidence below)

Fedora 44, GCC 16.2.1, CMake 4.3.0, Ninja 1.13.2 and the pinned JUCE 8.0.15
checkout were rechecked. Initial git status was clean; existing M2 sources were
preserved. No sub-agents, system packages, global audio policies, commits,
remotes, publication or later milestone features were introduced.

Final standalone/configuration checks:

```bash
cmake --preset linux-debug
cmake --preset linux-release
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure
```

Both standalone binaries build; debug **4/4** and release **4/4** tests pass:
`monitoring_and_routing`, `serial_rack_lifecycle`, `built_in_effect_signals`,
`device_adapter_lifecycle`. Release `ldd` has no missing linked libraries;
`git diff --check` passes. The dedicated signal suite is built by CMake and can
be run alone with `ctest --preset linux-debug -R built_in_effect_signals`.

Signal evidence:

- Overdrive zero Mix returns exact dry; minimum Drive / Level 0 dB is approximately
  unity for small signals before Tone. Saturation follows the wet Level, a -6 dB
  level change gives the measured amplitude ratio, 50% Mix matches dry/wet
  blending, extreme finite wet input stays finite/bounded and symmetric, and
  simultaneous parameter ramps stay continuous on a settled constant signal.
- EQ neutral returns exact dry. Sine-energy measurements at 8/44.1/48/96/192 kHz
  verify shelf midpoints and peak centers at -12/+6/+12 dB settings (midpoint
  half-gain and peak full-gain, tolerance 0.025 dB). Bass/treble asymptotes and
  opposite-band transparency are checked; the high shelf caps at 3.6 kHz at
  8 kHz. Rapid alternating extreme gain changes remain finite/bounded with
  tested low-frequency sample continuity at all those rates.
- Delay impulses verify first-echo time, fractional centroid (including 463.05
  samples for 10.5 ms at 44.1 kHz), separate stereo histories, and 0.5/0.25 repeat decay with
  50% Feedback. Maximum 85% Feedback converges under constant input, zero Mix
  returns dry, reset silences histories, and rapid 1/2000 ms requests crossfade
  without abrupt steps on the test sine. Maximum delay storage/ring wrap,
  reprepare to 48 kHz and new impulse timing are checked. One-sample vs 512-sample
  block partitions match, zero blocks do nothing, and activation is heap-free.
- Each M3 type has independent UUIDs/slots/controls across two instances and
  move/removal. Real Overdrive→EQ and EQ→Overdrive produce different measured
  energy; identity and controls stay fixed. Stateful live racks verify continuous
  bypass and structural reorder on a settled signal, fading the leading edge
  written to replacement history, continued hidden Delay processing, shortened tails
  on removal, Stop/rate restart history clearing, failed preparation retaining
  actual active Delay output, and stale completion retaining newer controls.
- The concurrent 1,000-edit stress test now adds mixed Overdrive/EQ/Delay graphs.
  Existing saturation, stale-generation, failure-during-fade, rollback, slot
  reservation and retirement accounting regressions remain covered. At quiescence
  one graph remains, peak ownership <=4, and built = reclaimed + owned.
- The actual standalone callback and shared JUCE `processBlock` activate/process
  all M3 effects with variable/oversized chunks and mono-to-stereo routing.
  Shared tail queries report conservative temporal bounds separately from zero
  processing latency. These adapter tests use a fake device, not physical audio.

Allocation/lifetime evidence: thread-local C++ new/delete auditing (including
aligned forms) reports **zero callback allocations and deallocations** in signal
processing, activation, live crossfades, rejection, bypass, oversized chunks,
retirement, actual standalone callback and shared processBlock. Source review
confirms Delay vector allocation/clearing only in off-audio prepare/reset and
ring destruction only after off-audio reclamation. Activation copies fixed-size
controls only. EQ/Overdrive histories and coefficients are fixed-size. All
processing loops are bounded by chunks, channels and the 16-slot capacity; no
processing locks, waits, serialization, logging, files, GUI or construction were
introduced. This does not audit external driver/server scheduling, C allocation
in external libraries, or certify worst-case callback deadlines.

Final full ASan/UBSan diagnostics:

```bash
cmake -S . -B build/adapter-sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DGUITARRACK_SANITIZERS=ON
cmake --build build/adapter-sanitize \
  --target guitarrack_tests guitarrack_rack_tests guitarrack_effect_tests guitarrack_adapter_tests -j 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/adapter-sanitize --output-on-failure
```

**4/4 pass** after the final core/adapter changes. Core-only diagnostics also
passed 3/3 earlier in this milestone. Leak detection remains disabled/unverified
in this restricted session; lifetime accounting is not a LeakSanitizer result.
Sanitizer timing and signal-test runtime are not audio performance benchmarks.

Retried the existing ThreadSanitizer configure command from the M2 section.
Compiler linking fails because `/usr/lib64/libtsan.so.2.0.0` is missing. **No TSan
executable/test ran.** Exact Fedora package command (not executed):
`sudo dnf install libtsan`. Once installed, build all core tests with the earlier
TSan configuration and run `ctest --test-dir build/core-thread-sanitize
--output-on-failure`. TSan remains pending.

Native UI attempt: `timeout 8 ./build/linux-release/GuitarRack_artefacts/Release/GuitarRack
--smoke-test` (absolute binary path used from `/tmp`) exited **139** before any
window was verified. An independent `XOpenDisplay` probe returned null; there is
no Xvfb executable or accessible `/dev/snd`. A GDB attempt was also blocked by
`ptrace: Operation not permitted`, so no stack/root cause was established. This
is a failed smoke check in the restricted session, not successful UI evidence;
visual layout, scrolling/numeric entry/keyboard and M3 launch on the owner's
desktop remain pending. Existing owner native-window evidence applies to the
previous build. No physical stream was tested or claimed.

Owner evidence retained verbatim in scope: real-guitar M2 rack test, Gain controls
work well, app appeared correct during their test. Pending checks remain:
individual/combined M3 live and dry-recording listening, tone/aliasing appraisal,
listening-based gain staging, detailed M2 live edits/bypass/Stop-Start, 10-minute
soak, actual backend/device format, physical disconnect/reconnect, device latency
and measured round trip, LeakSanitizer, TSan, performance headroom, host/VST3 and
Windows. See [README.md](../README.md) for the exact launch and M3 guitar checklist.


## M4 final verification — 2026-09-30

The owner reports: opened the M3 GUI on the Fedora desktop; Overdrive, EQ and
Delay work with a real guitar, including together in a rack. This is
**user-reported desktop and listening evidence**, not an agent hardware test.
It does not establish the 10-minute soak, disconnect/reconnect, actual format,
device latency, measured round trip, detailed click/tail/gain-staging appraisal,
LeakSanitizer or any other unreported check.

Rechecked Fedora 44, GCC 16.2.1, CMake 4.3.0, Ninja 1.13.2 and JUCE 8.0.15
commit `91ad83ae34a81e0833b1a2b0866f54846370ae53`. Initial git status was clean.
No owner changes were discarded; no packages, global audio policy, commits,
remotes, publication, extra effects, VST3 or Windows were introduced.

Final debug and release commands:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure
```

Both standalone binaries build and each suite passes **5/5**:
`monitoring_and_routing`, `serial_rack_lifecycle`, `built_in_effect_signals`,
`presets_and_persistence`, `device_adapter_lifecycle`.
Release `ldd` finds no missing libraries; `git diff --check` passes.

M4 evidence from `presets_and_persistence`:

- Tracked Clean/Crunch/Lead Delay fixtures pass the actual registry-aware runtime
  validator and match embedded factory configurations. Schema is Draft 2020-12;
  a separate Python jsonschema check was unavailable (module absent), so no
  independent schema-engine validation is claimed.
- Save/reopen and import/export preserve Unicode/Hebrew/emoji names, preset and
  instance UUIDs, effect versions, exact order and sparse slots, independent
  parameters and bypass. Save As preserves effect identities while changing
  only preset identity/name. Fresh session recall restores selection/rack/gains
  while stopped. Settings round trips are separate from tone, and missing exact
  device names return no selection rather than another microphone.
- Parameter, effect bypass, global/rack bypass, add/remove and reorder edits
  affect dirty state; restoring configuration clears it. Save/Discard/Cancel
  action logic and failed-save dirty/selection retention are tested. Actual
  native dialog clicks, close/restart UI and keyboard/layout remain owner checks.
- Tests reject duplicate UUIDs/slots/order references/JSON keys (including escaped
  duplicate keys), invalid UUIDs/slots/order, required unknown effects, mismatched
  versions, missing/wrong/out-of-range controls, numeric strings, invalid names,
  UTF-8/surrogates/nulls, NaN/Infinity/overflowed integers/exponents, malformed
  syntax, excessive nesting, oversized files and unsupported assets/versions.
- File replacement, reopen, bounded regular-file reads, nonblocking FIFO/directory
  rejection, failed writes/replacement, preservation
  of destinations and temporary-file cleanup are checked in unique temporary
  directories. Malformed library files are reported and excluded. Invalid/missing
  imports retain the previous edited tone and selection.
- Blocked/failed preparation, pre-acceptance cancellation, competing parser
  requests, stale builder results, control edits during parsing/at handoff,
  exact-slot replacement with different UUIDs and pending-load sample-rate
  rebuilds are exercised. Zero blocks do not activate partially. Live preset
  globals ramp continuously on a settled signal; Mute and running state persist.
- 300 competing full loads with a concurrent audio consumer converge to the last
  request. Graph ownership stays <=4, obsolete graphs reclaim to one, and no
  callback C++ heap operations occur. Existing 1,000-edit mixed-effect stress,
  signal/lifetime/queue-saturation regressions remain intact.
- Original versus serialized/restored combined Overdrive/EQ/two independent Delay
  configurations render the same reset-condition sine/impulse input for 76,800
  samples, with maximum tested sample difference <2e-6. This verifies
  configuration equivalence, not preservation of running delay history.

Adapter checks also activate full tone transactions through the actual standalone
callback and shared JUCE processBlock with oversized 1,200-frame blocks. They
verify heap-free transitions, stereo routing, unchanged stream-open count,
continued monitoring, and stopped loads that never open/authorize a stream.
These use a fake device, not the M-Track Solo.

RT source review: JSON, validation, snapshots, files, name/UUID operations and
factory construction remain off audio. Prepared graphs have fixed control views,
20 ms transition scratch, preconverted global gains and worker-owned histories.
Activation checks generation/revisions and copies bounded controls only. Callback
loops remain bounded by chunks/16 slots; no locks, waits, logs, UI, I/O or graph
creation/destruction enter callback paths. Retirement/reclamation remains off
thread. Allocation auditing covers normal processing, preset activation,
crossfades, cancellation/stale rejection and oversized adapter blocks. It does
not certify external driver/server internals or worst-case callback timing.

Full ASan/UBSan diagnostics (separate from performance):

```bash
cmake -S . -B build/adapter-sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DGUITARRACK_SANITIZERS=ON
cmake --build build/adapter-sanitize --target guitarrack_tests \
  guitarrack_rack_tests guitarrack_effect_tests guitarrack_adapter_tests \
  guitarrack_state_tests -j 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/adapter-sanitize --output-on-failure
```

**5/5 pass**, with leak detection disabled. A separate attempt:
`ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build/adapter-sanitize
-R presets_and_persistence --output-on-failure` fails at exit with
`LeakSanitizer has encountered a fatal error` and the ptrace limitation hint.
**LeakSanitizer remains unverified.** Graph accounting is not a leak scan.

Unlike the earlier M2/M3 attempts, TSan is now available in this session; no
package was installed by the agent. New actual automated evidence:

```bash
cmake -S . -B build/core-thread-sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DGUITARRACK_BUILD_STANDALONE=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=thread -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread
cmake --build build/core-thread-sanitize --target guitarrack_tests \
  guitarrack_rack_tests guitarrack_effect_tests guitarrack_adapter_tests \
  guitarrack_state_tests -j 2
ctest --test-dir build/core-thread-sanitize --output-on-failure
```

**5/5 pass** with no reported TSan race in these exercised core/state/adapter
paths. This supersedes the earlier missing-library blocker for this test scope;
it does not establish real driver races, GUI/dialog coverage, hardware timing or
performance. No broad TSan/hardware acceptance checkbox is inferred from the
owner listening report.

No `/dev/snd` is exposed. A fresh `XOpenDisplay` probe returned null, so an M4
native launch/visual/dialog check was not attempted or claimed. Owner M3 desktop
success does not verify the newly added M4 UI. Still pending: M4 native
save/switch/close/restart/import/export and listening checklist, dry-recording
and detailed tone/aliasing/gain-staging appraisal, detailed live edits, 10-minute
soak, actual backend/device format and latency, physical reconnect, measured
round trip, LeakSanitizer, performance headroom, host/VST3 and Windows.


## M4 extension — saved user-preset deletion (2026-09-30)

Environment rechecked: Fedora 44, GCC 16.2.1 (20260819), CMake 4.3.0,
Ninja 1.13.2, local JUCE 8.0.15 commit
`91ad83ae34a81e0833b1a2b0866f54846370ae53`. Existing uncommitted changes,
including AGENTS.md, were preserved. No packages, audio policies, commits or
later milestones were changed.

Actual final build/regression commands:

```bash
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure
```

Both complete standalone binaries and affected tests build; **5/5 pass in each
configuration**. Release `ldd` has no missing libraries. `git diff --check` passes.

New automated state checks use temporary managed stores, exported files and
asset fixtures, without touching the owner's library:

- Clean and dirty deletion; staged Save/Discard, dirty Cancel and confirmation
  Cancel preserve selection, rack, baseline and saved/recall bytes as appropriate.
- Hebrew/emoji/path-like names, duplicate display names, factory protection at
  session/store boundaries, and disabled eligibility for unnamed/unsaved imports.
- Fresh session/catalog reconstruction and recall after deletion, immediate
  matching-recall cleanup before close, normal-close persistence, retained
  unrelated recall/settings, and Save As from the detached working rack.
- Real directory-permission failures in Save, recall replacement and unlink;
  recall and pre-Save file rollback; missing/corrupt/UUID-mismatched managed files,
  symlink entry/directory rejection and nonregular targets. Each failure keeps
  the selection, catalog, rack and dirty state with an actionable error.
- Exported copies, an IR-like `.wav` asset, unrelated files, another preset with
  the same display name and protected factory UUID files are preserved.
- Competing parser requests, cancelled late parser completions, blocked/cancelled
  worker graphs, cached deleted selections and selection changes during
  confirmation cannot recreate/reselect/delete the wrong entry. Sample-rate
  restart does not republish the deleted association.
- Against an uninterrupted reference, deleting a preset with active Delay history
  produces **exactly identical subsequent samples (maximum difference 0)** with
  nonzero echoes. No graph request/build, control write or DSP reset occurs.

The actual standalone callback is exercised with the existing fake device:
Delete while running, muted and unmuted keeps stream authorization, callback,
open count, physical channel masks, actual configuration, mono-to-stereo output,
parameters, gains and rack bypass. Stopped deletion never opens or authorizes
monitoring. Callback heap audits pass; earlier signal/rack/state/adapter
regressions remain enabled. This is synthetic evidence, not a physical test.

Used the existing full diagnostic builds (ASan/UBSan enabled with
`GUITARRACK_SANITIZERS=ON`; TSan with `-fsanitize=thread
-fno-omit-frame-pointer` and matching link flag):

```bash
cmake --build build/adapter-sanitize --target guitarrack_tests \
  guitarrack_rack_tests guitarrack_effect_tests guitarrack_adapter_tests \
  guitarrack_state_tests -j 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/adapter-sanitize --output-on-failure
cmake --build build/core-thread-sanitize --target guitarrack_tests \
  guitarrack_rack_tests guitarrack_effect_tests guitarrack_adapter_tests \
  guitarrack_state_tests -j 2
ctest --test-dir build/core-thread-sanitize --output-on-failure
```

**ASan/UBSan 5/5 pass with leaks disabled; TSan 5/5 pass with no reported race in
these exercised paths.** Neither result establishes GUI/driver races or performance.
A separate actual attempt:

```bash
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build/adapter-sanitize \
  -R presets_and_persistence --output-on-failure
```

fails at exit: `LeakSanitizer has encountered a fatal error`, with the ptrace
inspection limitation hint. **LeakSanitizer remains unverified**, not complete.

RT source review: Delete/dirty dialogs and `PresetSession::deleteSelected` run on
the existing single control/message producer; all UUID, serialization, file,
catalog and lifetime work stays there. No callback method or DSP implementation
was changed. Parser results only read/validate; deletion supersedes their serials
and records the acknowledged tone generation. Delete is rejected while a preset
or graph transition is pending. Existing worker ownership and bounded audio
handoff/retirement remain unchanged. Filesystem rollback after an unrecoverable
fault is best effort and its failure is surfaced; external processes racing
path replacement are not coordinated by a cross-process transaction lock.

Current acceptance: the owner's earlier M4 desktop preset workflow report is
recorded as user-reported acceptance and supersedes its earlier pending status.
The **new Delete native dialog, Unicode rendering, initial Cancel focus/Enter,
Escape, keyboard/layout and real-guitar checklist remain pending**. A fresh
`XOpenDisplay` probe returned unavailable; `/dev/snd` is still unavailable, so no
native smoke launch or hardware test is claimed. Unreported soak, reconnect,
actual backend/device format/latency, measured round trip, listening/aliasing
appraisal, performance headroom, host/VST3 and Windows checks remain pending.


## M5 first catalog subset — final integrated verification

Started 2026-09-30; final verification completed 2026-10-01.

Fedora 44 x86-64, GCC 16.2.1, CMake 4.3.0, Ninja 1.13.2 and the same exact pinned
JUCE 8.0.15 commit were verified. Initial main worktree status was clean. The
Distortion and Reverb agents worked in separate detached worktrees under /tmp;
only their scoped DSP/test files were copied into main integration. The original
Git common directory is read-only in this environment, so detached worktrees
were created from a local bare clone. No owner files, commits, remotes, packages,
global audio policies or external publication were changed.

Final complete standalone/regression commands:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release --output-on-failure
```

**Debug 9/9 and Release 9/9 pass**. Release ldd has no missing linked libraries;
git diff --check passes; final build logs contain no compiler warnings/errors.
The nine suites retain the five M4 regressions and add `distortion_signals`,
`reverb_signals`, `cabinet_signals`, `ir_assets_and_bundles`.

New signal/lifecycle/state evidence:

- Distortion stronger clipping/harmonic distinction from tanh, Drive/Level/Mix,
  parameter continuity, finite/abnormal signals, reset, independent stereo/
  instances, zero/variable/oversized partitions and fixed 8-sample dry latency.
  Fixed 4× FIR coherent 9 kHz / 48 kHz test: native hard-clip alias/fundamental -11.2151 dB,
  oversampled -29.4823 dB, reduction 18.2672 dB. Residual aliasing and short-FIR
  bandwidth/voicing phase remain. This is a specific automated spectral comparison,
  not subjective guitar tone or a general alias-free result.
- Reverb impulse timing/decay, Size/damping response, independent stereo/no
  crossfeed, mono widening, exact zero Mix, reset/rate extremes 8–768 kHz, independent
  instances, fresh replacement history, smooth rapid controls, abnormal samples
  and a 51-second bounded render. No soak/driver result is inferred from that render.
- Cabinet random stereo convolution versus a direct known reference (maximum
  observed error 1.70e-8) across direct/
  FFT partition boundaries, impulse/tap timing, separate channels, level ratio,
  reset, first-use heap audits, variable/one/zero/oversized partitions, actual-rate
  resampling/timing/interior gain at 8/44.1/48/96/192/768 kHz, repeated preparation
  retaining its selected IR, maximum source and aggregate graph resource bounds.
- Every rack graph, including empty/rack bypass and 16 Distortions, has truthful
  fixed 128-sample latency; Distortion Mix 0 and individual bypass align. New graphs
  warm for 128 samples before 20 ms fade, fixing a padding-history discontinuity
  found by the existing continuity regressions. Noncommuting Distortion/Cabinet
  order produces different measured signals. All new types have independent
  instances and immutable UUID/slot/control preservation during reorder/removal.
  Existing 1000-edit concurrent stress now uses the six non-Gain catalog effects;
  four-graph accounting, retirement, stale/failure rollback and slot reuse remain.
- WAV tests cover PCM16/24/32 and float32, stereo/unsupported/empty/silent/nonfinite/
  out-of-full-scale/truncated/bad-format/rate/duration/file/gain limits. SHA-256
  empty/abc standard vectors pass; system sha256sum independently matches the
  tracked synthetic asset, whose bytes match embedded generation.
- Existing tracked M4 factories remain valid and output/configuration round trips
  pass. New factory fixtures match embedded definitions. Schema v2 carries exact
  instance/slot/version/order/parameters/asset identities without history/paths.
  `jsonschema` is unavailable in this environment, so no independent schema-engine
  validation is claimed; actual runtime strict validation/fixtures are exercised.
- A bundle imports into a distinct empty managed store after the original external
  WAV and source store disappear, preserving exact bytes and unnormalized samples.
  Traversal/absolute paths, checksum corruption, truncation, trailing/link records,
  size/count limits, manifest errors, source/managed directory/entry symlinks,
  corruption, FIFO and missing assets fail. Entire format validates before writes.
- Live IR replacement preserves selected preset/baseline and marks edits dirty.
  Invalid/missing file, pre-acceptance cancellation, control/reorder supersession,
  a blocked cancelled graph build and injected convolution preparation failure
  keep the previous cabinet/rack/controls/selection/dirty state. Managed Save As,
  bundle export/import, muted/8193-frame processing, actual-rate restart and
  reclamation are exercised. Deleting a saved IR preset retains shared real WAV
  bytes and the current sound; the existing M4 deletion/adapter tests still pass.

Full diagnostic builds, separate from performance:

```bash
cmake -S . -B build/adapter-sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DGUITARRACK_SANITIZERS=ON
cmake --build build/adapter-sanitize --target guitarrack_tests \
  guitarrack_rack_tests guitarrack_effect_tests guitarrack_distortion_tests \
  guitarrack_reverb_tests guitarrack_cabinet_tests guitarrack_asset_tests \
  guitarrack_state_tests guitarrack_adapter_tests -j 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/adapter-sanitize --output-on-failure

cmake -S . -B build/core-thread-sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DGUITARRACK_BUILD_STANDALONE=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=thread -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread
cmake --build build/core-thread-sanitize --target guitarrack_tests \
  guitarrack_rack_tests guitarrack_effect_tests guitarrack_distortion_tests \
  guitarrack_reverb_tests guitarrack_cabinet_tests guitarrack_asset_tests \
  guitarrack_state_tests guitarrack_adapter_tests -j 2
ctest --test-dir build/core-thread-sanitize --output-on-failure
```

**ASan/UBSan 9/9 with leak detection disabled; TSan 9/9 with no reported races**
in exercised signal/core/state/adapter paths. Diagnostic standalone GUI binaries
are not part of those test targets. LeakSanitizer remains unverified after the
previous restricted-process/ptrace fatal inspection failure; no new leak scan is
claimed. Lifetime accounting is not a leak scan.

Callback audit: new effects use fixed/pre-sized storage and scalar/complex
arithmetic. Distortion FIRs and Cabinet FFT tables are computed off audio;
Reverb rings and convolution kernels/histories allocate/clear off audio. Matching
activation copies only bounded small state; Cabinet/Reverb do not copy large
histories. Source inspection finds no C allocation, file/UI/serialization,
mutex/wait/logging, construction, destruction, library FFT initialization or
background preparation reachable from their process/activate paths. C++ heap
new/delete audits (including aligned forms) pass first use, activation, structural
and preset/IR crossfades, stale rejection, bypass/Mute, retirement, and oversized
shared monitoring/adapter calls. External driver/server allocation/scheduling is
not certified. Resources and boundary burst cost are documented in assets.md.

### Release synthetic processing-cost measurement

Run after sanitizer/test/build activity stopped:

```bash
./build/linux-release/guitarrack_catalog_benchmark
```

Intel Core i7-9700F @ nominal 3.00 GHz, eight logical CPUs, Fedora 44; GCC Release
`-O3 -DNDEBUG`. Rate 48000 Hz, chunks 256, nominal block period 5333.33 µs. One unpaced
process with normal scheduling; no affinity/real-time priority change. Each rack
warms 200 blocks and renders 1600 measured blocks (8.533 seconds of signal), with
small 220 Hz sine input generation included in timed wall-clock calls. Includes
rack processing/control reads/bypass/padding; excludes device/backend, GUI,
monitor trim/output/protection and measured round trip. Timing is per whole
callback chunk, so FFT-boundary work is included. The 500 ms test IR is an original
synthetic damped mathematical response. Dense rack is four repeats of
Distortion/Cabinet/Delay/Reverb, all at conservative descriptor defaults.

| Rack | Mean µs | p99 µs | Max µs | Mean / block period |
|---|---:|---:|---:|---:|
| Empty aligned rack | 4.15 | 4.46 | 9.06 | 0.078% |
| Distortion → synthetic Cabinet → EQ → Delay → Reverb | 225.07 | 232.14 | 336.98 | 4.22% |
| Same chain, 500 ms IR | 391.68 | 425.55 | 664.00 | 7.34% |
| 16 effects / four 500 ms IRs | 1553.68 | 2018.40 | 2430.80 | 29.13% |

Five-block two-graph transition probes: means 439.26/803.13/3300.55 µs and maxima
442.29/905.50/3497.94 µs for the three nonempty racks, respectively. These are
specific unpaced wall times under this session's scheduling, not worst-case
real-time guarantees, a device CPU meter, listening acceptance or measured
round-trip latency. Longer/more cabinets, higher rates or system load can cost
more; the 16-slot capacity is not a guaranteed comfortable rack on all hardware.

### Physical/native acceptance remains pending

Fresh XOpenDisplay probe returns unavailable, no Xvfb executable is available,
and /dev/snd is absent. No M5 native smoke launch, real stream, hardware listening,
IR file chooser/keyboard/dialog rendering or device latency check is claimed.
Owner's earlier M0–M4 reports are preserved. Pending: new-effect tone/aliasing/
gain-staging appraisal and factory tuning, native Delete/IR/bundle dialogs,
10-minute soak, physical reconnect, requested/actual configuration, device
latency and measured round trip, LeakSanitizer, real-host/VST3 and Windows.
Gate, Compressor, Chorus, Basic Amp, other modulation/pitch effects and later
products are untouched. M5 as a whole remains incomplete.


## 2026-10-01 — Experimental pedalboard branch

Environment: cloud Linux, GCC 14.2.0 (Debian 14.2.0-19), C++20, pthreads,
`-O2` for manual core/state builds. Existing configured CMake/Ninja and native
ALSA/Freetype/X11 development headers were unavailable. No system packages were
installed. Sources/headers for the repository's exact pinned JUCE 8.0.15 commit
`91ad83ae34a81e0833b1a2b0866f54846370ae53` were fetched through GitHub into ignored
`build/manual-juce`. JUCE core and its separate CompilationTime unit were built
for persistence checks; browser/curl stayed disabled. These are manual checks,
not a successful CMake/standalone link or a display/hardware test.

Manual executable results (all pass):

- MonitorTests: startup/routing, meters, protection, variable blocks and heap audit.
- RackTests: independent instances, order, transitions, stale/failed/saturated
  updates, reclamation and callback allocation audit.
- EffectTests: original Overdrive/EQ/Delay signals and lifecycle.
- DistortionTests, ReverbTests and CabinetTests: existing signal/latency/IR checks.
  Cabinet initially identified the documented synthetic WAV missing from the
  upload because `*.wav` was ignored. Restoring exactly the embedded analytic
  bytes and adding one ignore exception makes the fixture pass; SHA-256 remains
  `4a9ac650b2983ca0c49c138d11beccd2d0c8607ac9fccf47c537fbbf7bb7adae`.
- StateTests and AssetTests: exact factory fixture agreement for 45 factories,
  state round trips, dirty choices, rollback, cancellation/stale loads, library
  deletion and managed IR/portable bundles.
- ExperimentalTests: threshold/ratio compression, deep tremolo cycles, measured
  filter rejection, granular octave spectrum, ring-modulation sidebands, stereo
  chorus, default Delay v1/v2 exact digital agreement, coherent concurrent row
  updates, invalid modes, rates 8/44.1/48/96 kHz, variable blocks, activation
  lifetime and all 40 added racks rendered with callback allocation audits.
  The 220 Hz octave input produces a dominant peak near 438 Hz; grain sidebands
  and pitch error are recorded limits, not an exact-frequency harmonizer claim.
- PedalPresetTests: all 77 physical control round trips, Hebrew/emoji save/reload,
  factory identity protection, invalid UUID/value/schema and corrupt-entry isolation.

Both changed UI translation units (`RackEditor.cpp`, `PresetBar.cpp`) pass
`g++ -std=c++20 -Wall -Wextra -Wpedantic -fsyntax-only` with the pinned JUCE headers
and `JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1`, `JUCE_STANDALONE_APPLICATION=1`,
`JUCE_USE_CURL=0`, `JUCE_WEB_BROWSER=0`. This validates compilation syntax, not
linking or displayed layout. JSON Schema validation passes for all 45 tracked
racks and 77 expanded pedal voices; `git diff --check` is clean.

The new core suite also passes a separate GCC `-O1 -g` ASan/UBSan build, using:

```bash
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -pthread -Isrc src/core/*.cpp src/dsp/*.cpp \
  tests/ExperimentalTests.cpp tests/HeapAudit.cpp -o build/manual/experimental-sanitize
ASAN_OPTIONS=detect_leaks=0 ./build/manual/experimental-sanitize
```

Leak detection is unverified. A normal Fedora CMake build, native GUI/keyboard
acceptance, actual adapter checks for this branch, guitar listening/voicing
tuning, real-time deadline measurements, hardware stability and round trip
remain pending. Existing Fedora dependency commands are in dependencies.md.
No new live listening, device, plugin-host, Windows or performance result is claimed.

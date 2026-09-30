# Verification record — 2026-09-30

## Environment

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
| Added DSP latency | 0 samples for monitoring + M3 effects; Delay echo time is intentional musical delay |
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

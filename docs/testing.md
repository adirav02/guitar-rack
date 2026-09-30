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

## Owner report and pending physical acceptance

The owner confirms the native GUI opens on Fedora and live guitar monitoring
works with the M-Audio M-Track Solo, physical Input 2. PipeWire launch settings
256/48000 and 512/48000 both sounded acceptable; 256 is the current preference.
The owner subsequently reports testing the M2 rack with a real guitar: Gain
controls work well, and the application appeared correct during that test. This
is user-reported listening evidence. These statements do not verify duration,
actual format, device latency, startup silence, detailed edit/bypass/Stop-Start
acceptance or disconnect/reconnect. M3 UI and effect listening remain pending.

The restricted agent session still has no `/dev/snd` or access to the owner's
PipeWire service. Native GUI smoke testing is unavailable here (no usable desktop
surface or Xvfb); the earlier native launch report is owner evidence for M0/M1,
not visual verification of the new rack UI. Use the README's launch and live-rack
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
The acceptable-monitoring and working-Gain reports are the owner's listening
evidence; they do not validate subjective M3 tone. M3 software evidence follows
below; M4 and later milestones remain unimplemented.


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


## M3 final verification

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

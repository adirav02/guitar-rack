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

## Automated results

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

## Pending native desktop and physical acceptance

The GUI binary is built, but a native-window launch has not yet been verified on
the owner's desktop. Run:

```bash
./build/linux-release/GuitarRack_artefacts/Release/GuitarRack --smoke-test
./build/linux-release/GuitarRack_artefacts/Release/GuitarRack
```

Verify the window opens/closes, labels/controls fit, keyboard/numeric controls
work, and startup remains stopped. A restricted session has no `/dev/snd` and
cannot access the owner's PipeWire socket, so no real input was recorded or
monitored. The README contains the M-Track Solo guitar checklist. M1 hardware
acceptance requires at least 10 minutes of stable monitoring, correct Input 2
in both ears, gain/mute/stop listening checks and actual disconnect/reconnect.

Record the physical result here when tested:

| Field | Result |
|---|---|
| Device / backend | Pending |
| Requested / actual sample rate | Pending |
| Requested / actual callback buffer | Pending |
| Driver/device-reported input + output latency | Pending; estimate only |
| Added DSP latency | 0 samples for the M1 monitoring engine |
| Measured round trip / method | Pending; do not infer from buffer size |
| 10-minute monitoring, noise, clipping, xruns/dropouts | Pending |
| Real disconnect/reconnect and startup silence | Pending |

No host, Windows, ASIO, real-time performance or subjective guitar-tone results
are claimed. Later milestones are not implemented.

# M3 built-in effects and version 1 controls

These are shared native C++ DSP processors registered alongside Gain. All run
stereo internally after mono guitar mapping, add zero processing latency, and
use the existing fixed parameter bank. Delay echo time is intentional musical
delay, separate from processing/device latency and measured round trip.

Each row fixes the version 1 bank index, physical range and linear mapping:
`normalized = (physical - minimum) / (maximum - minimum)`. Generic IDs remain
`slot_XX_param_YY`; Gain's existing mapping is unchanged. Unused controls are
no-ops. These internal mappings are ready for the planned bank but no plugin or
host automation parameters are published in M3.

| Type ID (v1) | Bank index | Key / label | Range | Default | Smoothing |
|---|---|---|---|---|---|
| `gain` | 00 | `level_db` / Level | -60…+24 dB | 0 dB | 20 ms amplitude ramp |
| `overdrive` | 00 | `drive_db` / Drive | 0…36 dB | 6 dB | 20 ms pre-gain amplitude ramp |
| `overdrive` | 01 | `tone_hz` / Tone | 200…8000 Hz | 3000 Hz | 20 ms one-pole coefficient ramp |
| `overdrive` | 02 | `level_db` / Level | -24…+6 dB | -6 dB | 20 ms wet amplitude ramp |
| `overdrive` | 03 | `mix_percent` / Mix | 0…100% | 25% | 20 ms linear mix ramp |
| `eq` | 00 | `low_db` / Low | -12…+12 dB | 0 dB | 20 ms coefficient interpolation |
| `eq` | 01 | `mid_db` / Mid | -12…+12 dB | 0 dB | 20 ms coefficient interpolation |
| `eq` | 02 | `high_db` / High | -12…+12 dB | 0 dB | 20 ms coefficient interpolation |
| `delay` | 00 | `time_ms` / Time | 1…2000 ms | 350 ms | 100 ms read-head crossfade |
| `delay` | 01 | `feedback_percent` / Feedback | 0…85% | 25% | 20 ms linear feedback ramp |
| `delay` | 02 | `mix_percent` / Mix | 0…100% | 25% | 20 ms linear mix ramp |

## Overdrive

The signal is `tanh(input * 10^(Drive/20))`, followed by a one-pole low-pass
with pole `exp(-2*pi*Tone/sampleRate)`, then wet-only `10^(Level/20)` gain.
Tone's actual cutoff is capped at 0.45 times the sample rate. Output mixes the
unchanged dry signal and wet signal linearly. At zero Mix, output is exact dry;
zero Drive is approximately unity for small signals at Level 0 dB before Tone,
but it still soft-clips larger signals. There is no fully neutral all-wet setting.
The default 6 dB Drive, -6 dB Level and 25% Mix gives conservative initial gain
staging; raising Drive does not provide loudness compensation.

The symmetric tanh stage is bounded at ±1 before Tone/Level. Increasing Drive
compresses peaks and adds harmonics. This prototype runs at the native sample
rate with **no oversampling**: harmonics above Nyquist alias into the audio band,
particularly at high Drive or with bright input. The post-clip Tone filter cannot
remove alias components already folded into the passband. Direct distorted
guitar may sound harsh; no amp model, cabinet simulation, commercial-model
matching, polished tone, or listening validation is claimed. Oversampling and
its latency/dry alignment remain M5 work. Very small filter state is flushed to
zero to avoid denormals. Conservative filter-tail bound: 20 ms.

## EQ

A 150 Hz low shelf, 1 kHz peaking filter (Q = 1/sqrt(2)), and 4 kHz high shelf
run in that order. Shelves use slope S=1. At low sample rates each frequency is
capped at 0.45 times the actual rate (the high shelf is 3.6 kHz at 8 kHz).
The bilinear-transform coefficient equations follow the
[RBJ/W3C Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook/).
This is a project-owned implementation of those equations, with double-precision
direct-form-I history and coefficients; no external DSP dependency is added.

All three gains at 0 dB use exact identity coefficients. A +6 dB shelf has
+3 dB at its midpoint; a +6 dB peak has +6 dB at its center. Coefficients are
recomputed only when the target changes and interpolated per sample over 20 ms.
Normalized stable second-order denominator coefficients lie in a convex stability
region, so interpolation retains pole stability; this alone does not guarantee
subjective quality of every rapid modulation. History remains independent per
channel and instance. Automated sweeps of gain changes check bounded output and
continuity; listening remains pending. Conservative filter-tail bound: 250 ms.

Boosts can consume substantial headroom. EQ input/output history has an abnormal
signal guard at ±1,000,000 and non-finite samples become silence. This guard does
not operate in normal guitar ranges; the existing final ±0.98 protection remains
the audible output ceiling. Tiny output/state values flush to zero.

## Delay

Preparation allocates two rings of `ceil(2*sampleRate)+2` floats on the worker.
Storage never grows in processing. Separate stereo histories use fractional
linear interpolation with bounded wrap arithmetic. For each channel:

`ring[write] = input + Feedback * delayed`

`output = (1-Mix)*input + Mix*delayed`

Feedback is clamped to 0…0.85 and Mix to the validated bank range. At maximum
Feedback a constant signal converges to 1/(1-0.85) times its input in the wet
path; this is stable but louder, so use output headroom. Temporal storage is
guarded at ±1,000,000, non-finite input becomes silence, and tiny stored samples
flush to zero. Interpolation and crossfading use convex weights.

Time changes crossfade two fixed, interpolated read heads over 100 ms. Each fade
finishes before the newest queued target starts; rapid targets coalesce, with no
extra heads, loops, allocation or queue growth. Once changes cease the latest
time settles within at most two fades (200 ms). During a fade, mixed echoes can
comb-filter or change their spacing; no pitch-preserving or seamless musical
transition is promised. Mix/Feedback changes continue independently at 20 ms.

At fixed time an impulse first appears at Time, and successive repeats decay by
Feedback. At 2000 ms and 85% Feedback, the last audible repeats reach below -80 dB
relative to the first by 116 seconds; this is a conservative declared tail bound,
not an exact finite cutoff or a host acceptance result.

## Reset, bypass, edits and resources

- **Card Reset / double-click:** reset control values only, using the same smooth
  parameter path. History and bypass state are retained.
- **Effect/rack bypass:** 20 ms crossfade to dry. Processors continue receiving
  input and advancing state. Tails are hidden while bypassed; unbypassing can
  reveal remaining/current echoes. There is no spillover onto the bypassed dry.
- **Mute:** final output fades to silence; effects continue processing.
- **Stop, failure close, rate restart:** after callbacks drain, the worker clears
  histories and resets controls/smoothers to current bank values. Start begins
  with empty histories at the actual sample rate.
- **Structural add/remove/reorder:** the unchanged M2 worker/mailbox/reclamation
  contract builds a complete replacement, fading old/new outputs for 20 ms.
  Overdrive/EQ inherit only small matching-instance histories/smoothers at the
  same rate. Delay inherits small control/read-head state, **never copies its
  ring on audio**. Every replacement Delay starts with empty storage, even if
  only another card moved. Input written into the new history fades in over
  20 ms, preventing a hard leading edge when new echoes arrive. Old echoes fade
  out; replacement echoes need time to
  accumulate. Long tails may shorten, and wet-only chains can have an audible
  dip. Continuous delay history across structural edits is not an M3 claim.
- **Failure/stale build:** no activation, no active-history reset; the accepted
  graph and latest parameter bank remain authoritative.

At most 16 effects and four owned graphs (two processing) remain allowed. A
48 kHz Delay ring pair uses about 0.77 MB; 16 Delays across four graphs use about
49.2 MB, excluding other graph/UI memory. The existing accepted 768 kHz ceiling
bounds this worst-case delay storage at about 786.5 MB. Allocation failure rolls
back off audio; these are storage calculations, not measured CPU/load results.

The builder accepts zero-latency effects with finite nonnegative tails <=120 s
per effect. The shared adapter reports a conservative sum of stage tails, taking
the maximum of the active/fading graphs during transitions via a lock-free atomic.
Positive processing latency still requires future dry/transition compensation
and is rejected. Common-rate signal tests, callbacks and lifetime audits are
recorded in [testing.md](testing.md); physical listening is separate evidence.

# Built-in effects and fixed version 1 controls

These are shared native C++ DSP processors registered alongside Gain. All run
stereo internally after mono guitar mapping and use the existing fixed parameter bank.
M5 adds Distortion, Reverb and Cabinet/IR; the complete rack now has fixed
128-sample processing latency, including empty and bypassed racks. Individual
latency and alignment details follow below. Delay echo time is intentional musical
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
matching or polished tone is claimed. The owner subsequently reports that
Overdrive, EQ and Delay work with a real guitar, individually and together; detailed tone and
aliasing appraisal remain pending. Overdrive oversampling remains deferred; the separate M5 Distortion uses fixed 4× oversampling and aligned dry paths. Very small filter state is flushed to
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
continuity; the owner reports working live EQ, while detailed gain-staging
and tone appraisal remain pending. Conservative filter-tail bound: 250 ms.

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

The builder accepts 0–8 samples of stage latency and finite nonnegative tails <=120 s
per effect. The shared adapter reports a conservative sum of stage tails, taking
the maximum of the active/fading graphs during transitions via a lock-free atomic.
M5 compensates stage bypass and pads every graph to the same 128-sample latency. See the M5 section below. Common-rate signal tests, callbacks and lifetime audits are
recorded in [testing.md](testing.md); physical listening is separate evidence.


## M5 first catalog extension

All new controls use the existing linear normalization and stable slot IDs.
These are original DSP designs without commercial pedal/cabinet emulation claims.

| Type ID (v1) | Bank index | Key / label | Range | Default | Smoothing |
|---|---|---|---|---|---|
| `distortion` | 00 | `drive_db` / Drive | 0…48 dB | 12 dB | 20 ms amplitude |
| `distortion` | 01 | `tone_hz` / Tone | 400…8000 Hz | 2800 Hz | 20 ms pole |
| `distortion` | 02 | `level_db` / Level | -30…+6 dB | -12 dB | 20 ms wet amplitude |
| `distortion` | 03 | `mix_percent` / Mix | 0…100% | 50% | 20 ms linear |
| `reverb` | 00 | `size_percent` / Size | 0…100% | 35% | 20 ms feedback |
| `reverb` | 01 | `damping_percent` / Damping | 0…100% | 55% | 20 ms damping pole |
| `reverb` | 02 | `mix_percent` / Mix | 0…100% | 15% | 20 ms linear |
| `cabinet` | 00 | `level_db` / Level | -24…+6 dB | 0 dB | 20 ms amplitude |

Cabinet's IR identity is a structural asset selection, outside the automation
bank. Unused controls remain no-ops. Card Reset changes only controls; it keeps
the selected IR and ongoing histories. No host parameters are published yet.

### Distortion

A 90 Hz first-order high-pass tightens bass before Drive and **symmetric hard
clipping** at ±1. Interpolation and decimation use fixed 4× oversampling with
two 33-tap Blackman-windowed sinc FIRs, cutoff 90% of base Nyquist. Polyphase
interpolation and phase-zero decimation avoid unnecessary FIR work. Coefficients
are computed in prepare; processing has fixed arrays and no lazy initialization.
Post-clip one-pole Tone (capped at 0.45×rate), wet-only Level and linear Mix follow.
Stronger clipping and the bass voicing distinguish this from Overdrive's native
`tanh`. Drive does not compensate loudness; use Level/output headroom.

The two linear-phase FIRs introduce **8 base-rate samples** of fixed delay.
Internal dry Mix uses exactly 8 samples; stage bypass also delays dry by 8.
Mix 0% is exact delayed dry. Tone/high-pass add frequency-dependent phase, and
the short FIRs attenuate upper bandwidth. They do not make dry and voiced wet
spectrally identical. A coherent 9 kHz / 48 kHz high-drive test measured about
18.27 dB less alias/fundamental energy than native hard clipping (-11.22 versus
-29.48 dB). Residual aliasing remains; this one measurement is not a general
alias-free or subjective tone-quality claim. Tail declaration: 0.1 seconds.
Matching-instance activation copies only fixed small histories/smoothers.

### Reverb

The pinned JUCE `Reverb::processStereo` sums left and right into one wet excitation.
To preserve independent stereo input, this effect uses an original Schroeder
network per channel: eight parallel damped combs and four true allpasses.
Nominal prime comb lengths 1091/1181/1279/1361/1429/1499/1559/1621 and allpass
557/443/347/227 samples at 44.1 kHz scale during preparation. Right lengths add
23 reference samples, widening duplicated mono without crossfeeding stereo.
No third-party reverb source or new dependency is copied.

Size maps feedback 0.45…0.90 and changes **decay**, rather than geometric room
size or delay lengths. Damping maps a feedback low-pass pole 0…0.90. Input
injection is 0.12, eight comb outputs average, allpass coefficient is 0.5,
and Mix blends with unchanged dry. Maximum steady DC wet gain is 1.2.
There is no freeze, modulation or measured-room realism claim. Resources are
about 102 kB (99.5 KiB) at 44.1 kHz, 1.78 MB (1.70 MiB) at 768 kHz per instance. Zero buffering
latency; conservative 10-second tail bound, not an exact truncation time.

Reset clears all rings off audio. Replacement graphs keep only small smoothers;
new rings start empty, ingress ramps for 20 ms, and old tails fade out. Tails
can shorten and wet-only chains can dip. Individual/rack bypass hide the wet
tail while the effect keeps processing; unbypass can reveal current tails.
Mute keeps processing, Stop/failure/rate restart clear history. Seamless tail
preservation is outside this scope.

### Cabinet / IR

This is **linear convolution**, not a nonlinear amplifier model. It loads mono
PCM16/24/32 or IEEE float32 WAV IRs, 8000…192000 Hz, up to 500 ms, 96000 decoded
samples and 1 MiB of file bytes. Standard 16-byte format headers or 18-byte
headers with empty extension are supported; extensible/compressed/RF64 encodings
are rejected with a re-export instruction. Non-finite/out-of-full-scale data,
empty/silent energy ≤1e-16 and absolute tap sum >128 are rejected. No peak or
energy normalization, trimming or automatic substitution changes the sound.
Level applies after convolution; relative source IR levels remain meaningful.

Windowed-sinc resampling happens off-thread with a 32-source-sample half-width
expanded for downsampling (maximum 768 neighbors on either side), zero outside
source support, and source/target scaling for FIR gain. Interior impulse gain
and physical timing are tested. Finite edge truncation/rate conversion can
slightly change response; bytes at the original rate are preserved for export.
Processing supports the engine's 8…768 kHz range; resampled IR ≤384000 samples,
aggregate ≤768000 samples per graph. Over-budget graphs fail transactionally.

A direct first 256 taps plus 512-point radix-2 FFT partitioned tail gives **zero
added buffering latency** at any callback partition. At each internal 256-frame
boundary, transforms and bounded partition products prepare the next tail block;
independent channel history, overlap and direct rings never grow. IR onset/time
inside the file remains intentional response delay, distinct from buffering.
Prepared kernels, twiddles and bit-reversal tables require no first-use library
initialization. All decoding/resampling/kernel preparation and destruction occur
off audio. Boundaries have more work than other samples; measured synthetic cost
and limitations are in testing.md. Sixteen instances are a capacity ceiling,
not a promise every long-IR rack meets every hardware deadline.

Replacement retains no large convolution history. Input ingress fades for 20 ms;
old response fades out with the graph. Bypass/Mute/Stop/tail behavior matches
Reverb/Delay. Starter `synthetic-filter.wav` is an original analytic 4.2 kHz
low-pass minus 90 Hz low-pass, 512 taps at 48 kHz: **synthetic, not measured**.
It is embedded for source-independent startup and offered under CC0 with exact
provenance in assets/ir/README.md. User IRs keep the user's redistribution rights.

### Rack latency alignment

Every graph reserves **128 samples = 16 slots × 8 samples**, even when empty or
fully bypassed: 2.667 ms at 48 kHz, 2.902 ms at 44.1 kHz. The graph sums stage
latencies, delays individual dry bypass paths, then pads its output by the
remaining budget. Rack bypass has the same fixed delay. Distortion handles
its own Mix alignment; Cabinet/Reverb have no fixed sample delay to compensate.
This small constant budget avoids latency changes when editing or restoring
presets and keeps both old/new transition outputs aligned. It adds latency to
previous zero-latency racks; it is reported in the UI and shared processor.

Replacement graphs process 128 warm-up samples with the old graph audible before
the 20 ms output crossfade begins. This avoids a padding-history discontinuity.
No large histories are copied; warm-up does not preserve temporal tails. At most
two graphs process. Stop clears compensation rings off audio. Device-reported
I/O estimates and measured round trip remain separate; no physical latency
measurement has been made for this addition.


## Experimental pedalboard extension

The eight added processors, Delay v2 character controls, physical ranges, musical
wet delays, native-rate aliasing and granular pitch limitations are documented in
[pedalboard.md](pedalboard.md). Existing version 1 mappings/algorithms remain.
There are 15 effect types, 77 factory pedal voicings and 45 complete rack factories.

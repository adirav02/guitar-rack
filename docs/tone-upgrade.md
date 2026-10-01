# Tone and pedalboard upgrade — 2026-10-01

The upgrade extends the baseline `3511ab1ea2a49be8a3de6c107b0e07831bb63bdf`.
The initial checkout matched it and was clean. No owner changes were discarded,
no commits/pushes were made, and no packages, remote services or assets were
installed/purchased. C++20, pinned JUCE 8.0.15, native Linux standalone, shared
DSP and the existing bounded serial rack remain the implementation.

The catalog contains 42 effect types (43 exact version descriptors), 69 complete
sounds and 146 reusable pedal voicings. Its purpose is varied playable starting
points; counts are an inventory, not evidence of sound quality. The 45 legacy
sounds and 77 legacy voicings remain unchanged. New eight amp families, six drive
topologies, graphic/parametric EQ, gate, wah/envelope wah, phaser, flanger,
swell/hold, echo, three reverb networks, spectral expression pitch and Dual Amp
have separate stable IDs. Optional chainsaw, reverse/granular delay and stutter
are deferred. No VST3/Windows, plugin hosting or general routing framework was added.

## Protected owner sound

The owner explicitly confirmed listening approval for **Art Rock / Subterranean
Pulse**, preset UUID `50000000-0000-4000-8000-000000000008`. This approval applies
only to that exact legacy sound, not the catalog or new variants. Its settings
are project settings, not historical Radiohead settings. Its unchanged chain is
Clean Glass → Cabinet Unity → Subterranean Pulse → Analog Trail → Long Decay,
with rack output −12 dB. Amp v1 Drive 0, Bass −1, Mid +1, Treble +2 dB, Presence
65%, Sag 10%, Level −3 dB, Mix 100%; Cabinet v1 Level 0 dB and original checksum;
Tremolo v1 4 Hz / Depth 85 / Shape 20 / Width 0 / Mix 100%; Delay v2 420 ms /
Feedback 42 / Mix 28 / Analog 1 / Color 2600 Hz / Wow 0%; Reverb v1 Size 70 /
Damping 60 / Mix 25%. All identities, remaining controls and original IR bytes
are frozen in [compatibility fixtures](../tests/fixtures/legacy).

## Native playing workflow

**Choose Preset...** opens one owned, resizable JUCE window; repeated clicks focus
it. Search matches individual words across name, tags and inspiration. Source,
artist/style and gain filters plus UUID favorites narrow the list. Navigation
only updates details. Load, deliberate double-click or Enter routes through the
existing asynchronous preset transaction and Save/Discard/Cancel dirty guard.
Progress, cancellation and errors stay visible. Escape closes the browser;
Escape in the main window still requests Stop. An accepted graph transition
cannot be cancelled; it completes normally. No automatic audition was added.
Import/export/bundles, factory protection and confirmed user deletion remain.

Audio Setup starts collapsed; its header shows backend/device/channel choices,
configuration and status, with detailed diagnostics in the tooltip. Expanding
it changes layout only. Start/Stop/Mute,
rack bypass, trim/output, input/output meters, clipping warnings, processor
profile and status remain accessible. The tone toolbar keeps current name,
dirty indication and Save/Save As. Pedal Basic controls retain Gain/Drive and
Output/Level; All controls exposes the rest. Compact cards use 64-pixel control rows and keep engagement alongside the
expansion button; matching existing controls show their voicing name without
applying any parameters. Expansion and the scroll anchor use
instance UUIDs across rebuild/reorder. Local layout changes do not dirty tones.
Voicing remains an instance-only coherent control update with its existing history;
changing a cabinet uses an asset transaction and dirties that tone.

Default content size is 1280×720; constraints are 700×450 logical pixels, with
initial windows constrained to the available display. Browser minimum is
700×400. This removes the 1030-pixel trap and permits smaller logical sizes.
Below 600 logical pixels high, gain/meter rows remain compact even with Audio
Setup open so safety and rack-bypass controls retain space. Actual
1366×768/150% visual acceptance is still pending.
Local preferences v2 strictly store geometry, collapse, scroll/expansion,
search/filters/favorites and the explicitly chosen latency profile. V1 migrates
its height to at most 768, preserves device/channel/rate/buffer choices, starts
in Legacy 128 and adds empty local browser state. Device settings and metadata
stay outside portable tones. Native visual, HiDPI, focus/Escape and open-window
shutdown acceptance remains pending because this session cannot open a display.

## New signal processors

All amp families are original inspired networks, not measured circuit models or
commercial-model captures. Cabinets, chorus and other effects remain separate.

| Family | Distinction in the implemented network |
|---|---|
| Solid-State Clean | High-headroom soft rails, articulate early high-pass, optional bright; no sag knob |
| US Clean Combo | Early tone shaping, two asymmetric soft stages, lower headroom and volume cleanup |
| British Chime | Early asymmetric stage, interacting Bass/Treble shaping, second stage and meaningful late Cut |
| British Crunch | Two differently filtered gain stages, mid emphasis, power Master and Presence |
| British Hot-Rod | Extra asymmetric filtered stage and explicit interstage low-cut |
| Mark High Gain | Early Bass/Treble before clipping, separate Input/Lead Gain and restrained bass; late graphic EQ remains separate |
| Modern Tight | Three saturation stages with repeated bass rejection, low sag, Presence and Resonance |
| Recto Heavy | Broader bass, envelope-dependent low-end compression, biased stages and greater sag |

Master drives the final modeled power compression; Output is an independent
post gain. Tests distinguish all eight at matched RMS and verify Output scaling,
Master harmonics and early Mark Bass affecting later clipping. Low/edge/rhythm/
lead pedal voices expose differences without implying eight real amplifier captures.

The drives use independent gain and level: asymmetric mid boost/tightening;
Oxford early Bass plus later Contour/Treble; hard rails with post filter;
sustaining envelope compression/diode-style saturation; gated bias cutoff with
bounded nonlinear feedback; and a high-headroom tight boost with low-cut/attack.
Bias fuzz feedback is at most 0.65 and normally off at Stability 100. There is no
free-running oscillator, sine wavefolding stand-in or Fuzz Factory circuit claim.

All new amp/drive/dual networks use **8×, 129-tap Blackman sinc antialias filter
pairs**, 16 base samples of transport delay. Model filters run at the actual
internal rate and are separate from those antialias filters. The measured 4×/
65-tap candidate and selected 8×/129-tap spectra are retained in
[evidence](evidence/tone-upgrade). Residual aliases at extreme gain remain;
ADAA/circuit solvers were considered but not implemented. The final processing
path caches unchanged pole coefficients and rate-dependent envelope/DC constants;
amp gain/power conversions run once per base sample and are shared by internal
samples/channels. This reduces repeated math without changing topology, controls,
filter taps or latency. Both benchmark matrices are retained for comparison. This is not an
alias-free or album-match certificate. Legacy Distortion's 4× short FIR,
90 Hz high-pass, symmetric hard clip and one-pole Tone are untouched; legacy
Overdrive/Fuzz/Basic Amp still run their original native-rate algorithms.

Graphic EQ uses broad five-band peaks at 80/240/750/2200/6600 Hz plus Output.
Parametric EQ uses three frequency/gain/Q bands, HP/LP and Output. Legacy EQ v1
is unchanged. The linked stereo gate has threshold, range, attack, hold,
release and hysteresis, independent smoothed detection and gain. It normally
precedes amps/ambience; no hidden downstream input-key path was introduced.

Manual wah is a resonant band-pass with software Position/heel/toe/Q; envelope
wah adds independent sensitivity/direction/attack/release. Phaser continuously
processes six all-pass stages and blends four/six outputs, with bounded feedback.
Flanger uses a fractional short delay and stereo phase, not renamed chorus.
Echo supplies ms or BPM/subdivision/tap, digital/analog/tape, filtered feedback,
ping-pong and optional heads at 1/0.75/0.5 (weights 0.6/0.25/0.15). Time changes
use bounded coalesced 100-ms read-head fades; feedback is capped at 70% to keep
4-second repeats within its 120-second tail declaration. Legacy Delay v1/v2
remain executable with their original ranges and recipes.

Reverb has independently prepared 4-line room, diffused 6-line plate and
8-line Hadamard hall networks; line-length-dependent RT60 feedback, pre-delay,
damping and wet tone. All modes run for smooth changes. No spring claim exists.
Swell follows an envelope and raises volume. Audio Hold loops the captured last
500 ms with a 25-ms seam overlap, releases over 40 ms and expires by 120 seconds;
it requires Off then Hold to rearm. It sustains captured audio, not strings.

Expression Pitch uses a 2048-sample/H256 spectral sinusoidal model with a bounded
64-peak reconstruction, ±24 semitones, detune, software expression and aligned
dry Mix. It is a **monophonic starting point**; close chords lose separation,
attacks smear and low shifted notes can lose level. No chord mode, DigiTech
identity or under-3-ms claim is offered. Legacy granular Pitch v1 remains a
texture processor: its original 110-ms window and 8–118-ms wet-path delay
remain, without retroactively aligning its wet/dry sound. This algorithmic texture
delay is distinct from intentional echoes and the legacy 128-sample transport.
Held notes, endpoints, rates, bends, chords and impulse onset
are characterized separately in [testing](testing.md). Software expression can
later support local MIDI mappings, but MIDI was not added.

Dual Amp prepares two bounded branches using the same amp primitives, separate
model/gain/master/levels, Blend and shared tone/Output within 14 controls. Branch
changes crossfade at most two models per branch. Both feed **one downstream
Cabinet**: a documented approximation. Branch polarity and correlated 50/50
level are tested. This is parallel tone blending, not independent double tracking.
There are no hidden branch asset paths or dynamically nested host parameters.

## Latency, resources and tails

Legacy is still **128 samples**. Creative is an explicit **4096-sample session
profile**, chosen while stopped and remembered only in local preferences. Serial
stage delays sum; Dual Amp uses the branch maximum. Both graphs, individual
bypass and rack bypass align to the selected budget. A too-large rack or incompatible
profile change fails transactionally and retains the accepted sound/identity/
dirty state. Processor declarations update outside audio, preserving future VST3
latency reporting compatibility. Normal startup remains stopped, with no live
input opened. Remembering an explicitly chosen Creative profile is not a tone
migration; portable tone files contain no device/backend/profile setting.

At 48 kHz, Legacy 128 = 2.667 ms, Creative 4096 = 85.333 ms; 256 device frames =
5.333 ms per period. None is total I/O latency. Spectral pitch's 2048-sample
window/aligned dry = 42.667 ms at 48 kHz, but its measured octave impulse onset/
peak are 634/2159 samples before rack padding. Window onset/peak and intentional
echoes/pre-delay are reported separately; wet attacks are not perfectly dry aligned.
No measured physical round trip or transparent-chord claim exists.

All storage, effect construction, decoding/resampling and FFT preparation occur
off audio. Existing worker/coalesced request, maximum four owned graphs/two
processing, bounded acceptance/retirement and off-thread reclamation remain.
Variable/zero/oversized blocks use the core's bounded chunking. Each fixed stereo
transport ring now reserves 4096 samples/channel; graph bypass/padding storage is
about 544 KiB before DSP histories. Activation inherits only rings up to 128
samples; larger rings warm fresh while the old graph stays audible for the
profile budget, then a 20-ms crossfade. No huge tail copies or last `shared_ptr`
release occur in audio. Unchanged voicing updates preserve history; structural
changes may shorten echo/reverb tails and lose captured hold audio.

New stage tail bounds: amps/drives/dual 0.5 s; graphic/parametric EQ 1 s; gate
2 s; wah/envelope/phaser/swell 1 s; flanger 2 s; Echo/Hold 120 s; Space Reverb
40 s; Expression Pitch 0.6 s. Rack tail reporting sums serial bounds. See exact
code for legacy declarations. Short declaration values represent settling bounds,
not ideal IIR extinction; denormal state is flushed below 1e-24.

## Assets, identities and regeneration

The five additional cabinets are honestly labeled original synthetic responses:
clean/open 2×12, chime 2×12, British 4×12, tight modern 4×12 and dark/full 4×12.
Names describe intended voicing, not measured speakers/microphones. Every asset
has origin, CC0 license, original 48 kHz rate, checksum and 1-kHz normalization
in [catalog](../assets/ir/catalog.json). The old unnormalized synthetic asset is
unchanged. Measured-IR redistribution and speaker realism acceptance remain pending.
User IR loading, checksum validation, bounded convolution/resampling and portable
bundles remain. Unknown/missing assets produce errors with no tonal fallback.

Every factory/pedal/instance UUID is explicit in `assets/sounds/library.json`;
refs use pedal UUIDs, so reordering/renaming no longer changes identity. New seeds
use UUID5 of immutable recipe keys; after creation the JSON is authoritative.
`assets/sounds/catalog.json` is a strict separate version-1 catalog, not tone
state or automation. It contains character, artist/style, gain, tags, instrument/
input suggestions, amp/cab requirements, notes and evidence/listening status.
All new entries remain listening pending. There are no required song/section fields.

From the repository root, with the existing build/dependencies:

```bash
python3 tools/generate_sound_library.py
python3 tools/generate_sound_catalog.py
cmake --build --preset linux-release
./build/linux-release/guitarrack_export_sounds . > build/linux-release/registry.json
python3 tools/update_sound_schemas.py build/linux-release/registry.json
python3 tools/generate_effect_docs.py build/linux-release/registry.json
python3 tools/check_compatibility.py build/linux-release/guitarrack_export_sounds
```

`generate_cabinet_assets.py` regenerates only the five original analytic assets;
it never rewrites the legacy asset. `add_upgrade_library.py` is an idempotent
additive seed tool, not a replacement for edited library JSON. The exporter
refuses to overwrite semantically changed legacy fixtures. Do not regenerate
compatibility baselines to make failures pass. Full exact new control mappings
are in [tone-controls.md](tone-controls.md).

## Research and acceptance

The [official Mesa Mark IIC+ manual](https://mesa-boogie.imgix.net/135420-MarkIICplus_241127.pdf)
supports the early Bass/late graphic EQ distinction; our implementation applies
that topology principle, not circuit calibration or historical knob numbers.
[Bellamy's firsthand interview](https://www.premierguitar.com/artists/muses-matt-bellamy-unusual-collisions)
supports AC30 clarity alongside Diezel/modified-Marshall saturation; our Dual Amp
is an original shared-cab approximation. [Ed O'Brien's BOSS account](https://articles.boss.info/reverberations-ed-obrien-of-radiohead/)
supports DD-5-era echoes and LS-2 layering; our echo/hold recipes are original
standalone sounds. [Fidelman's firsthand 72 Seasons production account](https://www.guitarworld.com/features/greg-fidelman-metallica-72-seasons-gear-secrets)
and [Hetfield/Hammett's Black Album retrospective](https://www.guitarworld.com/features/metallica-the-black-album)
support Mesa/Marshall combinations and blending, not one transferable knob recipe.
[Marshall's official 2203 controls](https://www.marshall.com/gb/en/product/jcm800-2203-vintage-reissue-head)
support separate preamp/master/presence roles; our networks remain original approximations.
None authenticates our processor as a particular studio chain or album match.
Hysteria's opening bass riff is not presented as a guitar reproduction.

Use the [listening worksheet](listening.md) with identical recorded DI phrases,
matched perceived level and recorded instrument/input/output conditions. Synthetic
checks establish software behavior only. CUBE 10GX AUX/small speaker should be
judged separately from headphones/monitors; do not silently bypass cabinets or
change a tone based on output hardware. Native UI, physical routing/round trip,
owner guitar listening and artist-character acceptance remain pending. M5 as a
whole, VST3 and Windows remain incomplete.

# Experimental pedalboard and sound library

The native board has 15 effect types, 77 factory pedal voicings and 45 complete
rack presets. Forty complete racks are new. These are original starting points
for art rock, clean playing, heavy sounds, ambience and experimental noise.
Radiohead is a listening reference, not a promise to reproduce a recording.

## Playing and saving

Choose a full preset in the top bar or add a pedal. The signal follows numbered
pedals from left to right, then the next row; cables show the connections.
Earlier/Later changes that order while keeping the pedal's identity and settings.
The LED and ENGAGED/BYPASSED footswitch show whether it contributes to the sound.
Numeric entry, keyboard control, double-click defaults and Reset remain available.

Each pedal has a voicing menu. A voicing changes that pedal's controls together,
using the existing smooth DSP paths. It does not replace a graph or reset delay,
chorus, pitch or reverb history. Order, bypass, instance/slot identity, Cabinet
IR and all other pedals remain. Moving or replacing pedals and loading complete
racks still uses the existing graph crossfade, which can shorten tails.

Save voicing stores those controls under a new UUID/name in
`$XDG_DATA_HOME/GuitarRack/pedals` (fallback `~/.local/share/GuitarRack/pedals`).
UTF-8 names are supported. Saved voices appear on all matching type/version
pedals and survive restart. Cabinet voices save Level only; use a full rack
preset/bundle to save its selected IR. Full rack Save/Save As still preserves
all effects, order, controls, bypass and IR assets. Changing a pedal voice marks
the full working rack dirty. Saving a voice does not save the full rack.

## New controls (effect version 1)

| Effect | Controls in bank-index order |
|---|---|
| Fuzz | Drive 0–48 dB; Tone 400–8000 Hz; Fold 0–100%; Bias −50…50%; Gate 0–100%; Level −30…6 dB; Mix 0–100% |
| Basic Amp | Drive 0–36 dB; Bass/Mid/Treble −12…12 dB; Presence/Sag 0–100%; Level −30…6 dB; Mix 0–100% |
| Compressor | Threshold −48…0 dB; Ratio 1…12:1; Attack 0.5–100 ms; Release 10–1000 ms; Makeup 0–18 dB; Mix 0–100% |
| Chorus | Rate 0.05–8 Hz; Depth/Width/Mix 0–100% |
| Tremolo | Rate 0.05–20 Hz; Depth/Chop/Stereo/Mix 0–100% |
| Resonant Filter | Cutoff 100–6000 Hz; Resonance 0–90%; Auto Wah 0–100%; Rate 0.05–12 Hz; Sweep/Mix 0–100% |
| Pitch Shifter | Shift −12…12 semitones; Detune −50…50 cents; Width/Mix 0–100% |
| Ring Modulator | Carrier 20–2000 Hz; Mix/Stereo 0–100% |

All normalized mappings remain linear and controls smooth over 20 ms. Frequency
knobs use a more useful visual skew without changing their persisted mappings.

Delay v2 keeps indices 0–2 (Time, Feedback, Mix) and appends:
index 3 Mode (0 Digital, 1 Analog, 2 Tape), index 4 Color (500–12000 Hz),
index 5 Wow (0–100%). Digital retains the original delay output; Analog
low-passes the wet signal/feedback, and Tape adds bounded saturation and
up to 0.8 ms of 0.7 Hz wow. Switching mode crossfades the character using
20 ms smoothers. Time retains its original 100 ms read-head crossfade.

## DSP character and limits

Fuzz blends tanh clipping and sinusoidal wavefolding, adds bias, removes DC,
filters tone and smoothly gates quiet input. Gate maps to an input threshold
of 0–0.05 linear amplitude with a 5 ms gate smoother.
Basic Amp supplies sag-dependent soft clipping, a presence low-pass and the
existing three-band EQ. Neither processor is a measured tube/transistor model.
They run at native rate and can alias at aggressive settings. Cabinet remains
separate; every shipped rack cabinet uses the clearly labeled synthetic filter.
User mono cabinet IRs remain supported through the existing loader/bundle system.

Compressor links stereo peak detection, applies the physical threshold/ratio,
exponential attack/release, makeup and parallel mix. Chorus uses separate
stereo fractional delay lines with a 12 ms center and up to 8 ms modulation.
Tremolo blends sine and softened square gain envelopes, with stereo phase.
The filter is a bounded TPT state-variable low-pass with resonance, envelope
and LFO cutoff modulation; Auto Wah is an envelope-driven filter voice.
Ring modulation multiplies by a sine carrier, producing sum/difference tones.

Pitch uses two complementary windowed fractional read heads, a fixed 110 ms
grain window, and 8–118 ms intentional wet delay. It suits octave layers,
drones and textures. Grain sidebands, pitch error, flutter and transients
remain, especially for low notes/chords; it is not a transparent harmonizer.
The 220 Hz octave test has a dominant peak near 438 Hz, not an exact 440 Hz
claim. All zero-Mix paths remain exact dry; musical wet delay is separate from
the rack's unchanged fixed 128-sample transport latency.

New temporal resources allocate in preparation. Activation inherits only small
fixed state; fresh rings start empty and input fades in. Bypass hides tails while
processing continues, Mute keeps state running, and Stop clears it off audio.
Declared tails are 150 ms for Chorus/Pitch, 2 s for Compressor envelope release,
and 500 ms for other new processors. Existing Delay/Reverb tails remain unchanged.
The generic capacity is still 16 pedals, not a hardware performance guarantee.

## Try these racks

| Goal | Factory starting point |
|---|---|
| Clear, moving arpeggios | Clean / Glass Arpeggio; Art Rock / Fishes Motion |
| Chopped pulse | Art Rock / Subterranean Pulse; Experimental / Heartbeat |
| Heavy rhythm/lead | Art Rock / Paranoid Rhythm; Heavy / Tight Rhythm; Heavy / Octave Lead |
| Wide ambient layers | Ambient / Fifth Halo; Ambient / Octave Cloud; Ambient / Stereo Orbit |
| Gated, folded noise | Noise / Folded Teeth; Noise / Starved Radio; Noise / Machine Cut |
| Metallic/alien sounds | Noise / Metallic Bells; Noise / Alien Mouth; Noise / Carrier Drift |

Start with low interface volume and the preset's conservative output gain.
Play the same phrase, compare bypass at matched volume, and adjust the interface
gain and effect Level before judging Drive. Try different orders: pitch before
fuzz, filtering after fuzz, and tremolo after ambience produce distinct textures.
The presets have software signal checks; guitar listening/tuning remains pending.

## Compatibility and maintenance

The five original rack factories and all previous effect IDs/versions remain.
Restored Delay v1 has its original three controls and algorithm. New Delay
instances are v2; old instances are not silently upgraded. Registry lookup is
type plus version. Host IDs stay fixed; host publication remains a later milestone.
Portable rack schemas v1/v2 retain their shapes and add exact effect/version
control cases. The new pedal schema is `assets/presets/pedal-schema-v1.json`.
Unknown effects, fractional modes, missing keys and invalid values are rejected.

`assets/sounds/library.json` is the editable recipe source.
Run `python3 tools/generate_sound_library.py` after recipe changes.
After building the normal standalone configuration, run:

```bash
./build/linux-release/guitarrack_export_sounds . > build/linux-release/registry.json
python3 tools/update_sound_schemas.py build/linux-release/registry.json
```

This regenerates tracked rack fixtures, the original synthetic WAV and strict
schema cases from the actual native registry. Personal user presets are separate.

## Fedora acceptance

Build and run using the README commands. Confirm the board at narrow/wide window
sizes, tab/numeric/double-click interactions, LED footswitches and signal order.
While sustaining notes, switch a pedal voice and confirm another pedal's controls
and existing echoes stay. Save a Hebrew-named pedal voice, add another matching
pedal, apply it, restart, and verify it remains in the menu. Full Save As and
export/import should restore all 15 effect types and Delay versions.

Play the suggested racks with the M-Track Solo and report clipping, loudness
jumps, pitch artifacts, aliasing and xruns at the actual rate/buffer. Adjust the
new voices after listening. Native GUI display/link, live listening and hardware
deadline/latency checks have not been performed for this branch in the cloud.

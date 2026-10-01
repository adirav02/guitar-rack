# Radiohead and Muse song templates

These 24 complete factory sounds and 50 reusable pedal voices are additive.
This expansion brought the library to 93 complete sounds and 196 voices; the
subsequent [alternative collection](alternative-templates.md) brings it to
135 sounds and 256 voices. The [modern rock collection](modern-rock-templates.md)
brings the current catalog to 168 sounds and 305 voices. Every earlier sound
and voice retains its exact identity and settings, including the owner-approved
Art Rock / Subterranean Pulse. The new templates remain listening pending.

[Research, primary DSP sources and reference limitations](song-tone-research.md)
were recorded before project inspection and implementation. YouTube and the
specialist gear sites were blocked by the research environment. These are
inferred guitar starting points; no historical knob recipe or verified album
match is asserted. Amp networks and cabinet responses are original approximations.

## Choose and adjust

Build/launch the Fedora application using the [README](../README.md). In
**Choose Preset...**, search the song name or **Song templates**, select a sound,
inspect its details and press **Load**. The details include pickup/playing hints,
tuning, the actual ordered effects, and limitations. Search/select does not
change audio. **Save As** keeps your adjusted version without replacing a factory.
All 24 sounds fit the existing **Legacy 128** profile; Creative is unnecessary.
At 48 kHz the rack transport adds 2.667 ms, apart from device I/O and intentional
echo/reverb times. This is not measured round-trip latency or a CPU guarantee.

The chains are embedded factory presets in the application, including ordered
effects, saved controls, output levels and cabinet assets. Rebuild and restart
GuitarRack to replace an older executable. Set Source, Artist and Category to
**All** if a saved browser filter hides a song; search **Song templates** or
**Muse expansion**, then select and press **Load**.

For individual portable files, the exporter can produce all 99 song chains (including these 24)
with their cabinet WAVs included:

```bash
./build/linux-release/guitarrack_export_sounds --song-pack build/song-presets
```

In the updated application, press **Import**, choose a `.grbundle` file and
resolve any unsaved edits. **Save As** keeps your adjusted version. If using the
ZIP handoff, extract it first; the ZIP itself is not an application preset.
Older executables lacking the new effects/versions require rebuilding even
when importing a bundle. Export uses the compiled factory data and verifies
each bundle with the application's importer; existing edited bundles are retained
and reported instead of overwritten.

| Factory name (after artist prefix) | Intended guitar role | Starting ambience |
|---|---|---|
| Radiohead / Let Down - Clean Arpeggio | Bright, lightly compressed picking | 205-ms filtered echo, 16% wet; 1.8-s plate, 14% wet |
| Radiohead / Let Down - Space Layer | Separate sustained halo | Slow chorus; 410-ms echo, 25% wet; 3.6-s hall, 24% wet |
| Radiohead / No Surprises - Bell Clean | Rounded clean guitar | No echo/chorus; 1.2-s room, 11% wet |
| Radiohead / Subterranean Homesick Alien - Clean Foundation | Warm clear accompaniment | Gentle chorus; 400-ms echo, 15% wet; short plate |
| Radiohead / Subterranean Homesick Alien - Floating Lead | Moving, spacious clean lead | Slow stereo chorus; 400-ms echo, 42% feedback / 30% wet; 4.2-s hall, 27% wet |
| Radiohead / Weird Fishes - Interlocking Clean | Defined chiming arpeggio | Subtle chorus; 200-ms echo, 16% wet; short plate |
| Muse / Plug In Baby - Singing Fuzz | Stable aggressive guitar fuzz | Chime amp, synthetic chime cab and quiet plate |
| Muse / Stockholm Syndrome - Tight Rhythm | Tight palm-muted rhythm | Gate/boost/modern amp/cab/EQ; no delay/reverb |
| Muse / Starlight - Chiming Guitar | Lightly driven octave accompaniment | Eighth echo at adjustable 120 BPM, 14% wet; short plate |
| Muse / Knights of Cydonia - Western Guitar | Crunch and galloping articulation | 175-ms echo, 16% wet; small room |
| Muse / Space Lead - Sustained Echo | General sustaining lead texture | Hot-rod amp; 360-ms echo, 19% wet; quiet plate |
| Muse / Time Is Running Out - Guitar Fuzz | Mid-focused guitar adaptation | Sustain fuzz, crunch amp and quiet plate |

## More Muse song templates

Search **Muse expansion** or the song name in Choose Preset... for these 12
additional sounds. They use the same existing processors, preserve every prior
sound/voice, and require no Creative profile. Outputs are calibrated using the
same mathematical phrase as the earlier templates; guitar listening remains
pending. No song-specific studio gear/settings were independently verified. The owner
confirmed all seven **album versions** as the target. See the
[research status and per-song matching guide](muse-tone-research.md).

| Factory name | Intended guitar role | Chain character |
|---|---|---|
| Muse / Citizen Erased - Dense Distortion | Heavy low-register section | Sustain fuzz, controlled hot-rod gain, low-string EQ and small room |
| Muse / Citizen Erased - Clean Ending | Requested soft ending guitar | Light compression, clean combo, gentle chorus, 380-ms echo and 2.4-s plate |
| Muse / New Born - Fuzz Rhythm | Main guitar riff | Tight bias fuzz, forward chime, cabinet/EQ and small room |
| Muse / New Born - Filtered Fuzz Lead | Lead alternative | Bias fuzz, manual wah, chime and quiet 360-ms echo/plate |
| Muse / Hysteria - Guitar Drive | Guitar accompaniment/lead | Hard distortion, crunch amp, upper-mid focus and small plate |
| Muse / Psycho - Dry Riff | Punchy palm-muted riff | Gate, tight modern gain and cabinet/EQ; no delay/reverb |
| Muse / Reapers - Articulate Tapping | Tapping and legato | Light compression, controlled hot-rod gain and quiet plate; no hard gate |
| Muse / Reapers - Singing Lead | Sustained lead | Gentle gate, manual wah, hot-rod lead and filtered 300-ms echo |
| Muse / Defector - Broad Distortion | Broad rhythm/lead | Hot-rod saturation, fuller cabinet body and restrained plate |
| Muse / The Globalist - Spacious Clean Intro | Introductory guitar atmosphere | Clean amp, slow chorus, 460-ms echo and 4.8-s hall |
| Muse / The Globalist - Heavy Middle | Heavy middle-section guitar | Gate/boost, broad heavy amp, low-end EQ and small room |
| Muse / The Globalist - Clean Outro Guitar | Clean ending accompaniment | Rounded clean combo, light compression and short room |

Citizen Erased's low-register part requires suitable physical tuning/strings or
an extended-range instrument; the rack does not create another string. Its clean
ending template preserves soft guitar attacks without attempting to synthesize
piano. New Born's piano introduction and Hysteria's signature bass riff remain
separate instruments. The Globalist's clean outro is a guitar adaptation of a
piano-led section. Reapers' recorded pitch gestures are not automated here;
its lead template stays in Legacy 128. Wah Position is a manual knob, with no
new pedal/MIDI mapping. Section templates load independently using the existing
preset transaction; no automatic song progression or seamless tail promise is
added. Physical Drop D is suggested for New Born/Psycho; match other parts with
your instrument by ear. Delay times are adjustable starting points.

Start with instrument input and sensible interface gain, input trim 0 dB, and
low headphone volume. Dry picking peaks around -18 to -9 dBFS are a useful
starting region, not a pickup calibration. Adjust the **amp Gain** for clean
headroom before raising Output. For the space sounds, adjust echo/reverb Mix
before changing feedback or decay. A little wet sound can go a long way.

Let Down and Weird Fishes contain interlocking performances; delay does not play
another guitar part. No Surprises includes a separate glockenspiel. Starlight's
keyboard motif and Time Is Running Out's bass part are separate instruments.
Stockholm Syndrome's template asks for physical Drop D tuning. No hidden pitch
processor retunes strings. Echo times/120-BPM settings are editable starting
points, not verified studio timing.

## Stereo / Tape Echo v2

New echo instances use `echo` v2. Saved `echo` v1 instances/voices still load their
original 12-control algorithm. V2 retains indices 00–11 and adds:

- **Wet Low Cut (0 = off)** at index 12, 0–1000 Hz. A one-pole high-pass before
  character coloration removes bass/DC from both echoes and their recirculation.
  The dry guitar is untouched. Default 0 Hz disables the added filter.
- **Wow Rate** at index 13, 0.05–5 Hz, default 0.7 Hz. This sets the tape LFO's
  speed; the existing Wow amount has up to 1-ms modulation depth.
- **Wow Stereo Phase** at index 14, 0–100%, default 0%. The right tape read head
  shifts up to 180 degrees relative to the left. A nonzero Wow amount in Tape
  mode creates width from duplicated mono guitar; phase alone does not.

Analog/Digital mode has no tape wow. Ping-pong remains cross-channel feedback;
identical mono input alone does not alternate isolated echoes. Tempo/subdivision,
single/three heads, 100-ms time crossfades, 20-ms parameter ramps, maximum 70%
feedback, zero added transport latency and the 120-s tail declaration remain.
The phase modulation affects the principal head; auxiliary heads stay at their
original fixed fractions. The stage uses preallocated histories and fixed filter
state. It adds no asset access, allocation, ownership queue or routing branch
in the audio callback. Neutral added controls preserve the v1 sound within the
portable numerical tolerance.

## Reproduce the files and compare a DI

The additive seed script preserves existing JSON entries; edits to the JSON
remain authoritative. Run it only to seed missing templates:

```bash
python3 tools/add_song_templates.py
python3 tools/generate_sound_library.py
python3 tools/generate_sound_catalog.py
cmake --build --preset linux-release
./build/linux-release/guitarrack_export_sounds . > build/linux-release/registry.json
python3 tools/update_sound_schemas.py build/linux-release/registry.json
python3 tools/generate_effect_docs.py build/linux-release/registry.json
```

Use your original recorded dry mono guitar for perceptual comparison:

```bash
mkdir -p captures
./build/linux-release/guitarrack_render "Radiohead / Subterranean Homesick Alien - Floating Lead" captures/my-DI.wav captures/alien-lead.wav
./build/linux-release/guitarrack_render --catalog-audit captures/song-headroom.csv "Song templates"
./build/linux-release/guitarrack_render --catalog-audit captures/muse-headroom.csv "Muse expansion"
```

The optional audit argument is an exact metadata tag. The audit renders each
matching sound at 0.3/1/3 input strength using an original mathematical test
phrase. It checks finite stereo output and callback heap use, measures unclamped
peak/RMS, and suggests output changes; it does not mutate presets, establish
perceived loudness or establish guitar realism. A missing tag is an error.
See [verification](testing.md#song-template-verification-2026-10-01) and the
[listening worksheet](listening.md). Native desktop, physical routing, deadlines,
round trip and comparison against the supplied recording remain pending.

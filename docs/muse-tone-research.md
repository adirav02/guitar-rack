# Muse album-tone research and matching guide

Scope confirmed by the owner: the album versions of Citizen Erased, New Born,
Hysteria, Psycho, Reapers, Defector and The Globalist. Research attempted on
2026-10-01. The 12 added templates cover the guitar roles below; all remain
listening pending.

## What the research can establish here

A fresh attempt to read MuseWiki's Citizen Erased/New Born song pages and
Premier Guitar's Bellamy interview returned proxy-policy HTTP 403. A Guitar
World candidate interview request was also refused before its page could be
verified. The environment has no callable general web-search/audio-listening
tool. These are access limitations, not evidence that a particular effects
chain is correct. No original recording, isolated guitar stem or artist rig
interview was auditioned/read in this follow-up.

The Acumen query about Bellamy's amplifiers/effects returned album/chart news,
which does not establish tone and was excluded. GitHub searches for Bellamy,
Fuzz Factory and Citizen Erased found no credible documented effects chain.
A repository named musewiki was inspected and proved to be about the unrelated
Muse Dash game; it was excluded. The retrieved community MusicTree entries
([Citizen Erased](https://github.com/Gleifhe/MusicTree/blob/9a9761c3f6c5c91fa9a451dd7ced5235661f2c22/content/songs/citizen-erased-ms.md),
[Matt Bellamy](https://github.com/Gleifhe/MusicTree/blob/9a9761c3f6c5c91fa9a451dd7ced5235661f2c22/content/people/matt-bellamy.md))
contain personnel/album credits, not effect settings. They cannot authenticate
our presets or substitute for a firsthand recording/rig source.

The [earlier primary engineering research](song-tone-research.md) remains useful:
JUCE's envelope/DelayLine examples, Faust's reverberator structures, Web Audio's
signal graph specification and the RBJ EQ Cookbook support compressor timing,
filtered feedback, parameter smoothing, oversampling and reverb diffusion.
They establish DSP principles, not which pedal Bellamy used for a song.
The actual effect/control implementation and tests are the technical source of
truth for this rack. No new branded circuit/amp claim is made.

## How to get closer to each album guitar part

The musical targets and parameter choices in this table are **inferred starting
points** from general musical knowledge and the rack's measured behavior. The
numbers describe our implementation, not physical pedal knob positions or
verified studio settings. Album fidelity is unverified. An entire mixed master
contains bass, drums, keys, vocals and guitar layering that one guitar does not
produce.

| Song / guitar role | Working target and current template | First adjustments during reference A/B |
|---|---|---|
| Citizen Erased — distortion | Sustain fuzz (21-dB drive, 30% compression, 3.6-kHz tone) into a moderate hot-rod amp (16-dB gain), darker cab, 50-Hz post high-pass and forward 950-Hz mids | Match the low-register instrument/tuning before EQ. Reduce fuzz/amp gain if attacks disappear. Judge bass against the guitar part rather than the full mix's bass instrument. |
| Citizen Erased — clean ending | Clean combo (0-dB gain), light parallel compression, 13% slow chorus, filtered 380-ms echo at 20% wet and 2.4-s plate at 19% | Start with neck/split pickup and soft picking. If the reference guitar is drier, reduce echo/plate first; if it sounds wobbly, reduce chorus. The layered ending's piano is separate. |
| New Born — riff | Tight bias fuzz (27-dB drive, stability 95%, low gate), modest chime amp (7-dB gain), bright cab and controlled lows | Preserve note separation before adding saturation. Compare guitar-volume cleanup and string muting. Do not try to recreate the piano introduction with distortion. |
| New Born — lead | The riff foundation plus moderate manual wah and 19% filtered echo | Compare with wah bypassed first; add just the needed mid emphasis. Move Position manually when playing. The current chain does not perform the solo or its gestures. |
| Hysteria — guitar | Hard distortion with 100-Hz input low cut / 4.4-kHz filter, crunch support and +2.5 dB around 1.2 kHz | Separate the guitar sound from the signature bass riff. Focus on lead sustain, upper-mid clarity and pick attack; reduce saturation if fast guitar notes blur. |
| Psycho — riff | A dry tight modern amp (21-dB gain), modest bass rejection, +2.5-dB mids and no added delay/reverb | Match physical tuning and palm-muted articulation. Adjust gain/low end before the gate. Avoid judging the riff at a louder level than the reference. |
| Reapers — tapping | Light compression into controlled hot-rod gain (18 dB), tight cabinet/EQ and quiet plate; no hard gate | Match finger/tapping dynamics first. Raise sustain only enough for weak notes to speak. If noise rises, improve muting/input gain before adding a severe gate. |
| Reapers — lead | Hot-rod lead plus moderate manual wah, filtered 300-ms echo at 18% wet and a quiet plate | Match the base gain/filter sound before pitch gestures. The template omits pitch processing to retain normal latency; recorded pitch effects remain a known gap requiring reference verification. |
| Defector — guitar | Broad hot-rod saturation (22-dB gain), fuller/darker cab, restrained EQ and plate | Compare midrange body with Psycho's tighter/drier template at equal loudness. Keep chords distinct; multiple recording takes cannot be reproduced by increasing bass. |
| The Globalist — clean intro | Clear clean amp, slow 20% chorus, filtered 460-ms echo at 26% wet and 4.8-s hall at 28% | Compare dry attack first, then add only enough space. Playing sparse sustained/slide-like phrases matters; whistling and orchestration are separate. |
| The Globalist — heavy middle | Bass-tightening boost into broad heavy gain (22 dB), dark/full cab, post low-end control and small room | Match strings/tuning and attack. Reduce gain if riff timing becomes indistinct; keep long intro ambience out of this heavy section. |
| The Globalist — ending accompaniment | Rounded clean combo, gentle compression, no chorus/echo and a 1.5-s room at 13% | Treat this as guitar accompaniment for a piano-led ending. EQ, reverb or octave effects do not establish an authentic piano tone. |

Current tuning suggestions, times and section descriptions are starting guidance,
not independently authenticated album transcriptions. Check the exact part and
instrument before fixing a tuning claim. Moving to a live recording would need
its own reference because arrangements and rigs can differ.

## A repeatable matching procedure

1. Use the same dry guitar recording for every candidate. Record interface gain,
   pickup, guitar volume/tone, strings/tuning and the actual input peaks. Use the
   interface instrument input and avoid clipping the recorded DI.
2. Compare the same short guitar phrase and section against the album. When
   possible use an authorized isolated guitar reference; a complete master does
   not isolate guitar bass content, panning or room sound.
3. Match perceived playback level before judging tone. The new presets' nominal
   synthetic phrase RMS is close to 0.010, but that is only initial output
   calibration. Different guitar inputs require different final levels.
4. Tune in order: pickup/playing/tuning -> drive and amp compression -> cabinet
   and broad EQ -> modulation -> echo timing/mix -> reverb tone/decay. Preserve
   the attack before adding ambience. Adjust Output after Gain changes.
5. Compare soft/hard picks, sustained notes, palm mutes and chord transients.
   Keep only changes that improve the chosen phrase at equal playback level.
   Check stereo and mono playback so widening does not hide an EQ problem.
6. Save As a named user preset and record the accepted factory/user UUID plus
   control changes. An accepted tone is that particular chain and instrument
   setup; it does not approve other templates automatically.

The existing render tool can process your original DI without opening an audio
device; instructions and all template names are in [song templates](song-templates.md).
The [36-render headroom audit](evidence/muse-templates/headroom.csv) checks finite
output and callback heap use, not perceptual accuracy. All 12 new templates fit
Legacy 128; no pitch, automatic scenes, new effects or latency profile was added.

## Priority sources still requiring access

These are a reading/verification list, **not freshly accessed evidence**:

- [Premier Guitar — Muse's Matt Bellamy: Unusual Collisions](https://www.premierguitar.com/artists/muses-matt-bellamy-unusual-collisions): firsthand general rig context; any claims must be checked against recording era and song.
- [MuseWiki — Citizen Erased](https://www.musewiki.org/Citizen_Erased_(song)) and [New Born](https://www.musewiki.org/New_Born_(song)): secondary song/instrument context; follow their citations rather than treating unsourced fan settings as studio documentation.
- Song-specific recording interviews, production notes and licensed isolated guitar references for Hysteria, Psycho, Reapers, Defector and The Globalist: verify studio parts, clean/piano boundaries, amp/cab combinations and pitch gestures before claiming an album match.

The implementation and software verification are complete. Fresh artist-source
verification and album listening are **blocked/pending**, with the practical
matching guide supplied so that those limits are explicit.

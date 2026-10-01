# Guitar rack: Radiohead / Muse tone and audio engineering research

Research date: 2026-10-01. Completed before project inspection or implementation.

## Scope and evidence

Goal: playable, calibrated guitar effects for Let Down (clean arpeggio + optional space layer), No Surprises (clean guitar), Subterranean Homesick Alien (clean foundation + floating lead), and representative Muse textures. These are tone approximations, not verified transcriptions of studio rigs. Recordings combine multiple instruments, takes, amplifiers, microphones, stereo placement, and mastering; one live guitar cannot reproduce the entire mix.

The supplied reference is https://www.youtube.com/watch?v=-H3yyXC4gC8 . The environment's HTTP policy refused YouTube, thekingofgear.com, and Stanford CCRMA. No audio was auditioned and no song-specific hardware claims were verified. An Acumen query returned recent artist news but no useful tone evidence; those results were excluded. GitHub searches found no credible song-specific gear source. Do not present inferred pedal settings, exact tempos, or gear as established facts.

Primary engineering sources successfully fetched and read:

1. JUCE DelayLine: https://github.com/juce-framework/JUCE/blob/master/modules/juce_dsp/processors/juce_DelayLine.h — fractional delay interpolation; real-time delay changes should be smoothed. Linear interpolation trades some high-frequency response for low cost; Lagrange is an alternative.
2. JUCE Reverb: https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_basics/utilities/juce_Reverb.h — Freeverb-style parallel combs and serial allpasses, independent wet/dry and width, sample-rate scaling, 10 ms parameter smoothing, damping inside feedback.
3. JUCE Compressor: https://github.com/juce-framework/JUCE/blob/master/modules/juce_dsp/widgets/juce_Compressor.cpp — peak envelope ballistics; above-threshold gain uses ratio; attack and release belong to the envelope follower.
4. Faust reverbs: https://github.com/grame-cncm/faustlibraries/blob/master/reverbs.lib — Schroeder allpass diffusion and parallel comb banks, feedback delay networks and frequency-dependent decay.
5. W3C Web Audio: https://github.com/WebAudio/web-audio-api/blob/main/index.bs — interactive latency hint, native effects nodes, cyclic delay processing, waveshaper oversampling, graph lifecycle.
6. RBJ Audio EQ Cookbook: https://github.com/WebAudio/Audio-EQ-Cookbook/blob/gh-pages/audio-eq-cookbook.html — biquad high-pass, low-pass, shelves, peaking filters and sample-rate-dependent coefficients.

Engineering sources substantiate DSP design, not the artist associations below. Song recipes are informed musical starting points based on general knowledge and must be auditioned against the reference with the user's guitar/interface.

## Signal flow and gain staging

Start with interface instrument/high-impedance input; disable automatic gain, speech noise suppression and acoustic echo cancellation where browser capture supports it. Aim for unprocessed picking peaks around -18 to -9 dBFS and avoid clipping at the interface. Match gain using the actual pickup: single coils usually need more input than humbuckers. Hardware direct monitoring plus processed monitoring can cause comb filtering; monitor the intended route only.

Recommended ordering: input trim / gentle high-pass -> optional light gate -> compression -> drive or fuzz -> amp/cabinet shaping -> modulation -> filtered delay -> reverb -> output trim / protection. Delay and reverb wet branches should preserve dry articulation. Sustain-heavy fuzz can benefit from a different compressor/drive order, but one fixed chain should make its tradeoff clear.

Clean presets need light compression (rough starting point 2:1–3:1, attack 15–30 ms, release 100–200 ms). Keep pick transients and avoid driving the output limiter constantly. Space presets should use a very light gate or none so soft notes and long tails survive. Place gate before ambience.

Amp/cabinet shaping matters even for clean sounds: remove sub-bass roughly below 70–110 Hz; trim fizz above roughly 5–9 kHz; use broad shelves and moderate presence. Avoid claiming cabinet convolution unless using a real documented impulse response. Clean chime needs a modest upper-mid emphasis without harshness; driven Muse textures need tighter lows and rounded highs.

Drive/fuzz nonlinearities create harmonics above Nyquist; oversampling reduces aliasing, at added cost. Pre/post filtering and output compensation are necessary. A generic waveshaper can suggest aggressive Muse fuzz but does not reproduce the gated, bias-sensitive circuit behavior of a specific physical pedal.

## Delay and modulation

Time from tempo: quarter_ms = 60000 / bpm; eighth = quarter/2; dotted_eighth = quarter*0.75; triplet_eighth = quarter/3. Song tempo is a user-adjustable estimate, not a claim of exact studio timing. Keep free millisecond settings available; tempo subdivisions must remain internally consistent.

Delay feedback must be strictly below unity. Clamp parameters and reject non-finite values; high feedback plus gain/filter boosts can destabilize a loop. Low-pass feedback around 2.5–6 kHz and high-pass around 100–250 Hz keep echoes behind the dry guitar and reduce low-end buildup. Moderate mixes (10–30%) retain note definition. Modulating delay time adds pitch movement; fractional interpolation and smooth targets reduce clicks. A slow 0.15–0.6 Hz LFO with only a few ms of depth is appropriate for gentle tape/space movement.

Chorus is a short modulated wet delay mixed with dry guitar. Typical starting region: 12–25 ms base delay, 1–4 ms depth, 0.2–0.8 Hz rate, 10–25% mix. A wet-only modulation branch approximates vibrato. Different phase/delay offsets on left/right can provide width; duplicated mono signals do not create stereo width. Avoid extreme width that collapses poorly to mono.

## Reverb

A cloud of delay repeats alone is not a diffuse reverb. Parallel combs plus serial allpasses or a convolution/FDN architecture provide density. Damping in feedback shortens high-frequency decay; wet high-pass reduces boom. Predelay (about 15–35 ms) preserves the picking attack; short room/plate-like settings suit the clean parts and a longer damped hall suits floating lead.

Separate decay, wet amount and tone where practical. A long decay at low mix can add space while preserving phrasing. Do not call a reverb 'shimmer' without a pitch-shift path, or 'reverse' without a reverse-grain algorithm. A long hall may be described as a cloud/space texture. Tail continuity, bypass behavior and effect cleanup should be tested.

## Song recipes (inferred starting points)

### Radiohead — Let Down: clean arpeggio

Bright, lightly compressed clean tone; modest bass cut, chime without brittle treble. Start with very low modulation or none, filtered rhythmic delay around 180–230 ms at 12–20% wet and 15–25% feedback, short diffuse reverb around 1.5–2.5 s at 10–18% wet. Favor neck/middle or an equivalent clear pickup sound; light, even picking. The interlocking guitar arrangement and repeated figures are essential; delay alone does not play the second guitar part.

### Radiohead — Let Down: space layer

Separate optional template: the same clean foundation, gentle slow chorus, longer echo around 350–450 ms, 25–35% feedback, 20–28% wet; damped reverb around 3–4 s. Use sustained upper-register notes or volume swells. This supports the requested space texture without washing out the clean arpeggio template.

### Radiohead — No Surprises: clean guitar

Rounded, bell-like clean guitar, light compression, no distortion, almost no chorus, little or no rhythmic echo, small room/plate-like ambience around 1–1.8 s at 8–15% wet. The recognizable bell line includes a separate glockenspiel; processing a guitar is an approximation and cannot reproduce that instrument exactly. High-register picking and consistent dynamics help; do not use a huge hall by default.

### Radiohead — Subterranean Homesick Alien: clean foundation

Warm clear body, restrained bass, gently softened top end, a small amount of slow modulation, low-mix echo and moderate room/hall. Keep room for the lead part and sustained notes.

### Radiohead — Subterranean Homesick Alien: floating lead

Clean/edge-of-breakup foundation, slow chorus or vibrato, filtered modulated echo about 350–500 ms, feedback about 30–45%, wet about 22–35%, and a diffuse damped hall about 3–5 s with 20–35 ms predelay. Preserve an audible dry attack. Start with a bright pickup then reduce treble if harsh. The layered floating texture is a better target than generic heavy distortion or an unverified claim of pitch shifting. Tempo around 150 BPM (400 ms quarter) is only an adjustable starting assumption.

### Muse — Plug In Baby: fuzz lead

Aggressive saturated guitar, tight low end, forward mids, rounded cabinet highs and modest ambience. Start with fuzz rather than stacking excessive distortion; compensate output gain. Bridge pickup and precise articulation. Generic DSP fuzz approximates the texture; it does not replicate every physical fuzz circuit behavior.

### Muse — Stockholm Syndrome: heavy rhythm

High-gain drive/amp, controlled lows, focused upper mids, cabinet filtering, little ambience, and a light pre-drive gate. Keep delay off/low to preserve fast palm-muted rhythm. Playing, tuning and a layered recording matter as much as the gain knob.

### Muse — Starlight: chiming guitar

Clean to light crunch, moderate compression, broad chime, a small amount of delay and short reverb. Keep it a guitar accompaniment texture: the signature keyboard melody is not evidence for a guitar effects chain.

### Muse — ambient clean / space lead

Additional reusable templates can cover slow modulated clean echoes and sustained driven lead with a quiet rhythmic delay. Label these as inspired textures rather than exact studio emulations. Avoid labeling the signature Hysteria bass riff as an authenticated guitar preset.

## Implementation priorities and verification

Inspect existing architecture after this research: prefer improving the real DSP and exposing missing parameters over adding decorative effect labels. Every preset must have a distinct, useful chain and valid values. Add pickup/playing advice and clear approximation notes in visible preset details. Keep existing user presets compatible.

Useful validation: impulse response for expected delay onset and reverb decay, silence/non-finite/extreme-parameter stability, wet/dry endpoints and gain headroom, sample-rate behavior at 44.1/48 kHz, modulation width and node cleanup, actual audio rendering if supported, preset save/load and controls in a browser. Automated tests can verify engineering, but cannot establish perceptual song accuracy without reference listening and guitar audition.

## Additional Muse template design (later follow-up)

The later request adds templates for Citizen Erased (heavy + clean ending), New
Born, Hysteria, Psycho, Reapers, Defector and The Globalist. No additional verified
studio gear evidence or reference listening is claimed. These inferred roles use
the existing effects and the signal-flow principles above:

- Citizen Erased: dense sustain plus restrained hot-rod gain and low-register
  focus; a separate lightly compressed clean combo with slow chorus/echo/plate
  for the ending. Appropriate physical strings/tuning are required for low notes.
- New Born: focused bias fuzz into a modest chime amp for rhythm; separate manual
  wah/filtered echo for lead. The piano introduction is not an effects recipe.
- Hysteria: upper-mid saturated guitar accompaniment/lead. The signature opening
  riff is bass; a guitar template is not proof of a bass-recording match.
- Psycho: firm dry modern gain, controlled lows and a light gate; avoid washing
  out the riff with echo/reverb.
- Reapers: tapping needs controlled gain, even dynamics and no hard gate;
  sustained lead benefits from a modest manual wah and low-mix echo. Recorded
  pitch gestures are not automatically recreated, and no pitch processor is
  required by these templates.
- Defector: broad hot-rod saturation and a fuller synthetic cabinet, with a small
  amount of plate ambience; layering remains a separate performance task.
- The Globalist: distinct spacious clean intro, controlled heavy middle and
  rounded clean outro accompaniment. The piano-led ending is labelled as a
  guitar adaptation, not a synthesized piano tone.

These are independent presets. Switching sections uses the existing load/fade
mechanism; no automatic scene sequencing or guaranteed seamless long tails is
implied. All additions fit Legacy 128. Matching synthetic phrase RMS provides a
useful initial level comparison, with perceived guitar loudness still pending.

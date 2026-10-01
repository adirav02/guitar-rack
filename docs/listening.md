# Listening and desktop acceptance worksheet

Status: pending for the new upgrade. Only the exact legacy **Art Rock /
Subterranean Pulse** has the owner's explicit listening approval. Signal tests
and synthetic renders cannot approve artist character or album matching.

## Launch and choose a sound

```bash
PIPEWIRE_LATENCY=256/48000 pw-jack ./build/linux-release/GuitarRack_artefacts/Release/GuitarRack
```

Expand Audio Setup, choose the existing backend/interface, physical mono Input 2
and outputs 1/2; use Instrument mode and Direct/USB = USB on the M-Track Solo.
Start with low physical headphone volume, trim 0 dB and output −12 dB. Confirm
actual rate/buffer and clipping flags. Startup must remain stopped.

Open Choose Preset..., inspect a sound and press Load. Search/navigation alone
must leave audio and dirty indication unchanged. For Ambient Bloom or Expressive
Pitch Fuzz, Stop first, choose Creative 4096, then Load and Start. Creative adds
85.333 ms processor latency at 48 kHz; it is deliberately a higher-latency
texture/expression session. Other new sounds fit Legacy 128. Save As keeps the
factory original protected. Software Expression is the pitch card's rotary/numeric
control; Capture Hold requires prior audio, then Off → Hold and expires by 120 s.

## Render an identical DI

Record an original dry mono PCM/float WAV, at most 120 seconds and 8–192 kHz.
The offline tool uses no audio device and refuses to overwrite an output file:

```bash
mkdir -p captures
./build/linux-release/guitarrack_render --synthetic captures/synthetic-DI.wav
./build/linux-release/guitarrack_render "Radiohead / Oxford Contour Drive" captures/synthetic-DI.wav captures/oxford.wav
./build/linux-release/guitarrack_render "Radiohead / Ambient Bloom" captures/synthetic-DI.wav captures/bloom.wav --creative
# Replace the input with your recorded DI for actual listening acceptance.
```

Output is stereo float32 with an eight-second tail and no MonitorEngine peak
clamp. Long tails may continue beyond that file. Input trim and rack output apply;
processor profile delay is retained. A synthetic phrase is a mathematical signal,
not a recorded guitar or pickup calibration. Copying an input file does not prove
its originality/licensing; use a DI you have permission to process.

For A/B, render the same DI through both tones and adjust playback gain to equal
**perceived** loudness. Record those adjustments. Factory output calibration uses
phrase RMS/headroom only and does not establish perceived level matching.

## Record actual conditions and judgments

| Item | Owner entry |
|---|---|
| Date, application/build | |
| Guitar, string gauge, tuning | |
| Pickups/selector, guitar volume/tone | |
| Interface gain, input trim, raw/input peak | |
| Backend, actual rate and frames | |
| DSP profile; measured round trip if measured | |
| Headphones/monitors model and listening level | |
| CUBE 10GX AUX result, separately | |
| DI file and compared preset UUIDs | |
| Perceived level adjustments | |
| Accepted exact sound / rejected changes | |

Play soft/hard picks, open/barre/power chords, low palm mutes, bends, harmonics,
guitar-volume cleanup, decays and silence. Judge attack/fizz, note separation,
sustain, noise gating, usable dynamic range and standalone character. Pitch:
judge held notes, moving bends, attacks and chords separately; close chords are
currently an acknowledged weak case. Riffs should retain definition and modest
wetness; long ambience is intentional on ambient sounds. Record any clicks,
clipping/protection or excessive latency. Do not transfer approval to the catalog.

## Native desktop checks

- At 1280×720, 1366×768, a wide display and 150% scaling, inspect safety controls,
  numeric entry and board/browser scrolling. Expand/collapse must not dirty/change
  tone. Gain/Drive and Output must stay in Basic controls.
- Reopen/focus the single browser, navigate/search/favorite without loading,
  explicitly Load/Enter/double-click, and test Save/Discard/Cancel. Browser Escape
  closes it while playback continues; main Escape stops playback.
- Reorder several independent instances. Their expansion, anchored scroll and
  settings must follow UUIDs; voicing changes only that instance.
- Cancel queued loads, try missing/corrupt IRs and an incompatible pitch load in
  Legacy. Check current sound/dirty/identity retention and visible errors. After
  graph acceptance, cancellation must report that the transition finishes.
- Save/Save As UTF-8 names, import/export a bundle, delete a saved user sound,
  restart stopped and inspect local geometry/filters/favorites/latency profile.
  Quit with the browser and a file chooser open; no orphan window or hang.
- Test Start/Stop/Mute/bypasses, rapid edits, actual rate changes and reconnect.
  Run the existing ten-minute physical soak and measured round-trip checklist.

These checks remain unchecked until results with actual device/display conditions
are recorded. A successful Linux build does not verify Windows, DAW/VST3 or a
hardware callback deadline.
